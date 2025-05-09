#include "noise.h"

int main(int argc, char *argv[]) {
  int x, y;

  for (y = 0; y < 50; y++) {
    printf("|");
    for (x = 0; x < 50; x++) {
      int val = floorf(gf_noise((vec2s){x, y}, 0.1) * 255);
      printf("\x1b[48;2;%i;%i;%im \x1b[0m", val, val, val);
    }
    printf("|\n");
  }

  return 0;
}
