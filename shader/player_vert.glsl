#version 450 core
in vec2 aPos;
in vec2 aTexCoord;
layout(std140) uniform Matrices {
  mat4 projection;
  mat4 model;
};
uniform int spriteIndex = 80;

out vec2 texCoord;

// The player sprite sheet 22x8 sprites.
#define PLAYER_SPRITE_SHEET_CELL_COUNT_WIDTH  22
#define PLAYER_SPRITE_SHEET_CELL_COUNT_HEIGHT 8

void main() {
  vec2 offset = vec2(spriteIndex % PLAYER_SPRITE_SHEET_CELL_COUNT_WIDTH,
                     spriteIndex / PLAYER_SPRITE_SHEET_CELL_COUNT_WIDTH);
  offset /= vec2(PLAYER_SPRITE_SHEET_CELL_COUNT_WIDTH,
                 PLAYER_SPRITE_SHEET_CELL_COUNT_HEIGHT);

  gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
  texCoord    = vec2((aTexCoord.x / PLAYER_SPRITE_SHEET_CELL_COUNT_WIDTH),
                     aTexCoord.y / PLAYER_SPRITE_SHEET_CELL_COUNT_HEIGHT) +
             offset;
}
