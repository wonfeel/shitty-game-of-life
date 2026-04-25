#version 460 core

layout (location = 0) in vec3 VertexPos;
layout (location = 1) in vec3 VertexColor;

out vec3 Color;

uniform float scr_aspect;

void main()
{
	gl_Position = vec4(VertexPos.x, VertexPos.y, VertexPos.z, 1.0);
	Color = VertexColor;
}

