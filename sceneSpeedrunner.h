#ifndef SCENESPEEDRUNNER_H
#define SCENESPEEDRUNNER_H

#include "helper/scene.h"
#include "helper/glslprogram.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class SceneSpeedRuner : public Scene
{
private:
    // Bloom effect
    GLuint hdrFBO, blurFBO;
    GLuint hdrTex, blurTex1, blurTex2;
    GLuint fsQuadVAO;
    GLSLProgram bloomProg;
    
    // Particle system
    GLuint particleVAO[2], particleVBO[2];
    GLuint feedbackObj[2];
    GLuint initVelBuf, startTimeBuf;
    GLSLProgram particleProg;
    
    // Scene geometry
    GLuint sceneVAO;
    GLSLProgram sceneProg;
    
    // Game state
    float time;
    float deltaT;
    int currVB;
    int currTFB;
    int nParticles;
    float particleLifetime;
    
    // Player state
    glm::vec3 playerPos;
    glm::vec3 playerVel;
    glm::mat4 playerView;
    float yaw;
    float pitch;
    
    // Enemy positions
    std::vector<glm::vec3> enemyPositions;
    
    // Method helpers
    void compile();
    void initBloom();
    void initParticles();
    void createFullscreenQuad();
    void setMatrices();
    void updatePlayer(float dt);
    void renderScene();
    void renderBloom();
    void emitParticles();
    float randFloat();

public:
    SceneSpeedRuner();
    ~SceneSpeedRuner();

    void initScene();
    void update(float t);
    void render();
    void resize(int, int);
};

#endif // SCENESPEEDRUNNER_H
