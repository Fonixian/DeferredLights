#version 460

layout(location = 6) uniform vec3 color_in;
layout(location = 3) uniform vec3 position_in;

layout(location = 0) uniform mat4 view;
layout(location = 1) uniform float radius;

layout(location = 0) out vec3 color_out;
layout(location = 1) out vec4 position_r_out;


void main()
{
	color_out = color_in;
	vec4 pView = view * vec4(position_in,1);
	position_r_out = vec4(pView.xyz, radius);
}