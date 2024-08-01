#version 460

layout(location = 0) in vec4 FragPos;

layout(location = 1) uniform vec3 lightPos;
layout(location = 2) uniform float radius;

void main(){
	float lightDistance = length(FragPos.xyz - lightPos);
	lightDistance = lightDistance / radius;
	gl_FragDepth = lightDistance;
}