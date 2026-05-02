#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;

uniform vec3 lightDir;
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform bool fogEnabled;
uniform float fogDensity;
uniform vec3 fogColor;

void main()
{
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, -lightDir), 0.0);

    vec3 ambient = 0.25 * objectColor;
    vec3 diffuse = diff * objectColor;
    vec3 result = ambient + diffuse;

    // Apply fog
    if (fogEnabled)
    {
        float distance = length(FragPos - viewPos);
        float fogFactor = exp(-fogDensity * fogDensity * distance * distance);
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        result = mix(fogColor, result, fogFactor);
    }

    FragColor = vec4(result, 1.0);
}