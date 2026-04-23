#version 460 core

in float Transparency;
in vec2 TexCoord;

layout(location = 0) out vec4 FragColor;

uniform sampler2D ParticleTex;

void main()
{
    // Sample particle texture
    vec4 texColor = texture(ParticleTex, TexCoord);
    
    // Apply transparency based on particle age
    texColor.a *= Transparency;
    
    // Discard fully transparent pixels
    if(texColor.a < 0.01)
        discard;
    
    // Make particles bright for bloom effect
    FragColor = vec4(texColor.rgb * 1.5, texColor.a);
}
