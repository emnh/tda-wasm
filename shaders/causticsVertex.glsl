#version 300 es

precision mediump float;
in vec4 position;
in vec2 uv;
in vec3 normal;
out vec2 v_uv;
out vec3 v_pos;
out vec4 v_water;
out vec3 v_groundNormal;
out vec3 v_waterNormal;
out vec3 v_ray;
out vec3 v_oldPos;
out vec3 v_newPos;
uniform float u_time;
uniform float u_heightMultiplier;
uniform float u_normalMultiplier;
uniform float u_normalDifference;
uniform float u_causticsScale;
uniform float u_causticsHeight;
uniform int u_normalMethod;
uniform vec2 u_mapSize;
uniform vec3 u_light;
uniform vec3 u_eye;
uniform mat4 u_mvp;
uniform sampler2D u_heightmap;
uniform sampler2D u_watermap;

vec4 getHeightAndNormal(vec2 pos) {
  vec4 ret = texture(u_heightmap, pos);
  //ret.xz *= u_heightMultiplier;
  //ret.yzw = normalize(ret.yzw);
  ret.x *= u_heightMultiplier;
  ret.yzw = normalize(ret.yzw);
  return ret;
}

vec4 getWaterHeightAndNormal(vec2 pos) {
  vec4 ret = texture(u_watermap, pos);
  ret *= u_heightMultiplier;
  //ret.xz *= u_heightMultiplier;
  //ret.yzw = normalize(ret.yzw);
  return ret;
}

float getGroundHeight(vec2 pos) {
  return getHeightAndNormal(pos).x + getWaterHeightAndNormal(pos).x;
}

float getTotalHeight(vec2 pos) {
  return getHeightAndNormal(pos).x + getWaterHeightAndNormal(pos).x;
}

vec3 getWaterNormal(vec2 pos) {
  // TODO: waterResolution as uniform
  float waterResolution = 256.0;
  vec2 dx = vec2(1.0 / waterResolution, 0.0) * u_normalDifference;
  vec2 dy = vec2(0.0, 1.0 / waterResolution) * u_normalDifference;
  vec2 ndx = vec2(0.0);
  vec2 ndy = vec2(0.0);
  float x = getHeightAndNormal(pos + dx).x + getWaterHeightAndNormal(pos + dx).x;
  x -= getHeightAndNormal(pos - ndx).x + getWaterHeightAndNormal(pos - ndx).x;
  float y = getHeightAndNormal(pos + dy).x + getWaterHeightAndNormal(pos + dy).x;
  y -= getHeightAndNormal(pos - ndy).x + getWaterHeightAndNormal(pos - ndy).x;
  float r = length(vec2(dx.x + ndx.x, dx.y + ndx.y));
  vec3 normal =
    normalize(vec3(2.0 * x, 4.0 * u_heightMultiplier * r * u_normalMultiplier, 2.0 * y));
  return normal;
}

// Sobel filter
//float s[9] contains above samples
/*
[6][7][8]
[3][4][5]
[0][1][2]
vec3 n;
n.x = scale * -(s[2]-s[0]+2*(s[5]-s[3])+s[8]-s[6]);
n.y = scale * -(s[6]-s[0]+2*(s[7]-s[1])+s[8]-s[2]);
n.z = 1.0;
n = normalize(n);
*/
vec3 getWaterNormal2(vec2 pos) {
  // TODO: waterResolution as uniform
  float waterResolution = 256.0;
  vec2 dx = vec2(1.0 / waterResolution, 0.0) * u_normalDifference;
  vec2 dy = vec2(0.0, 1.0 / waterResolution) * u_normalDifference;
  float x = +
    (
    getTotalHeight(pos + dx - dy) -
    getTotalHeight(pos - dx - dy) +
    2.0 * (getTotalHeight(pos + dx) - getTotalHeight(pos - dx)) +
    getTotalHeight(pos + dx + dy) -
    getTotalHeight(pos - dx + dy)
    );
  float y = +
    (
    getTotalHeight(pos - dx + dy) -
    getTotalHeight(pos - dx - dy) +
    2.0 * (getTotalHeight(pos + dy) - getTotalHeight(pos - dy)) +
    getTotalHeight(pos + dx + dy) -
    getTotalHeight(pos + dx - dy)
    );
  float r = length(vec2(dx.x + dx.x, dx.y + dx.y));
  vec3 normal =
    normalize(vec3(x, 4.0 * u_heightMultiplier * r * u_normalMultiplier, y));
  return normal;
}

vec3 getGroundNormal2(vec2 pos) {
  // TODO: waterResolution as uniform
  float waterResolution = 256.0;
  vec2 dx = vec2(1.0 / waterResolution, 0.0) * u_normalDifference;
  vec2 dy = vec2(0.0, 1.0 / waterResolution) * u_normalDifference;
  float x = +
    (
    getGroundHeight(pos + dx - dy) -
    getGroundHeight(pos - dx - dy) +
    2.0 * (getGroundHeight(pos + dx) - getGroundHeight(pos - dx)) +
    getGroundHeight(pos + dx + dy) -
    getGroundHeight(pos - dx + dy)
    );
  float y = +
    (
    getGroundHeight(pos - dx + dy) -
    getGroundHeight(pos - dx - dy) +
    2.0 * (getGroundHeight(pos + dy) - getGroundHeight(pos - dy)) +
    getGroundHeight(pos + dx + dy) -
    getGroundHeight(pos + dx - dy)
    );
  float r = length(vec2(dx.x + dx.x, dx.y + dx.y));
  vec3 normal =
    normalize(vec3(x, 4.0 * u_heightMultiplier * r * u_normalMultiplier, y));
  return normal;
}

vec3 project(vec3 origin, vec3 ray, vec3 refractedLight) {
  //float t = 1.0;
  float t = u_causticsHeight * v_water.x;
  origin += ray * t;
  float tplane = (-origin.y - 1.0) / refractedLight.y;
  return origin + refractedLight * tplane;
}
 
void main()
{
  vec3 pos = position.xyz;
  float theta = u_time;
  //vec2 c = uv;
  vec2 c = pos.xy / u_mapSize + 0.5;
  v_uv = c + 0.0 * uv;
  vec4 hmn = getHeightAndNormal(c);
  vec4 wmn = getWaterHeightAndNormal(c);
  v_water = wmn;
  pos.z = hmn.x + wmn.x;
  pos.xyz = pos.xzy;

  v_pos = pos;
  if (u_normalMethod == 0) {
    v_waterNormal = getWaterNormal(c);
    v_groundNormal = hmn.yzw + 0.0 * normal;
  } else {
    v_waterNormal = getWaterNormal2(c);
    v_groundNormal = getGroundNormal2(c) + 0.0 * normal;
  }

	vec3 normal = v_waterNormal;
	//vec3 light = normalize(u_light);
	vec3 light = vec3(0.0, -1.0, 0.0);
	const float IOR_AIR = 1.0;
	const float IOR_WATER = 1.333;
	
	/* project the vertices along the refracted vertex ray */
	vec3 refractedLight = refract(-light, vec3(0.0, 1.0, 0.0), IOR_AIR / IOR_WATER);
	v_ray = refract(-light, normal, IOR_AIR / IOR_WATER);
	vec3 scale = 2.5 * u_causticsScale / vec3(u_mapSize.x, u_heightMultiplier, u_mapSize.y);
	v_oldPos = scale * project(pos.xyz, refractedLight, refractedLight);
	v_newPos = scale * project(pos.xyz + vec3(0.0, v_water.x, 0.0), v_ray, refractedLight);
	
	gl_Position = vec4((v_newPos.xz + refractedLight.xz / refractedLight.y), 0.0, 1.0);

  //gl_Position = vec4(2.0 * c - 1.0, 0.0, 1.0);
}
