#version 450 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(std140) uniform Matrices {
  mat4 projection;
  mat4 model;
};
out vec2 texCoord;

void main() {
  gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
  texCoord    = aTexCoord;
}
