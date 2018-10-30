#version 300 es

precision mediump float;
in vec4 position;
in vec2 uv;
in vec3 normal;
out vec2 v_uv;
out vec3 v_pos;
out vec3 v_nor;
out vec4 v_water;
out vec3 v_waterNormal;
uniform float u_time;
uniform vec3 u_light;
uniform vec3 u_eye;
uniform mat4 u_mvp;
uniform sampler2D u_heightmap;
uniform sampler2D u_watermap;

// TODO: rename file

const float heightMultiplier = 20.0;

vec4 getHeightAndNormal(vec2 pos) {
  vec4 ret = texture(u_heightmap, pos);
  ret.xz *= heightMultiplier;
  ret.yzw = normalize(ret.yzw);
  return ret;
}

vec4 getWaterHeightAndNormal(vec2 pos) {
  vec4 ret = texture(u_watermap, pos);
  ret *= heightMultiplier;
  //ret.xz *= heightMultiplier;
  //ret.yzw = normalize(ret.yzw);
  return ret;
}

float getTotalHeight(vec2 pos) {
  return getHeightAndNormal(pos).x + getWaterHeightAndNormal(pos).x;
}

vec3 getWaterNormal(vec2 pos) {
  // TODO: waterResolution as uniform
  float waterResolution = 256.0;
  vec2 dx = vec2(1.0 / waterResolution, 0.0);
  vec2 dy = vec2(0.0, 1.0 / waterResolution);
  vec2 ndx = dx; //vec2(0.0);
  vec2 ndy = dy; //vec2(0.0);
  float x = getHeightAndNormal(pos + dx).x + getWaterHeightAndNormal(pos + dx).x;
  x -= getHeightAndNormal(pos - ndx).x + getWaterHeightAndNormal(pos - ndx).x;
  float y = getHeightAndNormal(pos + dy).x + getWaterHeightAndNormal(pos + dy).x;
  y -= getHeightAndNormal(pos - ndy).x + getWaterHeightAndNormal(pos - ndy).x;
  vec3 normal = normalize(vec3(2.0 * x, -4.0 * heightMultiplier / waterResolution, 2.0 * y));
  return normal;
}

void main()
{
  vec3 pos = position.xyz;
  float theta = u_time;
  //vec2 c = uv;
  vec2 c = pos.xy / 100.0 + 0.5;
  v_uv = c + 0.0 * uv;
  vec4 hmn = getHeightAndNormal(c);
  vec4 wmn = getWaterHeightAndNormal(c);
  v_water = wmn;
  pos.z = hmn.x + wmn.x;
  pos.xyz = pos.xzy;

  vec3 nor = hmn.yzw + 0.0 * normal;
  v_nor = nor;
  v_pos = pos;
  v_waterNormal = getWaterNormal(c);
  v_nor = v_waterNormal;

  //pos = rotate(pos, vec3(0.0, 1.0, 1.0), sin(theta));
  //v_nor = rotate(nor, vec3(0.0, 1.0, 1.0), sin(theta));
  gl_Position = u_mvp * vec4(pos, 1.0);
  //gl_Position = vec4(pos, 1.0);
}
