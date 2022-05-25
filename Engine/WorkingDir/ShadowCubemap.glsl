///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef SHADOW_CUBEMAP

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
};

void main()
{

	gl_Position = uWorldMatrix * vec4(aPosition, 1.0);
}

#elif defined(GEOMETRY) ///////////////////////////////////////////////

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

uniform mat4 shadowMatrices[6];

out vec4 fragPos;

void main()
{
	for(int face = 0; face < 6; ++face)
	{
		gl_Layer = face;
		for(int i = 0; i < 3; ++i)
		{
			fragPos = gl_in[i].gl_Position;
			gl_Position = shadowMatrices[face] * fragPos;
			EmitVertex();
		}
		EndPrimitive();
	}
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec4 fragPos;

uniform vec3 lightPos;
uniform float farPlane;

void main()
{
	gl_FragDepth = length(fragPos.xyz - lightPos) / farPlane;
}
#endif
#endif