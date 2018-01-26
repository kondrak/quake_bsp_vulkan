#version 450

layout(binding = 1) uniform sampler2D sTexture;
layout(binding = 2) uniform sampler2D sLightmap;

layout(location = 0) in vec2 TexCoord;
layout(location = 1) in vec2 TexCoordLightmap;
layout(location = 2) flat in int renderLightmaps;
layout(location = 3) flat in int useLightmaps;
layout(location = 4) flat in int useAlphaTest;

layout(location = 0) out vec4 fragmentColor;

void main()
{
    vec4 baseTex  = texture(sTexture, TexCoord);
    vec4 lightMap = texture(sLightmap, TexCoordLightmap);

    if(renderLightmaps == 1)
    {
        fragmentColor = lightMap * 1.2;
    }
    else
    {
        if(useLightmaps == 0)
        {
            lightMap = vec4(1.0, 1.0, 1.0, 1.0);
        }

        if(useAlphaTest == 1)
        {
            if(baseTex.a >= 0.05)
                fragmentColor = baseTex * lightMap * 2.0; // make the output more vivid
            else
                discard;
        }
        else
        {
            fragmentColor = baseTex * lightMap * 2.0; // make the output more vivid
        }
    }
}
