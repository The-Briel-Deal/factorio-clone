#version 450 core
in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D playerTex;
uniform sampler2D playerTexMask;

void main() {
  vec4 playerTexColor     = texture(playerTex, texCoord);
  vec4 playerTexMaskColor = texture(playerTexMask, texCoord);
  // FragColor = mix(playerTexColor, playerTexMaskColor, 0.5);
  FragColor = playerTexColor;
}
