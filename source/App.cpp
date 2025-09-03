#include "App.h"
#include <switch.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>

#include <EGL/egl.h>    // EGL library
#include <EGL/eglext.h> // EGL extensions
#include <glad/glad.h>  // glad library (OpenGL loader)


App::App() {}
App::~App() { shutdown(); }

static void setMesaConfig()
{
    // optional env tweaks (kept from original sample)
}


bool App::initEgl(NWindow* win)
{
    s_display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!s_display_)
    {
        printf("eglGetDisplay failed: %d\n", eglGetError());
        return false;
    }
    eglInitialize(s_display_, nullptr, nullptr);
    if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE)
    {
        printf("eglBindAPI failed: %d\n", eglGetError());
        return false;
    }

    EGLConfig config;
    EGLint numConfigs;
    static const EGLint framebufferAttributeList[] =
    {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_RED_SIZE,     8,
        EGL_GREEN_SIZE,   8,
        EGL_BLUE_SIZE,    8,
        EGL_ALPHA_SIZE,   8,
        EGL_DEPTH_SIZE,   24,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };
    eglChooseConfig(s_display_, framebufferAttributeList, &config, 1, &numConfigs);
    if (numConfigs == 0)
    {
        printf("No EGL config found\n");
        return false;
    }

    s_surface_ = eglCreateWindowSurface(s_display_, config, win, nullptr);
    if (!s_surface_)
    {
        printf("eglCreateWindowSurface failed: %d\n", eglGetError());
        return false;
    }

    static const EGLint contextAttributeList[] =
    {
        EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
        EGL_CONTEXT_MAJOR_VERSION_KHR, 4,
        EGL_CONTEXT_MINOR_VERSION_KHR, 3,
        EGL_NONE
    };
    s_context_ = eglCreateContext(s_display_, config, EGL_NO_CONTEXT, contextAttributeList);
    if (!s_context_)
    {
        printf("eglCreateContext failed: %d\n", eglGetError());
        eglDestroySurface(s_display_, s_surface_);
        s_surface_ = nullptr;
        return false;
    }

    eglMakeCurrent(s_display_, s_surface_, s_surface_, s_context_);
    return true;
}

void App::deinitEgl()
{
    if (s_display_)
    {
        eglMakeCurrent(s_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (s_context_)
        {
            eglDestroyContext(s_display_, s_context_);
            s_context_ = nullptr;
        }
        if (s_surface_)
        {
            eglDestroySurface(s_display_, s_surface_);
            s_surface_ = nullptr;
        }
        eglTerminate(s_display_);
        s_display_ = nullptr;
    }
}

bool App::init()
{
    setMesaConfig();

    // Initialize EGL
    if (!initEgl(nwindowGetDefault()))
        return false;

    // Load GL function pointers
    gladLoadGL();

    // Load shaders from files (paths inside romfs)
    shader_ = std::make_unique<Shader>();
    if (!shader_->loadFromFiles("romfs:/shaders/vertex.glsl", "romfs:/shaders/fragment.glsl"))
    {
        printf("Failed to load/compile/link shaders\n");
        return false;
    }
    program_ = shader_->program();

    loc_mdlvMtx = glGetUniformLocation(program_, "uModel");
    loc_viewMtx = glGetUniformLocation(program_, "uView");
    loc_projMtx = glGetUniformLocation(program_, "uProj");

    loc_cameraPos = glGetUniformLocation(program_, "uCamPos");
    loc_lightDir = glGetUniformLocation(program_, "uLightDir");
    loc_lightColor = glGetUniformLocation(program_, "uLightColor");

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Setup projection uniform
    glUseProgram(program_);
    auto projMtx = glm::perspective(90.0f * glm::two_pi<float>() / 360.0f, 1280.0f / 720.0f, 0.01f, 1000.0f);
    glUniformMatrix4fv(loc_projMtx, 1, GL_FALSE, glm::value_ptr(projMtx));

    auto modelMtx = glm::mat4(1.0f);
    modelMtx = glm::scale(modelMtx, glm::vec3(0.5f, 0.5f, 0.5f));
    glUniformMatrix4fv(loc_mdlvMtx, 1, GL_FALSE, glm::value_ptr(modelMtx));

    glUniform3f(loc_cameraPos, camera_.getPosition().x, camera_.getPosition().y, camera_.getPosition().z);
    glUniform3f(loc_lightDir, 0.0f, -0.5f, -1.0f);
    glUniform3f(loc_lightColor, 1.0f, 1.0f, 1.0f);

    s_startTicks = armGetSystemTick();

    // Initialize input here so Camera can use it
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad_);

    // Load model
    model_ = std::make_unique<Model>(modelPath_);
    if (!model_->load())
    {
        printf("Failed to load model: %s\n", modelPath_.c_str());
        return false;
    }
    if (!model_->uploadToGPU())
    {
        printf("Failed to upload model to GPU\n");
        return false;
    }

    return true;
}

float App::getTime() const
{
    u64 elapsed = armGetSystemTick() - s_startTicks;
    return (elapsed * 625 / 12) / 1000000000.0f;
}

void App::sceneUpdate()
{
    auto projMtx = glm::perspective(45.0f * glm::two_pi<float>() / 360.0f, 1280.0f / 720.0f, 0.01f, 1000.0f);

    auto viewMtx = camera_.getViewMatrix();

    glm::mat4 model{1.0f};
    if (rotateModel_)
        model = glm::rotate(model, getTime() * glm::two_pi<float>() * 0.234375f / 2.0f, glm::vec3{0.0f, 1.0f, 0.0f});

    glUseProgram(program_);
    glUniformMatrix4fv(loc_viewMtx, 1, GL_FALSE, glm::value_ptr(viewMtx));
    glUniformMatrix4fv(loc_projMtx, 1, GL_FALSE, glm::value_ptr(projMtx));
    glUniformMatrix4fv(loc_mdlvMtx, 1, GL_FALSE, glm::value_ptr(model));


    glUniform3f(loc_cameraPos, camera_.getPosition().x, camera_.getPosition().y, camera_.getPosition().z);
}

void App::sceneRender()
{
    glClearColor(0x68 / 255.0f, 0xB0 / 255.0f, 0xD8 / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader_->use();

    GLuint baseColor = shader_->getUniformLocation("texBaseColor");
    GLuint metallic = shader_->getUniformLocation("texMetallic");
    GLuint roughness = shader_->getUniformLocation("texRoughness");
    GLuint ao = shader_->getUniformLocation("texAO");
    GLuint normal = shader_->getUniformLocation("texNormal");

    model_->draw(baseColor, metallic, roughness, ao, normal);
}

void App::sceneExit()
{
    if (program_)
    {
        glDeleteProgram(program_);
        program_ = 0;
    }
}

void App::shutdown()
{
    sceneExit();
    deinitEgl();
}

void App::run()
{
    // pad_ was initialized in init()
    float lastTime = getTime();

    while (appletMainLoop())
    {
        // Update pad
        padUpdate(&pad_);
        u32 kDown = padGetButtonsDown(&pad_);
        if (kDown & HidNpadButton_Plus)
            break;

       
        // Time
        float now = getTime();
        float dt = now - lastTime;
        if (dt < 0.0f) dt = 0.0f;
        lastTime = now;

         if (padGetButtons(&pad_) & HidNpadButton_Up)
            lightDir_.y += lightSpeed_ * dt;
        if (padGetButtons(&pad_) & HidNpadButton_Down)
            lightDir_.y -= lightSpeed_ * dt;
        if (padGetButtons(&pad_) & HidNpadButton_Left)
            lightDir_.x -= lightSpeed_ * dt;
        if (padGetButtons(&pad_) & HidNpadButton_Right)
            lightDir_.x += lightSpeed_ * dt;

        // Normalize so light direction stays consistent
        lightDir_ = glm::normalize(lightDir_);

        // Upload to shader
        glUseProgram(program_);
        glUniform3f(loc_lightDir, lightDir_.x, lightDir_.y, lightDir_.z);

                // Toggle rotation with X button
        if (kDown & HidNpadButton_X)
            rotateModel_ = !rotateModel_;


        // Update camera with current pad state
        camera_.update(&pad_, dt);

        // Update scene (model + upload model-view matrix)
        sceneUpdate();

        // Render
        sceneRender();
        eglSwapBuffers(s_display_, s_surface_);
    }
}
