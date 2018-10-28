#version 300 es

precision mediump float;
uniform float u_time;
uniform vec3 u_light;
uniform sampler2D u_tex;
uniform vec2 u_resolution;
in vec2 v_uv;
in vec3 v_nor;

out vec4 fragmentColor;

#define gl_FragColor fragmentColor

void main() {
  gl_FragColor = texture(u_tex, gl_FragCoord.xy / u_resolution);
}
