#include "gf_math.h"

float dotf(float xa, float ya, float xb, float yb) {
  return (xa * xb) + (ya * yb);
}

float frandom(vec2s v) {
  return fabsf(fmodf(sinf(dotf(v.x, v.y, 12.9898, 78.233)) * 43758.5453123, 1.0));
}

float gf_noise(vec2s v, float freq) {
	v.x *= freq;
	v.y *= freq;
  return frandom(v);
}

