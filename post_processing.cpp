#include "post_processing.h"

// Buffers en PSRAM
static uint8_t*  lumSmooth  = nullptr;
static uint16_t* bloomPing  = nullptr;
static uint16_t* bloomPong  = nullptr;

// ============================================================================
// ABSTRACCIÓN DE FORMATO DE COLOR (Zero-Cost Inlining)
// ============================================================================

// Desempaquetado
static inline void unpackColor(uint16_t c, uint8_t &r, uint8_t &g, uint8_t &b) {
    b = (c >> 8) & 0x1F;
    r = (c >> 3) & 0x1F;
    g = ((c & 0x07) << 3) | ((c >> 13) & 0x07);
}

// Tu función de empaquetado original
static inline uint16_t packColor(uint8_t r, uint8_t g, uint8_t b) {
    return ((b & 0x1F) << 8) | ((g & 0x07) << 13) | ((g & 0x38) >> 3) | ((r & 0x1F) << 3);
}

// Cálculo de Luminancia perceptualmente ponderada para DSP:
// Y ≈ 0.299*R + 0.587*G + 0.114*B (Aproximación en punto fijo rápido)
static inline uint8_t getFastLuminance(uint16_t color) {
    uint8_t r, g, b;
    unpackColor(color, r, g, b);
    // Ponderación de brillo: el ojo humano percibe mucho más el canal verde
    return (uint8_t)((r * 2 + g * 4 + b * 1) >> 3);
}

// ============================================================================
// GESTIÓN DE MEMORIA
// ============================================================================

void initPostProcessBuffers(int width, int height) {
    if (!lumSmooth) {
        lumSmooth = (uint8_t*)ps_malloc(width * height * sizeof(uint8_t));
    }
    if (!bloomPing) {
        bloomPing = (uint16_t*)ps_malloc(width * height * sizeof(uint16_t));
    }
    if (!bloomPong) {
        bloomPong = (uint16_t*)ps_malloc(width * height * sizeof(uint16_t));
    }
}

// ============================================================================
// FILTROS FIR DE CEL-SHADING
// ============================================================================

void applyLowPassFIR(uint16_t* fb, int width, int height) {
    for (int y = 1; y < height - 1; y++) {
        int rowPrev = (y - 1) * width;
        int rowCurr = y * width;
        int rowNext = (y + 1) * width;

        for (int x = 1; x < width - 1; x++) {
            uint32_t sum = 
                (getFastLuminance(fb[rowPrev + x - 1]) * 1) +
                (getFastLuminance(fb[rowPrev + x    ]) * 2) +
                (getFastLuminance(fb[rowPrev + x + 1]) * 1) +
                (getFastLuminance(fb[rowCurr + x - 1]) * 2) +
                (getFastLuminance(fb[rowCurr + x    ]) * 4) +
                (getFastLuminance(fb[rowCurr + x + 1]) * 2) +
                (getFastLuminance(fb[rowNext + x - 1]) * 1) +
                (getFastLuminance(fb[rowNext + x    ]) * 2) +
                (getFastLuminance(fb[rowNext + x + 1]) * 1);

            lumSmooth[rowCurr + x] = sum >> 4;
        }
    }
}

void applySobelOnSmoothed(uint16_t* fb, int width, int height, uint8_t threshold) {
    for (int y = 1; y < height - 1; y++) {
        int rowPrev = (y - 1) * width;
        int rowCurr = y * width;
        int rowNext = (y + 1) * width;

        for (int x = 1; x < width - 1; x++) {
            int16_t gx = (-1 * lumSmooth[rowPrev + x - 1]) + ( 1 * lumSmooth[rowPrev + x + 1])
                       + (-2 * lumSmooth[rowCurr + x - 1]) + ( 2 * lumSmooth[rowCurr + x + 1])
                       + (-1 * lumSmooth[rowNext + x - 1]) + ( 1 * lumSmooth[rowNext + x + 1]);

            int16_t gy = (-1 * lumSmooth[rowPrev + x - 1]) + (-2 * lumSmooth[rowPrev + x]) + (-1 * lumSmooth[rowPrev + x + 1])
                       + ( 1 * lumSmooth[rowNext + x - 1]) + ( 2 * lumSmooth[rowNext + x]) + ( 1 * lumSmooth[rowNext + x + 1]);

            int16_t mag = abs(gx) + abs(gy);

            if (mag > threshold) {
                fb[rowCurr + x] = 0x0000;
            }
        }
    }
}

// ============================================================================
// BLOOM MULTIFORMATO
// ============================================================================

void applyBloomFIR(uint16_t* fb, int width, int height, uint8_t thresholdLuma) {
    int totalPixels = width * height;

    // Etapa 1: Bright-Pass (Aislar fuentes de alta emisión lumínica)
    for (int i = 0; i < totalPixels; i++) {
        uint16_t c = fb[i];
        if (getFastLuminance(c) >= thresholdLuma) {
            bloomPing[i] = c;
        } else {
            bloomPing[i] = 0x0000;
        }
    }

    // Etapa 2A: Pasada Horizontal FIR [1, 4, 6, 4, 1] / 16 (bloomPing -> bloomPong)
    for (int y = 0; y < height; y++) {
        int row = y * width;
        for (int x = 2; x < width - 2; x++) {
            uint8_t r0, g0, b0, r1, g1, b1, r2, g2, b2, r3, g3, b3, r4, g4, b4;
            unpackColor(bloomPing[row + x - 2], r0, g0, b0);
            unpackColor(bloomPing[row + x - 1], r1, g1, b1);
            unpackColor(bloomPing[row + x    ], r2, g2, b2);
            unpackColor(bloomPing[row + x + 1], r3, g3, b3);
            unpackColor(bloomPing[row + x + 2], r4, g4, b4);

            uint16_t r = (r0 * 1 + r1 * 4 + r2 * 6 + r3 * 4 + r4 * 1) >> 4;
            uint16_t g = (g0 * 1 + g1 * 4 + g2 * 6 + g3 * 4 + g4 * 1) >> 4;
            uint16_t b = (b0 * 1 + b1 * 4 + b2 * 6 + b3 * 4 + b4 * 1) >> 4;

            bloomPong[row + x] = packColor((uint8_t)r, (uint8_t)g, (uint8_t)b);
        }
    }

    // Etapa 2B: Pasada Vertical FIR [1, 4, 6, 4, 1] / 16 + Mezcla Aditiva con Saturación
    for (int y = 2; y < height - 2; y++) {
        int row0 = (y - 2) * width;
        int row1 = (y - 1) * width;
        int row2 =  y      * width;
        int row3 = (y + 1) * width;
        int row4 = (y + 2) * width;

        for (int x = 2; x < width - 2; x++) {
            uint8_t r0, g0, b0, r1, g1, b1, r2, g2, b2, r3, g3, b3, r4, g4, b4;
            unpackColor(bloomPong[row0 + x], r0, g0, b0);
            unpackColor(bloomPong[row1 + x], r1, g1, b1);
            unpackColor(bloomPong[row2 + x], r2, g2, b2);
            unpackColor(bloomPong[row3 + x], r3, g3, b3);
            unpackColor(bloomPong[row4 + x], r4, g4, b4);

            uint16_t glowR = (r0 * 1 + r1 * 4 + r2 * 6 + r3 * 4 + r4 * 1) >> 4;
            uint16_t glowG = (g0 * 1 + g1 * 4 + g2 * 6 + g3 * 4 + g4 * 1) >> 4;
            uint16_t glowB = (b0 * 1 + b1 * 4 + b2 * 6 + b3 * 4 + b4 * 1) >> 4;

            // Extraer color base original
            uint8_t baseR, baseG, baseB;
            unpackColor(fb[row2 + x], baseR, baseG, baseB);

            // Suma aditiva con saturación independiente por canal
            uint16_t finalR = baseR + glowR;
            uint16_t finalG = baseG + glowG;
            uint16_t finalB = baseB + glowB;

            if (finalR > 31) finalR = 31;
            if (finalG > 63) finalG = 63;
            if (finalB > 31) finalB = 31;

            fb[row2 + x] = packColor((uint8_t)finalR, (uint8_t)finalG, (uint8_t)finalB);
        }
    }
} 