#version 300 es

precision mediump float;
uniform float u_time;
uniform vec3 u_light;
uniform sampler2D u_tex;
in vec2 v_uv;
in vec3 v_nor;

out vec4 fragmentColor;

#define gl_FragColor fragmentColor

void main() {
  vec3 light = normalize(u_light);
  float diffuse = dot(light, v_nor) + 0.5;
  vec3 col = vec3(0.0, 0.0, 1.0); 
  gl_FragColor = vec4(col * diffuse, 1.0);
}
