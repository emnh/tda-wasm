#version 300 es

precision mediump float;
in vec4 position;
in vec2 uv;
in vec3 normal;
out vec2 v_uv;
out vec3 v_nor;
uniform float u_time;
uniform mat4 u_mvp;
uniform vec2 u_resolution;

void main()
{
  v_uv = uv;
  v_nor = normal;
  vec2 pos = position.xy;
  gl_Position = vec4(pos, 0.0, 1.0);
}
