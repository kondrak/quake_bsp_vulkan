#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inVertex;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec2 TexCoord;
layout(location = 1) out vec3 textColor;

void main()
{
    gl_Position = vec4(inVertex, 1.0);
    TexCoord  = inTexCoord;
    textColor = inColor;
}
 