#version 450
layout(binding = 1) uniform sampler2D sTexture;

layout(location = 0) in vec2 TexCoord;
layout(location = 1) in vec3 textColor;

layout(location = 0) out vec4 fragmentColor;

void main()
{
    fragmentColor = texture(sTexture, TexCoord) * vec4(textColor, 1.0);
}
