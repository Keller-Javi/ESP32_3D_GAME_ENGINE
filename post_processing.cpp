#include "POST_PROCESSING.h"
#include <esp_heap_caps.h>
#include <stdlib.h>
#include <stdint.h>

// Puntero para el búfer auxiliar en PSRAM
static uint16_t* temp_fb = nullptr;
static int allocated_pixels = 0;

// Extrae canales de RGB565 (R: 5 bits, G: 6 bits, B: 5 bits)
static inline void unpackRGB565(uint16_t c, uint8_t &r, uint8_t &g, uint8_t &b) {
    r = (c >> 11) & 0x1F;
    g = (c >> 5)  & 0x3F;
    b =  c        & 0x1F;
}

// Empaqueta componentes a formato RGB565
static inline uint16_t packRGB565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)((r << 11) | (g << 5) | b);
}

// Desempaqueta según el orden de bits que envías por SPI/DMA
static inline void unpackScreenRGB565(uint16_t c, uint8_t &r, uint8_t &g, uint8_t &b) {
    r = (c >> 3) & 0x1F;
    g = (uint8_t)(((c & 0x07) << 3) | ((c >> 13) & 0x07));
    b = (c >> 8) & 0x1F;
}

// Reempaqueta con tu fórmula exacta para la pantalla
static inline uint16_t packScreenRGB565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((b & 0x1F) << 8) | ((g & 0x07) << 13) | ((g & 0x38) >> 3) | ((r & 0x1F) << 3));
}

void initPostProcessBuffers(int width, int height) {
    int total_pixels = width * height;
    
    // Si ya existe y el tamaño es diferente, liberamos
    if (temp_fb != nullptr && allocated_pixels != total_pixels) {
        free(temp_fb);
        temp_fb = nullptr;
    }

    if (temp_fb == nullptr) {
        // Reservamos memoria específicamente en la PSRAM externa
        temp_fb = (uint16_t*) heap_caps_malloc(total_pixels * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
        if (temp_fb != nullptr) {
            allocated_pixels = total_pixels;
        }
    }
}

// Búsqueda de mediana de 9 elementos mediante ordenamiento por inserción
static inline uint8_t median9(uint8_t a[9]) {
    for (int i = 1; i < 9; ++i) {
        uint8_t val = a[i];
        int j = i - 1;
        while (j >= 0 && a[j] > val) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = val;
    }
    return a[4]; // El elemento central (índice 4) es la mediana
}

void applyBlur_Average(uint16_t* fb, int width, int height)
{
    if (!fb || !temp_fb) return;

    const size_t pixels = (size_t)width * height;

    // Copia de la imagen original.
    // Es necesaria porque vamos modificando fb mientras
    // seguimos leyendo los vecinos.
    memcpy(temp_fb, fb, pixels * sizeof(uint16_t));

    for (int y = 1; y < height - 1; ++y)
    {
        const int row_prev = (y - 1) * width;
        const int row_curr = y * width;
        const int row_next = (y + 1) * width;

        for (int x = 1; x < width - 1; ++x)
        {
            // -------------------------------------------------
            //        3 × 3
            //
            //  p0 p1 p2
            //  p3 p4 p5
            //  p6 p7 p8
            // -------------------------------------------------

            uint8_t r, g, b;

            int r_sum = 0;
            int g_sum = 0;
            int b_sum = 0;

            // Fila superior
            unpackScreenRGB565(temp_fb[row_prev + x - 1], r, g, b);
            r_sum += r;
            g_sum += g;
            b_sum += b;

            unpackScreenRGB565(temp_fb[row_prev + x], r, g, b);
            r_sum += r;
            g_sum += g;
            b_sum += b;

            unpackScreenRGB565(temp_fb[row_prev + x + 1], r, g, b);
            r_sum += r;
            g_sum += g;
            b_sum += b;

            // Fila central
            unpackScreenRGB565(temp_fb[row_curr + x - 1], r, g, b);
            r_sum += r;
            g_sum += g;
            b_sum += b;

            unpackScreenRGB565(temp_fb[row_curr + x], r, g, b);
            r_sum += r;
            g_sum += g;
            b_sum += b;

            unpackScreenRGB565(temp_fb[row_curr + x + 1], r, g, b);
            r_sum += r;
            g_sum += g;
            b_sum += b;

            // Fila inferior
            unpackScreenRGB565(temp_fb[row_next + x - 1], r, g, b);
            r_sum += r;
            g_sum += g;
            b_sum += b;

            unpackScreenRGB565(temp_fb[row_next + x], r, g, b);
            r_sum += r;
            g_sum += g;
            b_sum += b;

            unpackScreenRGB565(temp_fb[row_next + x + 1], r, g, b);
            r_sum += r;
            g_sum += g;
            b_sum += b;

            // Media de los 9 píxeles.
            //
            // La división por 9 es exacta y solamente se hace
            // una vez por canal.
            uint8_t r_avg = r_sum / 9;
            uint8_t g_avg = g_sum / 9;
            uint8_t b_avg = b_sum / 9;

            // Reempaquetar a RGB565
            fb[row_curr + x] = packScreenRGB565(
                r_avg,
                g_avg,
                b_avg
            );
        }
    }
}


void applyBlur_Gaussian(uint16_t* fb, int width, int height) {
    if (!fb || !temp_fb) return;

    // Copia al búfer auxiliar para evitar retroalimentación en la convolución
    memcpy(temp_fb, fb, width * height * sizeof(uint16_t));

    for (int y = 1; y < height - 1; ++y) {
        int row_prev = (y - 1) * width;
        int row_curr = y * width;
        int row_next = (y + 1) * width;

        for (int x = 1; x < width - 1; ++x) {
            uint16_t n[9] = {
                temp_fb[row_prev + (x - 1)], temp_fb[row_prev + x], temp_fb[row_prev + (x + 1)],
                temp_fb[row_curr + (x - 1)], temp_fb[row_curr + x], temp_fb[row_curr + (x + 1)],
                temp_fb[row_next + (x - 1)], temp_fb[row_next + x], temp_fb[row_next + (x + 1)]
            };

            uint8_t r[9], g[9], b[9];
            for (int i = 0; i < 9; ++i) {
                unpackScreenRGB565(n[i], r[i], g[i], b[i]);
            }

            // Convolución FIR 2D por canal (pesos binomiales: 1, 2, 4)
            uint32_t r_sum = r[0] + (r[1] << 1) + r[2]
                           + (r[3] << 1) + (r[4] << 2) + (r[5] << 1)
                           + r[6] + (r[7] << 1) + r[8];

            uint32_t g_sum = g[0] + (g[1] << 1) + g[2]
                           + (g[3] << 1) + (g[4] << 2) + (g[5] << 1)
                           + g[6] + (g[7] << 1) + g[8];

            uint32_t b_sum = b[0] + (b[1] << 1) + b[2]
                           + (b[3] << 1) + (b[4] << 2) + (b[5] << 1)
                           + b[6] + (b[7] << 1) + b[8];

            // División por 16 (>> 4) y reempaquetado compatible con pantalla
            fb[row_curr + x] = packScreenRGB565(r_sum >> 4, g_sum >> 4, b_sum >> 4);
        }
    }
}


// Calcula luminancia Y en escala 0..250 (Y = 0.299R + 0.587G + 0.114B)
// r: 0..31, g: 0..63, b: 0..31
static inline uint8_t getLuminance(uint8_t r, uint8_t g, uint8_t b) {
    // 616*r + 600*g + 232*b cabe en uint16_t (máximo 64088)
    return (uint8_t)((616 * (uint16_t)r + 600 * (uint16_t)g + 232 * (uint16_t)b) >> 8);
}

void applyCelShading_Sobel(uint16_t* fb, int width, int height, uint8_t threshold) {
    if (!fb || !temp_fb) return;

    // Copia al búfer auxiliar para evitar retroalimentación en la convolución
    memcpy(temp_fb, fb, width * height * sizeof(uint16_t));

    for (int y = 1; y < height - 1; ++y) {
        int row_prev = (y - 1) * width;
        int row_curr = y * width;
        int row_next = (y + 1) * width;

        for (int x = 1; x < width - 1; ++x) {
            // Muestreo de la ventana 3x3 en el búfer auxiliar
            uint16_t n[9] = {
                temp_fb[row_prev + (x - 1)], temp_fb[row_prev + x], temp_fb[row_prev + (x + 1)],
                temp_fb[row_curr + (x - 1)], temp_fb[row_curr + x], temp_fb[row_curr + (x + 1)],
                temp_fb[row_next + (x - 1)], temp_fb[row_next + x], temp_fb[row_next + (x + 1)]
            };

            // Extracción de canales y cálculo de luminancia para cada celda
            uint8_t lum[9];

            for (int i = 0; i < 9; ++i) {
                uint8_t r, g, b;
                unpackScreenRGB565(n[i], r, g, b);
                lum[i] = getLuminance(r, g, b);
            }

            // --- FILTRO DE SOBEL (Pasa-altos direccional) ---
            // Gradiente Horizontal Gx (K_x: Sobel Vertical)
            // [-1, 0, 1; -2, 0, 2; -1, 0, 1]
            int16_t gx = ((int16_t)lum[2] + ((int16_t)lum[5] << 1) + (int16_t)lum[8])
                       - ((int16_t)lum[0] + ((int16_t)lum[3] << 1) + (int16_t)lum[6]);

            // Gradiente Vertical Gy (K_y: Sobel Horizontal)
            // [-1, -2, -1; 0, 0, 0; 1, 2, 1]
            int16_t gy = ((int16_t)lum[6] + ((int16_t)lum[7] << 1) + (int16_t)lum[8])
                       - ((int16_t)lum[0] + ((int16_t)lum[1] << 1) + (int16_t)lum[2]);

            // Magnitud del gradiente normalizada al rango 0..255 (división por 4 con >> 2)
            uint16_t g_mag = (abs(gx) + abs(gy)) >> 2;

            if (g_mag > threshold) {
                // DISCONTINUIDAD DETECTADA: Tinta negra de contorno
                fb[row_curr + x] = 0x0000;
            }
        }
    }
}

void applyCelShading_Laplacian(uint16_t* fb, int width, int height, uint8_t threshold){
    if (!fb || !temp_fb) return;

    // Copia al búfer auxiliar para evitar retroalimentación en la convolución
    memcpy(temp_fb, fb, width * height * sizeof(uint16_t));

    for (int y = 1; y < height - 1; ++y) {
        int row_prev = (y - 1) * width;
        int row_curr = y * width;
        int row_next = (y + 1) * width;

        for (int x = 1; x < width - 1; ++x) {
            // Muestreo de la ventana 3x3 en el búfer auxiliar
            uint16_t n[9] = {
                temp_fb[row_prev + (x - 1)], temp_fb[row_prev + x], temp_fb[row_prev + (x + 1)],
                temp_fb[row_curr + (x - 1)], temp_fb[row_curr + x], temp_fb[row_curr + (x + 1)],
                temp_fb[row_next + (x - 1)], temp_fb[row_next + x], temp_fb[row_next + (x + 1)]
            };

            // Extracción de canales y cálculo de luminancia para cada celda
            uint8_t lum[9];

            for (int i = 0; i < 9; ++i) {
                uint8_t r, g, b;
                unpackScreenRGB565(n[i], r, g, b);
                lum[i] = getLuminance(r, g, b);
            }

            // --- FILTRO DE SOBEL (Pasa-altos direccional) ---
            // [-1, 0, 1; -2, 0, 2; -1, 0, 1]
            int16_t g = (int16_t)lum[1] + (int16_t)lum[3] - ((int16_t)lum[4] << 2) + (int16_t)lum[5] + (int16_t)lum[7];

            if (g > threshold) {
                // DISCONTINUIDAD DETECTADA: Tinta negra de contorno
                fb[row_curr + x] = 0x0000;
            }
        }
    }
}

void applyAntialiassing_Sobel(uint16_t* fb, int width, int height, uint8_t threshold){
    if (!fb || !temp_fb) return;

    // Copia al búfer auxiliar para evitar retroalimentación en la convolución
    memcpy(temp_fb, fb, width * height * sizeof(uint16_t));

    for (int y = 1; y < height - 1; ++y) {
        int row_prev = (y - 1) * width;
        int row_curr = y * width;
        int row_next = (y + 1) * width;

        for (int x = 1; x < width - 1; ++x) {
            // Muestreo de la ventana 3x3 en el búfer auxiliar
            uint16_t n[9] = {
                temp_fb[row_prev + (x - 1)], temp_fb[row_prev + x], temp_fb[row_prev + (x + 1)],
                temp_fb[row_curr + (x - 1)], temp_fb[row_curr + x], temp_fb[row_curr + (x + 1)],
                temp_fb[row_next + (x - 1)], temp_fb[row_next + x], temp_fb[row_next + (x + 1)]
            };

            // Extracción de canales y cálculo de luminancia para cada celda
            uint8_t lum[9];

            for (int i = 0; i < 9; ++i) {
                uint8_t r, g, b;
                unpackScreenRGB565(n[i], r, g, b);
                lum[i] = getLuminance(r, g, b);
            }

            // --- FILTRO DE SOBEL (Pasa-altos direccional) ---
            // Gradiente Horizontal Gx (K_x: Sobel Vertical)
            // [-1, 0, 1; -2, 0, 2; -1, 0, 1]
            int16_t gx = ((int16_t)lum[2] + ((int16_t)lum[5] << 1) + (int16_t)lum[8])
                       - ((int16_t)lum[0] + ((int16_t)lum[3] << 1) + (int16_t)lum[6]);

            // Gradiente Vertical Gy (K_y: Sobel Horizontal)
            // [-1, -2, -1; 0, 0, 0; 1, 2, 1]
            int16_t gy = ((int16_t)lum[6] + ((int16_t)lum[7] << 1) + (int16_t)lum[8])
                       - ((int16_t)lum[0] + ((int16_t)lum[1] << 1) + (int16_t)lum[2]);

            // Magnitud del gradiente normalizada al rango 0..255 (división por 4 con >> 2)
            uint16_t gradient = (abs(gx) + abs(gy)) >> 2;

            if (gradient > threshold) {
                // DISCONTINUIDAD DETECTADA: suavizar borde
                uint8_t r[9], g[9], b[9];
                for (int i = 0; i < 9; ++i) {
                    unpackScreenRGB565(n[i], r[i], g[i], b[i]);
                }

                uint32_t r_sum = r[0] + (r[1] << 1) + r[2]
                           + (r[3] << 1) + (r[4] << 2) + (r[5] << 1)
                           + r[6] + (r[7] << 1) + r[8];

                uint32_t g_sum = g[0] + (g[1] << 1) + g[2]
                            + (g[3] << 1) + (g[4] << 2) + (g[5] << 1)
                            + g[6] + (g[7] << 1) + g[8];

                uint32_t b_sum = b[0] + (b[1] << 1) + b[2]
                            + (b[3] << 1) + (b[4] << 2) + (b[5] << 1)
                            + b[6] + (b[7] << 1) + b[8];

                fb[row_curr + x] = packScreenRGB565(r_sum >> 4, g_sum >> 4, b_sum >> 4);
            }
        }
    }
}

void applyAntialiassing_SobelMedian(uint16_t* fb, int width, int height, uint8_t threshold){
    if (!fb || !temp_fb) return;

    // Copia al búfer auxiliar para evitar retroalimentación en la convolución
    memcpy(temp_fb, fb, width * height * sizeof(uint16_t));

    for (int y = 1; y < height - 1; ++y) {
        int row_prev = (y - 1) * width;
        int row_curr = y * width;
        int row_next = (y + 1) * width;

        for (int x = 1; x < width - 1; ++x) {
            // Muestreo de la ventana 3x3 en el búfer auxiliar
            uint16_t n[9] = {
                temp_fb[row_prev + (x - 1)], temp_fb[row_prev + x], temp_fb[row_prev + (x + 1)],
                temp_fb[row_curr + (x - 1)], temp_fb[row_curr + x], temp_fb[row_curr + (x + 1)],
                temp_fb[row_next + (x - 1)], temp_fb[row_next + x], temp_fb[row_next + (x + 1)]
            };

            // Extracción de canales y cálculo de luminancia para cada celda
            uint8_t lum[9];

            for (int i = 0; i < 9; ++i) {
                uint8_t r, g, b;
                unpackScreenRGB565(n[i], r, g, b);
                lum[i] = getLuminance(r, g, b);
            }

            // --- FILTRO DE SOBEL (Pasa-altos direccional) ---
            // Gradiente Horizontal Gx (K_x: Sobel Vertical)
            // [-1, 0, 1; -2, 0, 2; -1, 0, 1]
            int16_t gx = ((int16_t)lum[2] + ((int16_t)lum[5] << 1) + (int16_t)lum[8])
                       - ((int16_t)lum[0] + ((int16_t)lum[3] << 1) + (int16_t)lum[6]);

            // Gradiente Vertical Gy (K_y: Sobel Horizontal)
            // [-1, -2, -1; 0, 0, 0; 1, 2, 1]
            int16_t gy = ((int16_t)lum[6] + ((int16_t)lum[7] << 1) + (int16_t)lum[8])
                       - ((int16_t)lum[0] + ((int16_t)lum[1] << 1) + (int16_t)lum[2]);

            // Magnitud del gradiente normalizada al rango 0..255 (división por 4 con >> 2)
            uint16_t gradient = (abs(gx) + abs(gy)) >> 2;

            if (gradient > threshold) {
                // DISCONTINUIDAD DETECTADA: suavizar borde
                uint8_t r, g, b;

                int r_sum = 0;
                int g_sum = 0;
                int b_sum = 0;

                // Fila superior
                unpackScreenRGB565(temp_fb[row_prev + x - 1], r, g, b);
                r_sum += r;
                g_sum += g;
                b_sum += b;

                unpackScreenRGB565(temp_fb[row_prev + x], r, g, b);
                r_sum += r;
                g_sum += g;
                b_sum += b;

                unpackScreenRGB565(temp_fb[row_prev + x + 1], r, g, b);
                r_sum += r;
                g_sum += g;
                b_sum += b;

                // Fila central
                unpackScreenRGB565(temp_fb[row_curr + x - 1], r, g, b);
                r_sum += r;
                g_sum += g;
                b_sum += b;

                unpackScreenRGB565(temp_fb[row_curr + x], r, g, b);
                r_sum += r;
                g_sum += g;
                b_sum += b;

                unpackScreenRGB565(temp_fb[row_curr + x + 1], r, g, b);
                r_sum += r;
                g_sum += g;
                b_sum += b;

                // Fila inferior
                unpackScreenRGB565(temp_fb[row_next + x - 1], r, g, b);
                r_sum += r;
                g_sum += g;
                b_sum += b;

                unpackScreenRGB565(temp_fb[row_next + x], r, g, b);
                r_sum += r;
                g_sum += g;
                b_sum += b;

                unpackScreenRGB565(temp_fb[row_next + x + 1], r, g, b);
                r_sum += r;
                g_sum += g;
                b_sum += b;

                // Media de los 9 píxeles.
                //
                // La división por 9 es exacta y solamente se hace
                // una vez por canal.
                uint8_t r_avg = r_sum / 9;
                uint8_t g_avg = g_sum / 9;
                uint8_t b_avg = b_sum / 9;

                fb[row_curr + x] = packScreenRGB565(r_avg, g_avg, b_avg);
            }
        }
    }
}

void applyAntialiassing_Laplacian(uint16_t* fb, int width, int height, uint8_t threshold){
    if (!fb || !temp_fb) return;

    // Copia al búfer auxiliar para evitar retroalimentación en la convolución
    memcpy(temp_fb, fb, width * height * sizeof(uint16_t));

    for (int y = 1; y < height - 1; ++y) {
        int row_prev = (y - 1) * width;
        int row_curr = y * width;
        int row_next = (y + 1) * width;

        for (int x = 1; x < width - 1; ++x) {
            // Muestreo de la ventana 3x3 en el búfer auxiliar
            uint16_t n[9] = {
                temp_fb[row_prev + (x - 1)], temp_fb[row_prev + x], temp_fb[row_prev + (x + 1)],
                temp_fb[row_curr + (x - 1)], temp_fb[row_curr + x], temp_fb[row_curr + (x + 1)],
                temp_fb[row_next + (x - 1)], temp_fb[row_next + x], temp_fb[row_next + (x + 1)]
            };

            // Extracción de canales y cálculo de luminancia para cada celda
            uint8_t lum[9];

            for (int i = 0; i < 9; ++i) {
                uint8_t r, g, b;
                unpackScreenRGB565(n[i], r, g, b);
                lum[i] = getLuminance(r, g, b);
            }

            // --- FILTRO DE SOBEL (Pasa-altos direccional) ---
            // [-1, 0, 1; -2, 0, 2; -1, 0, 1]
            int16_t g = (int16_t)lum[1] + (int16_t)lum[3] - ((int16_t)lum[4] << 2) + (int16_t)lum[5] + (int16_t)lum[7];

            if (g > threshold) {
                // DISCONTINUIDAD DETECTADA: suavizar borde
                uint8_t r[9], g[9], b[9];
                for (int i = 0; i < 9; ++i) {
                    unpackScreenRGB565(n[i], r[i], g[i], b[i]);
                }

                uint32_t r_sum = r[0] + (r[1] << 1) + r[2]
                           + (r[3] << 1) + (r[4] << 2) + (r[5] << 1)
                           + r[6] + (r[7] << 1) + r[8];

                uint32_t g_sum = g[0] + (g[1] << 1) + g[2]
                            + (g[3] << 1) + (g[4] << 2) + (g[5] << 1)
                            + g[6] + (g[7] << 1) + g[8];

                uint32_t b_sum = b[0] + (b[1] << 1) + b[2]
                            + (b[3] << 1) + (b[4] << 2) + (b[5] << 1)
                            + b[6] + (b[7] << 1) + b[8];

                fb[row_curr + x] = packScreenRGB565(r_sum >> 4, g_sum >> 4, b_sum >> 4);
            }
        }
    }
}

void applyBloom(uint16_t* fb, int width, int height, uint8_t threshold){
    return;
}