#include "render_world.h"
#include "car.h"

#define PI 3.14

Scene world;
Camera camera;

// Movement
float velocity_translate = 4.5;

Mesh car;
Mesh car2;

void setup(void) {
  Serial.begin(115200);

  setScreen(1500);
  
  setDirectionLight({15.0, 15.5, -30});

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

  world.numObjects = 0;
  world.objects[world.numObjects++] = &car;
  world.objects[world.numObjects++] = &car2;
}


void loop() {
  if ((car.position.z < 290) || (car.position.z > 2500)){
    velocity_translate = -velocity_translate;
    if (car.rotation.y == 0) car.rotation.y = PI;
    else car.rotation.y = 0;
  }

  car.position.z += velocity_translate;
  
  float angle = -atan2(car.position.x - camera.position.x, car.position.z - camera.position.z);

  camera.rotation.y = angle; 

  renderWorld(world);
}