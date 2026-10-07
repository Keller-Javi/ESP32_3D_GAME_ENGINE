#ifndef ENGINE_CONFIG
#define ENGINE_CONFIG
#include <Arduino.h>

// SCREEN
#define HEIGHT            320 // Resolution to render
#define WIDTH             240 // Resolution to render
#define CENTER_X          120 // Half of resolution to render
#define CENTER_Y          160 // Half of resolution to render
#define COLOR_DEPTH       16  // Don't change this, because I work with pointers of 16 bits.

// SCENE
#define BACKGROUND        0x0C63 // Swap the bytes: 0x630C becomes 0x0C63 for correct color rendering.
#define MINIMUM_BRIGHTNESS  0.4
#define MAX_VERTICES      500
#define MAX_TRIANGLES     1000
#define MAX_OBJECTS       5
#define NEAR_PLANE        1.0f
#define FAR_PLANE         750.0f
#define FOG_COLOR         0x0C63 // Swap the bytes: 0x630C becomes 0x0C63 for correct color rendering.
#define FOG_START         400.0f
#define FOG_END           750.0f

#endif