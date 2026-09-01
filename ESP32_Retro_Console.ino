#include "render_world.h"
#include "assets.h"
#include "terrain.h"

#define PI 3.14

Scene world;
Camera camera;
Terrain terrain;

// Handle para la tarea de renderizado
TaskHandle_t RenderTaskHandle = NULL;

// Tarea exclusiva del Core 0
void RenderTask(void *pvParameters) {
  for (;;) {
    // 1. Espera a que el Core 1 le ordene dibujar
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    // 2. Ejecuta el pipeline completo de render y display
    renderWorld(world);

    // 3. Cede control para alimentar al Watchdog del Core 0
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

// Movement
float velocity_translate = 4.0;

Mesh car;
Mesh car2;

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
  car2.position = {-100, 100, -150};
  car2.rotation = {PI, PI, 0};
  car2.texture = {128, 128, car4};

  terrain.mesh.texture = {128,128,grass};
  terrain.create(20, 20, 200);
  terrain.mesh.position = {500,125,250};

  world.numObjects = 0;
  world.objects[world.numObjects++] = &car;
  world.objects[world.numObjects++] = &car2;
  world.objects[world.numObjects++] = &terrain.mesh;

  // Crear la tarea anclada al Core 0
  xTaskCreatePinnedToCore(
    RenderTask,         // Función
    "RenderEngineTask", // Nombre
    8192,               // Stack size en palabras/bytes
    NULL,               // Parámetros
    1,                  // Prioridad
    &RenderTaskHandle,  // Handle
    0                   // Core ID (0)
  );
}


void loop() {
	// Lógica del juego
  if ((car.position.z < -1200) || (car.position.z > 700)){
    velocity_translate = -velocity_translate;
    if (car.rotation.y == 0) car.rotation.y = PI;
    else car.rotation.y = 0;
  }

  car.position.z += velocity_translate;

  camera.follow(car, 350, 100, 250);
	
	//Actualización de la escena
  // --- Sincronización con Core 0 ---
  if (RenderTaskHandle != NULL) {
    xTaskNotifyGive(RenderTaskHandle); // Dispara el render
  }

  // Pequeña pausa o delay de framerate para no saturar el bus
  vTaskDelay(pdMS_TO_TICKS(16)); // ~60 FPS para la lógica
}