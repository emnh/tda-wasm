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


// END ASHIMA

float sn(vec2 pos) {
	return abs(snoise(pos));
}

float getHeight(vec2 pos) {
	float c1 = 0.1;
	float c2 = 0.002;
	float c3 = 0.0005;
	float c4 = 0.00025;
	float c5 = 0.01;
  float value =
    sn(pos * 1.63)
    * c1 
    + sn(pos * 10.0)
    * c2
    + sn(pos * 20.0)
    * c3
    + sn(pos * 40.0)
    * c4
    + sn(pos * 20.0)
    * c5;
	float minValue = (c1 + c2 + c3 + c4 + c5) * -0.0;
	float maxValue = (c1 + c2 + c3 + c4 + c5) * 1.0;

	/*
	value = 2.0 * (value - minValue) / (maxValue - minValue);
	value = 0.0;
	minValue = 0.0;
	maxValue = 0.0;
	*/
	for (int i = 0; i < 8; i++) {
		float f = 0.6;
		value += pow(f, float(i)) * sn(pos * pow(2.0, float(i)));
		minValue += pow(f, float(i)) * -0.0;
		maxValue += pow(f, float(i)) * 1.0;
	}
	value = 2.0 * (value - minValue) / (maxValue - minValue);

  return value;
}

void main() {
	vec2 c = gl_FragCoord.xy / u_resolution;
  float height = getHeight(c);

  //float delta = 0.001;
  vec2 delta = 1.0 / u_resolution;
  float dx = getHeight(c + vec2(delta.x, 0.0)) - getHeight(c - vec2(delta.x, 0.0));
  float dy = getHeight(c + vec2(0.0, delta.y)) - getHeight(c - vec2(0.0, delta.y));
  vec3 nor = normalize(vec3(2.0 * dx, -4.0 * delta.x, 2.0 * dy));

  gl_FragColor = vec4(height, nor);
}
