///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef TEXTURED_PATRICE

#if defined(VERTEX) ///////////////////////////////////////////////////

// TODO: Write your vertex shader here
layout(location=0) in vec3 aPosition;
//layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;
//layout(location=3) in vec3 aTangent;
//layout(location=4) in vec3 aBiTangent;

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;

	//we will usually not define the clipping scale manually...
	//it is usually computed by the projection matrix. Because
	//we are not passing uniform transforms yet, we increase
	//the clipping scale so that Patrick fits the screen
	float clippingScale = 5.0;
	gl_Position = vec4(aPosition, clippingScale);

	// Patrick look away from the camera by default, so i flip it here
	gl_Position.z = -gl_Position.z;
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;

uniform sampler2D uTexture;

layout(location = 0)out vec4 oColor;

void main()
{
	oColor = texture(uTexture,vTexCoord);
}
#endif
#endif