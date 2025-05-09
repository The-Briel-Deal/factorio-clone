#include "gf_math.h"

float dotf(float xa, float ya, float xb, float yb) {
  return (xa * xb) + (ya * yb);
}

float frandom(vec2s v) {
  return fabsf(
      fmodf(sinf(dotf(v.x, v.y, 12.9898, 78.233)) * 43758.5453123, 1.0));
}

float mix(float a, float b, float fract) {
  assert(fract <= 1.0);
  assert(fract >= 0.0);
  a *= fract;
  b *= (1 - fract);

  return a + b;
}

float fade(float f) {
  return (6 * powf(f, 5)) - (15 * pow(f, 4)) + (10 * pow(f, 3));
}

float gf_noise(vec2s v, float freq) {
  v.x *= freq;
  v.y *= freq;

  float top   = ceilf(v.y);
  float bot   = floorf(v.y);
  float right = ceilf(v.x);
  float left  = floorf(v.x);

  vec2s top_left  = {top, left};
  vec2s top_right = {top, right};
  vec2s bot_left  = {bot, left};
  vec2s bot_right = {bot, right};

  float top_left_rand  = frandom(top_left);
  float top_right_rand = frandom(top_right);
  float bot_left_rand  = frandom(bot_left);
  float bot_right_rand = frandom(bot_right);

  float top_mixed = mix(top_right_rand, top_left_rand, v.x - left);
  float bot_mixed = mix(bot_right_rand, bot_left_rand, v.x - left);
  float mixed     = mix(top_mixed, bot_mixed, v.y - bot);
  float faded     = fade(mixed);

  return mixed;
}
