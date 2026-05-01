#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in float aAge;
layout(location = 2) in float aLifetime;

uniform mat4 view;
uniform mat4 projection;
uniform float ParticleSize;

out float AgePct;

void main()
{
    AgePct = clamp(aAge / aLifetime, 0.0, 1.0);

    vec4 viewPos = view * vec4(aPosition, 1.0);
    gl_Position = projection * viewPos;

    // Simpler on-screen point size (no division by depth) so particles are visible
    float size = mix(24.0, 8.0, AgePct);
    gl_PointSize = size * ParticleSize;
}