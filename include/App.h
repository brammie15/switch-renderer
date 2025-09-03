#ifndef APP_H
#define APP_H

#include "Model.h"
#include <EGL/egl.h>
#include <memory>
#include <switch.h>
#include <Shader.h>
#include <Camera.h>

class App
{
public:
    App();
    ~App();

    bool init();
    void run();
    void shutdown();

private:
    bool initEgl(NWindow* win);
    void deinitEgl();

    void sceneInit();
    void sceneUpdate();
    void sceneRender();
    void sceneExit();

    float getTime() const;

private:
    EGLDisplay s_display_{nullptr};
    EGLContext s_context_{nullptr};
    EGLSurface s_surface_{nullptr};

    GLuint program_{0};

    GLint loc_mdlvMtx{-1};
    GLint loc_viewMtx{-1};
    GLint loc_projMtx{-1};

    GLint loc_cameraPos{-1};
    GLint loc_lightDir{-1};
    GLint loc_lightColor{-1};

    std::unique_ptr<Model> model_;
    std::unique_ptr<Shader> shader_;

    // std::string modelPath_{"romfs:/cat/cat.obj"};
    // std::string modelPath_{"/switch/models/cat_cube/cat_cube.obj"};
    std::string modelPath_{"/switch/models/fire_hydrant/FireHydrantMesh.obj"};

    PadState pad_{};
    Camera camera_;

    u64 s_startTicks{0};

    glm::vec3 lightDir_{0.0f, -0.5f, -1.0f}; 
    float lightSpeed_ = 1.0f;
    bool rotateModel_ = true;
};

#endif // APP_H