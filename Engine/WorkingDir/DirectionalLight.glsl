///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef DIRECTIONAL_LIGHT

#if defined(VERTEX) ///////////////////////////////////////////////////

// TODO: Write your vertex shader here
layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aTexCoord;

out vec2 lTexCoord;

void main()
{
	lTexCoord = aTexCoord;
	gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

// TODO: Write your fragment shader here

in vec2 lTexCoord;

struct Light
{
	vec3 color;
	vec3 direction;
	float radius;
};
layout(binding = 0, std140) uniform GlobalParams
{
	mat4 uViewMatrix;
	Light uLight;
};

uniform sampler2D uTextureAlb;
uniform sampler2D uTextureNorm;
uniform sampler2D uTextureDepth;
uniform sampler2D uTexturePos;

layout(location = 0)out vec4 oColor;

void main()
{
	//vec2 tCoords = gl_FragCoord.xy / textureSize(uTextureAlb, 0);
	vec2 tCoords = lTexCoord;
	vec3 albedo = texture(uTextureAlb,tCoords).rgb;
	vec3 normals = texture(uTextureNorm,tCoords).rgb;
	vec3 depth = texture(uTextureDepth,tCoords).rgb;
	vec3 position = texture(uTexturePos,tCoords).rgb;

	oColor = vec4(albedo,1.);
	float intensity = max(dot(normals, normalize(uLight.direction)),0.0);
	oColor *= vec4( uLight.color * intensity, 1.0 );
	//oColor = vec4(vec3(intensity), 1.0);
}
#endif
#endif