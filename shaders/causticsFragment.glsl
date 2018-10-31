#version 300 es

precision mediump float;
uniform float u_time;
uniform float u_sunIntensity;
uniform float u_lightsIntensity;
uniform float u_beamIntensity;
uniform float u_heightMultiplier;
uniform float u_lightRadius;
uniform float u_waterNormalFactor;
uniform float u_fresnel;
uniform float u_occlusion;
uniform float u_debugTest;
uniform int u_refractMethod;
uniform int u_reflectMethod;
uniform int u_numLights;
uniform vec2 u_mapSize;
uniform vec3 u_light;
uniform vec3 u_eye;
uniform sampler2D u_tex;
uniform samplerCube u_sky;
uniform sampler2D u_waterTex;
uniform sampler2D u_heightmap;
uniform sampler2D u_watermap;
in vec2 v_uv;
in vec3 v_pos;
in vec4 v_water;
in vec3 v_groundNormal;
in vec3 v_waterNormal;
in vec3 v_raw;
in vec3 v_oldPos;
in vec3 v_newPos;

out vec4 fragmentColor;

const float eps = 1.0e-10;

#define gl_FragColor fragmentColor

void main() {
  gl_FragColor.rgba = vec4(v_uv, 0.0, 1.0);
	float oldArea = length(dFdx(v_oldPos)) * length(dFdy(v_oldPos));
  float newArea = length(dFdx(v_newPos)) * length(dFdy(v_newPos));
  gl_FragColor = vec4(oldArea / newArea * 0.2);
}
