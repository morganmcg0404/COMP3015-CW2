#version 460 core

// Input from VAO
layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec3 VertexVelocity;
layout(location = 2) in float VertexAge;

// For next frame (transform feedback output)
out vec3 Position;
out vec3 Velocity;
out float Age;

// Fragment shader attributes (pass 2)
out float Transparency;
out vec2 TexCoord;

uniform int Pass;
uniform float Time;
uniform float DeltaT;
uniform vec3 Gravity = vec3(0.0, -0.98, 0.0);
uniform vec3 EmitterPos = vec3(0.0, 0.0, 0.0);
uniform mat4 MVP;
uniform mat4 ModelViewMatrix;

// Point sprite offsets for particle geometry
const vec2 offsets[6] = vec2[](
    vec2(-0.5, -0.5),
    vec2(0.5, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

const vec2 texCoords[6] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0)
);

uniform float ParticleLifetime = 5.5;
uniform float ParticleSize = 0.05;

void main()
{
    if(Pass == 1) {
        // Transform feedback pass - update physics
        float age = VertexAge + DeltaT;
        
        vec3 pos = VertexPosition;
        vec3 vel = VertexVelocity;
        
        // Apply gravity
        vel += Gravity * DeltaT;
        
        // Update position
        pos += vel * DeltaT;
        
        // Output for next frame
        Position = pos;
        Velocity = vel;
        Age = age;
    }
    else if(Pass == 2) {
        // Render instanced particle geometry
        float age = VertexAge;
        float agePct = age / ParticleLifetime;
        
        if(age >= 0.0 && age < ParticleLifetime) {
            Transparency = 1.0 - agePct;
            
            // Apply vertex offset for particle quad
            vec3 posCam = vec3(ModelViewMatrix * vec4(VertexPosition, 1.0));
            posCam += vec3(offsets[gl_VertexID] * ParticleSize, 0.0);
            
            gl_Position = MVP * vec4(posCam, 1.0);
            TexCoord = texCoords[gl_VertexID];
        }
        else {
            Transparency = 0.0;
            gl_Position = vec4(0.0);
        }
    }
}
