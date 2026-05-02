#version 330 core
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 lightDir;
uniform vec3 viewPos;

void main()
{
    // Normalize vectors
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 lightDirNorm = normalize(-lightDir);
    
    // Ambient lighting
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * objectColor;
    
    // Diffuse lighting with toon quantization (4 levels)
    float diff = max(dot(norm, lightDirNorm), 0.0);
    diff = floor(diff * 4.0) / 4.0;  // Quantize to 4 levels
    vec3 diffuse = diff * objectColor;
    
    // Specular lighting with toon quantization
    float specularStrength = 0.5;
    vec3 reflectDir = reflect(-lightDirNorm, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    spec = step(0.5, spec);  // Hard edge specular (on/off)
    vec3 specular = specularStrength * spec * vec3(1.0);
    
    // Combine
    vec3 result = ambient + diffuse + specular;
    
    // Toon outline effect: darken edges
    float edgeFactor = dot(norm, viewDir);
    if (edgeFactor < 0.2)
    {
        // Darken near edges for outline effect
        result *= 0.6;
    }
    
    FragColor = vec4(result, 1.0);
}
