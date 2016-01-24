#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec2 vertexPosition_screenspace;
layout(location = 1) in vec2 vertexUV;

// Output data ; will be interpolated for each fragment.
out vec2 UV;

void main(){
	// Output position of the vertex, in clip space
	gl_Position =  vec4(vertexPosition_screenspace,0,1);
	
	// UV of the vertex. No special space for this one.
	UV = vertexUV;
}

