#include "terrain.h"

void Terrain::create(int width, int height, int tileSize)
{
  this->width = width;
  this->height = height;
  this->tileSize = tileSize;

  Serial.printf("width=%d height=%d\n", width, height);
  Serial.printf("numVertices=%d\n", (width+1)*(height+1));
  Serial.printf("numFaces=%d\n", width*height*2);

  vertices = (Point16*)heap_caps_malloc(
    (width + 1) * (height + 1) * sizeof(Point16), MALLOC_CAP_SPIRAM);
  
  uv = (UV*)heap_caps_malloc(
    (width + 1) * (height + 1) * sizeof(UV), MALLOC_CAP_SPIRAM);

  faces = (Triangle*)heap_caps_malloc(
    width * height * 2 * sizeof(Triangle), MALLOC_CAP_SPIRAM);

  facesUV = (Triangle*)heap_caps_malloc(
    width * height * 2 * sizeof(Triangle), MALLOC_CAP_SPIRAM);

  create_vertices();
  create_faces();

  mesh.vertices = vertices;
  mesh.faces = faces;
  mesh.texcoords = uv;
  mesh.faces_texcoords = facesUV;

  mesh.numVertices = (width + 1) * (height + 1);
  mesh.numFaces = width * height * 2;
}

void Terrain::create_vertices(){
  int v = 0;

  for(int z = 0; z <= height; z++)
  {
      for(int x = 0; x <= width; x++)
      {
          vertices[v] =
          {
              (int16_t)((x - width/2) * tileSize),
              0,
              (int16_t)((z - height/2) * tileSize)
          };

          uv[v] =
          {
              (float)(x * mesh.texture.width),
              (float)(z * mesh.texture.height)
          };

          v++;
      }
  }
}

void Terrain::create_faces(){
  int t = 0;

  for(int z = 0; z < height; z++)
  {
      for(int x = 0; x < width; x++)
      {
          int i0 = z * (width + 1) + x;
          int i1 = i0 + 1;
          int i2 = i0 + width + 1;
          int i3 = i2 + 1;

          faces[t] = {i0, i1, i3};
          facesUV[t] = {i0, i1, i3};
          t++;

          faces[t] = {i0, i3, i2};
          facesUV[t] = {i0, i3, i2};
          t++;
      }
  }
}