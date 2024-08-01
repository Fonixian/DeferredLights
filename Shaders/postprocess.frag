#version 460

layout(location=0) in vec2 vs_out_tex;
layout(location=0) out vec4 fs_out_col;
layout(binding = 0) uniform sampler2D frameTex;
layout(binding = 1) uniform sampler2D ssao;
layout(binding = 2) uniform sampler2D diffuse;

void main()
{
    //SSAO BLUR
	vec2 texelSize = 1.0 / vec2(textureSize(ssao, 0));
    float result = 0.0;
    for (int x = -2; x < 2; ++x) 
    {
        for (int y = -2; y < 2; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(ssao, vs_out_tex + offset).x;
        }
    }
    result = result * 0.0625; // result / 16

    vec4 diffuseColor =  texture(diffuse, vs_out_tex);
    vec3 color = texture(frameTex, vs_out_tex).xyz + diffuseColor.xyz * result * vec3(0.2);
	fs_out_col = vec4(color, diffuseColor.a);
    //fs_out_col = vec4(vec3(result),1);
}