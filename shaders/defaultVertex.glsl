#version 300 es

precision mediump float;
in vec4 position;
in vec2 uv;
in vec3 normal;
out vec2 v_uv;
out vec3 v_nor;
uniform float u_time;
uniform mat4 u_mvp;

void main()
{
  v_uv = uv;
	v_nor = normal;
  vec3 pos = position.xyz;
	pos.y += 10.0;
  gl_Position = u_mvp * vec4(pos, 1.0);
}
