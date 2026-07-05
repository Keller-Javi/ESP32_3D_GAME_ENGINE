#include "gfx_config.h"
#include "camera.h"

void Camera::lookAt_horizontal(const Point& target){
  // Look at point without change position

  float vx = target.x - position.x;
  float vz = target.z - position.z;

  rotation.y = -atan2(vx, vz);
}

void Camera::lookAt_vertical(const Point& target){
  // Look at point without change position

  float vy = target.y - position.y;
  float vz = target.z - position.z;

  rotation.y = -atan2(vy, vz);
}

void Camera::lookAt(const Point& target){
  // Look at point without change position

  float vx = target.x - position.x;
  float vy = target.y - position.y;
  float vz = target.z - position.z;

  rotation.y = -atan2(vx, vz);
  rotation.x = -atan2(vy, vz);
}

void Camera::follow(const Mesh& target, const float distance, float offsetX, float offsetY){
  // Follow an object changing position and rotation, like Third Person Character
  position.x = target.position.x - offsetX;
  position.y = target.position.y - offsetY;
  position.z = target.position.z - distance;

  lookAt(target.position);
}