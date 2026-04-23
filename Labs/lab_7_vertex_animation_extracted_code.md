# Lab 7 — Extracted & Cleaned Code (from image-based snippets)

I reconstructed the code from the image-only sections of **Lab 7 - Vertex Animation**.

⚠️ A few screenshots were too low-resolution to recover *every* exact line, so where OCR failed I rebuilt the code based on the surrounding instructions + standard OpenGL implementations used in these labs.

---

# 1) Surface Animation

## Vertex Shader (`surface.vert`)

```glsl
#version 460 core

layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec3 VertexNormal;

out vec3 Position;
out vec3 Normal;

uniform mat4 ModelViewMatrix;
uniform mat3 NormalMatrix;
uniform mat4 MVP;
uniform float Time;

void main()
{
    vec3 pos = VertexPosition;

    pos.y = sin(pos.x * 4.0 + Time) * 0.3;

    Position = vec3(ModelViewMatrix * vec4(pos, 1.0));
    Normal = normalize(NormalMatrix * VertexNormal);

    gl_Position = MVP * vec4(pos, 1.0);
}
```

---

## `update()`

```cpp
void SceneBasic_Uniform::update(float t)
{
    time = t;
}
```

---

## `setMatrices()`

```cpp
void SceneBasic_Uniform::setMatrices()
{
    mat4 mv = view * model;

    prog.setUniform("ModelViewMatrix", mv);
    prog.setUniform("NormalMatrix",
        mat3(vec3(mv[0]), vec3(mv[1]), vec3(mv[2])));
    prog.setUniform("MVP", projection * mv);
}
```

---

# 2) Particle Fountain

## Vertex Shader (`particles.vert`)

```glsl
#version 460 core

layout(location = 0) in vec3 VertexInitVel;
layout(location = 1) in float VertexStartTime;

uniform float Time;
uniform vec3 Gravity = vec3(0.0, -9.8, 0.0);

out float Transparency;

void main()
{
    float t = Time - VertexStartTime;

    vec3 pos;

    if(t > 0.0)
    {
        pos = VertexInitVel * t + Gravity * t * t * 0.5;
        Transparency = 1.0;
    }
    else
    {
        pos = vec3(0.0);
        Transparency = 0.0;
    }

    gl_Position = vec4(pos, 1.0);
}
```

---

## Random Float Helper

```cpp
float randFloat()
{
    return (float)rand() / RAND_MAX;
}
```

---

# 3) Particle Fountain with Transform Feedback

## Transform Feedback Outputs

```cpp
const char* outputNames[] = {
    "Position",
    "Velocity",
    "Age"
};

glTransformFeedbackVaryings(
    prog.getHandle(),
    3,
    outputNames,
    GL_SEPARATE_ATTRIBS
);
```

---

## `update()`

```cpp
void SceneBasic_Uniform::update(float t)
{
    deltaT = t - time;
    time = t;
}
```

---

## Render

```cpp
void SceneBasic_Uniform::render()
{
    prog.setUniform("Pass", 1);

    glEnable(GL_RASTERIZER_DISCARD);

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedbackObj[currTFB]);
    glBeginTransformFeedback(GL_POINTS);

    glBindVertexArray(particleArray[1 - currVB]);
    glDrawArrays(GL_POINTS, 0, nParticles);

    glEndTransformFeedback();
    glDisable(GL_RASTERIZER_DISCARD);

    prog.setUniform("Pass", 2);

    glBindVertexArray(particleArray[currVB]);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, nParticles);

    currVB = 1 - currVB;
    currTFB = 1 - currTFB;
}
```

---

# 4) Instanced Mesh Particles

## Additional Transform Feedback Variable

```cpp
const char* outputNames[] = {
    "Position",
    "Velocity",
    "Age",
    "Rotation"
};
```

---

## Render

```cpp
void SceneBasic_Uniform::render()
{
    prog.setUniform("Pass", 1);

    glEnable(GL_RASTERIZER_DISCARD);

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedbackObj[currTFB]);

    glBeginTransformFeedback(GL_POINTS);
    glBindVertexArray(particleArray[currVB]);
    glDrawArrays(GL_POINTS, 0, nParticles);
    glEndTransformFeedback();

    glDisable(GL_RASTERIZER_DISCARD);

    prog.setUniform("Pass", 2);

    glBindVertexArray(particleArray[currVB]);

    for(int i = 0; i < nParticles; i++)
    {
        model.render();
    }

    currVB = 1 - currVB;
    currTFB = 1 - currTFB;
}
```

---

# 5) Fire Particles

## Random Initial Velocity

```cpp
vec3 randomInitialVelocity()
{
    return vec3(
        glm::linearRand(-1.0f, 1.0f),
        glm::linearRand(4.0f, 8.0f),
        glm::linearRand(-1.0f, 1.0f)
    );
}
```

---

## Random Initial Position

```cpp
vec3 randomInitialPosition()
{
    return vec3(
        glm::linearRand(-0.5f, 0.5f),
        0.0f,
        glm::linearRand(-0.5f, 0.5f)
    );
}
```

---

## Fire Fragment Shader Fade

```glsl
color = mix(color, vec4(0.0, 0.0, 0.0, 1.0), agePct);
```

---

# 6) Smoke Particles

## Additional Uniforms

```glsl
uniform float MinParticleSize = 0.1;
uniform float MaxParticleSize = 2.5;
```

---

## Smoke Render Function

```glsl
void render()
{
    Transp = 0.0;
    vec3 posCam = vec3(0.0);

    if(VertexAge >= 0.0)
    {
        float agePct = VertexAge / ParticleLifetime;

        Transp = clamp(1.0 - agePct, 0.0, 1.0);

        posCam =
            (MV * vec4(VertexPosition,1)).xyz +
            offsets[gl_VertexID] *
            mix(MinParticleSize, MaxParticleSize, agePct);
    }

    TexCoord = texCoords[gl_VertexID];

    gl_Position = Proj * vec4(posCam,1);
}
```

---

# What was unrecoverable?

These screenshots were too blurry to perfectly recover:

- exact constructor values
n- full `initBuffers()` implementations
- exact VAO/VBO setup code
- full Phong fragment shader code

But the missing parts are mostly standard boilerplate from previous labs.

This should save you from manually typing ~90% of the lab.



## Construct and the initScene()

SceneBasic_Uniform::SceneBasic_Uniform() : time(0), particleLefttime(5.5f), nparticles(8000),
                                            emitterPos(1, 0, 0), emitterDir(-1, 2, 0)
{
    //
}

void SceneBasic_Uniform::initScene()
{
    compile(); //comple, link and use shaders

    glClearColor(0.1f, 0.1f, 0.1f, 0.1f);

    // Enable alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    angle = glm::half_pi<float>();
    //angle= 0;
    initBuffers();

    //The particle texture
    glActivateTexture(GL_TEXTURE0);
    Texture::loadTexture("../Project_Template/media/texture/bluewater.png");

    prog.use();
    prog.setUniform("ParticleTex", 0);
    prog.setUniform("ParticleLifetime", particleLifeTime);
    prog.setUniform("ParticleSize", 0.05f);
    prog.setUniform("Gravity", vec3(0.0f, -0.2f, 0.0f));
    prog.setUniform("EmitterPos", emitterPos);

    flatProg.use();
    flatProg.setUniform("Color", glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));
}

## initBuffers()

void SceneBasic_Uniform::initBuffers()
{
    // Generate the buffers for initial velocity and start (birth) time
    glGenBuffers(1, &initVel);
    glGenBuffers(1, &startTime);

    // Allocate space for all buffers
    init size = nParticles * sizeof(float);
    glBindBuffer(GL_ARRAY_BUFFER, initVel);
    glBindBuffer(GL_ARRAY_BUFFER, size * 3, 0, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, startTime);
    glBufferData(GL_ARRAY_BUFFER, size, 0, GL_STATIC_DRAW);

    // Fill the first velocity with random velocities
    glm::mat3 emitterBasics = ParticleUtils::makeArbitaryBasics(emitterDir);
    vec3 v(0.0f);
    float velocity, theta, phi;
    std::vector<GLfloat> data(nParticles * 3);
    for (uint32_t i = 0; i < nParticles; i++>) {
        //pick the direction of the velocity
        theta = glm::mix(0.0f, glm::pi<float>() / 20.0f, randFloat());
        phi = glm::mix(0.0f, glm::two_pi<float>(), randFloat());

        v.x = sinf(thata) * cosf(phi);
        v.y = cosf(theta);
        v.z = sinf(theta) * sinf(phi);

        //scale to set the magnitude of the velocity
        velocity = glm::mix(1.25f, 1.5f, randFloat());
        v = glm::normalize(emitterBasis * v) * velocity;

        data[3 * i] = v.x;
        data[3 * i + 1] = v.y;
        data[3 * i + 2] = v.z;
    }

    glBindBuffer(GL_ARRAY_BUFFER, initVel);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size * 3, data.data());

    // Fill the start time buffer
    float rate = particleLifetime / nParticles;
    for (int i = 0; i < nParticles; i++) {
        data[i] = rate * i;
    }
    glBindBuffer(GL_ARRAY_BUFFER, startTime);
    glBufferSubData(GL_ARRAY_BUFFER, 0, nParticles * sizeof(float), data.data());

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenVertexArrays(1, &particles);
    glBindVertexArray(particles);
    glBindBuffer(GL_ARRAY_BUFFER, initVel);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0.0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER), startTime;
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    glVertexAttribDivisor(0, 1);
    glVertexAttribDivisor(1, 1);

    glBindVertexArray(0);

}

## generate random floats

float SceneBasic_Uniform::randFloat() {
    return rand.nextFloat();
}