///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef NO_FRAGMENT

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;

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

	gl_Position = uViewProjectionMatrix * uWorldMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

void main()
{
}
#endif
#endif