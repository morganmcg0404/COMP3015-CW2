#version 330 core

in float AgePct;
out vec4 FragColor;

void main()
{
    // Bright HDR output so particles contribute to bloom
    float intensity = mix(3.5, 1.5, AgePct);
    vec3 hotColor = vec3(1.0, 0.45, 0.0) * intensity;
    FragColor = vec4(hotColor, 1.0);
}