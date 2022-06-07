///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef REL_GEO_PASS

#if defined(VERTEX) ///////////////////////////////////////////////////

// TODO: Write your vertex shader here
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;
layout(location=3) in vec3 aTangent;
layout(location=4) in vec3 aBiTangent;

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
};

layout(binding = 3, std140) uniform viewProjMat
{
	mat4 uViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vPosition;
out vec3 vNormal;
out vec3 tangentLocalSpace;
out vec3 biTangentLocalSpace;
out vec3 normalLocalSpace;

void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0) );
	vNormal = normalize(vec3( uWorldMatrix * vec4(aNormal, 0.0) ));
	tangentLocalSpace = aTangent;
	biTangentLocalSpace = aBiTangent;
	normalLocalSpace = aNormal;
	gl_Position = uViewProjectionMatrix * uWorldMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;
in vec3 tangentLocalSpace;
in vec3 biTangentLocalSpace;
in vec3 normalLocalSpace;

uniform sampler2D uTexture;
uniform sampler2D normalMap;
uniform sampler2D heightMap;

layout(binding = 2, std140) uniform CameraParams
{
	vec3 cameraPos;
	float zNear;
	float zFar;
};

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
};

layout(location = 0)out vec4 oColor;
layout(location = 1)out vec4 albedo;
layout(location = 2)out vec4 nColor;
layout(location = 3)out vec4 depth;
layout(location = 4)out vec4 outPos;

float LinearizeDepth(float depth)
{
	float z = depth * 2.0 - 1.0;
	return (2.0 * zNear * zFar) / (zFar + zNear - z * (zFar - zNear));
}
void main()
{
	vec3 T = normalize(tangentLocalSpace);
	vec3 B = normalize(biTangentLocalSpace);
	vec3 N = normalize(normalLocalSpace);
	mat3 TBN = mat3(T,B,N);

	//displacement
	vec3 viewDir = normalize((TBN * cameraPos) - (TBN * vPosition));
	float heightScale = 0.05f;
	const float minLayers = 8.0f;
	const float maxLayers = 64.0f;
	float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0f,0.0f,1.0f), viewDir)));
	float layerDepth = 1.0f / numLayers;
	float currentLayerDepth = 0.0f;
	
	vec2 S = viewDir.xy / viewDir.z * heightScale;
	//vec2 S = viewDir.xy * heightScale;
	vec2 deltaUVs = S / numLayers;
	
	vec2 UVs = vTexCoord;
	float currentDepthMapValue = 1.0f - texture(heightMap, UVs).r;
	
	//Loop till the point on the heightmap is "hit"
	while(currentLayerDepth < currentDepthMapValue)
	{
		UVs -= deltaUVs;
		currentDepthMapValue = 1.0f - texture(heightMap, UVs).r;
		currentLayerDepth += layerDepth;
	}
	
	//Apply Occlusion (interpolate with prev value)
	vec2 prevTexCoords = UVs + deltaUVs;
	float afterDepth = currentDepthMapValue - currentLayerDepth;
	float beforeDepth = 1.0f - texture(heightMap, prevTexCoords).r - currentLayerDepth + layerDepth;
	float weight = afterDepth / (afterDepth - beforeDepth);
	UVs = prevTexCoords * weight + UVs * (1.0f - weight);
	
	//discard fragments outside the range of the texture
	if(UVs.x > 1.0 || UVs.y > 1.0 || UVs.x < 0.0 || UVs.y < 0.0)
		discard;

	albedo = texture(uTexture,UVs);
	outPos = vec4(vPosition,1.);
	nColor = vec4(vNormal,1.);
	depth = vec4(vec3(LinearizeDepth(gl_FragCoord.z) / zFar),1.);
	oColor = albedo;


	vec3 tangentSpaceNormal = texture(normalMap, UVs).xyz *2.0 - vec3(1.0);
	vec3 localSpaceNormal = TBN * tangentSpaceNormal;
	//vec3 viewSpaceNormal = normalize(worldViewMatrix * vec4(localSpaceNormal, 0.0)).xyz;
	vec3 worldSpaceNormal = normalize(uWorldMatrix * vec4(localSpaceNormal, 0.0)).xyz;
	nColor = vec4(worldSpaceNormal,1.);
}
#endif
#endif