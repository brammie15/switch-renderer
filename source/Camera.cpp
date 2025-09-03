#include "Camera.h"
#include <switch.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>

Camera::Camera() {}

glm::mat4 Camera::getViewMatrix() const
{
    // Calculate forward vector from yaw/pitch
    glm::vec3 forward;
    forward.x = cosf(pitch_) * sinf(yaw_);
    forward.y = sinf(pitch_);
    forward.z = cosf(pitch_) * cosf(yaw_);
    forward   = glm::normalize(forward);

    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 center = position_ + forward;
    return glm::lookAt(position_, center, up);
}

void Camera::update(const PadState* pad, float dt)
{
    if (!pad || dt <= 0.0f)
        return;

    // Read left and right stick positions from libnx
    HidAnalogStickState ls = padGetStickPos(pad, 0);
    HidAnalogStickState rs = padGetStickPos(pad, 1);

    // Normalize (HidAnalogStickState uses s32, range approximately -32768..32767)
    const float norm = 32768.0f;
    float lx = ls.x / norm;
    float ly = ls.y / norm;
    float rx = rs.x / norm;
    float ry = rs.y / norm;

    // Deadzone
    const float deadzone = 0.15f;
    if (fabsf(lx) < deadzone) lx = 0.0f;
    if (fabsf(ly) < deadzone) ly = 0.0f;
    if (fabsf(rx) < deadzone) rx = 0.0f;
    if (fabsf(ry) < deadzone) ry = 0.0f;

    // Rotation from right stick
    yaw_   += -rx * lookSensitivity_ * dt;
    pitch_ += ry * lookSensitivity_ * dt; // fixed: no extra inversion
    const float maxPitch = glm::radians(89.0f);
    if (pitch_ > maxPitch) pitch_ = maxPitch;
    if (pitch_ < -maxPitch) pitch_ = -maxPitch;

    // Movement from left stick (relative to camera orientation)
    glm::vec3 forward;
    forward.x = cosf(pitch_) * sinf(yaw_);
    forward.y = 0.0f; // restrict vertical stick movement
    forward.z = cosf(pitch_) * cosf(yaw_);
    forward   = glm::normalize(forward);

    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

    // fixed: remove negative ly
    position_ += (forward * ly + right * lx) * moveSpeed_ * dt;

    // Vertical movement using ZL / ZR
    u64 buttons = padGetButtons(pad);
    if (buttons & HidNpadButton_ZL)
        position_.y -= moveSpeed_ * dt;
    if (buttons & HidNpadButton_ZR)
        position_.y += moveSpeed_ * dt;

    // Quick reset to origin with Y + A
    if ((buttons & HidNpadButton_Y) && (buttons & HidNpadButton_A)) {
        position_ = glm::vec3(0.0f, 0.0f, 0.0f);
        yaw_ = 0.0f;
        pitch_ = 0.0f;
    }

    // Debug: print camera position
    // printf("Camera position: %f, %f, %f\n", position_.x, position_.y, position_.z);
    // printf("Camera Rotation: %f, %f\n", yaw_, pitch_);
}
