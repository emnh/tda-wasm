#version 300 es

precision mediump float;
uniform float u_time;
uniform vec3 u_light;
uniform sampler2D u_heightmap;
uniform sampler2D u_watermap;
uniform vec2 u_resolution;
uniform vec2 u_axis;
in vec2 v_uv;
in vec3 v_nor;

out vec4 fragmentColor;

#define gl_FragColor fragmentColor

vec2 exchange(float ground, float height, vec2 c, vec2 offset) {
	vec4 nbGround = texture(u_heightmap, c + offset);
	vec4 nbWater = texture(u_watermap, c + offset);
	float nbHeight = nbWater.x;
	float diff = (nbGround.x + nbHeight) - (ground + height);
	float value = clamp(diff * 0.5 * 0.65, -height * 0.25, nbHeight * 0.25);
	return vec2(value, nbHeight - value);
}

void main() {
	vec2 c = gl_FragCoord.xy / u_resolution;

	vec4 water = texture(u_watermap, c);
	float height = water.x;
	
	vec4 hmn = texture(u_heightmap, c);
	float ground = hmn.x;

	vec2 delta = 1.0 / u_resolution;
	vec2 dx = vec2(delta.x, 0.0);
	vec2 dy = vec2(0.0, delta.y);

	float v = water.w;

	vec2 xchg = 
			(
				exchange(ground, height, c, dx) + 
				exchange(ground, height, c, -dx) +
				exchange(ground, height, c, dy) + 
				exchange(ground, height, c, -dy)
			);
	float diff = xchg.x;
	float avg = xchg.y;
	v += (avg - height) * 2.0;
	//v += 1.0 * avgdiff * 2.0;
	//v *= 0.995;

	// Limit v to exchange
	v = clamp(v, -height * 0.25, diff);

	float newHeight = height + v;

	// Clamp
	newHeight = clamp(newHeight, 0.0, 1.0);

  gl_FragColor = vec4(vec3(newHeight, water.yz * 0.5), v);
	gl_FragColor = water;
  //gl_FragColor = vec4(1.0);
}
