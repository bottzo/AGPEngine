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
uniform sampler2D shadowMap;

uniform mat4 lightSpaceMatrix;

layout(location = 0)out vec4 oColor;

//https://www.youtube.com/watch?v=9g-4aJhCnyY
float HardShadow(vec4 fragPosLightSpace, vec3 normals, vec3 lightDir)
{
	float shadow = 0.0f;
	vec3 lightCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	//if we are inside the frustum of the shadow map projection matrix
	if(lightCoords.z <=1.0f)
	{
		//make it to the same range coordinates as the depth buffer
		lightCoords = (lightCoords + 1.0f) / 2.0f;

		float closestDepth = texture(shadowMap, lightCoords.xy).r;
		float currentDepth = lightCoords.z;

		//bias per evitar els quadradets de shadow
		float bias = max(0.025f * (1.0f - dot(normals,lightDir)), 0.0005f);
		if(currentDepth > closestDepth + bias)
			shadow = 1.0f;
	}
	return shadow;
}

float SoftShadow(vec4 fragPosLightSpace, vec3 normals, vec3 lightDir)
{
	float shadow = 0.0f;
	vec3 lightCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	//if we are inside the frustum of the shadow map projection matrix
	if(lightCoords.z <=1.0f)
	{
		//make it to the same range coordinates as the depth buffer
		lightCoords = (lightCoords + 1.0f) / 2.0f;
		float currentDepth = lightCoords.z;
		//bias per evitar els quadradets de shadow
		float bias = max(0.025f * (1.0f - dot(normals,lightDir)), 0.0005f);

		int sampleRadius = 2;
		vec2 pixelSize = 1.0 / textureSize(shadowMap, 0);
		for(int y = -sampleRadius; y <= sampleRadius; ++y)
		{
			for(int x = -sampleRadius; x <= sampleRadius; ++x)
			{
				float closestDepth = texture(shadowMap, lightCoords.xy + vec2(x,y) * pixelSize).r;
				if(currentDepth > closestDepth + bias)
					shadow += 1.0f;
			}
		}
		//divide by the amount of samples we took
		shadow /= pow((sampleRadius * 2 + 1), 2);
	}
	return shadow;
}

void main()
{
	//vec2 tCoords = gl_FragCoord.xy / textureSize(uTextureAlb, 0);
	vec2 tCoords = lTexCoord;

	vec3 albedo = texture(uTextureAlb,tCoords).rgb;
	vec3 normals = texture(uTextureNorm,tCoords).rgb;
	vec3 depth = texture(uTextureDepth,tCoords).rgb;
	vec3 position = texture(uTexturePos,tCoords).rgb;

	vec3 lightDir = normalize(uLight.direction);

	//ambient
	vec3 ambient = uLight.color * 0.15f; 

	//diffuse
	float intensity = max(dot(normals, lightDir),0.0);
	vec3 difCol = uLight.color * intensity;

	//specular
	float matSpecularity = 160.;
	vec3 viewDir = normalize(cameraPos - position);
	vec3 reflectDir = normalize(reflect(lightDir, normals));
	float spec = dot(viewDir, reflectDir);
	//spec = clamp(spec,0.,1.);
	vec3 specCol = vec3(0.);
	if (spec > 0) {
		spec = pow(spec, matSpecularity);
		specCol = uLight.color * spec *.2f;
	}

	//shadow
	vec4 fragPosLightSpace = lightSpaceMatrix * vec4(position, 1.0);
	//float shadow = HardShadow(fragPosLightSpace, normals, lightDir);
	float shadow = SoftShadow(fragPosLightSpace, normals, lightDir);
	
	oColor = vec4((ambient + (1.-shadow) * (difCol+specCol)) * albedo ,1.);
	//oColor = vec4((ambient + (difCol+specCol)) * albedo ,1.);
	//oColor *= vec4(specCol * 0.2f + difCol * 0.8f, 1.); 

	//oColor = vec4(specCol, 1.0);
}
#endif
#endif