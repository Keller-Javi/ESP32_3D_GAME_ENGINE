#include "render_world.h"
#include "car.h"

Scene world;

// Movement
float velocity = 2.5;

Mesh car;

void setup(void) {
  Serial.begin(115200);

  setScreen(1500);
  
  setDirectionLight({15.0, 15.5, -30});

  // Importación de nuestro objeto en la escena
  car.vertices = car_vertices;
  car.faces = car_faces;
  car.texcoords = car_texcoords;
  car.faces_texcoords = car_faces_texcoords;
  car.numVertices = sizeof(car_vertices)/sizeof(car_vertices[0]);
  car.numFaces = sizeof(car_faces)/sizeof(car_faces[0]);
  car.position = {0, 100, 250};
  car.rotation = {3.14, 0, 0};
  car.texture = {128, 128, car4};

  world.numObjects = 0;
  world.objects[world.numObjects++] = &car;
}


void loop() {
  if ((car.position.x < -100) || (car.position.x > 100)) velocity = -velocity;
  
  car.position.x += velocity;
  car.rotation.y += 0.025;

  renderWorld(world);
}