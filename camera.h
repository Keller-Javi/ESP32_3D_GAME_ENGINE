#ifndef CAMERA
#define CAMERA

#include "engine_types.h"
#include <Arduino.h>


class Camera
{
public:
    Point position;
    Point rotation;
    float fov;

    void lookAt(const Point& target);
    void lookAt_vertical(const Point& target);
    void lookAt_horizontal(const Point& target);

    void follow(const Mesh& target, const float distance, float offsetX = 0.0f, float offsetY = 0.0f); // void Camera::follow(const Mesh& target, const float distance, float offsetX = 0, float offsetY = 0)
};

#endif