#version 460

layout(location=0) in vec2 vs_out_tex0;

out vec4 fs_out_diffuse;

layout(binding = 0) uniform sampler2D texImage;

void main(void) {
	fs_out_diffuse = vec4(texture(texImage, vs_out_tex0).xyz, 1);
}