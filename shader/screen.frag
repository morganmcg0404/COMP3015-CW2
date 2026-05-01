#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
uniform sampler2D screenTexture;
uniform bool vignetteEnabled;

void main()
{
    // Sample the HDR color buffer attached to the FBO
    vec3 hdrColor = texture(screenTexture, TexCoords).rgb;

    float vignette = 1.0;
    if (vignetteEnabled)
    {
        // Subtle vignette toward the edges of the screen
        vec2 centered = TexCoords - vec2(0.5);
        vignette = 1.0 - dot(centered, centered) * 1.35;
        vignette = clamp(vignette, 0.72, 1.0);
    }

    // Simple tonemapping (optional): here we just output the sampled color
    FragColor = vec4(hdrColor * vignette, 1.0);
}