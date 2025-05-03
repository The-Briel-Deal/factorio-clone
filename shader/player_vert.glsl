#version 450 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 1) uniform mat4 model;
layout(location = 0) uniform mat4 projection;
out vec2 texCoord;

void main() {
  gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
  texCoord    = aTexCoord;
}
