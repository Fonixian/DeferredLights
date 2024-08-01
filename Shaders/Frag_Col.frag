#version 430

// Per fragment variables coming from the pipeline
in vec3 vs_out_color;

// Outgoing values - fragment color
out vec4 fs_out_col;

void main()
{
	fs_out_col = vec4( vs_out_color, 1.0 );
}
