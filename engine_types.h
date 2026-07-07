#ifndef ENGINE_TYPES
#define ENGINE_TYPES

struct Point {
  float  x;
  float  y;
  float  z;
};

struct Point16 // Este formato es para ahorrar memoria a costa de presición
{
    int16_t x;
    int16_t y;
    int16_t z;
};

struct Triangle {
    int a;
    int b;
    int c;
};

struct Point2D {
  int x;
  int y;
};

struct Color { // HASTA EL MOMENTO NO SE USA, ELIMINAR SI NO HACE FALTA
  uint16_t r;
  uint16_t g;
  uint16_t b;
};

struct Texture
{
    uint16_t width;
    uint16_t height;

    const uint16_t* pixels;
};

struct UV
{
    uint8_t u; // Lo del formato es para ahorrar memoria
    uint8_t v;
};

struct Vertex
{
    Point position;
    UV uv;
};

struct Mesh
{
    Point16* vertices;
    Triangle* faces;
    UV* texcoords;
    Triangle* faces_texcoords;

    Texture texture;

    int numVertices;
    int numFaces;

    Point position;
    Point rotation;
};

struct RenderTriangle
{
    Point2D p1;
    Point2D p2;
    Point2D p3;

    UV uv1;
    UV uv2;
    UV uv3;

    float depth;
    
    Texture* texture;

    uint16_t light_intensity;
};
#endif