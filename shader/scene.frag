#version 460 core

in vec3 Position;
in vec3 Normal;
in vec2 TexCoord;

layout(location = 0) out vec4 FragColor;

void main()
{
    // Enhanced Blinn-Phong with multiple lights for better visibility
    vec3 n = normalize(Normal);
    
    // Light 1: From sun (top-right-front)
    vec3 lightDir1 = normalize(vec3(2.0, 3.0, 2.0));
    vec3 viewDir = normalize(-Position);
    vec3 halfDir1 = normalize(lightDir1 + viewDir);
    
    // Ambient
    vec3 ambient = vec3(0.3, 0.3, 0.35);  // Slightly blue-tinted ambient
    
    // Diffuse
    float diff1 = max(dot(n, lightDir1), 0.0);
    vec3 diffuse1 = diff1 * vec3(1.0, 1.0, 0.95) * 0.8;
    
    // Specular
    float spec1 = pow(max(dot(n, halfDir1), 0.0), 32.0);
    vec3 specular1 = spec1 * vec3(1.0) * 1.2;
    
    // Light 2: From opposite side for depth
    vec3 lightDir2 = normalize(vec3(-1.0, 0.5, -1.0));
    vec3 halfDir2 = normalize(lightDir2 + viewDir);
    float diff2 = max(dot(n, lightDir2), 0.0);
    vec3 diffuse2 = diff2 * vec3(0.95, 0.9, 1.0) * 0.3;  // Blue-ish fill light
    
    vec3 baseColor = vec3(0.8, 0.8, 0.8);
    vec3 finalColor = ambient + (diffuse1 + specular1) + diffuse2;
    finalColor *= baseColor;
    
    FragColor = vec4(finalColor, 1.0);
}
