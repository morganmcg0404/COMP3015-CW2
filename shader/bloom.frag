#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform float bloomStrength;
uniform bool vignetteEnabled;

void main()
{
    vec3 hdrColor = texture(scene, TexCoords).rgb;
    vec3 bloom = texture(bloomBlur, TexCoords).rgb;

    float vignette = 1.0;
    if (vignetteEnabled)
    {
        vec2 centered = TexCoords - vec2(0.5);
        vignette = 1.0 - dot(centered, centered) * 1.35;
        vignette = clamp(vignette, 0.72, 1.0);
    }

    vec3 finalColor = (hdrColor + bloom * bloomStrength) * vignette;

    // Small square crosshair in the center of the screen
    vec2 p = TexCoords - vec2(0.5);
    p.x *= 16.0 / 9.0;
    float hLine = step(abs(p.y), 0.0015) * step(abs(p.x), 0.012);
    float vLine = step(abs(p.x), 0.0015) * step(abs(p.y), 0.012);
    float crosshair = clamp(hLine + vLine, 0.0, 1.0);

    finalColor = mix(finalColor, vec3(1.0), crosshair * 0.9);

    // Keep bloom subtle so the scene does not wash out
    FragColor = vec4(finalColor, 1.0);
}