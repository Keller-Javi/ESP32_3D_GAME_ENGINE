#ifndef RENDER_WORLD
#define RENDER_WORLD

#include <SPI.h>
#include "scene.h"
#include "engine_config.h"
#include "gfx_config.h"

void setScreen(int init_screen_time);
void initScreen(LGFX_Sprite *canvas);
void FPSScreen(LGFX_Sprite *canvas);
void setDirectionLight(Point lgt);
void prepareObject(Mesh& instance, Camera& camera);
void proyectObject(Vertex v1, Vertex v2, Vertex v3, Texture &texture, Camera& camera, Point vec_norm, int faceA);
Vertex intersectNearPlane(const Vertex& a, const Vertex& b, float nearPlane);
void renderWorld(Scene &world);
void drawTexturedTriangle(Point2D p1, Point2D p2, Point2D p3, UV uv1, UV uv2, UV uv3, const Texture& tex, uint16_t light_intensity, LGFX_Sprite *canvas);
void swapTriangles(int a, int b);
int partition(int low, int high);
void quickSort(int low, int high);
Point normalVector(int a, int b, int c);
bool faceVisible(int a, Point norm_vec);
uint16_t faceIntensity(Point norm_vec);

#endif




















