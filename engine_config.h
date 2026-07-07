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
#define BACKGROUND        0x0000
#define MINIMUM_BRITNESS  0.4
#define MAX_VERTICES      500
#define MAX_TRIANGLES     1000
#define MAX_OBJECTS       5

#endif