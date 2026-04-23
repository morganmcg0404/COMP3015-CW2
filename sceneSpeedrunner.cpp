#include "sceneSpeedrunner.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cstdlib>
#include <cmath>

SceneSpeedRuner::SceneSpeedRuner()
    : time(0.0f), deltaT(0.016f), currVB(0), currTFB(0), 
      nParticles(8000), particleLifetime(5.5f),
      yaw(-90.0f), pitch(-25.0f)
{
    playerPos = glm::vec3(0.0f, 3.0f, 8.0f);
    playerVel = glm::vec3(0.0f);
    
    // Setup enemy positions (fixed arena spawns)
    enemyPositions.push_back(glm::vec3(5.0f, 0.5f, -5.0f));
    enemyPositions.push_back(glm::vec3(-5.0f, 0.5f, -5.0f));
    enemyPositions.push_back(glm::vec3(5.0f, 0.5f, 5.0f));
    enemyPositions.push_back(glm::vec3(-5.0f, 0.5f, 5.0f));
    enemyPositions.push_back(glm::vec3(0.0f, 0.5f, -8.0f));
}

SceneSpeedRuner::~SceneSpeedRuner()
{
    // Cleanup
    glDeleteBuffers(2, particleVBO);
    glDeleteVertexArrays(2, particleVAO);
    glDeleteTransformFeedbacks(2, feedbackObj);
    glDeleteBuffers(1, &initVelBuf);
    glDeleteBuffers(1, &startTimeBuf);
    glDeleteFramebuffers(1, &hdrFBO);
    glDeleteFramebuffers(1, &blurFBO);
    glDeleteTextures(1, &hdrTex);
    glDeleteTextures(1, &blurTex1);
    glDeleteTextures(1, &blurTex2);
    glDeleteVertexArrays(1, &fsQuadVAO);
    glDeleteVertexArrays(1, &sceneVAO);
}

void SceneSpeedRuner::compile()
{
    try {
        bloomProg.compileShader("shader/bloom.vert", GLSLShader::VERTEX);
        bloomProg.compileShader("shader/bloom.frag", GLSLShader::FRAGMENT);
        bloomProg.link();
        
        particleProg.compileShader("shader/particle.vert", GLSLShader::VERTEX);
        particleProg.compileShader("shader/particle.frag", GLSLShader::FRAGMENT);
        
        // Setup transform feedback varyings
        const char* outputNames[] = { "Position", "Velocity", "Age" };
        glTransformFeedbackVaryings(particleProg.getHandle(), 3, outputNames, GL_SEPARATE_ATTRIBS);
        
        particleProg.link();
        
        sceneProg.compileShader("shader/scene.vert", GLSLShader::VERTEX);
        sceneProg.compileShader("shader/scene.frag", GLSLShader::FRAGMENT);
        sceneProg.link();
    }
    catch (GLSLProgramException& e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }
}

void SceneSpeedRuner::createFullscreenQuad()
{
    // Create a fullscreen quad for post-processing passes
    glGenVertexArrays(1, &fsQuadVAO);
    glBindVertexArray(fsQuadVAO);
    
    GLuint quadVBO;
    glGenBuffers(1, &quadVBO);
    
    // NDC coordinates + texcoords
    float fsQuad[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f
    };
    
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fsQuad), fsQuad, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

void SceneSpeedRuner::initBloom()
{
    // Create HDR framebuffer
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    
    // Create HDR texture
    glGenTextures(1, &hdrTex);
    glBindTexture(GL_TEXTURE_2D, hdrTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrTex, 0);
    
    // Depth buffer for scene rendering
    GLuint depthRBO;
    glGenRenderbuffers(1, &depthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Framebuffer not complete!" << std::endl;
    
    // Create blur framebuffer
    glGenFramebuffers(1, &blurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, blurFBO);
    
    // Blur textures
    glGenTextures(1, &blurTex1);
    glBindTexture(GL_TEXTURE_2D, blurTex1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glGenTextures(1, &blurTex2);
    glBindTexture(GL_TEXTURE_2D, blurTex2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurTex1, 0);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneSpeedRuner::initParticles()
{
    // Generate particle vertex arrays and buffers
    glGenVertexArrays(2, particleVAO);
    glGenBuffers(2, particleVBO);
    glGenTransformFeedbacks(2, feedbackObj);
    glGenBuffers(1, &initVelBuf);
    glGenBuffers(1, &startTimeBuf);
    
    // Initial velocity data
    glBindBuffer(GL_ARRAY_BUFFER, initVelBuf);
    glBufferData(GL_ARRAY_BUFFER, nParticles * 3 * sizeof(float), nullptr, GL_STATIC_DRAW);
    
    glm::mat3 emitterBasis = glm::mat3(1.0f);
    std::vector<glm::vec3> initialVelocities(nParticles);
    
    for (int i = 0; i < nParticles; ++i) {
        float theta = glm::mix(0.0f, glm::pi<float>() / 20.0f, randFloat());
        float phi = glm::mix(0.0f, glm::two_pi<float>(), randFloat());
        
        glm::vec3 v;
        v.x = sinf(theta) * cosf(phi);
        v.y = cosf(theta);
        v.z = sinf(theta) * sinf(phi);
        
        float velocity = glm::mix(1.25f, 1.5f, randFloat());
        v = glm::normalize(v) * velocity;
        
        initialVelocities[i] = v;
    }
    
    glBufferSubData(GL_ARRAY_BUFFER, 0, nParticles * 3 * sizeof(float), 
                   glm::value_ptr(initialVelocities[0]));
    
    // Start time data
    glBindBuffer(GL_ARRAY_BUFFER, startTimeBuf);
    glBufferData(GL_ARRAY_BUFFER, nParticles * sizeof(float), nullptr, GL_STATIC_DRAW);
    
    std::vector<float> startTimes(nParticles);
    float rate = particleLifetime / nParticles;
    for (int i = 0; i < nParticles; ++i) {
        startTimes[i] = rate * i;
    }
    glBufferSubData(GL_ARRAY_BUFFER, 0, nParticles * sizeof(float), startTimes.data());
    
    // Setup VAOs and particle buffers for ping-pong
    for (int i = 0; i < 2; ++i) {
        glBindVertexArray(particleVAO[i]);
        glBindBuffer(GL_ARRAY_BUFFER, particleVBO[i]);
        glBufferData(GL_ARRAY_BUFFER, nParticles * (3 + 3 + 1) * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(6 * sizeof(float)));
        
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedbackObj[i]);
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, particleVBO[i]);
        
        glBindVertexArray(0);
    }
}

void SceneSpeedRuner::initScene()
{
    compile();
    
    glClearColor(0.3f, 0.4f, 0.5f, 1.0f);  // Nice sky blue
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    createFullscreenQuad();
    initBloom();
    initParticles();
    
    // Create a simple cube for testing
    glGenVertexArrays(1, &sceneVAO);
    glBindVertexArray(sceneVAO);
    
    GLuint vbo;
    glGenBuffers(1, &vbo);
    
    // Simple cube: 2 triangles per face, 6 faces = 36 vertices
    // Format: x, y, z, nx, ny, nz
    float vertices[] = {
        // Front (z+)
        -1, -1,  1,  0,  0,  1,
         1, -1,  1,  0,  0,  1,
         1,  1,  1,  0,  0,  1,
        -1, -1,  1,  0,  0,  1,
         1,  1,  1,  0,  0,  1,
        -1,  1,  1,  0,  0,  1,
        // Back (z-)
         1, -1, -1,  0,  0, -1,
        -1, -1, -1,  0,  0, -1,
        -1,  1, -1,  0,  0, -1,
         1, -1, -1,  0,  0, -1,
        -1,  1, -1,  0,  0, -1,
         1,  1, -1,  0,  0, -1,
        // Top (y+)
        -1,  1, -1,  0,  1,  0,
        -1,  1,  1,  0,  1,  0,
         1,  1,  1,  0,  1,  0,
        -1,  1, -1,  0,  1,  0,
         1,  1,  1,  0,  1,  0,
         1,  1, -1,  0,  1,  0,
        // Bottom (y-)
        -1, -1, -1,  0, -1,  0,
         1, -1, -1,  0, -1,  0,
         1, -1,  1,  0, -1,  0,
        -1, -1, -1,  0, -1,  0,
         1, -1,  1,  0, -1,  0,
        -1, -1,  1,  0, -1,  0,
        // Right (x+)
         1, -1, -1,  1,  0,  0,
         1, -1,  1,  1,  0,  0,
         1,  1,  1,  1,  0,  0,
         1, -1, -1,  1,  0,  0,
         1,  1,  1,  1,  0,  0,
         1,  1, -1,  1,  0,  0,
        // Left (x-)
        -1, -1,  1,  -1,  0,  0,
        -1, -1, -1,  -1,  0,  0,
        -1,  1, -1,  -1,  0,  0,
        -1, -1,  1,  -1,  0,  0,
        -1,  1, -1,  -1,  0,  0,
        -1,  1,  1,  -1,  0,  0,
    };
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    // Setup projection matrix
    projection = glm::perspective(glm::radians(60.0f), 
                                 (float)width / height, 0.1f, 100.0f);
}

void SceneSpeedRuner::update(float t)
{
    deltaT = t - time;
    time = t;
    
    updatePlayer(deltaT);
    
    // Update view matrix from player position
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    view = glm::lookAt(playerPos, playerPos + front, up);
}

void SceneSpeedRuner::updatePlayer(float dt)
{
    // Basic WASD movement (placeholder)
    // In full implementation, would read keyboard input
    
    // Apply gravity
    playerVel.y -= 9.8f * dt;
    
    // Update position
    playerPos += playerVel * dt;
    
    // Simple ground collision
    if (playerPos.y <= 0.0f) {
        playerPos.y = 0.0f;
        playerVel.y = 0.0f;
    }
}

void SceneSpeedRuner::setMatrices()
{
    glm::mat4 mv = view * model;
    glm::mat3 normalMat = glm::mat3(glm::vec3(mv[0]), glm::vec3(mv[1]), glm::vec3(mv[2]));
    glm::mat4 mvp = projection * mv;
    
    // Only set uniforms on the currently bound program
    sceneProg.setUniform("ModelViewMatrix", mv);
    sceneProg.setUniform("NormalMatrix", normalMat);
    sceneProg.setUniform("MVP", mvp);
}

void SceneSpeedRuner::renderScene()
{
    sceneProg.use();
    
    // Draw floor (scaled cube)
    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.5f, 0.0f));
    model = glm::scale(model, glm::vec3(20.0f, 0.1f, 20.0f));
    setMatrices();
    glBindVertexArray(sceneVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    
    // Draw enemy markers (cubes)
    for (size_t i = 0; i < enemyPositions.size(); ++i) {
        model = glm::translate(glm::mat4(1.0f), enemyPositions[i]);
        model = glm::scale(model, glm::vec3(0.5f, 1.0f, 0.5f));
        setMatrices();
        glBindVertexArray(sceneVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
}

void SceneSpeedRuner::render()
{
    // Render scene to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);  // Render to screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, width, height);
    
    sceneProg.use();
    
    // Render the arena and enemies
    renderScene();
}

void SceneSpeedRuner::resize(int w, int h)
{
    width = w;
    height = h;
    glViewport(0, 0, w, h);
    projection = glm::perspective(glm::radians(60.0f), 
                                 (float)w / h, 0.1f, 100.0f);
}

float SceneSpeedRuner::randFloat()
{
    return static_cast<float>(rand()) / RAND_MAX;
}
