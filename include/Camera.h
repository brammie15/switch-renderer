#ifndef CAMERA_H
#define CAMERA_H

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <switch.h>

class Camera
{
public:
    Camera();

    void setPosition(const glm::vec3& pos) { position_ = pos; }
    const glm::vec3& position() const { return position_; }

    // Update the camera using libnx pad input. 'pad' may be nullptr in tests.
    // dt is seconds elapsed since last update.
    void update(const PadState* pad, float dt);

    // Get the view matrix (for use as view * model)
    glm::mat4 getViewMatrix() const;

    glm::vec3 getPosition(){
        return position_;
    }

private:
    glm::vec3 position_{0.0f, 0.0f, -10.0f};
    float yaw_{0.0f};   // radians
    float pitch_{0.0f}; // radians
    float moveSpeed_{3.0f};      // units per second
    float lookSensitivity_{3.0f}; // radians per second per normalized stick unit
};

#endif // CAMERA_H
