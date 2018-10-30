#version 300 es

precision mediump float;
uniform float u_time;
uniform float u_sunIntensity;
uniform float u_lightsIntensity;
uniform float u_beamIntensity;
uniform float u_heightMultiplier;
uniform int u_refractMethod;
uniform vec3 u_light;
uniform vec3 u_eye;
uniform sampler2D u_tex;
uniform sampler2D u_waterTex;
uniform sampler2D u_heightmap;
uniform sampler2D u_watermap;
in vec2 v_uv;
in vec3 v_pos;
in vec3 v_nor;
in vec4 v_water;
in vec3 v_waterNormal;

out vec4 fragmentColor;

const float eps = 1.0e-10;

#define gl_FragColor fragmentColor

// BEGIN ASHIMA

//
// Description : Array and textureless GLSL 2D simplex noise function.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : stegu
//     Lastmod : 20110822 (ijm)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise
//               https://github.com/stegu/webgl-noise
//

vec3 mod289(vec3 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec2 mod289(vec2 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec3 permute(vec3 x) {
  return mod289(((x*34.0)+1.0)*x);
}

float snoise(vec2 v)
  {
  const vec4 C = vec4(0.211324865405187,  // (3.0-sqrt(3.0))/6.0
                      0.366025403784439,  // 0.5*(sqrt(3.0)-1.0)
                     -0.577350269189626,  // -1.0 + 2.0 * C.x
                      0.024390243902439); // 1.0 / 41.0
// First corner
  vec2 i  = floor(v + dot(v, C.yy) );
  vec2 x0 = v -   i + dot(i, C.xx);

// Other corners
  vec2 i1;
  //i1.x = step( x0.y, x0.x ); // x0.x > x0.y ? 1.0 : 0.0
  //i1.y = 1.0 - i1.x;
  i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
  // x0 = x0 - 0.0 + 0.0 * C.xx ;
  // x1 = x0 - i1 + 1.0 * C.xx ;
  // x2 = x0 - 1.0 + 2.0 * C.xx ;
  vec4 x12 = x0.xyxy + C.xxzz;
  x12.xy -= i1;

// Permutations
  i = mod289(i); // Avoid truncation effects in permutation
  vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
		+ i.x + vec3(0.0, i1.x, 1.0 ));

  vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
  m = m*m ;
  m = m*m ;

// Gradients: 41 points uniformly over a line, mapped onto a diamond.
// The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)

  vec3 x = 2.0 * fract(p * C.www) - 1.0;
  vec3 h = abs(x) - 0.5;
  vec3 ox = floor(x + 0.5);
  vec3 a0 = x - ox;

// Normalise gradients implicitly by scaling m
// Approximation of: m *= inversesqrt( a0*a0 + h*h );
  m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );

// Compute final noise value at P
  vec3 g;
  g.x  = a0.x  * x0.x  + h.x  * x0.y;
  g.yz = a0.yz * x12.xz + h.yz * x12.yw;
  return 130.0 * dot(m, g);
}








//
// Description : Array and textureless GLSL 2D/3D/4D simplex 
//               noise functions.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : stegu
//     Lastmod : 20110822 (ijm)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise
//               https://github.com/stegu/webgl-noise
// 

vec4 mod289(vec4 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x) {
     return mod289(((x*34.0)+1.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

float snoise(vec3 v)
  { 
  const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

// First corner
  vec3 i  = floor(v + dot(v, C.yyy) );
  vec3 x0 =   v - i + dot(i, C.xxx) ;

// Other corners
  vec3 g = step(x0.yzx, x0.xyz);
  vec3 l = 1.0 - g;
  vec3 i1 = min( g.xyz, l.zxy );
  vec3 i2 = max( g.xyz, l.zxy );

  //   x0 = x0 - 0.0 + 0.0 * C.xxx;
  //   x1 = x0 - i1  + 1.0 * C.xxx;
  //   x2 = x0 - i2  + 2.0 * C.xxx;
  //   x3 = x0 - 1.0 + 3.0 * C.xxx;
  vec3 x1 = x0 - i1 + C.xxx;
  vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
  vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

// Permutations
  i = mod289(i); 
  vec4 p = permute( permute( permute( 
             i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0 )) 
           + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

// Gradients: 7x7 points over a square, mapped onto an octahedron.
// The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
  float n_ = 0.142857142857; // 1.0/7.0
  vec3  ns = n_ * D.wyz - D.xzx;

  vec4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)

  vec4 x_ = floor(j * ns.z);
  vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

  vec4 x = x_ *ns.x + ns.yyyy;
  vec4 y = y_ *ns.x + ns.yyyy;
  vec4 h = 1.0 - abs(x) - abs(y);

  vec4 b0 = vec4( x.xy, y.xy );
  vec4 b1 = vec4( x.zw, y.zw );

  //vec4 s0 = vec4(lessThan(b0,0.0))*2.0 - 1.0;
  //vec4 s1 = vec4(lessThan(b1,0.0))*2.0 - 1.0;
  vec4 s0 = floor(b0)*2.0 + 1.0;
  vec4 s1 = floor(b1)*2.0 + 1.0;
  vec4 sh = -step(h, vec4(0.0));

  vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

  vec3 p0 = vec3(a0.xy,h.x);
  vec3 p1 = vec3(a0.zw,h.y);
  vec3 p2 = vec3(a1.xy,h.z);
  vec3 p3 = vec3(a1.zw,h.w);

//Normalise gradients
  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;

// Mix final noise value
  vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
  m = m * m;
  return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1), 
                                dot(p2,x2), dot(p3,x3) ) );
}










// END ASHIMA


float rand(vec2 co){
  return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main2()
{
  gl_FragColor.r = gl_FragCoord.x / 640.0;
  gl_FragColor.g = gl_FragCoord.y / 480.0;
  gl_FragColor.b = (sin(5.0 * u_time) + 1.0) / 2.0;
}

void main3( void ) {

  vec2 resolution = vec2(640.0);
  float time = u_time;

	vec2 position = ( gl_FragCoord.xy / resolution.xy );

	float color = 0.0;
	color += sin( position.x * cos( time / 15.0 ) * 80.0 ) + cos( position.y * cos( time / 15.0 ) * 10.0 );
	color += sin( position.y * sin( time / 10.0 ) * 40.0 ) + cos( position.x * sin( time / 25.0 ) * 40.0 );
	color += sin( position.x * sin( time / 5.0 ) * 10.0 ) + sin( position.y * sin( time / 35.0 ) * 80.0 );
	color *= sin( time / 10.0 ) * 0.5;

	gl_FragColor = vec4( vec3( color, color * 0.5, sin( color + time / 3.0 ) * 0.75 ), 1.0 );

}

void main4()
{
  vec2 resolution = vec2(640.0);
  float time = u_time;

  vec2 p = v_uv.xy;
	//vec2 p=(2.0*gl_FragCoord.xy-resolution)/max(resolution.x,resolution.y);
	for(int i=1;i<50;i++)
	{
		vec2 newp=p;
		float speed = 10.0; // speed control
		newp.x+=0.6/float(i)*sin(float(i)*p.y+time/(100.0/speed)+0.3*float(i))+1.0;
		newp.y+=0.6/float(i)*sin(float(i)*p.x+time/(100.0/speed)+0.3*float(i+10))-1.4;
		p=newp;
	}
	vec3 col=vec3(0.5*sin(3.0*p.x)+0.5,0.5*sin(3.0*p.y)+0.5,sin(p.x+p.y));
	gl_FragColor=vec4(col, 1.0);
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
  vec3 normal = normalize(vec3(2.0 * x, -4.0 * u_heightMultiplier / waterResolution, 2.0 * y));
  return normal;
}

float getTotalHeight(vec2 pos) {
  return getHeightAndNormal(pos).x + getWaterHeightAndNormal(pos).x;
}

vec3 getGroundTexture(vec2 uv) {
  vec3 col1 = texture(u_tex, 10.0 * uv).rgb;
  vec3 col2 = texture(u_tex, uv).rgb;
  return mix(col1, col2, 0.5);
}

vec3 getWaterLight(vec3 lightDir, vec3 normal, vec3 eye) {
  //vec3 eye = u_eye;
  //vec3 eye = vec3(v_pos.x, 40.0, v_pos.z);
  //vec3 eye = vec3(1000.0, 0.0, 1000.0);
  //vec3 eyeDir = vec3(0.0, -1.0, 0.0);
  vec3 incomingRay = normalize(v_pos - eye);
  const float IOR_AIR = 1.0;
  const float IOR_WATER = 1.333;
  //float refractIndex = 1.0 / (1.0 / 1.333);
  float refractIndex = IOR_AIR / IOR_WATER;

  //vec3 overWaterNormal = vec3(0.0, 1.0, 0.0);
  vec3 overWaterNormal = normal;
  vec3 reflectionDir = normalize(reflect(incomingRay, overWaterNormal));
  float reflectedLight = dot(reflectionDir, lightDir);

  //float diffuseLight = dot(refractionDir, normal);
  //vec3 refractionDir = normalize(refract(lightDir, incomingRay, refractIndex));
  //vec3 refractionDir = refract(incomingRay, normal, refractIndex);
  //vec3 refractionDir = normalize(refract(lightDir, normal, refractIndex));
  //vec3 overWaterNormal = normal;
  //overWaterNormal = normalize(mix(overWaterNormal, normal, 0.5));
  vec3 refractionDir = normalize(refract(incomingRay, overWaterNormal, refractIndex));
  vec3 refractionDirLight = normalize(refract(lightDir, overWaterNormal, refractIndex));
  //vec3 refractionDir = normalize(refract(incomingRay, normal, refractIndex));
  //float refractedLight = dot(refractionDir2, lightDir);

  float wh = v_water.x; // / u_heightMultiplier;
  float t = wh / refractionDir.y;
  vec3 p = v_pos - abs(t) * refractionDir;
  vec2 uv = p.xz / 100.0 + 0.5;
  wh /= u_heightMultiplier;

  //vec3 groundNormal = vec3(0.0, 1.0, 0.0);
  vec3 groundNormal = getHeightAndNormal(uv).yzw;
  vec3 bottomLight = normalize(reflect(refractionDir, groundNormal));
  float t2 = abs(bottomLight.y) / wh;
  vec3 p2 = p + abs(t2) * bottomLight;
  vec2 uv2 = p2.xz / 100.0 + 0.5;
  //vec3 waterNormal = getWaterNormal(uv2);
  //waterNormal *= -1.0;
  vec3 waterNormal = vec3(0.0, -1.0, 0.0);
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
    
  // TODO: map size 100.0
  vec2 waterVelocity = v_water.yz;
  //vec3 col3 = wh > 0.0 ? 5.0 * vec3(0.0, 0.2, 1.0) : vec3(1.0); // texture(u_waterTex, 20.0 * v_uv).rgb;
  vec3 waterColor = 2.0 * vec3(0.5, 0.5, 2.0);
  //col3 = 0.0 * col3 + col3 * texture(u_tex, (uv + u_time) * 10.0 * clamp(waterVelocity, -100.0, 100.0)).rgb;
  //col3 = 2.0 * col3 * getGroundTexture(uv).rgb; // + u_time) * 10.0 * clamp(waterVelocity, -100.0, 100.0)).rgb;
  waterColor = mix(waterColor, waterColor * 0.2, clamp(pow(wh, 1.0), 0.0, 1.0));
  vec3 groundColor = 4.0 * normalize(vec3(1.0)) * vec3(length(waterColor));
  groundColor *= getGroundTexture(uv).rgb; // + u_time) * 10.0 * clamp(waterVelocity, -100.0, 100.0)).rgb;
  //float occlusion = clamp(pow(wh, 1.0) / pow(1.0 + refractedLight, 10.0), 0.0, 1.0);
  float occlusion = clamp(pow(wh, refractedLight), 0.0, 1.0);
  vec3 col3 = 1.0 * mix(groundColor, waterColor, occlusion);
  //
  vec3 sky = 0.5 * vec3(0.5, 0.5, 1.0);

  float fresnel = mix(0.5, 1.0, pow(1.0 - dot(normal, -incomingRay), 3.0));
  fresnel *= 0.02;
  //fresnel *= occlusion;

  return mix(col3 * refractedLight, sky * reflectedLight, fresnel);
}

vec3 combine(vec3 d, vec3 a) {
  //return max(d, a);
  return d + max(vec3(0.0), a);
}

vec3 getDiffuse(vec3 normal, bool isWater) {

  //vec3 eye = vec3(0.0, 20.0, 0.0);
  vec3 eye = u_eye;
  vec3 diffuse = vec3(0.0);
  vec3 diffuseWater = vec3(0.0);
  int maxi = 25;
  float time = u_time * 0.5;
  float noise1 = snoise(vec2(0.0, time));
  float noise2 = snoise(vec2(1234.5234, time));
  float noise3 = (snoise(vec3(v_pos.xz * 0.1, time)) + 1.0) * 0.5;
  for (int i = 1; i <= maxi; i++) {
    float theta = 3.14 * 2.0 * float(i) / float(maxi - 1); // + 2.0 * time / 5.0;
    //vec3 color = vec3((cos(theta) + 1.0) / 2.0, float(i) / float(maxi - 1), (sin(theta) + 1.0) / 2.0);
    float r = rand(vec2(float(i), -float(i)));
    float g = rand(vec2(13.423 * float(i), -20.562 * float(i)));
    float b = rand(vec2(17.8925 * float(i), -4.2593 * float(i)));
    vec3 color = vec3(r, g, b);
    color.g /= 2.0;
    color = normalize(color);
    //vec3 light = 50.0 * vec3(cos(theta), -1.0, sin(theta));
    float rnd1 = rand(vec2(float(i), -float(i)));
    float rnd2 = (rand(vec2(13.423 * float(i), -20.562 * float(i))) - 0.5) * 2.0;
    float rnd3 = (rand(vec2(17.8925 * float(i), -4.2593 * float(i))) - 0.5) * 2.0;
    //float lx = cos(theta) * (noise1 + rnd2) / 2.0;
    //float ly = sin(theta) * (noise2 + rnd3) / 2.0;
    //vec3 light = 5.0 * 1.41 * 50.0 * rnd1 * vec3(lx, 0.0, ly);
    //float lx = cos(theta);
    //float ly = sin(theta);
    //vec3 light = mix(0.0, 40.0, clamp(pow(float(i) / float(maxi / 2), 0.9), 0.0, 1.0)) * vec3(lx, 0.0, ly);
    float sq = round(sqrt(float(maxi)));
    float lx = mod(float(i), sq) / (sq - 1.0) - 0.5;
    float ly = float(i / int(sq)) / (sq - 1.0) - 0.5;
    vec3 light = 50.0 * vec3(lx, 0.0, ly);
    //lx = sign(lx) * pow(abs(lx), 0.33);
    //ly = sign(ly) * pow(abs(ly), 0.33);
    // TODO: 100.0 is size of map
    float radius = 15.0;
    //light.y = getTotalHeight(light.xz / 100.0 + 0.5) - radius;
    vec2 uv = light.xz / 100.0 + 0.5;
    //light.y = getHeightAndNormal(uv).x + (getWaterHeightAndNormal(uv).x > 0.0 ? 5.0 : 5.0);
    light.y = getHeightAndNormal(uv).x; // + 1.0 * radius * (sin(2.0 * u_time + float(i)) + 0.0) * 1.0;
    //vec3 lightdir = 5.0 * 1.41 * 50.0 * rnd1 * vec3(lx, -1.0, ly);
    //vec3 lightdir1 = (v_pos - closestPointOnCircle(light, radius, v_pos));
    vec3 closest = closestPointOnCircle(light, radius, v_pos);
    light.xz = closest.xz;
    //light = distance(v_pos, light) < distance(v_pos, closest) ? light : closest;
    vec3 lightdir = v_pos - light;
    //vec3 lightdir = mix(lightdir1, lightdir2, 1.0);
    //float d = distance(light.xz, v_pos.xz);
    float d = distance(light, v_pos);
    vec3 nlight = normalize(lightdir);
    /*
    d =
      d <= radius ? 
      pow((sin(1.0 * 2.0 * 3.1415926 * d / radius) + 1.0) * 0.5, 1.01 * noise3) : 
      d;
      */
    d = pow(d / radius, 2.0);
    d *= 4.0;
    //d *= 0.2;
    d *= radius;

    float sw = 0.1; //(sin(1.0 * u_time) + 1.0) * 0.5;
    vec3 tnormal = normalize(vec3(sw, 1.0, sw) * normal);
    diffuseWater =
      combine(diffuseWater, u_lightsIntensity * color * getWaterLight(nlight, tnormal, eye) / (eps + d));

    // diffuse = max(diffuse, 1.0 * color * dot(nlight, normal) / d);
  }
  maxi = 2;
  for (int i = 0; i < maxi; i++) {
    float theta = 3.14 * 2.0 * float(i) / float(maxi - 1) + 2.0 * u_time / 5.0;
    float theta2 = theta + 2.0 * atan(v_pos.x, v_pos.z);
    float r = rand(vec2(float(i), -float(i)));
    float g = rand(vec2(13.423 * float(i), -20.562 * float(i)));
    float b = rand(vec2(17.8925 * float(i), -4.2593 * float(i)));
    vec3 color = vec3(r, g, b);
    // color = normalize(color);
    vec3 light2 = 50.0 * vec3(cos(theta2), -1.0, sin(theta2));
    float d2 = (sin((atan(light2.x, light2.z) - atan(v_pos.x, v_pos.z))) + 1.0) / 2.0;
    float rrm = (sin(1.0 * length(v_pos.xz) - 20.0 * u_time) + 1.0) / 4.0;
    //float rrm = mod(rr, 5.0) > 2.5 ? 1.0 : 0.0;
    d2 = pow(0.95 + d2 + rrm, 20.0);
    //vec3 nlight = normalize(vec3(normal.x, 0.0, normal.z));
    vec3 nlight = normalize(v_pos - vec3(0.0, 0.0, 0.0));

    diffuseWater = combine(diffuseWater, 
        u_beamIntensity * 2.0 * color * getWaterLight(nlight, normal, eye) / (eps + d2));
    
    // diffuse = max(diffuse, 2.0 * color * dot(nlight, normal) / (1.0 + d2));
  }
  //diffuse /= float(maxi);
  float intensity = 1.0 * u_sunIntensity * min(0.1, 1.0 / (eps + length(v_pos.xz / 50.0) / sqrt(2.0)));
  //float intensity = min(2.0, 1.0 / (eps + distance(v_pos, eye) / 50.0) / sqrt(2.0));
  //float intensity = 1.0 * (sin(u_time) + 1.0) * 0.5;
  //float intensity = 2.0;

  //diffuseWater = combine(diffuseWater, getWaterLight(normalize(eye), normal, eye) * intensity);
  diffuseWater = combine(diffuseWater, getWaterLight(normalize(u_light), normal, eye) * intensity);
  
  /*
  time = u_time * 1.0;
  float disp = 0.5;
  float noise4 = (snoise(vec3(10.0 * fract(v_pos.xz * 1.32464 / 100.23542), time)));
  float noise5 = (snoise(vec3(10.0 * fract(v_pos.xz * 1.52343 / 100.42546) + vec2(14.23, 432.23), time)));
  vec3 light = mix(v_pos, v_pos + 0.1 * noise5 * vec3(cos(noise4), 0.0, sin(noise4)), disp);
  float radius = 25.0;
  float height = getHeightAndNormal(light.xz / 100.0 + 0.5).x + radius;
  light.y = height;
  vec3 lightdir = mix(normal, v_pos - light, disp);
  // lightdir =
  //   normalize(vec3(mix(normal.x, noise4, disp), mix(normal.y, 0.0, disp), mix(normal.z, noise5, disp)));
  float d = distance(light, v_pos);
  vec3 nlight = normalize(lightdir);
  d = pow(d / radius, 2.0);
  d *= 0.2;
  normal = vec3(0.0, 1.0, 0.0);
  diffuseWater = combine(diffuseWater, getWaterLight(nlight, normal) / (1.0 + d));
  */

  diffuse += dot(normalize(u_light), normal) * intensity;
  return (isWater ? diffuseWater : diffuse);
}

void main() {
  //vec3 col = vec3(v_uv.xy, 0.0);
  //gl_FragColor = vec4(col, 1.0);
  //float dx = getHeight(v_pos.xy + vec2(delta, 0.0)) - getHeight(v_pos.xy);
  //float dy = getHeight(v_pos.xy + vec2(0.0, delta)) - getHeight(v_pos.xy);
  vec3 light = normalize(u_light);
  // float diffuse = dot(light, v_nor) + 0.5;
  vec3 diffuse = getDiffuse(normalize(v_nor), false);
  //vec4 col2 = texture(u_tex, vec2(rand(v_uv), rand(-v_uv)));
  //vec3 col1 = texture(u_tex, 10.0 * v_uv).rgb;
  //vec3 col2 = texture(u_tex, v_uv).rgb;
  //gl_FragColor = vec4(mix(col1, col2, 0.5) * diffuse, 1.0);
  gl_FragColor.rgb = getGroundTexture(v_uv).rgb * diffuse;
  //gl_FragColor = vec4(diffuse, 1.0);
  vec2 waterVelocity = v_water.yz;
  //waterVelocity.x = abs(waterVelocity.x) < 1.0 ? sign(waterVelocity.x) : waterVelocity.x;
  //waterVelocity.y = abs(waterVelocity.y) < 1.0 ? sign(waterVelocity.y) : waterVelocity.y;
  float wh = v_water.x / u_heightMultiplier;
  if (wh >= 0.0) {
    vec3 col3 = vec3(0.0, 0.2, 1.0); // texture(u_waterTex, 20.0 * v_uv).rgb;
    //col3 = vec3(0.0, 0.5, 1.0) * (1.0 + snoise(v_uv + 0.1 * u_time * v_uv * v_water.yz)) / 2.0;
    col3 = 0.0 * col3 + col3 * texture(u_tex, (v_uv + u_time) * 10.0 * clamp(waterVelocity, -100.0, 100.0)).rgb;
    //float diffuse2 = dot(light, v_waterNormal) + 0.5;
    //vec3 diffuse2 = getDiffuse(v_waterNormal);
    //col3 = (col3 + 1.0) * diffuse / 2.0;
    
    // col3 = 10.0 * mix(1.0, 0.5, v_water.x / u_heightMultiplier) * col3 * diffuse;
    // gl_FragColor = vec4(mix(gl_FragColor.rgb * 1.2, col3, clamp(v_water.x, 0.0, 1.0)), 1.0);
    diffuse = getDiffuse(normalize(v_nor), true);
    gl_FragColor.rgb = diffuse; // mix(gl_FragColor.rgb, diffuse, min(wh / 0.001, 1.0));
  }
  // float minw = 0.1;
  // float maxw = 0.3;
  // gl_FragColor.rgb = mix(gl_FragColor.rgb, diffuse, clamp((wh - minw) / (maxw - minw), 0.0, 1.0));
  gl_FragColor.a = 1.0;
}
