#include "render_world.h"
#include "car.h"
#include "ground.h"

#define PI 3.14

Scene world;
Camera camera;

// Movement
float velocity_translate = 4.5;

Mesh car;
Mesh car2;
Mesh terrain;

void setup(void) {
  Serial.begin(115200);

  setScreen(1500);
  
  setDirectionLight({15.0, -15.5, -30});

  // Definimos la camara de nuestra escena
  camera.position = {350, 0, -100};
  camera.rotation = {0, 0, 0};
  camera.fov = 90;

  world.camera = &camera;

  // Importación de nuestro objeto en la escena
  car.vertices = car_vertices;
  car.faces = car_faces;
  car.texcoords = car_texcoords;
  car.faces_texcoords = car_faces_texcoords;
  car.numVertices = sizeof(car_vertices)/sizeof(car_vertices[0]);
  car.numFaces = sizeof(car_faces)/sizeof(car_faces[0]);
  car.position = {100, 100, 300};
  car.rotation = {PI, PI, 0};
  car.texture = {128, 128, car4};

  car2.vertices = car_vertices;
  car2.faces = car_faces;
  car2.texcoords = car_texcoords;
  car2.faces_texcoords = car_faces_texcoords;
  car2.numVertices = sizeof(car_vertices)/sizeof(car_vertices[0]);
  car2.numFaces = sizeof(car_faces)/sizeof(car_faces[0]);
  car2.position = {-100, 100, 450};
  car2.rotation = {PI, PI, 0};
  car2.texture = {128, 128, car4};

  create_vertices();
  create_faces();

  terrain.vertices = terrain_vertices;
  terrain.faces = terrain_faces;
  terrain.texcoords = terrain_uv;
  terrain.faces_texcoords = terrain_faces_uv;

  terrain.numVertices =(TERRAIN_SIZE+1)*(TERRAIN_SIZE+1);
  terrain.numFaces = TERRAIN_SIZE*TERRAIN_SIZE*2;

  terrain.position = {500,115,1000};
  terrain.rotation = {0,0,0};

  terrain.texture = {128,128,grass};

  world.numObjects = 0;
  world.objects[world.numObjects++] = &car;
  world.objects[world.numObjects++] = &car2;
  world.objects[world.numObjects++] = &terrain;
}


void loop() {
  if ((car.position.z < -150) || (car.position.z > 1000)){
    velocity_translate = -velocity_translate;
    if (car.rotation.y == 0) car.rotation.y = PI;
    else car.rotation.y = 0;
  }

  car.position.z += velocity_translate;

  //camera.lookAt_horizontal(car.position);
  camera.follow(car, 350, 100, 250);

  renderWorld(world);
}

void create_vertices(){
  int v = 0;

  for(int z = 0; z <= TERRAIN_SIZE; z++)
  {
      for(int x = 0; x <= TERRAIN_SIZE; x++)
      {
          terrain_vertices[v] =
          {
              (int16_t)((x - TERRAIN_SIZE/2) * TILE_SIZE),
              0,
              (int16_t)((z - TERRAIN_SIZE/2) * TILE_SIZE)
          };

          terrain_uv[v] =
          {
              (float)(x * 128),
              (float)(z * 128)
          };

          v++;
      }
  }
}

void create_faces(){
  int t = 0;

  for(int z = 0; z < TERRAIN_SIZE; z++)
  {
      for(int x = 0; x < TERRAIN_SIZE; x++)
      {
          int i0 = z * (TERRAIN_SIZE + 1) + x;
          int i1 = i0 + 1;
          int i2 = i0 + TERRAIN_SIZE + 1;
          int i3 = i2 + 1;

          terrain_faces[t] = {i0, i1, i3};
          terrain_faces_uv[t] = {i0, i1, i3};
          t++;

          terrain_faces[t] = {i0, i3, i2};
          terrain_faces_uv[t] = {i0, i3, i2};
          t++;
      }
  }
}