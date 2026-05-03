#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 NormalOut;
layout (location = 2) out vec4 ObjectIdOut;

in vec3 Normal;
in vec3 FragPos;
in vec4 FragPosLightSpace;

uniform vec3 lightDir;
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform sampler2D shadowMap;
uniform float objectId;

void main()
{
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, -lightDir), 0.0);

    vec3 ambient = 0.25 * objectColor;
    vec3 diffuse = diff * objectColor;

    // Shadow calculation
    vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float shadow = 0.0;
    if (projCoords.x >= 0.0 && projCoords.x <= 1.0 &&
        projCoords.y >= 0.0 && projCoords.y <= 1.0 &&
        projCoords.z >= 0.0 && projCoords.z <= 1.0)
    {
        float currentDepth = projCoords.z;
        float ndotl = max(dot(norm, -lightDir), 0.0);
        float bias = max(0.0015 * (1.0 - ndotl), 0.00008);

        // 5x5 PCF further smooths shadow pixelation.
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

    vec3 lighting = ambient + (1.0 - shadow) * diffuse;
    FragColor = vec4(lighting, 1.0);

    // Pack normal (world-space) into 0..1 range for post-processing edge detection
    NormalOut = vec4(normalize(norm) * 0.5 + 0.5, 1.0);
    ObjectIdOut = vec4(objectId, 0.0, 0.0, 1.0);
}