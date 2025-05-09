#include "gf_math.h"
#include <math.h>
#include <stdio.h>

float dotf(float xa, float ya, float xb, float yb) {
  return (xa * xb) + (ya * yb);
}

float frandom(float x, float y) {
  return fabsf(fmodf(sinf(dotf(x, y, 12.9898, 78.233)) * 43758.5453123, 1.0));
}

float gf_noise(vec2s v, float freq) {
  return frandom(v.x, v.y);
}

