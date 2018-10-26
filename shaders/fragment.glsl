#version 300 es

precision mediump float;
uniform float u_time;
uniform sampler2D u_tex;
in vec2 v_uv;
in vec3 v_nor;

out vec4 fragmentColor;

#define gl_FragColor fragmentColor

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

void main() {
  //vec3 col = vec3(v_uv.xy, 0.0);
  //gl_FragColor = vec4(col, 1.0);
  float delta = 0.001;
  //float dx = getHeight(v_pos.xy + vec2(delta, 0.0)) - getHeight(v_pos.xy);
  //float dy = getHeight(v_pos.xy + vec2(0.0, delta)) - getHeight(v_pos.xy);
  vec3 light = normalize(vec3(1.0, 2.0, 0.0));
  float diffuse = dot(light, v_nor);
  gl_FragColor = texture(u_tex, 10.0 * v_uv) * diffuse;
}
