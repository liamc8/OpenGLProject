#version 430

layout (location=0) in vec3 vertPos;
//layout (location=1) in vec2 texCoord;
//layout (location=2) in vec3 vertNormal;

uniform mat4 shadowMVP;
//layout (binding=1) uniform sampler2D h;	// for height map

void main(void)
{	
	//vec4 p = vec4(vertPos,1.0) + vec4((vertNormal*((texture(h, texCoord).r)/5.0f)),1.0f);
	gl_Position = shadowMVP * vec4(vertPos,1.0);
}
