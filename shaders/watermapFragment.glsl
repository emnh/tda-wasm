#version 300 es

precision mediump float;
uniform float u_time;
uniform float u_tick;
uniform float u_centralTurbulence1;
uniform float u_centralTurbulence2;
uniform float u_rain;
uniform float u_evaporation;
uniform float u_viscosity;
uniform float u_waveDecay;
uniform float u_waterTransfer;
uniform vec3 u_light;
uniform sampler2D u_heightmap;
uniform sampler2D u_watermap;
uniform vec2 u_resolution;
uniform vec2 u_axis;
in vec2 v_uv;
in vec3 v_nor;

out vec4 fragmentColor;

#define gl_FragColor fragmentColor

vec4 exchange(float ground, float height, vec2 c, vec2 offset) {
	vec4 nbGround = texture(u_heightmap, c + offset);
	vec4 nbWater = texture(u_watermap, c + offset);
	float nbHeight = nbWater.x;
	float diff = (nbGround.x + nbHeight) - (ground + height);
	float value = clamp(diff * 0.5 * 0.65, -height * 0.5, nbHeight * 0.5);
	//float value = clamp(diff * 0.5, -height * 0.5, nbHeight * 0.5);
	return vec4(value, offset * value, nbHeight * 0.5);
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

	vec4 diff = 
			(
				exchange(ground, height, c, u_axis) + 
				exchange(ground, height, c, -u_axis)
			);
	
	float v = water.w;

	// float timeDelta = u_tick * 60.0 * 2.0;
	float timeDelta = 1.0;
	float transfer = timeDelta * u_waterTransfer * 0.1 * diff.x + v;
	//transfer = clamp(transfer, -height * 0.5, diff.w);
	//transfer = clamp(transfer, diff.x < 0.0 ? diff.x : -height * 0.5, diff.x > 0.0 ? diff.x : 0.0);

	//float newHeight = height + v;
	float newHeight = height + transfer;

	// Rain
	//newHeight += 0.0000002;
	newHeight += 0.0000005 * u_rain;
	if (abs(sin(u_time)) < 0.01) {
		//newHeight += 0.01;
	}

	// Waterfall
	float time = 1.0 * u_time;
	float f = 0.01;
	if (distance(v_uv, vec2(0.5)) < 0.1 && abs(sin(u_time)) < 0.5) {
		newHeight += u_centralTurbulence1 * f * (sin(20.0 * time) + 0.0) * 1.0;
	}
	if (distance(v_uv, vec2(0.5) + 0.25 * vec2(cos(time), sin(time))) < 0.01) {
		newHeight += u_centralTurbulence2 * f;
	}

	// Sink at edges
	if (max(abs(v_uv.x - 0.5), abs(v_uv.y - 0.5)) > 0.49
			// Sink at center
			//distance(v_uv, vec2(0.5)) < 0.01) {
		) {
		//newHeight = 0.0;
		//v = 0.0;
	}
	// Evaporate
	/*
	newHeight *= 0.999997;
	newHeight *= 0.9999;
	*/
	newHeight *= (1.0 - 0.0001 * u_evaporation);
	//newHeight -= 0.0000005 * u_evaporation;

	// Clamp
	newHeight = clamp(newHeight, 0.0, max(0.0, 2.0 - ground));

	// Velocity update
	v = mix(v, newHeight - height, 1.0 - u_viscosity);
	//v *= 0.995;
	v *= (1.0 - 0.005 * u_waveDecay);
	//v *= 0.99995;

	// Clamp
	v = clamp(v, -1.0, 1.0);

  gl_FragColor = vec4(vec3(newHeight, water.yz + u_axis * (newHeight - height)), v);
  //gl_FragColor = vec4(1.0);
}
