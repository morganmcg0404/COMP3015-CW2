#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec4 FragPosLightSpace;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 NormalOut;

uniform vec3 objectColor;
uniform vec3 lightDir;
uniform vec3 viewPos;
uniform sampler2D shadowMap;

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
    
    // Keep specular subtle to avoid a camera-following white hotspot on the ground.
    float specularStrength = 0.08;
    vec3 reflectDir = reflect(-lightDirNorm, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 24.0);
    spec = floor(spec * 3.0) / 3.0;
    vec3 specular = specularStrength * spec * objectColor;
    
    // Shadow calculation (same depth map pipeline as standard shader)
    vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float shadow = 0.0;
    if (projCoords.x >= 0.0 && projCoords.x <= 1.0 &&
        projCoords.y >= 0.0 && projCoords.y <= 1.0 &&
        projCoords.z >= 0.0 && projCoords.z <= 1.0)
    {
        float currentDepth = projCoords.z;
        float ndotl = max(dot(norm, lightDirNorm), 0.0);
        float bias = max(0.0015 * (1.0 - ndotl), 0.00008);

        vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
        float pcf = 0.0;
        for (int x = -2; x <= 2; ++x)
        {
            for (int y = -2; y <= 2; ++y)
            {
                float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
                pcf += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
            }
        }
        shadow = pcf / 25.0;
    }

    // Combine with shadow attenuation on direct terms only
    vec3 result = ambient + (1.0 - shadow) * (diffuse + specular);
    
    // Toon outline effect: only darken near true silhouettes.
    // The previous threshold darkened large ground areas at grazing angles,
    // making toon shading look like it only existed in a circle around camera.
    float ndotv = max(dot(norm, viewDir), 0.0);
    float outline = smoothstep(0.08, 0.02, ndotv);
    result = mix(result, result * 0.75, outline);
    
    FragColor = vec4(result, 1.0);

    // Write normals out for edge detection (pack to 0..1)
    NormalOut = vec4(normalize(norm) * 0.5 + 0.5, 1.0);
}
