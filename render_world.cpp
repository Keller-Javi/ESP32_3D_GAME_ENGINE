#include "render_world.h"

// LIGHT 
Point light = {15.0, -1.5, -3};

// BUFFERING 
static LGFX lcd;
LGFX_Sprite canvas[2] = { LGFX_Sprite(&lcd), LGFX_Sprite(&lcd) };
uint8_t bufferIdx = 0;

// TRANSFORM - PROYECT
Point rotated[MAX_VERTICES];
RenderTriangle renderList[MAX_TRIANGLES];
float near = 1.0f;

int trianglesCount = 0;

// Benchmark vars
unsigned long fpsTimer = 0;
unsigned long frameCount = 0;
int fps = 0;
int visibleTriangles = 0;
int trianglesCountPerSec = 0;

int descartadosZ = 0;
int descartadosBackface = 0;
int descartadosArea = 0;

void setScreen(int init_screen_time)
{
  lcd.init();
  lcd.setRotation(1);

  for (int i = 0; i < 2; i++) {
    canvas[i].setPsram(true); // <--- Obliga a LovyanGFX a alojarlo en PSRAM
    canvas[i].createSprite(HEIGHT, WIDTH);
  }

  lcd.setColorDepth(COLOR_DEPTH);

  initScreen(&canvas[0]);
  delay(init_screen_time);
}

void initScreen(LGFX_Sprite* canvas)
{
  canvas->fillScreen(0x0000);

  canvas->setTextColor(0xFFFF);
  canvas->setTextSize(2);
  canvas->setCursor(70, CENTER_X);
  canvas->println("HELLO WORLD!");
  canvas->setTextColor(TFT_RED);
  canvas->setCursor(50, CENTER_X + 20);
  canvas->println("...............");

  canvas->pushSprite(0, 0);
}

void FPSScreen(LGFX_Sprite* canvas)
{
  canvas->setTextColor(0xFFFF);
  canvas->setTextSize(2);
  canvas->setCursor(0, 0);
  canvas->print("FPS: ");
  canvas->println(fps);

  canvas->setCursor(0, (WIDTH-20));
  canvas->print("Triangles: ");
  canvas->println(visibleTriangles / fps);
}

void setDirectionLight(Point lgt)
{
  light = lgt;
  float len = sqrt(light.x*light.x + light.y*light.y + light.z*light.z);
  light.x /= len;
  light.y /= len;
  light.z /= len;
}

void prepareObject(Mesh& instance, Camera& camera)
{
  // Rotaciones del objeto 3D
  float sin_x = sin(instance.rotation.x);
  float cos_x = cos(instance.rotation.x);

  float sin_y = sin(instance.rotation.y);
  float cos_y = cos(instance.rotation.y);

  // Rotaciones de la camara
  float sin_cam_y = sin(-camera.rotation.y);
  float cos_cam_y = cos(-camera.rotation.y);

  float sin_cam_x = sin(-camera.rotation.x);
  float cos_cam_x = cos(-camera.rotation.x);

  for (int i = 0; i < instance.numVertices; i++) {
    float x = instance.vertices[i].x;
    float y = instance.vertices[i].y;
    float z = instance.vertices[i].z;
    float xr = x;
    float yr = y;
    float zr = z;

    // Rotación X
    yr = y * cos_x - z * sin_x;
    zr = y * sin_x + z * cos_x; 
    
    // Rotación Y
    xr = x * cos_y - zr * sin_y;
    zr = x * sin_y + zr * cos_y;

    // Para la posición del objeto
    xr += instance.position.x;
    yr += instance.position.y;
    zr += instance.position.z;

    // Para la posición de la camara
    xr -= camera.position.x;
    yr -= camera.position.y;
    zr -= camera.position.z;

    // Rotación del objeto respecto a la camara
    float tx = xr * cos_cam_y - zr * sin_cam_y;
    float tz = xr * sin_cam_y + zr * cos_cam_y;

    xr = tx;
    zr = tz;

    float ty = yr * cos_cam_x - zr * sin_cam_x;
    tz = yr * sin_cam_x + zr * cos_cam_x;

    yr = ty;
    zr = tz;

    rotated[i].x = xr;
    rotated[i].y = yr;
    rotated[i].z = zr;
  }

  // Proyección y dibujo en pantalla
  for (int i = 0; i < instance.numFaces; i++) {

    if (trianglesCount >= MAX_TRIANGLES){
      Serial.println("OVERFLOW");
      break;
    }

    // Primero verifico si el triangulo no está fuera de la pantalla. ESTO GENERA ERRORES VISUALES A VECES
    // El z < 1 ya es en base a la camara, por lo que si se sale de la camara no dibuja ese triangulo.
    int cont = 0;
    
    Vertex v1;
    Vertex v2;
    Vertex v3;

    v1.position = rotated[instance.faces[i].a];
    v2.position = rotated[instance.faces[i].b];
    v3.position = rotated[instance.faces[i].c];

    v1.uv = instance.texcoords[instance.faces_texcoords[i].a];
    v2.uv = instance.texcoords[instance.faces_texcoords[i].b];
    v3.uv = instance.texcoords[instance.faces_texcoords[i].c];

    if(v1.position.z < 1) cont++;
    if(v2.position.z < 1) cont++;
    if(v3.position.z < 1) cont++;

    if(cont == 3) {descartadosZ++; continue;} // los tres vertices fuera de la camara
    else if(cont == 1) { // Un solo vertice fuera de la camara
      // Primero ordenamos A < B < C
      /*   A
          /|
         / |
        /  |
       B   |
       \   |
      --P--Q--- Near Plane --> Z = 1
         \ |
           C
      Buscamos ordenar tal que C < Near Plane y A > B > Near Plane */
      if (v1.position.z < v2.position.z){
        Vertex aux = v1;
        v2 = v1;
        v1 = aux;
      }
      
      if (v2.position.z < v3.position.z){
        Vertex aux = v2;
        v2 = v3;
        v3 = aux;
      }
      
      if (v1.position.z < v2.position.z){
        Vertex aux = v1;
        v2 = v1;
        v1 = aux;
      }

      // Calculamos la intersección con el Near Plane
      Vertex P = intersectNearPlane(v1, v3, near);
      Vertex Q = intersectNearPlane(v2, v3, near);
      /*Acá tenemos:
           A
          /|
         / |
        /  |
       B   |
       \   |
        P--Q
      Un triangulo se forma entre A, P y B
      Y el otro entre P, Q y B*/

      Point vec_norm = normalVector(instance.faces[i].a, instance.faces[i].b, instance.faces[i].c);
      proyectObject(v1, P, v2, instance.texture, camera, vec_norm, instance.faces[i].a);
      proyectObject(P, Q, v2, instance.texture, camera, vec_norm, instance.faces[i].a);
    }
    else if(cont == 2) { // Dos vertices fuera de la camara
      // Primero ordenamos A < B < C
      /*   C
          /\
         /  \
      --P----Q--- Near Plane --> Z = 1
       /      \
      A--------B
      
      Buscamos ordenar tal que C > Near Plane y A < B < Near Plane */
      if (v1.position.z > v2.position.z){
        Vertex aux = v1;
        v2 = v1;
        v1 = aux;
      }
      
      if (v2.position.z > v3.position.z){
        Vertex aux = v2;
        v2 = v3;
        v3 = aux;
      }
      
      if (v1.position.z > v2.position.z){
        Vertex aux = v1;
        v2 = v1;
        v1 = aux;
      }

      // Calculamos la intersección con el Near Plane
      Vertex P = intersectNearPlane(v3, v1, near);
      Vertex Q = intersectNearPlane(v3, v2, near);

      Point vec_norm = normalVector(instance.faces[i].a, instance.faces[i].b, instance.faces[i].c);
      proyectObject(v3, P, Q, instance.texture, camera, vec_norm, instance.faces[i].a);
    }
    else{
      // Si el trangulo está en la camara
      Point vec_norm = normalVector(instance.faces[i].a, instance.faces[i].b, instance.faces[i].c);
      proyectObject(v1, v2, v3, instance.texture, camera, vec_norm, instance.faces[i].a);
    }
  }
}

Vertex intersectNearPlane(const Vertex& a, const Vertex& b, float nearPlane)
{
    float t = (nearPlane - a.position.z) /
              (b.position.z - a.position.z);

    Vertex r;

    r.position.x = a.position.x + (b.position.x - a.position.x) * t;
    r.position.y = a.position.y + (b.position.y - a.position.y) * t;
    r.position.z = nearPlane;

    r.uv.u = a.uv.u + (b.uv.u - a.uv.u) * t;
    r.uv.v = a.uv.v + (b.uv.v - a.uv.v) * t;

    return r;
}

void proyectObject(Vertex v1, Vertex v2, Vertex v3, Texture &texture, Camera& camera, Point vec_norm, int faceA){
  if (faceVisible(faceA, vec_norm)){
      Point2D p1 = {(v1.position.x * camera.fov) / v1.position.z + CENTER_X, (v1.position.y  * camera.fov) / v1.position.z + CENTER_Y};
      Point2D p2 = {(v2.position.x * camera.fov) / v2.position.z + CENTER_X, (v2.position.y  * camera.fov) / v2.position.z + CENTER_Y};
      Point2D p3 = {(v3.position.x * camera.fov) / v3.position.z + CENTER_X, (v3.position.y  * camera.fov) / v3.position.z + CENTER_Y};
      
      int area = (p2.x - p1.x) * (p3.y - p1.y) - (p3.x - p1.x) * (p2.y - p1.y);

      if (area == 0) {descartadosArea++ ; return;}   // Hay triangulos planos que hay que ignorarlos ya que no se ven practicamente y se pierde potencia de computo el querer dibujarlos

      renderList[trianglesCount].p1 = p1;
      renderList[trianglesCount].p2 = p2;
      renderList[trianglesCount].p3 = p3;

      renderList[trianglesCount].uv1 = v1.uv;
      renderList[trianglesCount].uv2 = v2.uv;
      renderList[trianglesCount].uv3 = v3.uv;

      renderList[trianglesCount].texture = &texture;

      renderList[trianglesCount].light_intensity = faceIntensity(vec_norm);
      renderList[trianglesCount].depth = (v1.position.z + v2.position.z + v3.position.z)*0.33;

      trianglesCount++;
  }
  else {descartadosBackface++;}
}

void renderWorld(Scene& scene)
{
  // Benchmark
  frameCount++;

  if (millis() - fpsTimer >= 1000)
  {
      fps = frameCount;
      frameCount = 0;

      visibleTriangles = trianglesCountPerSec;
      trianglesCountPerSec = 0;
      fpsTimer = millis();
  }

  // Start pipeline of the mf world
  trianglesCount = 0; // -> Esto es para siempre saber cuantos triángulos tengo en cada frame y no caer en terreno que haya valores basura

  uint32_t t0 = millis();

  for (int i = 0; i < scene.numObjects; i++){
    prepareObject(*scene.objects[i], *scene.camera);
  }

  uint32_t t1 = millis();

  quickSort(0, trianglesCount - 1);

  uint32_t t2 = millis();
  canvas[bufferIdx].fillSprite(BACKGROUND); // Set backgraund color

  for(int i = 0; i < trianglesCount; i++){
    drawTexturedTriangle(renderList[i].p1,renderList[i].p2,renderList[i].p3,
                          renderList[i].uv1, renderList[i].uv2, renderList[i].uv3,
                          *renderList[i].texture, renderList[i].light_intensity, &canvas[bufferIdx]);
  }

  FPSScreen(&canvas[bufferIdx]);

  uint32_t t3 = millis();

  canvas[bufferIdx].pushSprite(0,0);

  bufferIdx = 1 - bufferIdx;

  uint32_t t4 = millis();

  trianglesCountPerSec += trianglesCount;

  Serial.printf(
    "Transform:%lu ms Sort:%lu ms Draw:%lu ms Push:%lu ms\n",
    t1-t0,
    t2-t1,
    t3-t2,
    t4-t3
  );
  Serial.printf(
    "Z:%d  Back:%d  Area:%d\n",
    descartadosZ,
    descartadosBackface,
    descartadosArea
  );
  descartadosZ = 0;
  descartadosBackface = 0;
  descartadosArea = 0;
}

void drawTexturedTriangle(Point2D p1, Point2D p2, Point2D p3, UV uv1, UV uv2, UV uv3, const Texture& tex, uint16_t light_intensity, LGFX_Sprite *canvas)
{
  // 1. Ordenar los vértices por Y
  if (p1.y > p2.y) { Point2D t = p1; p1 = p2; p2 = t; UV tuv = uv1; uv1 = uv2; uv2 = tuv; }
  if (p2.y > p3.y) { Point2D t = p2; p2 = p3; p3 = t; UV tuv = uv3; uv3 = uv2; uv2 = tuv; }
  if (p1.y > p2.y) { Point2D t = p1; p1 = p2; p2 = t; UV tuv = uv1; uv1 = uv2; uv2 = tuv; }

  // 2. Calcular las pendientes de avance vertical para cada uno de los 3 lados
  float dy13 = p3.y - p1.y;
  float dx13 = (dy13 != 0) ? (p3.x - p1.x) / dy13 : 0;
  float du13 = (dy13 != 0) ? (uv3.u - uv1.u) / dy13 : 0;
  float dv13 = (dy13 != 0) ? (uv3.v - uv1.v) / dy13 : 0;

  float dy12 = p2.y - p1.y;
  float dx12 = (dy12 != 0) ? (p2.x - p1.x) / dy12 : 0;
  float du12 = (dy12 != 0) ? (uv2.u - uv1.u) / dy12 : 0;
  float dv12 = (dy12 != 0) ? (uv2.v - uv1.v) / dy12 : 0;

  float dy23 = p3.y - p2.y;
  float dx23 = (dy23 != 0) ? (p3.x - p2.x) / dy23 : 0;
  float du23 = (dy23 != 0) ? (uv3.u - uv2.u) / dy23 : 0;
  float dv23 = (dy23 != 0) ? (uv3.v - uv2.v) / dy23 : 0;

  // 3. Bucle principal que recorre el triángulo de arriba a abajo (Scanline)
  for (int y = p1.y; y <= p3.y; y++) {
    if (y < 0) y = 0;
    else if (y > WIDTH-1) break;

    // Puntos de inicio (A) y fin (B) del segmento horizontal actual
    float xa, ua, va;
    float xb, ub, vb;

    // Lado largo: Siempre va desde el vértice 1 al 3
    float t13 = y - p1.y;
    xa = p1.x + dx13 * t13;
    ua = uv1.u + du13 * t13;
    va = uv1.v + dv13 * t13;

    // Lados cortos: Depende de si estamos en la mitad superior o inferior
    if (y < p2.y) {
      // Mitad superior: Avanzamos del vértice 1 al 2
      float t12 = y - p1.y;
      xb = p1.x + dx12 * t12;
      ub = uv1.u + du12 * t12;
      vb = uv1.v + dv12 * t12;
    } else {
      // Mitad inferior: Avanzamos del vértice 2 al 3
      float t23 = y - p2.y;
      xb = p2.x + dx23 * t23;
      ub = uv2.u + du23 * t23;
      vb = uv2.v + dv23 * t23;
    }

    // Asegurarnos de que 'xa' sea siempre el lado izquierdo de la pantalla
    if (xa > xb) {
      float temp;
      temp = xa; xa = xb; xb = temp;
      temp = ua; ua = ub; ub = temp;
      temp = va; va = vb; vb = temp;
    }

    // 4. Dibujar la línea horizontal (Segmento de textura)
    float dx = xb - xa;
    if (dx > 0) { // Protección contra división por cero
      float du = (ub - ua) / dx;
      float dv = (vb - va) / dx;

      float u = ua;
      float v = va;
      
      int x_start = (int)xa;
      int x_end = (int)xb;

      if (x_start < 0){
        int offset = -x_start; // Cuántos píxeles nos pasamos
        u += du * offset;      // Adelantamos la textura matemáticamente
        v += dv * offset;
        x_start = 0;           // Forzamos a empezar en el borde izquierdo de la pantalla
      }
      else if (x_end > HEIGHT-1){
        x_end = HEIGHT-1;
      }

      for (int x = x_start; x <= x_end; x++) {
        int texX = (int)u % tex.width;
        int texY = (int)v % tex.height;
        if (texX < 0) texX = 0;
        // Controlar que las UV no se salgan de la textura
        if (texY < 0) texY = 0;

        // Leer el color de la textura mapeada
        uint16_t color = tex.pixels[texY * tex.width + texX];
        
        // DESEMPAQUETAR los canales usando máscaras de bits
        uint16_t r = (color >> 11) & 0x1F;
        uint16_t g = (color >> 5) & 0x3F;
        uint16_t b = color & 0x1F;

        // APLICAR LUZ usando operaciones enteras rápidas (Shift derecho >> 8 reemplaza la división)
        r = (r * light_intensity) >> 8;
        g = (g * light_intensity) >> 8;
        b = (b * light_intensity) >> 8;

        // REEMPAQUETAR en un nuevo uint16_t RGB565
        uint16_t lit_color = (r << 11) | (g << 5) | b;

        // Pintar en el lienzo virtual
        canvas->drawPixel(x, y, lit_color);

        // Avanzar al siguiente píxel de la textura
        u += du;
        v += dv;
      }
    }
  }
}

void swapTriangles(int a, int b)
{
    RenderTriangle temp = renderList[a];
    renderList[a] = renderList[b];
    renderList[b] = temp;
}

int partition(int low, int high)
{
    float pivot = renderList[high].depth;

    int i = low - 1;

    for(int j = low; j < high; j++)
    {
        if(renderList[j].depth > pivot)
        {
            i++;
            swapTriangles(i, j);
        }
    }

    swapTriangles(i + 1, high);

    return i + 1;
}

void quickSort(int low, int high)
{
    if(low < high)
    {
        int pi = partition(low, high);

        quickSort(low, pi - 1);
        quickSort(pi + 1, high);
    }
}

Point normalVector(int a, int b, int c)
{
    Point A = rotated[a];
    Point B = rotated[b];
    Point C = rotated[c];

    float abx = B.x - A.x;
    float aby = B.y - A.y;
    float abz = B.z - A.z;

    float acx = C.x - A.x;
    float acy = C.y - A.y;
    float acz = C.z - A.z;

    // Producto cruz
    float nx = aby*acz - abz*acy;
    float ny = abz*acx - abx*acz;
    float nz = abx*acy - aby*acx;

    // Normalización
    float nlen = sqrt(nx*nx + ny*ny + nz*nz);

    nx /= nlen;
    ny /= nlen;
    nz /= nlen;

    return {nx, ny, nz};
}

bool faceVisible(int a, Point norm_vec)
{
    Point A = rotated[a];

    float vx = - A.x;
    float vy = - A.y;
    float vz = - A.z;

    // Producto escalar
    float dot = norm_vec.x*vx + norm_vec.y*vy + norm_vec.z*vz;

    return dot >= 0;
}

uint16_t faceIntensity(Point norm_vec)
{
    float intensity = norm_vec.x*light.x + norm_vec.y*light.y + norm_vec.z*light.z;

    if (intensity < MINIMUM_BRITNESS) // Esto es para añadir iluminación a la escena 
        intensity = MINIMUM_BRITNESS;

    // OPTIMIZACIÓN DE LUZ: En vez de usar float usamos punto fijo (0 a 256) y sí evitamos hacer multiplicaciones float por cada píxel.
    return (uint16_t)(intensity * 256.0f);}