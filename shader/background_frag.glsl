#version 450 core
in vec2 texCoord;
in vec4 testColor;
out vec4 FragColor;

uniform bool debug           = false;
uniform bool debug_noise_vis = false;
uniform sampler2D backgroundTex;

void main() {
  FragColor = texture(backgroundTex, texCoord);
  if (debug) {
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
  }
  if (debug_noise_vis) {
    FragColor = mix(FragColor, testColor, 0.5);
  }
}
