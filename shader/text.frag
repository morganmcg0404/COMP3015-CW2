#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D glyphTexture;
uniform vec3 textColor;

void main()
{
    // Sample the glyph bitmap (RED channel contains the alpha data)
    float alpha = texture(glyphTexture, TexCoords).r;
    
    // Discard fully transparent pixels
    if (alpha < 0.01)
        discard;
    
    // Mix text color with the sampled alpha
    FragColor = vec4(textColor, alpha);
}
