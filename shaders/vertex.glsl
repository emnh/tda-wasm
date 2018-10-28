#version 300 es

precision mediump float;
in vec4 position;
in vec2 uv;
in vec3 normal;
out vec2 v_uv;
out vec3 v_nor;
uniform float u_time;
uniform mat4 u_mvp;
uniform sampler2D u_heightmap;

// TODO: rename file

vec4 getHeightAndNormal(vec2 pos) {
  vec4 ret = texture(u_heightmap, pos);
  ret.xz *= 20.0;
  ret.yzw = normalize(ret.yzw);
  return ret;
}

void main()
{
  v_uv = uv;
  vec3 pos = position.xyz;
  float theta = u_time;
  vec2 c = uv;
  vec4 hmn = getHeightAndNormal(c);
  pos.z = hmn.x;
  pos.xyz = pos.xzy;

  vec3 nor = hmn.yzw + 0.0 * normal;
  v_nor = nor;

  //pos = rotate(pos, vec3(0.0, 1.0, 1.0), sin(theta));
  //v_nor = rotate(nor, vec3(0.0, 1.0, 1.0), sin(theta));
  gl_Position = u_mvp * vec4(pos, 1.0);
  //gl_Position = vec4(pos, 1.0);
}
