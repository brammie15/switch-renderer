#include "Model.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>
#include <cstdio>
#include <string>
#include "Image.h"

Model::Model(const std::string& path) : path_(path) {}

Model::~Model()
{
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);

    for (auto& mat : materials_)
    {
        if (mat.baseColorTex) glDeleteTextures(1, &mat.baseColorTex);
        if (mat.metallicTex)  glDeleteTextures(1, &mat.metallicTex);
        if (mat.roughnessTex) glDeleteTextures(1, &mat.roughnessTex);
        if (mat.aoTex)        glDeleteTextures(1, &mat.aoTex);
        if (mat.normalTex)    glDeleteTextures(1, &mat.normalTex);
    }
}

static std::string getDirname(const std::string& path)
{
    size_t p = path.find_last_of("/\\");
    return (p == std::string::npos) ? std::string() : path.substr(0, p + 1);
}

GLuint Model::createDefaultTexture(const unsigned char color[4])
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, color);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

bool Model::load()
{
    tinyobj::ObjReaderConfig reader_config;
    reader_config.triangulate = true;
    reader_config.mtl_search_path = getDirname(path_);

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(path_, reader_config))
    {
        if (!reader.Error().empty())
            printf("TinyObjReader ERROR: %s\n", reader.Error().c_str());
        return false;
    }

    if (!reader.Warning().empty())
        printf("TinyObjReader WARN: %s\n", reader.Warning().c_str());

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();
    const auto& tinyMaterials = reader.GetMaterials();

    // Convert tinyobj materials to our Material struct
    materials_.resize(tinyMaterials.size());
    std::string baseDir = getDirname(path_);
    unsigned char white[4] = { 255,255,255,255 };
    for (size_t i = 0; i < tinyMaterials.size(); ++i)
    {
        const auto& tmat = tinyMaterials[i];
        Material& mat = materials_[i];
        mat.name = tmat.name;

        mat.baseColorFactor[0] = tmat.diffuse[0];
        mat.baseColorFactor[1] = tmat.diffuse[1];
        mat.baseColorFactor[2] = tmat.diffuse[2];
        mat.metallicFactor = tmat.metallic;
        mat.roughnessFactor = tmat.roughness;
        mat.aoFactor = 1.0f;

        // Load textures if they exist
        auto loadTex = [&](const std::string& fname) -> GLuint {
            if (fname.empty()) return createDefaultTexture(white);
            std::string texPath = fname;
            if (texPath[0] != '/' && !baseDir.empty()) texPath = baseDir + texPath;
            Image img(texPath, 4);
            if (img.data())
            {
                GLuint tex = 0;
                glGenTextures(1, &tex);
                glBindTexture(GL_TEXTURE_2D, tex);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.width(), img.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data());
                glGenerateMipmap(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, 0);
                return tex;
            }
            return createDefaultTexture(white);
        };

        printf("Found BaseColor at %s\n", tmat.diffuse_texname.c_str());
        mat.baseColorTex = loadTex(tmat.diffuse_texname);

        printf("Found Metallic at %s\n", tmat.metallic_texname.c_str());
        mat.metallicTex  = loadTex(tmat.metallic_texname);
        
        printf("Found Roughness at %s\n", tmat.roughness_texname.c_str());
        mat.roughnessTex = loadTex(tmat.roughness_texname);
        
        printf("Found AO at %s\n", tmat.ambient_texname.c_str());
        mat.aoTex        = loadTex(tmat.ambient_texname);
        
        printf("Found Normal at %s\n", tmat.normal_texname.c_str());
        mat.normalTex    = loadTex(tmat.normal_texname);
    }

    // Build vertices and submeshes
    std::unordered_map<int, std::vector<Vertex>> temp;
    auto copyVec3 = [](int idx, const std::vector<float>& data, const float def[3], float out[3]){
        if (idx >= 0){ out[0]=data[3*idx]; out[1]=data[3*idx+1]; out[2]=data[3*idx+2]; }
        else { out[0]=def[0]; out[1]=def[1]; out[2]=def[2]; }
    };
    auto copyVec2 = [](int idx, const std::vector<float>& data, const float def[2], float out[2]){
        if (idx >= 0){ out[0]=data[2*idx]; out[1]=data[2*idx+1]; }
        else { out[0]=def[0]; out[1]=def[1]; }
    };
    float defPos[3] = {0,0,0}, defNormal[3]={0,0,1}, defTex[2]={0,0};

    for (const auto& shape : shapes)
    {
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f)
        {
            int matid = (f < shape.mesh.material_ids.size()) ? shape.mesh.material_ids[f] : -1;
            size_t fv = shape.mesh.num_face_vertices[f];
            for (size_t v=0; v<fv; ++v)
            {
                tinyobj::index_t idx = shape.mesh.indices[index_offset+v];
                Vertex vert{};
                copyVec3(idx.vertex_index, attrib.vertices, defPos, vert.position);
                copyVec3(idx.normal_index, attrib.normals, defNormal, vert.normal);
                copyVec2(idx.texcoord_index, attrib.texcoords, defTex, vert.texcoord);
                temp[matid].push_back(vert);
            }
            index_offset += fv;
        }
    }

    vertices_.clear();
    submeshes_.clear();
    auto addSubmesh = [&](int matid){
        auto it = temp.find(matid);
        if(it != temp.end() && !it->second.empty()){
            Submesh sm{matid, vertices_.size(), it->second.size()};
            vertices_.insert(vertices_.end(), it->second.begin(), it->second.end());
            submeshes_.push_back(sm);
        }
    };
    addSubmesh(-1);
    for (size_t i=0;i<materials_.size();++i) addSubmesh(i);

    printf("Model loaded: %s (%zu vertices, %zu submeshes, %zu materials)\n",
           path_.c_str(), vertices_.size(), submeshes_.size(), materials_.size());

    return true;
}

bool Model::uploadToGPU()
{
    if(vertices_.empty()) return false;

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices_.size()*sizeof(Vertex), vertices_.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoord));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

void Model::draw(int baseColorLoc, int metallicLoc, int roughnessLoc, int aoLoc, int normalLoc) const
{
    if(vao_==0 || vertices_.empty()) return;

    glBindVertexArray(vao_);

    for(const auto& sm: submeshes_)
    {
        const Material* mat = (sm.material_id >= 0 && sm.material_id < (int)materials_.size())
                              ? &materials_[sm.material_id] : nullptr;
        if(mat)
        {
            if(baseColorLoc>=0){ glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, mat->baseColorTex); glUniform1i(baseColorLoc, 0); }
            if(metallicLoc>=0){ glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, mat->metallicTex); glUniform1i(metallicLoc, 1); }
            if(roughnessLoc>=0){ glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, mat->roughnessTex); glUniform1i(roughnessLoc, 2); }
            if(aoLoc>=0){ glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, mat->aoTex); glUniform1i(aoLoc, 3); }
            if(normalLoc>=0){ glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, mat->normalTex); glUniform1i(normalLoc, 4); }
        }

        glDrawArrays(GL_TRIANGLES, (GLint)sm.first, (GLsizei)sm.count);
    }

    glBindVertexArray(0);
}
