#ifndef POST_PROCESSING
#define POST_PROCESSING

#include <Arduino.h>

// Inicialización de buffers en PSRAM
void initPostProcessBuffers(int width, int height);

// Cel-Shading (FIR Pasa-Bajos + FIR Sobel)
void applyLowPassFIR(uint16_t* fb, int width, int height);
void applySobelOnSmoothed(uint16_t* fb, int width, int height, uint8_t threshold);

// Bloom (FIR Separable 5-taps)
// thresholdLuma: 0 a 125 (típico: 75 a 100 para capturar solo zonas muy brillantes)
void applyBloomFIR(uint16_t* fb, int width, int height, uint8_t thresholdLuma);

#endif