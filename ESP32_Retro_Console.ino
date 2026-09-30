#include "render_world.h"
#include "assets.h"
#include "terrain.h"

#define BTN_BLUR 15
#define BTN_CEL_SHADING 16
#define BTN_ANTIALIAS 17
#define BTN_BLOOM 18

#define PI 3.14

Scene world;
Camera camera;
Terrain terrain;

bool lastAntialiasButton = HIGH;
bool lastCelButton       = HIGH;
bool lastBloomButton     = HIGH;
bool lastBlurButton      = HIGH;

// Movement
float velocity_translate = 4.0;

Mesh car;
Mesh car2;

void setup(void) {
  Serial.begin(115200);

  pinMode(BTN_ANTIALIAS, INPUT_PULLUP);
  pinMode(BTN_CEL_SHADING, INPUT_PULLUP);
  pinMode(BTN_BLOOM, INPUT_PULLUP);
  pinMode(BTN_BLUR, INPUT_PULLUP);

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
  car2.position = {-100, 100, -150};
  car2.rotation = {PI, PI, 0};
  car2.texture = {128, 128, car4};

  terrain.mesh.texture = {128,128,grass};
  terrain.create(5, 5, 750);
  terrain.mesh.position = {500,125,250};

  world.numObjects = 0;
  world.objects[world.numObjects++] = &car;
  world.objects[world.numObjects++] = &car2;
  world.objects[world.numObjects++] = &terrain.mesh;
}


void loop() {
  // =========================
  // Botones de configuración
  // =========================

  bool antialiasButton = digitalRead(BTN_ANTIALIAS);
  bool celButton       = digitalRead(BTN_CEL_SHADING);
  bool bloomButton     = digitalRead(BTN_BLOOM);
  bool blurButton     = digitalRead(BTN_BLUR);

  // Detectar nueva pulsación
  if (lastAntialiasButton == HIGH && antialiasButton == LOW) {
    antialias++;

    if (antialias == 4) antialias = 0;
  }

  if (lastCelButton == HIGH && celButton == LOW) {
    cel_shading++;

    if (cel_shading == 3) cel_shading = 0;
  }

  if (lastBloomButton == HIGH && bloomButton == LOW) {
    bloom++;

    if (bloom == 3) bloom = 0;
  }

  if (lastBlurButton == HIGH && blurButton == LOW) {
    blur++;

    if (blur == 3) blur = 0;
  }

  lastAntialiasButton = antialiasButton;
  lastCelButton       = celButton;
  lastBloomButton     = bloomButton;
  lastBlurButton      = blurButton;

  // =====================================================
  // LÓGICA DEL JUEGO
  // =====================================================
  if ((car.position.z < -800) || (car.position.z > 700)){
    velocity_translate = -velocity_translate;
    if (car.rotation.y == 0) car.rotation.y = PI;
    else car.rotation.y = 0;
  }

  car.position.z += velocity_translate;

  camera.follow(car, 250, 100, 200);
	
	update(world);

  // Pequeña pausa o delay de framerate para no saturar el bus
  vTaskDelay(pdMS_TO_TICKS(16)); // ~60 FPS para la lógica
}

