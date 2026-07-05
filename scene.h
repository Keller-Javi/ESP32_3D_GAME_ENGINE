#ifndef SCENE
#define SCENE

#include "engine_config.h"
#include "camera.h"

struct Scene
{
  Mesh* objects[MAX_OBJECTS];
  int numObjects;

  Camera* camera;
};

#endif