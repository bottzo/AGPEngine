///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef POINT_LIGHT

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;

out vec2 lTexCoord;
out vec3 lNormal;
out vec3 lPosition;

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

struct Light
{
	vec3 color;
	vec3 direction;
	float radius; //TODO: use the radius instead of the direction??? if not get rid of the radius variable
};
layout(binding = 0, std140) uniform GlobalParams
{
	Light uLight;
};

void main()
{
	lTexCoord = aTexCoord;
	vec3 lFrag = vec3(uWorldMatrix * vec4(aPosition, 1.0) );
	lNormal = normalize(vec3( uWorldMatrix * vec4(aNormal, 0.0)));

	//center of the sphere
	lPosition.xyz = lFrag + (-lNormal * uLight.radius);
	//counter position of the sphere
	//vec3 lFragCounter = lFrag + -lNormal * uLight.radius * 2.;

	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 lTexCoord;
in vec3 lNormal;
in vec3 lPosition;

struct Light
{
	vec3 color;
	vec3 direction;
	float radius; //TODO: use the radius instead of the direction??? if not get rid of the radius variable
};
layout(binding = 0, std140) uniform GlobalParams
{
	Light uLight;
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
	float constant = uLight.direction.x;
	float linear = uLight.direction.y;
	float quadratic = uLight.direction.z;
	float distance = length(lPosition - position);
	float attenuation = 1.0 / (constant + linear*distance + quadratic*distance*distance);
	vec3 lDir = normalize(lPosition - position);//lNormal;
	float intensity = max(dot(normals, lDir),0.0) * attenuation;
	oColor *= vec4(uLight.color * intensity, 1.0);
	//oColor = vec4(vec3(intensity), 1.0);
}
#endif
#endif