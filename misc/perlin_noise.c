#include <math.h>
#include <stdio.h>

float dotf(float xa, float ya, float xb, float yb) {
  return (xa * xb) + (ya * yb);
}

float frandom(float x, float y) {
  return fabsf(fmodf(sinf(dotf(x, y, 12.9898, 78.233)) * 43758.5453123, 1.0));
}

float my_noise(float x, float y) {
  return frandom(x, y);
}

int main(int argc, char *argv[]) {
  int x, y;

  for (y = 0; y < 50; y++) {
    printf("|");
    for (x = 0; x < 50; x++) {
      int val = floorf(my_noise(x, y) * 255);
      printf("\x1b[48;2;%i;%i;%im \x1b[0m", val, val, val);
    }
    printf("|\n");
  }

  return 0;
}
