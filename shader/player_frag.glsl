#version 450 core
in vec2 texCoord;
in vec2 texCoordMask;
out vec4 FragColor;

uniform sampler2D playerTex;
uniform sampler2D playerTexMask;

void main() {
  vec4 playerTexColor     = texture(playerTex, texCoord);
  vec4 playerTexMaskColor = texture(playerTexMask, texCoordMask);
  FragColor = mix(playerTexColor, playerTexMaskColor, playerTexMaskColor.a);
  // FragColor = playerTexMaskColor;
}
