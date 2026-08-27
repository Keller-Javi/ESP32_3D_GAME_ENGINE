#ifndef GROUND
#define GROUND 

#include <Arduino.h>
#include "engine_types.h"
#include <esp_heap_caps.h>

class Terrain
{
public:
    Mesh mesh;

    void create(int width, int height, int tileSize);
private:
  int width;      // cantidad de tiles en X
  int height;     // cantidad de tiles en Z
  int tileSize;

  Point16* vertices;
  UV* uv;
  Triangle* faces;
  Triangle* facesUV;

  void create_vertices();
  void create_faces();
};

#endif