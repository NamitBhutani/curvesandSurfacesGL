#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum KeyInput
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
};

const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float ROLL_SPEED = 50.0f;
const float SENSITIVITY = 0.05f;
const float ZOOM = 45.0f;

class Camera
{
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;
    float yaw;
    float pitch;
    float roll;
    float speed;
    float rollSpeed;
    float sensitivity;
    float zoom;

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : position(position), worldUp(up), yaw(yaw), pitch(pitch), roll(0.0f), front(glm::vec3(0.0f, 0.0f, -1.0f)), speed(SPEED), rollSpeed(ROLL_SPEED), sensitivity(SENSITIVITY), zoom(ZOOM)
    {
        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(position, position + front, up);
    }

    void ProcessKeyboard(KeyInput direction, float deltaTime)
    {
        float velocity = speed * deltaTime;
        if (direction == FORWARD)
            position += front * velocity;
        if (direction == BACKWARD)
            position -= front * velocity;
        if (direction == LEFT)
            position -= right * velocity;
        if (direction == RIGHT)
            position += right * velocity;
    }

    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        yaw += xoffset * sensitivity;
        pitch += yoffset * sensitivity;

        if (constrainPitch)
        {
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
        }

        updateCameraVectors();
    }

    void ProcessMouseScroll(float yoffset)
    {
        zoom -= yoffset;
        if (zoom < 1.0f)
            zoom = 1.0f;
        if (zoom > 45.0f)
            zoom = 45.0f;
    }

    void ProcessRoll(float offset)
    {
        roll += offset * rollSpeed;
        if (roll > 180.0f)
            roll -= 360.0f;
        if (roll < -180.0f)
            roll += 360.0f;
        updateCameraVectors();
    }

private:
    void updateCameraVectors()
    {
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(front);
        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));

        if (roll != 0.0f)
        {
            glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(roll), front);
            glm::vec3 upRot = glm::normalize(glm::vec3(rot * glm::vec4(up, 0.0f)));
            right = glm::normalize(glm::cross(front, upRot));
            up = glm::normalize(glm::cross(right, front));
        }
    }
};
#endif