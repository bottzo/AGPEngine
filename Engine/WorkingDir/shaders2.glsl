///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef TEXTURED_PATRICE

#if defined(VERTEX) ///////////////////////////////////////////////////

// TODO: Write your vertex shader here
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;
//layout(location=3) in vec3 aTangent;
//layout(location=4) in vec3 aBiTangent;

struct Light
{
	vec3 color;
	vec3 direction;
	vec3 position;
	unsigned int type;
};
layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vPosition;
out vec3 vNormal;
out vec3 uViewDir;

void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0) );
	vNormal = normalize(vec3( uWorldMatrix * vec4(aNormal, 0.0) ));
	uViewDir = uCameraPosition - vPosition;
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;
in vec3 uViewDir;

uniform sampler2D uTexture;

struct Light
{
	vec3 color;
	vec3 direction;
	vec3 position;
	unsigned int type;
};
layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

layout(location = 0)out vec4 oColor;
layout(location = 1)out vec4 albedo;
layout(location = 2)out vec4 nColor;
layout(location = 3)out vec4 depth;

void main()
{
	albedo = texture(uTexture,vTexCoord);
	nColor = vec4(vNormal,1.);
	depth = vec4(vec3(gl_FragCoord.z),1.);

	oColor = albedo;
	for(int i = 0; i < uLightCount && i < 16; ++i)
	{
		if(uLight[i].type == 0)  //directional light
		{
			float intensity = max(dot(vNormal, normalize(uLight[i].direction)),0.0);
			oColor += vec4( uLight[i].color * intensity, 0.0 );
		}
		else if(uLight[i].type == 1) //point light
		{
			float constant = uLight[i].direction.x;
			float linear = uLight[i].direction.y;
			float quadratic = uLight[i].direction.z;
			float distance = length(uLight[i].position - vPosition);
			float attenuation = 1.0 / (constant + linear*distance + quadratic*distance*distance);
			vec3 lDir = normalize(uLight[i].position - vPosition);
			float intensity = max(dot(vNormal, lDir),0.0) * attenuation;
			oColor += vec4(uLight[i].color * intensity, 0.0);
		}
	}
}
#endif
#endif