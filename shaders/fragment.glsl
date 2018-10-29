#version 300 es

precision mediump float;
uniform float u_time;
uniform vec3 u_light;
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


vec3 getDiffuse(vec3 normal) {
  vec3 diffuse = vec3(0.0);
  int maxi = 20;
  float time = u_time / 100.0;
  float noise1 = snoise(vec2(0.0, time));
  float noise2 = snoise(vec2(1234.5234, time));
  float noise3 = (snoise(vec3(v_pos.xz / 10.0, u_time)) + 1.0) / 2.0;
  for (int i = 0; i < maxi; i++) {
    float theta = 3.14 * 2.0 * float(i) / float(maxi - 1) + 2.0 * u_time / 5.0;
    //vec3 color = vec3((cos(theta) + 1.0) / 2.0, float(i) / float(maxi - 1), (sin(theta) + 1.0) / 2.0);
    float r = rand(vec2(float(i), -float(i)));
    float g = rand(vec2(13.423 * float(i), -20.562 * float(i)));
    float b = rand(vec2(17.8925 * float(i), -4.2593 * float(i)));
    vec3 color = vec3(r, g, b);
    //color = normalize(color);
    //vec3 light = 50.0 * vec3(cos(theta), -1.0, sin(theta));
    float rnd1 = rand(vec2(float(i), -float(i)));
    float rnd2 = (rand(vec2(13.423 * float(i), -20.562 * float(i))) - 0.5) * 2.0;
    float rnd3 = (rand(vec2(17.8925 * float(i), -4.2593 * float(i))) - 0.5) * 2.0;
    float lx = cos(theta) * (noise1 + rnd2) / 2.0;
    float ly = sin(theta) * (noise2 + rnd3) / 2.0;
    //lx = sign(lx) * pow(abs(lx), 0.33);
    //ly = sign(ly) * pow(abs(ly), 0.33);
    vec3 light = 5.0 * 1.41 * 50.0 * rnd1 * vec3(lx, 0.0, ly);
    // TODO: 100.0 is size of map
    float radius = 20.0;
    light.y = getTotalHeight(light.xz / 100.0 + 0.5) + radius - 5.0;
    //vec3 lightdir = 5.0 * 1.41 * 50.0 * rnd1 * vec3(lx, -1.0, ly);
    //vec3 lightdir = -(v_pos - closestPointOnCircle(light, radius, v_pos));
    vec3 lightdir = v_pos - light;
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
    diffuse = max(diffuse, 1.0 * color * dot(nlight, normal) / d);
  }
  maxi = 4;
  for (int i = 0; i < maxi; i++) {
    float theta = 3.14 * 2.0 * float(i) / float(maxi - 1) + 2.0 * u_time / 5.0;
    float theta2 = theta + 2.0 * atan(v_pos.x, v_pos.z);
    float r = rand(vec2(float(i), -float(i)));
    float g = rand(vec2(13.423 * float(i), -20.562 * float(i)));
    float b = rand(vec2(17.8925 * float(i), -4.2593 * float(i)));
    vec3 color = vec3(r, g, b);
    //color = normalize(color);
    vec3 light2 = 50.0 * vec3(cos(theta2), -1.0, sin(theta2));
    float d2 = (sin((atan(light2.x, light2.z) - atan(v_pos.x, v_pos.z))) + 1.0) / 2.0;
    d2 = pow(1.0 + d2, 100.0);
    vec3 nlight = normalize(vec3(normal.x, 0.0, normal.z));
    diffuse = max(diffuse, 2.0 * color * dot(nlight, normal) / (1.0 + d2));
  }
  //diffuse /= float(maxi);
  float intensity = 0.5;
  diffuse += dot(normalize(u_light), normal) * intensity;
  return vec3(diffuse);
}

void main() {
  //vec3 col = vec3(v_uv.xy, 0.0);
  //gl_FragColor = vec4(col, 1.0);
  //float dx = getHeight(v_pos.xy + vec2(delta, 0.0)) - getHeight(v_pos.xy);
  //float dy = getHeight(v_pos.xy + vec2(0.0, delta)) - getHeight(v_pos.xy);
  vec3 light = normalize(u_light);
  // float diffuse = dot(light, v_nor) + 0.5;
  vec3 diffuse = getDiffuse(v_nor);
  vec3 col1 = texture(u_tex, 10.0 * v_uv).rgb;
  //vec4 col2 = texture(u_tex, vec2(rand(v_uv), rand(-v_uv)));
  vec3 col2 = texture(u_tex, v_uv).rgb;
  gl_FragColor = vec4(mix(col1, col2, 0.5) * diffuse, 1.0);
  //gl_FragColor = vec4(diffuse, 1.0);
  vec2 waterVelocity = v_water.yz;
  waterVelocity.x = abs(waterVelocity.x) < 1.0 ? sign(waterVelocity.x) : waterVelocity.x;
  waterVelocity.y = abs(waterVelocity.y) < 1.0 ? sign(waterVelocity.y) : waterVelocity.y;
  if (v_water.x > 0.0) {
    vec3 col3 = vec3(0.0, 0.2, 1.0); // texture(u_waterTex, 20.0 * v_uv).rgb;
    //col3 = vec3(0.0, 0.5, 1.0) * (1.0 + snoise(v_uv + 0.1 * u_time * v_uv * v_water.yz)) / 2.0;
    col3 = col3 + col3 * texture(u_tex, v_uv + 0.1 * u_time * clamp(waterVelocity, -100.0, 100.0)).rgb;
    //float diffuse2 = dot(light, v_waterNormal) + 0.5;
    //vec3 diffuse2 = getDiffuse(v_waterNormal);
    col3 = (col3 + 1.0) * diffuse / 2.0;
    gl_FragColor = vec4(mix(gl_FragColor.rgb * 1.2, col3, clamp(v_water.x, 0.0, 1.0)), 1.0);
  }
}
