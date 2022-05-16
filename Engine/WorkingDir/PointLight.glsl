///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef POINT_LIGHT

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;

out vec2 lTexCoord;
out vec3 lPosition;
out vec3 lNormal;

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};


void main()
{
	lTexCoord = aTexCoord;
	lPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0) );
	lNormal = normalize(vec3( uWorldMatrix * vec4(aNormal, 0.0) ));

	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 lTexCoord;
in vec3 lPosition;
in vec3 lNormal;

struct Light
{
	vec3 color;
	vec3 direction;
	float radius; //TODO: use the radius instead of the direction??? if not get rid of the radius variable
};
layout(binding = 0, std140) uniform GlobalParams
{
	uint i; //Current light index
	Light uLight[1];
};

uniform sampler2D uTextureAlb;
uniform sampler2D uTextureNorm;
uniform sampler2D uTextureDepth;
uniform sampler2D uTexturePos;

layout(location = 0)out vec4 oColor;

void main()
{
	vec2 tCoords = gl_FragCoord.xy / textureSize(uTextureAlb, 0);
	//vec2 tCoords = lTexCoord;
	vec3 albedo = texture(uTextureAlb,tCoords).rgb;
	vec3 normals = texture(uTextureNorm,tCoords).rgb;
	vec3 depth = texture(uTextureDepth,tCoords).rgb;
	vec3 position = texture(uTexturePos,tCoords).rgb;

	oColor = vec4(albedo,1.);
	float constant = uLight[i].direction.x;
	float linear = uLight[i].direction.y;
	float quadratic = uLight[i].direction.z;
	float distance = length(lPosition - position);
	float attenuation = 1.0 / (constant + linear*distance + quadratic*distance*distance);
	vec3 lDir = normalize(lPosition - position);
	float intensity = max(dot(normalize(normals), lDir),0.0) * attenuation;
	oColor += vec4(uLight[i].color * intensity, 0.0);
	//oColor = vec4(normals,1.);
}
#endif
#endif