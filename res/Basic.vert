#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject
{
    mat4 ModelViewProjectionMatrix;
    float worldScaleFactor;
    int renderLightmaps;
    int useLightmaps;
    int useAlphaTest;
} ubo;

layout(location = 0) in vec3 inVertex;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec2 inTexCoordLightmap;

layout(location = 0) out vec2 TexCoord;
layout(location = 1) out vec2 TexCoordLightmap;
layout(location = 2) out int renderLightmaps;
layout(location = 3) out int useLightmaps;
layout(location = 4) out int useAlphaTest;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = ubo.ModelViewProjectionMatrix * vec4(inVertex * ubo.worldScaleFactor, 1.0);
    TexCoord = inTexCoord;
    TexCoordLightmap = inTexCoordLightmap;
    renderLightmaps = ubo.renderLightmaps;
    useLightmaps = ubo.useLightmaps;
    useAlphaTest = ubo.useAlphaTest;
}
