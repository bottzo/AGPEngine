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

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
};

layout(binding = 3, std140) uniform viewProjMat
{
	mat4 uViewProjectionMatrix;
};

void main()
{
	lTexCoord = aTexCoord;
	vec3 lFrag = vec3(uWorldMatrix * vec4(aPosition, 1.0) );
	lNormal = normalize(vec3( uWorldMatrix * vec4(aNormal, 0.0)));

	//center of the sphere
	//lPosition.xyz = lFrag + (-lNormal * uLight.radius);

	gl_Position = uViewProjectionMatrix * uWorldMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 lTexCoord;
in vec3 lNormal;

struct Light
{
	vec3 color;
	vec3 direction;
	float radius; //TODO: use the radius instead of the direction??? if not get rid of the radius variable
	vec3 pos;
};
layout(binding = 0, std140) uniform GlobalParams
{
	Light uLight;
};
layout(binding = 2, std140) uniform CameraParams
{
	vec3 cameraPos;
	float zNear;
	float zFar;
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

	//ambient
	vec3 ambient = uLight.color * 0.15f;

	//diffuse
	float constant = uLight.direction.x;
	float linear = uLight.direction.y;
	float quadratic = uLight.direction.z;
	float distance = length(position - uLight.pos);
	float attenuation = 1.0 / (constant + linear*distance + quadratic*distance*distance);

	vec3 lDir = normalize(position - uLight.pos);//lNormal;
	float intensity = max(dot(normals, lDir),0.0) * attenuation;
	vec3 difCol = uLight.color * intensity;

	//specular
	float matSpecularity = 160.;
	vec3 cameraFragDir = normalize(cameraPos - position);
	vec3 reflectDir = normalize(reflect(lDir, normals));
	float spec = dot(cameraFragDir, reflectDir);
	//spec = clamp(spec,0.,1.);
	vec3 specCol = vec3(0.);
	if (spec > 0) {
		spec = pow(spec, matSpecularity);
		specCol = uLight.color * spec *.2f;
	}

	//oColor = vec4((ambient + (difCol)) * albedo ,1.);
	oColor = vec4((ambient + (difCol+specCol)) * albedo ,1.);
	//oColor *= vec4(specCol * 0.2f + difCol * 0.8f, 1.);
}
#endif
#endif