#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <vector>
#include <glad/glad.h>

#include "tiny_obj_loader.h"

struct Vertex {
    float position[3];
    float normal[3];
    float texcoord[2];
    float tangent[3];
    float bitangent[3];
};

struct Submesh
{
    int material_id; // -1 for no material
    size_t first;    // first vertex index in the consolidated vertex array
    size_t count;    // number of vertices
};

// New Material struct
struct Material
{
    std::string name;

    GLuint baseColorTex = 0;

    // Optional PBR maps
    GLuint metallicTex  = 0;
    GLuint roughnessTex = 0;
    GLuint aoTex        = 0;
    GLuint normalTex    = 0;

    float baseColorFactor[3] = {1.0f, 1.0f, 1.0f};
    float metallicFactor     = 1.0f;
    float roughnessFactor    = 1.0f;
    float aoFactor           = 1.0f;

    bool isDiffuseOnly() const {
        return metallicTex == 0 && roughnessTex == 0 && aoTex == 0 && normalTex == 0;
    }
};

class Model
{
public:
    explicit Model(const std::string& path);
    ~Model();

    bool load();              // Load OBJ + PBR textures
    bool uploadToGPU();       // Upload vertex buffer
    void draw(int baseColorLoc, int metallicLoc, int roughnessLoc, int aoLoc, int normalLoc) const; // Draw model with materials

    size_t vertexCount() const { return vertices_.size(); }

private:
    std::string path_;
    std::vector<Vertex> vertices_;
    std::vector<Submesh> submeshes_;

    std::vector<Material> materials_; // Replaces tinyobj::material_t
    std::vector<GLuint> default_textures_; // default 1x1 textures for missing maps

    // GL objects
    GLuint vao_{0};
    GLuint vbo_{0};

    static GLuint createDefaultTexture(const unsigned char color[4]);
};

#endif // MODEL_H