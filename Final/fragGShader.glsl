#version 430

in vec2 tc;
in vec3 vNormal, vLightDir, vVertPos, vHalfVec;
in vec4 shadow_coord;

out vec4 fragColor;

struct PositionalLight
{	vec4 ambient, diffuse, specular;
	vec3 position;
};

struct Material
{	vec4 ambient, diffuse, specular;
	float shininess;
};


uniform vec4 globalAmbient;
uniform PositionalLight light;
uniform Material material;
uniform mat4 norm_matrix;
uniform mat4 shadowMVP;
uniform mat4 mv_matrix;
uniform mat4 proj_matrix;

layout (binding=0) uniform sampler2D t;	// for texture
layout (binding=1) uniform sampler2D h;	// for height map
layout (binding=2) uniform sampler2DShadow shadowTex;


void main(void)
{	

	
	vec3 L = normalize(vLightDir);
	vec3 N = normalize(vNormal);
	vec3 V = normalize(-vVertPos);
	vec3 H = normalize(vHalfVec);

	// compute light reflection vector, with respect N:
	vec3 R = normalize(reflect(-L, N));
	
	// get the angle between the light and surface normal:
	float cosTheta = dot(L,N);
	
	// angle between the view vector and reflected light:
	float cosPhi = dot(V,R);

	float inShadow = textureProj(shadowTex, shadow_coord);
	// compute ADS contributions (per pixel):

	vec3 ambient = ((globalAmbient * material.ambient) + (light.ambient * material.ambient)).xyz;
	vec3 diffuse = light.diffuse.xyz * material.diffuse.xyz * max(cosTheta,0.0);
	vec3 specular = light.specular.xyz * material.specular.xyz * pow(max(cosPhi,0.0), material.shininess);

	fragColor = 0.5* texture(t, tc)  + vec4((ambient + diffuse + specular), 1.0) ;
	
	if (inShadow != 0.0)
	{	fragColor += light.diffuse * material.diffuse * max(dot(L,N),0.0)
			+ light.specular * material.specular
			* pow(max(dot(H,N),0.0),material.shininess*3.0);
	}




}
