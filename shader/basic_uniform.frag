#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;

uniform vec3 lightDir;
uniform vec3 viewPos;
uniform vec3 objectColor;

void main()
{
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, -lightDir), 0.0);

    vec3 ambient = 0.25 * objectColor;
    vec3 diffuse = diff * objectColor;

    FragColor = vec4(ambient + diffuse, 1.0);
}