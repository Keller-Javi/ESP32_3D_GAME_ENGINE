#ifndef POST_PROCESSING
#define POST_PROCESSING

#include <Arduino.h>

// Inicializa la memoria en PSRAM una sola vez al inicio
void initPostProcessBuffers(int width, int height);

void applyBlur_Average(uint16_t* fb, int width, int height);

// Anti-aliasing lineal FIR pasa-bajos (Convolución Gaussiana 3x3)
void applyBlur_Gaussian(uint16_t* fb, int width, int height);

void applyCelShading_Sobel(uint16_t* fb, int width, int height, uint8_t threshold);

void applyCelShading_Laplacian(uint16_t* fb, int width, int height, uint8_t threshold);

void applyAntialiassing_Laplacian(uint16_t* fb, int width, int height, uint8_t threshold);

void applyAntialiassing_Sobel(uint16_t* fb, int width, int height, uint8_t threshold);

void applyAntialiassing_SobelMedian(uint16_t* fb, int width, int height, uint8_t threshold);

void applyBloom(uint16_t* fb, int width, int height, uint8_t threshold);

#endif