#version 450 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in int aTexIndex;
layout(location = 1) uniform mat4 model;
layout(location = 0) uniform mat4 projection;
layout(std140) uniform TileState {
  int tileSize;
  int tilesPerRow;
};
layout(location = 5) uniform vec4 textureMap[16];

out vec2 texCoord;

void main() {
  vec4 texCoords     = textureMap[aTexIndex];
  float texTop       = texCoords.x;
  float texBottom    = texCoords.y;
  float texLeft      = texCoords.z;
  float texRight     = texCoords.w;
  int tileWidth      = gl_InstanceID % (tilesPerRow);
  int tileHeight     = gl_InstanceID / (tilesPerRow);
  vec4 tileOffset    = vec4(tileSize * tileWidth, tileSize * tileHeight, 0, 0);
  vec4 worldPosition = model * vec4(aPos, 0.0, 1.0);
  vec4 offsetWorldPosition = worldPosition + tileOffset;
  gl_Position              = projection * offsetWorldPosition;
  texCoord                 = vec2(0.0, 0.0);
  if (aTexCoord.x == 0.0) {
    texCoord.x = texLeft;
  } else {
    texCoord.x = texRight;
  }
  if (aTexCoord.y == 0.0) {
    texCoord.y = texBottom;
  } else {
    texCoord.y = texTop;
  }
}
