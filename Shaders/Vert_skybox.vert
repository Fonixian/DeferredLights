#version 430

// Variables coming from the VBO
layout (location = 0 ) in vec3 vs_in_pos;

// Variables going forward through the pipeline
out vec3 vs_out_pos;

// External parameters of the shader
uniform mat4 world;
uniform mat4 viewProj;

void main()
{
	gl_Position = (viewProj * world * vec4( vs_in_pos, 1 )).xyww;	// [x,y,w,w] => after homogeneous division [x/w, y/w, 1]

	vs_out_pos = vs_in_pos;
}