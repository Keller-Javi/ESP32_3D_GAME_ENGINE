#include "render_world.h"

int fov = 90;
Point light = {15.0, -1.5, -3};

static LGFX lcd;
static LGFX_Sprite canvas(&lcd);

Point rotated[MAX_VERTICES];
RenderTriangle renderList[MAX_TRIANGLES];

int trianglesCount = 0;

// Benchmark vars
unsigned long fpsTimer = 0;
unsigned long frameCount = 0;
int fps = 0;
int visibleTriangles = 0;
int trianglesCountPerSec = 0;

void setScreen(int init_screen_time)
{
  lcd.init();

  lcd.setColorDepth(COLOR_DEPTH);
  canvas.createSprite(HEIGHT, WIDTH);

  initScreen();
  delay(init_screen_time);
}

void initScreen()
{
  canvas.fillScreen(0x0000);

  canvas.setTextColor(0xFFFF);
  canvas.setTextSize(2);
  canvas.setCursor(50,CENTER_Y);
  canvas.println("HELLO WORLD!");
  canvas.setTextColor(TFT_RED);
  canvas.setCursor(30,CENTER_Y+20);
  canvas.println("...............");

  canvas.pushSprite(0,0);
}

void FPSScreen()
{
  canvas.setTextColor(0xFFFF);
  canvas.setTextSize(2);
  canvas.setCursor(0,0);
  canvas.print("FPS: ");
  canvas.println(fps);
  canvas.setCursor(0,HEIGHT-20);
  canvas.print("Triangles: ");
  canvas.println(visibleTriangles/fps);
}

void setDirectionLight(Point lgt)
{
  light = lgt;
  float len = sqrt(light.x*light.x + light.y*light.y + light.z*light.z);
  light.x /= len;
  light.y /= len;
  light.z /= len;
}

void prepareObject(Mesh& instance)
{
  // Rotaciones del objeto 3D
  float sin_x = sin(instance.rotation.x);
  float cos_x = cos(instance.rotation.x);

  float sin_y = sin(instance.rotation.y);
  float cos_y = cos(instance.rotation.y);

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

    rotated[i].x = xr;
    rotated[i].y = yr;
    rotated[i].z = zr;
  }

  // Proyección y dibujo en pantalla
  for (int i = 0; i < instance.numFaces; i++) {
    // Primero verifico si el triangulo no está fuera de la pantalla. ESTO GENERA ERRORES VISUALES A VECES
    if(rotated[instance.faces[i].a].z < 1 || rotated[instance.faces[i].b].z < 1 || rotated[instance.faces[i].c].z < 1) continue;

    Point vec_norm = normalVector(instance.faces[i].a, instance.faces[i].b, instance.faces[i].c);

    if (faceVisible(instance.faces[i].a, vec_norm)){
      Point2D p1 = {(rotated[instance.faces[i].a].x * fov) / rotated[instance.faces[i].a].z + CENTER_X, (rotated[instance.faces[i].a].y  * fov) / rotated[instance.faces[i].a].z + CENTER_Y};
      Point2D p2 = {(rotated[instance.faces[i].b].x * fov) / rotated[instance.faces[i].b].z + CENTER_X, (rotated[instance.faces[i].b].y  * fov) / rotated[instance.faces[i].b].z + CENTER_Y};
      Point2D p3 = {(rotated[instance.faces[i].c].x * fov) / rotated[instance.faces[i].c].z + CENTER_X, (rotated[instance.faces[i].c].y  * fov) / rotated[instance.faces[i].c].z + CENTER_Y};
      
      int area = (p2.x - p1.x) * (p3.y - p1.y) - (p3.x - p1.x) * (p2.y - p1.y);

      if (area == 0) continue;   // Hay triangulos planos que hay que ignorarlos ya que no se ven practicamente y se pierde potencia de computo el querer dibujarlos

      renderList[trianglesCount].p1 = p1;
      renderList[trianglesCount].p2 = p2;
      renderList[trianglesCount].p3 = p3;

      renderList[trianglesCount].uv1 = instance.texcoords[instance.faces_texcoords[i].a];
      renderList[trianglesCount].uv2 = instance.texcoords[instance.faces_texcoords[i].b];
      renderList[trianglesCount].uv3 = instance.texcoords[instance.faces_texcoords[i].c];

      renderList[trianglesCount].texture = &instance.texture;

      renderList[trianglesCount].light_intensity = faceIntensity(vec_norm);
      renderList[trianglesCount].depth = (rotated[instance.faces[i].a].z + rotated[instance.faces[i].b].z + rotated[instance.faces[i].c].z)*0.33;

      trianglesCount++;
      if (trianglesCount >= MAX_TRIANGLES){
          Serial.println("OVERFLOW");
          return;
      }
    }
  }
}

void renderWorld(Scene &scene)
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
    prepareObject(*scene.objects[i]);
  }

  uint32_t t1 = millis();

  quickSort(0, trianglesCount - 1);

  uint32_t t2 = millis();
  canvas.fillSprite(BACKGROUND); // Set backgraund color

  for(int i = 0; i < trianglesCount; i++){
    drawTexturedTriangle(renderList[i].p1,renderList[i].p2,renderList[i].p3,
                          renderList[i].uv1, renderList[i].uv2, renderList[i].uv3,
                          *renderList[i].texture, renderList[i].light_intensity);
  }

  FPSScreen();

  uint32_t t3 = millis();

  canvas.pushSprite(0,0);

  uint32_t t4 = millis();

  trianglesCountPerSec += trianglesCount;

  Serial.printf(
    "Transform:%lu ms Sort:%lu ms Draw:%lu ms Push:%lu ms\n",
    t1-t0,
    t2-t1,
    t3-t2,
    t4-t3
  );
}

void drawTexturedTriangle(Point2D p1, Point2D p2, Point2D p3, UV uv1, UV uv2, UV uv3, const Texture& tex, uint16_t light_intensity)
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

      for (int x = (int)xa; x <= (int)xb; x++) {
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
        canvas.drawPixel(x, y, lit_color);

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

    // Cámara en (0,0,0)
    float vx = -A.x;
    float vy = -A.y;
    float vz = -A.z;

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
    return (uint16_t)(intensity * 256.0f);
}