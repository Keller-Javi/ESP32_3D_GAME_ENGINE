# DOCUMENTACIÓN

Aquí se explica de manera breve cómo utilizar este motor gráfico. 
Lo único que debe hacer es clonar este repositorio y trabajar el código del archivo .ino, utilizando las características que se van a mencionar más adelante.

## Variables de inicio:

Esto cambia según tu pantalla:

```cpp
// SCREEN
#define HEIGHT            240 // Resolution to render
#define WIDTH             240 // Resolution to render
#define CENTER_X          120 // Half of resolution to render
#define CENTER_Y          120 // Half of resolution to render
#define COLOR_DEPTH       16  // Don't change this, because I work with pointers of 16 bits.
```

Aquí van las variables predefinidas de la escena, estas debes cambiarlas para lograr lo que buscas entre cantidad y rendimiento:

```cpp
// SCENE
#define BACKGROUND        TFT_BLACK
#define MINIMUM_BRIGHTNESS  0.2
#define MAX_VERTICES      300
#define MAX_TRIANGLES     600
#define MAX_OBJECTS       5
```

BACKGROUND → color de fondo de la escena

MINIMUM_BRITNESS → qué tan oscura va a ser la escena, en este caso, la iluminación va desde el 20% al 100 %

MAX_VERTICES → debe ser el máximo de vértices respecto al objeto más grande 

MAX_TRIANGLES → máximo de triángulos por escena

MAX_OBJECTS → máximo de objetos por escena


## Como van los #include:

```#include "render_world.h"``` → Importa todo lo que se necesita para la escena, objetos, etc.

```#include "terrain.h"``` -> En este tenemos la generación de un terreno (está en proceso).

```#include "assets.h"``` -> Se recomienda poner los assets (como modelos 3D y texturas) en uno o más archivos .h



## Objetos:

### Estructuras primitivas:

Esto se usa para calcular las posiciones dentro del mundo:

```cpp
struct Point {
  float  x;
  float  y;
  float  z;
};
```

Los modelos 3D se considera usar punto fijo para reducir el uso de memoria:

```cpp
struct Point16 // Este formato es para ahorrar memoria a costa de precisión
{
    int16_t x;
    int16_t y;
    int16_t z;
};
```

Para las caras de los modelos 3D tenemos:

```cpp
struct Triangle {
    int a;
    int b;
    int c;
};
```

Para darle la textura a nuestro modelo:

```cpp
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
```

### Objetos de la escena:

Para hacer un juego necesitas primero definir los objetos de la escena con la estructura Mesh:

```cpp
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
```

Con un script en Python convierto un modelo 3D .obj en formato .c y tengo las siguientes variables:

```cpp
Point16 car_vertices[] = {...};
UV car_texcoords[] = {...};
Triangle car_faces[] = {...};
Triangle car_faces_texcoords[] = {...};
uint16_t car4[] = {...};
```

En la función setup() creo el objeto de la siguiente forma:

```cpp
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
```

### Escena:

Por otro lado, tenemos al mundo o la escena:

```cpp
struct Scene
{
  Mesh* objects[MAX_OBJECTS];
  int numObjects;
  
  Camera* camera;
};
```

su manera de usar es definiendo de manera global: 

```
Scene world;
```

Y luego agregar los objetos de la siguiente manera:

```cpp
world.numObjects = 0; // La primera vez que definimos mejor ponerlo a cero
world.objects[world.numObjects++] = &car;
```

### Cámara:

Esta es la cámara de la escena:

```cpp
class Camera
{
public:
    Point position;
    Point rotation;
    float fov;

    void lookAt(const Point& target);
    void lookAt_vertical(const Point& target);
    void lookAt_horizontal(const Point& target);

    void follow(const Mesh& target, const float distance, float offsetX = 0.0f, float offsetY = 0.0f);
};
```

Para la escena debes crear un objeto Cámara:

```
Camera camera;
```

para luego asignarle en el Setup():

```
world.camera = &camera;
```

Por otro lado tenemos las funciones, lookAt() cámara fija que mira a una posición con sus variantes horizontal y vertical. Luego follow() que sigue a un objetivo con una perspectiva fija en este.

Ejemplo de uso de una de las funciones:

```
camera.follow(car, 350, 100, 250);
```

## Funciones:

void setScreen(int init_screen_time); → Prepara la pantalla creando el canvas, inicializando el DMA y muestra una pantalla de inicio según el tiempo definido en la variable init_screen_time (valor en ms).

void setDirectionLight(Point lgt); → Para definir la dirección del sol en la escena.

void renderWorld(Scene &world); → Se debe llamar en el loop para renderizar el mundo.

Ejemplo de cómo se ve el loop:

```cpp
void loop() {
	// Lógica del juego
  if ((car.position.z < -150) || (car.position.z > 1000)){
    velocity_translate = -velocity_translate;
    if (car.rotation.y == 0) car.rotation.y = PI;
    else car.rotation.y = 0;
  }

  car.position.z += velocity_translate;

  camera.follow(car, 350, 100, 250);
	
	//Actualización de la escena
  renderWorld(world);
}
```