#version 300 es

precision mediump float;
in vec4 position;
in vec2 uv;
out vec2 v_uv;
uniform float u_time;
uniform mat4 u_mvp;

vec2 rotate(vec2 v, float a) {
  v_uv = uv;
	float s = sin(a);
	float c = cos(a);
	mat2 m = mat2(c, s, -s, c);
	return m * v;
}

mat4 rotationMatrix(vec3 axis, float angle) {
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

vec3 rotate(vec3 v, vec3 axis, float angle) {
	mat4 m = rotationMatrix(axis, angle);
	return (m * vec4(v, 1.0)).xyz;
}

void main()
{
  v_uv = uv;
  vec3 pos = position.xyz;
  float theta = u_time;
  pos = rotate(pos, vec3(0.0, 1.0, 1.0), theta);
  gl_Position = u_mvp * vec4(pos, 1.0);
  //gl_Position = vec4(pos, 1.0);
}
