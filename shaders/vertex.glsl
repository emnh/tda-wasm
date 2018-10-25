precision mediump float;
attribute vec4 position;
uniform float u_time;

vec2 rotate(vec2 v, float a) {
	float s = sin(a);
	float c = cos(a);
	mat2 m = mat2(c, s, -s, c);
	return m * v;
}

void main()
{
  vec3 pos = position.xyz;
  float theta = u_time;
  pos.x += 0.25;
  pos.x *= 2.0;
  pos.xy = rotate(pos.xy, theta);
  gl_Position = vec4(pos, 1.0);
}
