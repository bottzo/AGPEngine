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
	//vec2 tCoords = gl_FragCoord.xy / textureSize(uTextureAlb, 0);
	vec2 tCoords = lTexCoord;
	vec3 albedo = texture(uTextureAlb,tCoords).rgb;
	vec3 normals = texture(uTextureNorm,tCoords).rgb;
	vec3 depth = texture(uTextureDepth,tCoords).rgb;
	vec3 position = texture(uTexturePos,tCoords).rgb;

	vec3 lightDir = normalize(uLight.direction);

	//diffuse
	oColor = vec4(albedo,1.);
	float intensity = max(dot(normals, lightDir),0.0);
	vec3 difCol = uLight.color * intensity;

	//specular
	float matSpecularity = 160.;
	vec3 cameraFragDir = normalize(cameraPos - position);
	vec3 reflectDir = normalize(reflect(lightDir, normals));
	float spec = dot(cameraFragDir, reflectDir);
	//spec = clamp(spec,0.,1.);
	vec3 specCol = vec3(0.);
	if (spec > 0) {
		spec = pow(spec, matSpecularity);
		specCol = uLight.color * spec;
	}
	
	oColor *= vec4(specCol * 0.2f + difCol * 0.8f, 1.); 

	//oColor = vec4(specCol, 1.0);
}
#endif
#endif