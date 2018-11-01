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
uniform sampler2D u_caustics;
in vec2 v_uv;
in vec3 v_pos;
in vec4 v_water;
in vec3 v_groundNormal;
in vec3 v_waterNormal;

out vec4 fragmentColor;

const float eps = 1.0e-10;

#define gl_FragColor fragmentColor

float rand(vec2 co){
  return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec3 closestPointOnCircle(vec3 C, float R, vec3 P) {
  vec3 V = P - C;
  return C + V / length(V) * R;
}

vec4 getHeightAndNormal(vec2 pos) {
  vec4 ret = texture(u_heightmap, pos);
  ret.xz *= u_heightMultiplier;
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

float getTotalHeight(vec2 pos) {
  return getHeightAndNormal(pos).x + getWaterHeightAndNormal(pos).x;
}

vec3 getGroundTexture(vec2 uv) {
  vec3 col1 = texture(u_tex, 10.0 * uv).rgb;
  vec3 col2 = texture(u_tex, uv).rgb;
  return mix(col1, col2, 0.5);
}

vec3 getWaterLight(vec3 lightDir, vec3 normal, vec3 eye, bool isSky) {
  vec3 incomingRay = normalize(v_pos - eye);
  const float IOR_AIR = 1.0;
  const float IOR_WATER = 1.333;
  float refractIndex = IOR_AIR / IOR_WATER;

  vec3 overWaterNormal = normal;

  vec3 refractionDir = normalize(refract(incomingRay, overWaterNormal, refractIndex));
  vec3 refractionDirLight = normalize(refract(lightDir, overWaterNormal, refractIndex));

  float wh = v_water.x;
  float t = wh / refractionDir.y;
  vec3 p = v_pos - abs(t) * refractionDir;
  vec2 uv = p.xz / u_mapSize + 0.5;
  
  vec3 groundNormal = getHeightAndNormal(uv).yzw;
  vec3 bottomLight = normalize(reflect(refractionDir, groundNormal));
  float t2 = abs(bottomLight.y) / wh;
  vec3 p2 = p + abs(t2) * bottomLight;
  vec2 uv2 = p2.xz / u_mapSize + 0.5;
  vec3 waterNormal = vec3(0.0, -1.0, 0.0);
  //vec3 waterNormal = getWaterNormal(uv2);
  //waterNormal *= -1.0;
  vec3 refractionDir2 = normalize(refract(bottomLight, waterNormal, 1.0 / refractIndex));
	float refractedLight = 0.0;
	if (u_refractMethod == 0 || u_refractMethod == 4) {
		refractedLight += 1.0 * clamp(dot(refractionDir2, lightDir), 0.0, 1.0);
	}
	if (u_refractMethod == 1 || u_refractMethod == 4) {
		refractedLight += 1.0 * clamp(dot(refractionDir, refractionDirLight), 0.0, 1.0);
	}
	if (u_refractMethod == 2 || u_refractMethod == 4) {
		refractedLight += 1.0 * clamp(dot(bottomLight, refractionDirLight), 0.0, 1.0);
	}
	if (u_refractMethod == 3 || u_refractMethod == 4) {
		refractedLight += 1.0 * clamp(dot(bottomLight, lightDir), 0.0, 1.0);
	}
	if (u_refractMethod == 4) {
		refractedLight *= 0.25;
	}
  refractedLight = max(refractedLight, 0.0);

  float maxDist = length(vec3(u_mapSize, u_heightMultiplier));
  if (abs(t) > maxDist) {
    //t = 0.0;
    //refractedLight = 0.0;
  }
  
  float normalizedDepth = wh / u_heightMultiplier;
  //float rayTravelDist = max(normalizedDepth, (2.0 * abs(t) + abs(t2)) / length(u_mapSize));
  //float rayTravelDist = max(normalizedDepth, (2.0 * abs(t)) / length(u_mapSize));
  //float rayTravelDist = max(normalizedDepth, (2.0 * abs(t)) / length(u_mapSize));
  float rayTravelDist = normalizedDepth;
    
  vec2 waterVelocity = v_water.yz;
  vec3 waterBaseColor = vec3(0.25, 0.5, 1.25);
  vec3 groundColor = normalize(vec3(1.0)) * vec3(length(waterBaseColor));
  groundColor *= getGroundTexture(uv).rgb;
  //float occlusion = clamp(u_occlusion * pow(rayTravelDist, 1.0 + refractedLight), 0.0, 1.0);
  float occlusion = clamp(u_occlusion * rayTravelDist, 0.0, 1.0);
  occlusion = pow(occlusion, u_debugTest) * 2.0;
  vec3 refractedWaterColor = mix(waterBaseColor, 0.2 * waterBaseColor, clamp(occlusion - 1.0, 0.0, 1.0));
  refractedWaterColor = mix(groundColor * waterBaseColor, refractedWaterColor, clamp(occlusion, 0.0, 1.0));

  vec3 sky = vec3(1.0);
  // fresnel code is adapted for top down view, so let eye be such
  eye = vec3(0.0, 100.0, 0.0);
  incomingRay = normalize(v_pos - eye);
  if (isSky) {
    vec3 reflectionDir = normalize(reflect(incomingRay, overWaterNormal));
    if (u_reflectMethod == 1) {
      reflectionDir = normalize(reflect(lightDir, overWaterNormal));
    }
    if (u_reflectMethod == 2) {
      vec3 n = vec3(0.0, 1.0, 0.0);
      reflectionDir = normalize(reflect(lightDir, n));
    }
    if (u_reflectMethod == 3) {
      reflectionDir = normalize(reflect(lightDir, overWaterNormal));
    }
    // lookup sky
    sky = texture(u_sky, reflectionDir).rgb;
    // sun flare
    // sky += vec3(pow(max(0.0, dot(lightDir, reflectionDir)), 5000.0)) * vec3(10.0, 8.0, 6.0);
  } else {
    vec3 reflectionDir = normalize(reflect(incomingRay, overWaterNormal));
    sky = vec3(max(0.0, dot(lightDir, reflectionDir)));
  }

  float fresnel = mix(0.25, 1.0, pow(1.0 - dot(normal, -incomingRay), 3.0));
  fresnel *= u_fresnel;

  vec3 waterColor = mix(refractedWaterColor * refractedLight, sky, fresnel);

  float groundDiffuse = max(0.0, dot(normal, -lightDir));
  vec3 terrainColor = mix(groundColor * groundDiffuse, waterColor, clamp(normalizedDepth * 10.0, 0.0, 1.0));

  return terrainColor;
}

vec3 combine(vec3 d, vec3 a) {
  return d + max(vec3(0.0), a);
}

vec3 getDiffuse(vec3 normal, bool isWater) {

  vec3 eye = u_eye;
  vec3 diffuseWater = vec3(0.0);
  int maxi = u_numLights;
  float time = u_time * 0.5;
  for (int i = 1; i <= maxi; i++) {
    float theta = 3.14 * 2.0 * float(i) / float(maxi - 1); // + 2.0 * time / 5.0;
    float r = rand(vec2(float(i), -float(i)));
    float g = rand(vec2(13.423 * float(i), -20.562 * float(i)));
    float b = rand(vec2(17.8925 * float(i), -4.2593 * float(i)));
    vec3 color = vec3(r, g, b);
    color.g /= 2.0;
    color = normalize(color);
    float rnd1 = rand(vec2(float(i), -float(i)));
    float rnd2 = (rand(vec2(13.423 * float(i), -20.562 * float(i))) - 0.5) * 2.0;
    float rnd3 = (rand(vec2(17.8925 * float(i), -4.2593 * float(i))) - 0.5) * 2.0;
    float sq = round(sqrt(float(maxi)));
    float lx = mod(float(i), sq) / (sq - 1.0) - 0.5;
    float ly = float(i / int(sq)) / (sq - 1.0) - 0.5;
    vec3 light = 50.0 * vec3(lx, 0.0, ly);
    float radius = u_lightRadius;
    vec2 uv = light.xz / u_mapSize + 0.5;
    light.y = getHeightAndNormal(uv).x;
    vec3 closest = closestPointOnCircle(light, radius, v_pos);
    light.xz = closest.xz;
    uv = light.xz / u_mapSize + 0.5;
    light.y = getHeightAndNormal(uv).x;
    vec3 lightdir = v_pos - light;
    float d = distance(light, v_pos);
    vec3 nlight = normalize(lightdir);
    d = pow(d / radius, 2.0);
    d *= 4.0;
    d *= radius;

    float sw = u_waterNormalFactor;
    vec3 tnormal = normalize(vec3(sw, 1.0, sw) * normal);
    diffuseWater =
      combine(diffuseWater, u_lightsIntensity * color * getWaterLight(nlight, tnormal, eye, false) / (eps + d));
  }
  maxi = 2;
  for (int i = 0; i < maxi; i++) {
    float theta = 3.14 * 2.0 * float(i) / float(maxi - 1) + 2.0 * u_time / 5.0;
    float theta2 = theta + 2.0 * atan(v_pos.x, v_pos.z);
    float r = rand(vec2(float(i), -float(i)));
    float g = rand(vec2(13.423 * float(i), -20.562 * float(i)));
    float b = rand(vec2(17.8925 * float(i), -4.2593 * float(i)));
    vec3 color = vec3(r, g, b);
    vec3 light2 = 50.0 * vec3(cos(theta2), -1.0, sin(theta2));
    float d2 = (sin((atan(light2.x, light2.z) - atan(v_pos.x, v_pos.z))) + 1.0) / 2.0;
    float rrm = (sin(1.0 * length(v_pos.xz) - 20.0 * u_time) + 1.0) / 4.0;
    d2 = pow(0.95 + d2 + rrm, 20.0);
    vec3 nlight = normalize(v_pos - vec3(0.0, 0.0, 0.0));

    diffuseWater = combine(diffuseWater, 
        u_beamIntensity * 2.0 * color * getWaterLight(nlight, normal, eye, false) / (eps + d2));
  }
  float intensity = 1.0 * u_sunIntensity;
  
  vec2 uv2 = v_pos.xz / u_mapSize + 0.5;
  float caustic = 2.0 * max(0.2, texture(u_caustics, uv2).x);
  intensity *= (1.0 + caustic);

  diffuseWater = combine(diffuseWater, getWaterLight(normalize(u_light), normal, eye, true) * intensity);
  
  return diffuseWater;
}

void main() {
  vec3 diffuse = getDiffuse(normalize(v_waterNormal), true);
  gl_FragColor.rgba = vec4(diffuse, 1.0);
}
