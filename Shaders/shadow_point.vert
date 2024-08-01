#version 460

layout(location=0) in vec3 vs_in_pos;

layout(location = 0) uniform mat4 M;

void main()
{
	gl_Position = M * vec4( vs_in_pos, 1 );
}