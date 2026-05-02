#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

#include "Mesh.h"
#include "Plane.h"

#include <fstream>
#include <sstream>

// -------- Forward declarations --------
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void updatePlayerController();
void spawnFireExplosion(const glm::vec3& position);
void spawnRandomEnemy();
float randFloat();

// Resolution constants
const int windowWidth = 1920;
const int windowHeight = 1080;

// Camera
float lastX = windowWidth / 2.0f;
float lastY = windowHeight / 2.0f;
float yaw = -90.0f;
float pitch = 0.0f;
float fov = 45.0f;

glm::vec3 cameraPos = glm::vec3(0.0f, 1.7f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

glm::vec3 playerPosition = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 playerVelocity = glm::vec3(0.0f);
const float playerEyeHeight = 1.7f;
const float playerMoveSpeed = 6.0f;
const float playerSprintMultiplier = 1.8f;
const float playerJumpSpeed = 7.5f;
const float playerGravity = 18.0f;
bool playerGrounded = true;

float playerMana = 100.0f;
const float playerMaxMana = 100.0f;
const float fireballManaCost = 10.0f;
const float playerManaRegenRate = 15.0f;

float playerHealth = 100.0f;
const float playerMaxHealth = 100.0f;
const float enemyDamageDelay = 0.5f;  // Time enemy must be in range before damage
const float enemyDamageAmount = 5.0f;  // Damage per hit
const float enemyDamageInterval = 1.0f;  // How often damage is dealt if in range

int playerScore = 0;

bool gamePaused = false;
bool pauseKeyPressed = false;
bool pauseClickPressed = false;
glm::vec2 mousePosition = glm::vec2(0.0f);

bool firstMouse = true;
bool bloomEnabled = true;
bool bloomKeyPressed = false;
bool vignetteEnabled = true;
bool vignetteKeyPressed = false;
bool toonShadingEnabled = true;
bool toonKeyPressed = false;
bool edgeEnabled = true;
bool edgeKeyPressed = false;
float bloomStrength = 0.35f;
float brightThreshold = 0.7f;
bool bloomStrengthUpPressed = false;
bool bloomStrengthDownPressed = false;
bool brightThresholdUpPressed = false;
bool brightThresholdDownPressed = false;
bool fireKeyPressed = false;
bool fireballKeyPressed = false;
bool jumpKeyPressed = false;

struct FireParticle
{
    glm::vec3 position;
    glm::vec3 velocity;
    float age;
    float lifetime;
};

glm::vec3 fireEmitterPos = glm::vec3(0.0f, 0.0f, 0.0f);
std::vector<FireParticle> fireParticles;

unsigned int fireVAO = 0;
unsigned int fireVBO = 0;
unsigned int fireShader = 0;
const unsigned int fireParticleCount = 512;
unsigned int activeFireParticleCount = 0;  // Track how many particles are actually alive

// Forward-declare function so it can be used in input handling
void respawnFireParticle(struct FireParticle& particle, bool randomAge);

// Fireball projectile
struct Fireball {
    bool active = false;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 velocity = glm::vec3(0.0f);
    float life = 0.0f;
    float maxLife = 5.0f;
    float radius = 0.2f;
    float lastTrailEmit = 0.0f;
};

std::vector<Fireball> fireballs;

struct Enemy {
    bool active = true;
    glm::vec3 position = glm::vec3(8.0f, 0.0f, -8.0f);
    float radius = 0.6f;
    float health = 3.0f;
    float moveSpeed = 2.2f;
    float proximityTime = 0.0f;  // How long enemy has been in damage range
    float lastDamageTime = -1.0f;  // When damage was last dealt
};

std::vector<Enemy> enemies;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float gameTime = 0.0f;
float enemySpawnAccumulator = 0.0f;
const float initialEnemySpawnInterval = 8.0f;
const float minEnemySpawnInterval = 1.5f;
const float enemySpawnRampRate = 0.025f;

unsigned int hdrFBO;
unsigned int colorBuffer;
unsigned int rboDepth;
unsigned int normalBuffer;
unsigned int sceneDepth;
unsigned int brightFBO;
unsigned int brightBuffer;
unsigned int pingpongFBO[2];
unsigned int pingpongColorbuffers[2];

// -------- Input --------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && !pauseKeyPressed)
    {
        glfwSetWindowShouldClose(window, true);
        pauseKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_RELEASE)
        pauseKeyPressed = false;

    double cursorX = 0.0;
    double cursorY = 0.0;
    glfwGetCursorPos(window, &cursorX, &cursorY);
    mousePosition = glm::vec2(static_cast<float>(cursorX) / static_cast<float>(windowWidth),
                              1.0f - static_cast<float>(cursorY) / static_cast<float>(windowHeight));

    float moveSpeed = playerMoveSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
        moveSpeed *= playerSprintMultiplier;

    glm::vec3 moveDirection(0.0f);
    glm::vec3 flatForward = glm::vec3(cameraFront.x, 0.0f, cameraFront.z);
    if (glm::length(flatForward) > 0.0001f)
        flatForward = glm::normalize(flatForward);
    else
        flatForward = glm::vec3(0.0f, 0.0f, -1.0f);

    glm::vec3 flatRight = glm::normalize(glm::cross(flatForward, cameraUp));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        moveDirection += flatForward;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        moveDirection -= flatForward;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        moveDirection -= flatRight;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        moveDirection += flatRight;

    if (glm::length(moveDirection) > 0.0001f)
        playerPosition += glm::normalize(moveDirection) * moveSpeed * deltaTime;

    // Player-enemy collision detection
    const float playerRadius = 0.5f;
    for (auto& enemy : enemies)
    {
        if (!enemy.active)
            continue;

        glm::vec3 toEnemy = enemy.position - playerPosition;
        float distance = glm::length(toEnemy);
        float minDistance = playerRadius + enemy.radius;

        if (distance < minDistance && distance > 0.001f)
        {
            // Push player away from enemy
            glm::vec3 pushDirection = glm::normalize(toEnemy);
            playerPosition -= pushDirection * (minDistance - distance);
        }
    }

    // Clamp player position to arena bounds
    const float arenaMin = -50.0f;
    const float arenaMax = 50.0f;
    playerPosition.x = glm::clamp(playerPosition.x, arenaMin + playerRadius, arenaMax - playerRadius);
    playerPosition.z = glm::clamp(playerPosition.z, arenaMin + playerRadius, arenaMax - playerRadius);

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && playerGrounded && !jumpKeyPressed)
    {
        playerVelocity.y = playerJumpSpeed;
        playerGrounded = false;
        jumpKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
        jumpKeyPressed = false;

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !bloomKeyPressed)
    {
        bloomEnabled = !bloomEnabled;
        bloomKeyPressed = true;
        std::cout << "Bloom " << (bloomEnabled ? "ON" : "OFF") << std::endl;
    }

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
        bloomKeyPressed = false;

    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS && !vignetteKeyPressed)
    {
        vignetteEnabled = !vignetteEnabled;
        vignetteKeyPressed = true;
        std::cout << "Vignette " << (vignetteEnabled ? "ON" : "OFF") << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_RELEASE)
        vignetteKeyPressed = false;

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && !toonKeyPressed)
    {
        toonShadingEnabled = !toonShadingEnabled;
        toonKeyPressed = true;
        std::cout << "Toon Shading " << (toonShadingEnabled ? "ON" : "OFF") << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE)
        toonKeyPressed = false;

    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS && !edgeKeyPressed)
    {
        edgeEnabled = !edgeEnabled;
        edgeKeyPressed = true;
        std::cout << "Edge Detection " << (edgeEnabled ? "ON" : "OFF") << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_RELEASE)
        edgeKeyPressed = false;

    if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS && !bloomStrengthUpPressed)
    {
        bloomStrength += 0.05f;
        if (bloomStrength > 2.0f) bloomStrength = 2.0f;
        bloomStrengthUpPressed = true;
        std::cout << "Bloom strength: " << bloomStrength << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_RELEASE)
        bloomStrengthUpPressed = false;

    if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS && !bloomStrengthDownPressed)
    {
        bloomStrength -= 0.05f;
        if (bloomStrength < 0.0f) bloomStrength = 0.0f;
        bloomStrengthDownPressed = true;
        std::cout << "Bloom strength: " << bloomStrength << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_RELEASE)
        bloomStrengthDownPressed = false;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && !brightThresholdUpPressed)
    {
        brightThreshold += 0.1f;
        if (brightThreshold > 5.0f) brightThreshold = 5.0f;
        brightThresholdUpPressed = true;
        std::cout << "Bright threshold: " << brightThreshold << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_RELEASE)
        brightThresholdUpPressed = false;

    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && !brightThresholdDownPressed)
    {
        brightThreshold -= 0.1f;
        if (brightThreshold < 0.0f) brightThreshold = 0.0f;
        brightThresholdDownPressed = true;
        std::cout << "Bright threshold: " << brightThreshold << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_RELEASE)
        brightThresholdDownPressed = false;

    // Throw a fireball where the camera is looking when F is pressed
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fireballKeyPressed)
    {
        fireballKeyPressed = true;

        if (playerMana < fireballManaCost)
            return;

        playerMana = std::max(0.0f, playerMana - fireballManaCost);
        
        // Calculate right and down vectors for bottom-right spawn position
        glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));
        glm::vec3 down = -cameraUp;
        
        Fireball fireball;
        fireball.active = true;
        // Spawn from bottom-right of camera
        fireball.position = cameraPos + cameraFront * 0.5f + right * 0.4f + down * 0.3f;
        
        // Calculate target point ahead on the crosshair (camera front direction)
        // Aim directly at center - the spawn offset naturally steers it correctly
        glm::vec3 targetPoint = cameraPos + cameraFront * 30.0f;
        
        // Calculate direction from spawn to target
        glm::vec3 direction = glm::normalize(targetPoint - fireball.position);
        fireball.velocity = direction * 50.0f;
        
        fireball.life = 0.0f;
        fireball.lastTrailEmit = 0.0f;

        std::cout << "Thrown fireball from: " << fireball.position.x << ", " << fireball.position.y << ", " << fireball.position.z << std::endl;
        fireballs.push_back(fireball);
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE)
        fireballKeyPressed = false;
}

void updatePlayerController()
{
    playerMana = std::min(playerMaxMana, playerMana + playerManaRegenRate * deltaTime);

    playerVelocity.y -= playerGravity * deltaTime;
    playerPosition += playerVelocity * deltaTime;

    if (playerPosition.y <= 0.0f)
    {
        playerPosition.y = 0.0f;
        playerVelocity.y = 0.0f;
        playerGrounded = true;
    }

    cameraPos = playerPosition + glm::vec3(0.0f, playerEyeHeight, 0.0f);
}

void spawnFireExplosion(const glm::vec3& position)
{
    int n = std::min<size_t>(64, fireParticles.size());
    for (int j = 0; j < n; ++j) {
        auto& p = fireParticles[j];
        p.position = position + glm::vec3((randFloat() - 0.5f) * 0.5f, (randFloat() * 0.4f), (randFloat() - 0.5f) * 0.5f);
        p.velocity = glm::normalize(glm::vec3((randFloat() - 0.5f), randFloat() * 1.5f, (randFloat() - 0.5f))) * (2.0f + randFloat() * 2.0f);
        p.age = 0.0f;
        p.lifetime = 0.6f + randFloat() * 0.8f;
    }

    std::vector<float> gpuData;
    gpuData.reserve(fireParticles.size() * 5);
    for (const auto& particle : fireParticles) {
        gpuData.push_back(particle.position.x);
        gpuData.push_back(particle.position.y);
        gpuData.push_back(particle.position.z);
        gpuData.push_back(particle.age);
        gpuData.push_back(particle.lifetime);
    }
    glBindBuffer(GL_ARRAY_BUFFER, fireVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, gpuData.size() * sizeof(float), gpuData.data());
}

void spawnRandomEnemy(Enemy& enemy)
{
    const float arenaMin = -50.0f;
    const float arenaMax = 50.0f;
    const float spawnOffset = 2.0f;
    const float minDistanceFromPlayer = 12.0f;

    glm::vec3 pos(0.0f);
    for (int attempt = 0; attempt < 12; ++attempt)
    {
        int side = static_cast<int>(randFloat() * 4.0f) % 4;

        switch (side)
        {
        case 0: // left
            pos = glm::vec3(arenaMin + spawnOffset, 0.0f, arenaMin + randFloat() * (arenaMax - arenaMin));
            break;
        case 1: // right
            pos = glm::vec3(arenaMax - spawnOffset, 0.0f, arenaMin + randFloat() * (arenaMax - arenaMin));
            break;
        case 2: // bottom
            pos = glm::vec3(arenaMin + randFloat() * (arenaMax - arenaMin), 0.0f, arenaMin + spawnOffset);
            break;
        default: // top
            pos = glm::vec3(arenaMin + randFloat() * (arenaMax - arenaMin), 0.0f, arenaMax - spawnOffset);
            break;
        }

        if (glm::length((pos + glm::vec3(0.0f, 1.0f, 0.0f)) - playerPosition) >= minDistanceFromPlayer)
            break;
    }

    enemy.position = pos;
    enemy.health = 3.0f;
    enemy.active = true;
    enemy.proximityTime = 0.0f;
    enemy.lastDamageTime = -1.0f;
}

void updateEnemy()
{
    const float damageRange = 2.0f;  // Maximum range for damage
    const float playerRadius = 0.5f;

    for (auto& enemy : enemies)
    {
        if (!enemy.active)
            continue;

        glm::vec3 toPlayer = playerPosition - enemy.position;
        toPlayer.y = 0.0f;

        float distance = glm::length(toPlayer);
        float minSeparation = playerRadius + enemy.radius + 0.15f;
        float stopDistance = std::max(damageRange * 0.9f, minSeparation);

        // Move toward player, but stop at attack standoff distance so enemies don't push player.
        if (distance > stopDistance && distance > 0.001f)
        {
            glm::vec3 direction = toPlayer / distance;
            float maxStep = enemy.moveSpeed * deltaTime;
            float step = std::min(maxStep, distance - stopDistance);
            enemy.position += direction * step;
        }

        enemy.position.y = 0.0f;

        // Check if enemy is in damage range of player
        float attackDistance = glm::length(glm::vec3(playerPosition.x - enemy.position.x, 0.0f, playerPosition.z - enemy.position.z));
        if (attackDistance < damageRange)
        {
            // Enemy is in range, accumulate proximity time
            enemy.proximityTime += deltaTime;

            // Deal damage if proximity time exceeds delay and enough time has passed since last damage
            if (enemy.proximityTime >= enemyDamageDelay)
            {
                if (enemy.lastDamageTime < 0.0f || (gameTime - enemy.lastDamageTime) >= enemyDamageInterval)
                {
                    playerHealth = std::max(0.0f, playerHealth - enemyDamageAmount);
                    enemy.lastDamageTime = gameTime;
                    std::cout << "Player hit! Health: " << playerHealth << std::endl;
                }
            }
        }
        else
        {
            // Enemy out of range, reset proximity time
            enemy.proximityTime = 0.0f;
        }

        // Enemy-enemy collision detection
        for (auto& otherEnemy : enemies)
        {
            if (!otherEnemy.active || &enemy == &otherEnemy)
                continue;

            glm::vec3 toOther = otherEnemy.position - enemy.position;
            float distance = glm::length(toOther);
            float minDistance = enemy.radius + otherEnemy.radius;

            if (distance < minDistance && distance > 0.001f)
            {
                // Push both enemies apart
                glm::vec3 pushDirection = glm::normalize(toOther);
                enemy.position -= pushDirection * (minDistance - distance) * 0.5f;
                otherEnemy.position += pushDirection * (minDistance - distance) * 0.5f;
            }
        }
    }
}

// -------- Mouse --------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (gamePaused)
    {
        firstMouse = true;
        return;
    }

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float sensitivity = 0.1f;
    float xoffset = (xpos - lastX) * sensitivity;
    float yoffset = (lastY - ypos) * sensitivity;

    lastX = xpos;
    lastY = ypos;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    cameraFront = glm::normalize(front);
}

// -------- Resize --------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// -------- File loader --------
std::string readFile(const char* path)
{
    std::ifstream file(path);

    if (!file.is_open())
    {
        std::cout << "ERROR: Could not open shader file: " << path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// -------- Shader creation --------
unsigned int createShader(const char* vertexPath, const char* fragmentPath)
{
    std::string vertexCode = readFile(vertexPath);
    std::string fragmentCode = readFile(fragmentPath);

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    int success;
    char infoLog[512];

    // Vertex shader
    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);

    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cout << "ERROR: Vertex shader compilation failed\n" << infoLog << std::endl;
    }

    // Fragment shader
    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cout << "ERROR: Fragment shader compilation failed\n" << infoLog << std::endl;
    }

    // Shader program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertex);
    glAttachShader(shaderProgram, fragment);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR: Shader linking failed\n" << infoLog << std::endl;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return shaderProgram;
}

std::vector<float> createSphere(float radius, int sectorCount, int stackCount)
{
    std::vector<float> vertices;

    for (int i = 0; i <= stackCount; ++i)
    {
        float stackAngle = glm::pi<float>() / 2 - i * glm::pi<float>() / stackCount;
        float xy = radius * cos(stackAngle);
        float z = radius * sin(stackAngle);

        for (int j = 0; j <= sectorCount; ++j)
        {
            float sectorAngle = j * 2 * glm::pi<float>() / sectorCount;

            float x = xy * cos(sectorAngle);
            float y = xy * sin(sectorAngle);

            // position
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // normal (not really needed for emissive, but Mesh expects it)
            vertices.push_back(x / radius);
            vertices.push_back(y / radius);
            vertices.push_back(z / radius);
        }
    }

    // indices → convert to triangles
    std::vector<float> finalVerts;

    for (int i = 0; i < stackCount; ++i)
    {
        int k1 = i * (sectorCount + 1);
        int k2 = k1 + sectorCount + 1;

        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2)
        {
            // triangle 1
            for (int k : {k1, k2, k1 + 1})
            {
                for (int m = 0; m < 6; m++)
                    finalVerts.push_back(vertices[k * 6 + m]);
            }

            // triangle 2
            for (int k : {k1 + 1, k2, k2 + 1})
            {
                for (int m = 0; m < 6; m++)
                    finalVerts.push_back(vertices[k * 6 + m]);
            }
        }
    }

    return finalVerts;
}

std::vector<float> createCube(float size)
{
    float h = size * 0.5f;

    std::vector<float> verts = {
        // front
        -h, -h,  h,  0, 0, 1,
         h, -h,  h,  0, 0, 1,
         h,  h,  h,  0, 0, 1,
         h,  h,  h,  0, 0, 1,
        -h,  h,  h,  0, 0, 1,
        -h, -h,  h,  0, 0, 1,

        // back
        -h, -h, -h,  0, 0,-1,
        -h,  h, -h,  0, 0,-1,
         h,  h, -h,  0, 0,-1,
         h,  h, -h,  0, 0,-1,
         h, -h, -h,  0, 0,-1,
        -h, -h, -h,  0, 0,-1,

        // left
        -h, -h, -h, -1, 0, 0,
        -h, -h,  h, -1, 0, 0,
        -h,  h,  h, -1, 0, 0,
        -h,  h,  h, -1, 0, 0,
        -h,  h, -h, -1, 0, 0,
        -h, -h, -h, -1, 0, 0,

        // right
         h, -h, -h,  1, 0, 0,
         h, -h,  h,  1, 0, 0,
         h,  h,  h,  1, 0, 0,
         h,  h,  h,  1, 0, 0,
         h,  h, -h,  1, 0, 0,
         h, -h, -h,  1, 0, 0,

        // top
        -h,  h, -h,  0, 1, 0,
        -h,  h,  h,  0, 1, 0,
         h,  h,  h,  0, 1, 0,
         h,  h,  h,  0, 1, 0,
         h,  h, -h,  0, 1, 0,
        -h,  h, -h,  0, 1, 0,

        // bottom
        -h, -h, -h,  0,-1, 0,
         h, -h, -h,  0,-1, 0,
         h, -h,  h,  0,-1, 0,
         h, -h,  h,  0,-1, 0,
        -h, -h,  h,  0,-1, 0,
        -h, -h, -h,  0,-1, 0,
    };

    return verts;
}

std::vector<float> createCapsule(float radius, float cylinderHeight, int sectorCount, int hemisphereStacks)
{
    struct Ring {
        float y;
        float ringRadius;
        glm::vec3 normalCenter;
        bool isCylinder;
    };

    std::vector<Ring> rings;

    // Top pole
    rings.push_back({ cylinderHeight * 0.5f + radius, 0.0f, glm::vec3(0.0f, cylinderHeight * 0.5f + radius, 0.0f), false });

    // Top hemisphere
    for (int i = 1; i < hemisphereStacks; ++i)
    {
        float theta = (static_cast<float>(i) / hemisphereStacks) * (glm::pi<float>() * 0.5f);
        float y = cylinderHeight * 0.5f + radius * cos(theta);
        float ringRadius = radius * sin(theta);
        rings.push_back({ y, ringRadius, glm::vec3(0.0f, cylinderHeight * 0.5f, 0.0f), false });
    }

    // Cylinder top seam
    rings.push_back({ cylinderHeight * 0.5f, radius, glm::vec3(0.0f, 0.0f, 0.0f), true });
    // Cylinder bottom seam
    rings.push_back({ -cylinderHeight * 0.5f, radius, glm::vec3(0.0f, 0.0f, 0.0f), true });

    // Bottom hemisphere
    for (int i = hemisphereStacks - 1; i >= 1; --i)
    {
        float theta = (static_cast<float>(i) / hemisphereStacks) * (glm::pi<float>() * 0.5f);
        float y = -cylinderHeight * 0.5f - radius * cos(theta);
        float ringRadius = radius * sin(theta);
        rings.push_back({ y, ringRadius, glm::vec3(0.0f, -cylinderHeight * 0.5f, 0.0f), false });
    }

    // Bottom pole
    rings.push_back({ -cylinderHeight * 0.5f - radius, 0.0f, glm::vec3(0.0f, -cylinderHeight * 0.5f - radius, 0.0f), false });

    std::vector<float> vertices;

    auto emitVertex = [&](const glm::vec3& position, const glm::vec3& normal)
    {
        vertices.push_back(position.x);
        vertices.push_back(position.y);
        vertices.push_back(position.z);
        vertices.push_back(normal.x);
        vertices.push_back(normal.y);
        vertices.push_back(normal.z);
    };

    for (size_t r = 0; r + 1 < rings.size(); ++r)
    {
        const Ring& ringA = rings[r];
        const Ring& ringB = rings[r + 1];

        for (int s = 0; s < sectorCount; ++s)
        {
            float a0 = static_cast<float>(s) / sectorCount * glm::two_pi<float>();
            float a1 = static_cast<float>(s + 1) / sectorCount * glm::two_pi<float>();

            glm::vec3 pA0(ringA.ringRadius * cos(a0), ringA.y, ringA.ringRadius * sin(a0));
            glm::vec3 pA1(ringA.ringRadius * cos(a1), ringA.y, ringA.ringRadius * sin(a1));
            glm::vec3 pB0(ringB.ringRadius * cos(a0), ringB.y, ringB.ringRadius * sin(a0));
            glm::vec3 pB1(ringB.ringRadius * cos(a1), ringB.y, ringB.ringRadius * sin(a1));

            glm::vec3 nA0 = ringA.ringRadius > 0.0001f ? glm::normalize(pA0 - ringA.normalCenter) : glm::vec3(0.0f, ringA.y > 0.0f ? 1.0f : -1.0f, 0.0f);
            glm::vec3 nA1 = ringA.ringRadius > 0.0001f ? glm::normalize(pA1 - ringA.normalCenter) : glm::vec3(0.0f, ringA.y > 0.0f ? 1.0f : -1.0f, 0.0f);
            glm::vec3 nB0 = ringB.ringRadius > 0.0001f ? glm::normalize(pB0 - ringB.normalCenter) : glm::vec3(0.0f, ringB.y > 0.0f ? 1.0f : -1.0f, 0.0f);
            glm::vec3 nB1 = ringB.ringRadius > 0.0001f ? glm::normalize(pB1 - ringB.normalCenter) : glm::vec3(0.0f, ringB.y > 0.0f ? 1.0f : -1.0f, 0.0f);

            if (ringA.isCylinder)
            {
                nA0 = glm::vec3(cos(a0), 0.0f, sin(a0));
                nA1 = glm::vec3(cos(a1), 0.0f, sin(a1));
            }
            if (ringB.isCylinder)
            {
                nB0 = glm::vec3(cos(a0), 0.0f, sin(a0));
                nB1 = glm::vec3(cos(a1), 0.0f, sin(a1));
            }

            // Triangle 1
            emitVertex(pA0, nA0);
            emitVertex(pB0, nB0);
            emitVertex(pA1, nA1);

            // Triangle 2
            emitVertex(pA1, nA1);
            emitVertex(pB0, nB0);
            emitVertex(pB1, nB1);
        }
    }

    return vertices;
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
unsigned int blurShader;
// Shadow mapping
const unsigned int SHADOW_WIDTH = 4096;
const unsigned int SHADOW_HEIGHT = 4096;
unsigned int depthMapFBO = 0;
unsigned int depthMap = 0;
unsigned int depthShader = 0;

float randFloat()
{
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

glm::vec3 randomFireVelocity()
{
    return glm::vec3(
        (randFloat() - 0.5f) * 0.8f,
        2.0f + randFloat() * 2.5f,
        (randFloat() - 0.5f) * 0.8f
    );
}

void respawnFireParticle(FireParticle& particle, bool randomAge = false)
{
    particle.position = fireEmitterPos + glm::vec3(
        (randFloat() - 0.5f) * 0.25f,
        0.0f,
        (randFloat() - 0.5f) * 0.25f
    );
    particle.velocity = randomFireVelocity();
    particle.lifetime = 1.1f + randFloat() * 0.9f;
    particle.age = randomAge ? randFloat() * particle.lifetime : 0.0f;
}

void initFireParticles()
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    fireParticles.resize(fireParticleCount);
    
    // Initialize all particles as expired so nothing renders on startup
    for (size_t i = 0; i < fireParticles.size(); ++i)
    {
        fireParticles[i].position = glm::vec3(0.0f);
        fireParticles[i].velocity = glm::vec3(0.0f);
        fireParticles[i].lifetime = 1.0f;
        fireParticles[i].age = 2.0f;  // Mark as expired so updateFireParticles skips it
    }

    glGenVertexArrays(1, &fireVAO);
    glGenBuffers(1, &fireVBO);

    glBindVertexArray(fireVAO);
    glBindBuffer(GL_ARRAY_BUFFER, fireVBO);
    glBufferData(GL_ARRAY_BUFFER, fireParticles.size() * 5 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(4 * sizeof(float)));

    glBindVertexArray(0);
}

void updateFireParticles(float dt)
{
    unsigned int activeCount = 0;
    
    for (auto& particle : fireParticles)
    {
        particle.age += dt;

        if (particle.age >= particle.lifetime)
        {
            continue;  // Skip expired particles, don't respawn them
        }

        activeCount++;  // Count this as an active particle
        
        particle.position += particle.velocity * dt;
        particle.velocity.y += 0.7f * dt;
        particle.velocity.x *= 0.995f;
        particle.velocity.z *= 0.995f;
    }

    activeFireParticleCount = activeCount;  // Store the active count

    std::vector<float> gpuData;
    gpuData.reserve(fireParticles.size() * 5);

    for (const auto& particle : fireParticles)
    {
        gpuData.push_back(particle.position.x);
        gpuData.push_back(particle.position.y);
        gpuData.push_back(particle.position.z);
        gpuData.push_back(particle.age);
        gpuData.push_back(particle.lifetime);
    }

    glBindBuffer(GL_ARRAY_BUFFER, fireVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, gpuData.size() * sizeof(float), gpuData.data());
}

void renderFireParticles(const glm::mat4& view, const glm::mat4& projection)
{
    glUseProgram(fireShader);
    glUniformMatrix4fv(glGetUniformLocation(fireShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(fireShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1f(glGetUniformLocation(fireShader, "ParticleSize"), 1.0f);

    // Render opaque fire particles (no blending)
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    glBindVertexArray(fireVAO);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(activeFireParticleCount));
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);

        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    }

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// -------- Main --------
int main()
{
    glfwInit();

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // Set window hints for borderless fullscreen
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Scene", NULL, NULL);
    if (!window)
    {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    {
        std::stringstream title;
        title << "Scene | Score: " << playerScore;
        glfwSetWindowTitle(window, title.str().c_str());
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_PROGRAM_POINT_SIZE);

    // Create and bind HDR framebuffer, then attach floating-point color buffer
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    // Create floating point color buffer (HDR)
    glGenTextures(1, &colorBuffer);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA16F,   // HDR format (important!)
        windowWidth,
        windowHeight,
        0,
        GL_RGBA,
        GL_FLOAT,
        NULL
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        colorBuffer,
        0
    );

    // Create a normal buffer (second color attachment)
    glGenTextures(1, &normalBuffer);
    glBindTexture(GL_TEXTURE_2D, normalBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normalBuffer, 0);

    // Depth texture so we can sample scene depth in post-processing
    glGenTextures(1, &sceneDepth);
    glBindTexture(GL_TEXTURE_2D, sceneDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, windowWidth, windowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sceneDepth, 0);

    // Check completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "ERROR: HDR Framebuffer not complete!" << std::endl;
    }

    // Tell OpenGL we will render to both color attachments 0 and 1
    GLenum attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Bright-pass framebuffer for bloom extraction
    glGenFramebuffers(1, &brightFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, brightFBO);

    glGenTextures(1, &brightBuffer);
    glBindTexture(GL_TEXTURE_2D, brightBuffer);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA16F,
        windowWidth,
        windowHeight,
        0,
        GL_RGBA,
        GL_FLOAT,
        NULL
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        brightBuffer,
        0
    );

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "ERROR: Bright-pass framebuffer not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Ping-pong framebuffers for blur
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);

    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA16F,
            windowWidth,
            windowHeight,
            0,
            GL_RGBA,
            GL_FLOAT,
            NULL
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            pingpongColorbuffers[i],
            0
        );

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cout << "ERROR: Ping-pong framebuffer not complete!" << std::endl;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Viewport + resize callback
    glViewport(0, 0, windowWidth, windowHeight);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Mouse
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);

    // Sky color
    glClearColor(0.5f, 0.7f, 1.0f, 1.0f);

    // Mesh
    Mesh plane(planeVertices);

    std::vector<float> sphereVerts = createSphere(1.0f, 36, 18);
    Mesh sun(sphereVerts);

    std::vector<float> cubeVerts = createCube(1.0f);
    Mesh arenaBlock(cubeVerts);

    std::vector<float> capsuleVerts = createCapsule(0.65f, 2.4f, 36, 10);
    Mesh enemyMesh(capsuleVerts);

    // Shader
    unsigned int shader = createShader(
        "shader/basic_uniform.vert",
        "shader/basic_uniform.frag"
    );

	unsigned int emissiveShader = createShader(
    "shader/emissive.vert",
    "shader/emissive.frag"
	);

    unsigned int screenShader = createShader(
    "shader/screen.vert",
    "shader/bloom.frag"
    );

    unsigned int brightShader = createShader(
        "shader/screen.vert",
        "shader/brightpass.frag"
    );

    blurShader = createShader(
        "shader/screen.vert",
        "shader/blur.frag"
    );

    fireShader = createShader(
        "shader/fire.vert",
        "shader/fire.frag"
    );

    unsigned int toonShader = createShader(
        "shader/toon.vert",
        "shader/toon.frag"
    );

    // Depth (shadow) shader
    depthShader = createShader("shader/depth.vert", "shader/depth.frag");

    glUseProgram(screenShader);
    glUniform1i(glGetUniformLocation(screenShader, "scene"), 0);
    glUniform1i(glGetUniformLocation(screenShader, "bloomBlur"), 1);

    glUseProgram(blurShader);
    glUniform1i(glGetUniformLocation(blurShader, "image"), 0);

    glUseProgram(brightShader);
    glUniform1i(glGetUniformLocation(brightShader, "scene"), 0);

    initFireParticles();

    for (int i = 0; i < 3; ++i)
    {
        Enemy enemy;
        spawnRandomEnemy(enemy);
        enemies.push_back(enemy);
    }

    // Create depth framebuffer for shadow mapping
    glGenFramebuffers(1, &depthMapFBO);

    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR: Shadow Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Uniform locations are fetched from the active shader each frame so toon/basic stay in sync.

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        gameTime += deltaTime;
        enemySpawnAccumulator += deltaTime;

        float enemySpawnInterval = std::max(minEnemySpawnInterval, initialEnemySpawnInterval - (gameTime * enemySpawnRampRate));
        if (enemySpawnAccumulator >= enemySpawnInterval)
        {
            enemySpawnAccumulator = 0.0f;
            Enemy newEnemy;
            spawnRandomEnemy(newEnemy);
            enemies.push_back(newEnemy);
            std::cout << "Enemy spawned. Total enemies: " << enemies.size() << std::endl;
        }

        processInput(window);
        
        updatePlayerController();
        updateEnemy();

            // Update fire particles each frame so they spawn and animate naturally
            updateFireParticles(deltaTime);
            // Update thrown fireballs
            for (size_t i = 0; i < fireballs.size(); ) {
                Fireball& fireball = fireballs[i];

            // small gravity
            fireball.velocity += glm::vec3(0.0f, -9.8f, 0.0f) * (0.25f * deltaTime);
            fireball.position += fireball.velocity * deltaTime;
            fireball.life += deltaTime;

            // Emit trail particles at intervals
            if (fireball.life - fireball.lastTrailEmit > 0.03f) {
                fireball.lastTrailEmit = fireball.life;
                // Spawn multiple trail particles each time to make it more visible
                int trailCount = 0;
                for (auto& p : fireParticles) {
                    if (p.age >= p.lifetime && trailCount < 3) {
                        p.position = fireball.position + glm::vec3(
                            (randFloat() - 0.5f) * 0.3f,
                            (randFloat() - 0.5f) * 0.3f,
                            (randFloat() - 0.5f) * 0.3f
                        );
                        // Trail particles don't rise, just slight horizontal randomization
                        p.velocity = glm::vec3(
                            (randFloat() - 0.5f) * 0.3f,
                            0.0f,
                            (randFloat() - 0.5f) * 0.3f
                        );
                        p.age = 0.0f;
                        p.lifetime = 0.3f + randFloat() * 0.2f;
                        trailCount++;
                    }
                    if (trailCount >= 3) break;
                }
                // Upload buffer immediately so GPU sees the new trail particles
                if (trailCount > 0) {
                    std::vector<float> gpuData;
                    gpuData.reserve(fireParticles.size() * 5);
                    for (const auto& particle : fireParticles) {
                        gpuData.push_back(particle.position.x);
                        gpuData.push_back(particle.position.y);
                        gpuData.push_back(particle.position.z);
                        gpuData.push_back(particle.age);
                        gpuData.push_back(particle.lifetime);
                    }
                    glBindBuffer(GL_ARRAY_BUFFER, fireVBO);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, gpuData.size() * sizeof(float), gpuData.data());
                }
            }

            // enemy hit detection
            bool hitEnemy = false;
            for (auto& enemy : enemies)
            {
                if (!enemy.active)
                    continue;

                glm::vec3 enemyHitCenter = enemy.position + glm::vec3(0.0f, 1.0f, 0.0f);
                float enemyHitDistance = fireball.radius + 0.9f;
                if (glm::length(fireball.position - enemyHitCenter) <= enemyHitDistance)
                {
                    spawnFireExplosion(fireball.position);
                    enemy.health -= 1.0f;
                    std::cout << "Enemy hit! Health: " << enemy.health << std::endl;
                    if (enemy.health <= 0.0f)
                    {
                        playerScore += 100;
                        {
                            std::stringstream title;
                            title << "Scene | Score: " << playerScore;
                            glfwSetWindowTitle(window, title.str().c_str());
                        }
                        spawnRandomEnemy(enemy);
                        std::cout << "Enemy defeated. Score: " << playerScore << ". Respawning immediately." << std::endl;
                    }

                    fireballs.erase(fireballs.begin() + static_cast<std::vector<Fireball>::difference_type>(i));
                    hitEnemy = true;
                    break;
                }
            }
            if (hitEnemy)
                continue;

            // Wall collision detection
            const float arenaMin = -50.0f;
            const float arenaMax = 50.0f;
            const float wallCollisionBuffer = 0.5f;
            
            if (fireball.position.x < arenaMin + wallCollisionBuffer ||
                fireball.position.x > arenaMax - wallCollisionBuffer ||
                fireball.position.z < arenaMin + wallCollisionBuffer ||
                fireball.position.z > arenaMax - wallCollisionBuffer)
            {
                spawnFireExplosion(fireball.position);
                fireballs.erase(fireballs.begin() + static_cast<std::vector<Fireball>::difference_type>(i));
                continue;
            }

            // impact or timeout
            if (fireball.position.y <= 0.1f || fireball.life >= fireball.maxLife) {
                spawnFireExplosion(fireball.position);

                // remove this fireball after impact
                fireballs.erase(fireballs.begin() + static_cast<std::vector<Fireball>::difference_type>(i));
                continue;
            }

                ++i;
            }

        // -- 1) Render scene to depth map from light's POV (shadow map) --
        // Position the sun past the northeast corner of the arena
        // Arena walls are at approximately +/-50.5; place sun beyond the NE corner
        float arenaCorner = 50.5f;
        float sunOffset = 20.0f; // how far past the corner
        glm::vec3 sunPos = glm::vec3(arenaCorner + sunOffset, 60.0f, arenaCorner + sunOffset);
        // Use a stable target at arena center so distant objects don't fall out of shadow coverage
        glm::vec3 lightTarget = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 lightDir = glm::normalize(lightTarget - sunPos);
        glm::mat4 lightProjection, lightView, lightSpaceMatrix;
        float near_plane = 1.0f;
        float far_plane = 500.0f;
        // Fixed arena-wide coverage (arena is roughly 100x100), plus margin for walls and enemies
        float orthoSize = 110.0f;
        lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, near_plane, far_plane);
        // Use the sun world position as the light position for the depth pass
        glm::vec3 lightPos = sunPos;
        // Keep light view stable over arena to avoid distance-based shadow popping
        lightView = glm::lookAt(lightPos, lightTarget, glm::vec3(0.0f, 1.0f, 0.0f));
        lightSpaceMatrix = lightProjection * lightView;

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        glUseProgram(depthShader);
        glUniformMatrix4fv(glGetUniformLocation(depthShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        // Render scene geometry to depth map
        // floor
        glm::mat4 modelDepth = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(depthShader, "model"), 1, GL_FALSE, glm::value_ptr(modelDepth));
        plane.draw();

        // walls (same transforms as main pass)
        glm::mat4 wallModelDepth = glm::mat4(1.0f);
        wallModelDepth = glm::translate(wallModelDepth, glm::vec3(0.0f, 2.5f, -50.5f));
        wallModelDepth = glm::scale(wallModelDepth, glm::vec3(100.0f, 5.0f, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(depthShader, "model"), 1, GL_FALSE, glm::value_ptr(wallModelDepth));
        arenaBlock.draw();

        wallModelDepth = glm::mat4(1.0f);
        wallModelDepth = glm::translate(wallModelDepth, glm::vec3(0.0f, 2.5f, 50.5f));
        wallModelDepth = glm::scale(wallModelDepth, glm::vec3(100.0f, 5.0f, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(depthShader, "model"), 1, GL_FALSE, glm::value_ptr(wallModelDepth));
        arenaBlock.draw();

        wallModelDepth = glm::mat4(1.0f);
        wallModelDepth = glm::translate(wallModelDepth, glm::vec3(-50.5f, 2.5f, 0.0f));
        wallModelDepth = glm::scale(wallModelDepth, glm::vec3(1.0f, 5.0f, 100.0f));
        glUniformMatrix4fv(glGetUniformLocation(depthShader, "model"), 1, GL_FALSE, glm::value_ptr(wallModelDepth));
        arenaBlock.draw();

        wallModelDepth = glm::mat4(1.0f);
        wallModelDepth = glm::translate(wallModelDepth, glm::vec3(50.5f, 2.5f, 0.0f));
        wallModelDepth = glm::scale(wallModelDepth, glm::vec3(1.0f, 5.0f, 100.0f));
        glUniformMatrix4fv(glGetUniformLocation(depthShader, "model"), 1, GL_FALSE, glm::value_ptr(wallModelDepth));
        arenaBlock.draw();

        // enemies
        for (const auto& enemy : enemies)
        {
            if (!enemy.active) continue;
            glm::mat4 enemyModelDepth = glm::translate(glm::mat4(1.0f), enemy.position + glm::vec3(0.0f, 1.0f, 0.0f));
            enemyModelDepth = glm::scale(enemyModelDepth, glm::vec3(0.55f));
            glUniformMatrix4fv(glGetUniformLocation(depthShader, "model"), 1, GL_FALSE, glm::value_ptr(enemyModelDepth));
            enemyMesh.draw();
        }

        // fireballs
        for (const auto& fireball : fireballs)
        {
            if (!fireball.active) continue;
            glm::mat4 ballModelDepth = glm::translate(glm::mat4(1.0f), fireball.position);
            ballModelDepth = glm::scale(ballModelDepth, glm::vec3(0.35f));
            glUniformMatrix4fv(glGetUniformLocation(depthShader, "model"), 1, GL_FALSE, glm::value_ptr(ballModelDepth));
            sun.draw();
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, windowWidth, windowHeight);

        // -- 2) Render scene as usual to HDR framebuffer --
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Choose which shader to use based on toon shading toggle
        unsigned int activeShader = toonShadingEnabled ? toonShader : shader;
        glUseProgram(activeShader);

        int viewLoc = glGetUniformLocation(activeShader, "view");
        int projLoc = glGetUniformLocation(activeShader, "projection");
        int modelLoc = glGetUniformLocation(activeShader, "model");
        int normalMatrixLoc = glGetUniformLocation(activeShader, "normalMatrix");
        int lightLoc = glGetUniformLocation(activeShader, "lightDir");
        int viewPosLoc = glGetUniformLocation(activeShader, "viewPos");
        int objectColorLoc = glGetUniformLocation(activeShader, "objectColor");

        // Matrices
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(fov), static_cast<float>(windowWidth) / static_cast<float>(windowHeight), 0.1f, 500.0f);
        glm::mat4 model = glm::mat4(1.0f);

        // Send uniforms
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::transpose(glm::inverse(model)))));

        glUniform3fv(lightLoc, 1, glm::value_ptr(lightDir));
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));

        // Bind shadow map to texture unit 2 for sampling in shader
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glUniform1i(glGetUniformLocation(activeShader, "shadowMap"), 2);
        glUniformMatrix4fv(glGetUniformLocation(activeShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        // Arena floor
        glUniform3f(objectColorLoc, 0.28f, 0.28f, 0.30f);
        plane.draw();

        // Arena walls
        glUniform3f(objectColorLoc, 0.40f, 0.35f, 0.30f);

        glm::mat4 wallModel = glm::mat4(1.0f);
        wallModel = glm::translate(wallModel, glm::vec3(0.0f, 2.5f, -50.5f));
        wallModel = glm::scale(wallModel, glm::vec3(100.0f, 5.0f, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wallModel));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::transpose(glm::inverse(wallModel)))));
        arenaBlock.draw();

        wallModel = glm::mat4(1.0f);
        wallModel = glm::translate(wallModel, glm::vec3(0.0f, 2.5f, 50.5f));
        wallModel = glm::scale(wallModel, glm::vec3(100.0f, 5.0f, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wallModel));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::transpose(glm::inverse(wallModel)))));
        arenaBlock.draw();

        wallModel = glm::mat4(1.0f);
        wallModel = glm::translate(wallModel, glm::vec3(-50.5f, 2.5f, 0.0f));
        wallModel = glm::scale(wallModel, glm::vec3(1.0f, 5.0f, 100.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wallModel));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::transpose(glm::inverse(wallModel)))));
        arenaBlock.draw();

        wallModel = glm::mat4(1.0f);
        wallModel = glm::translate(wallModel, glm::vec3(50.5f, 2.5f, 0.0f));
        wallModel = glm::scale(wallModel, glm::vec3(1.0f, 5.0f, 100.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wallModel));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::transpose(glm::inverse(wallModel)))));
        arenaBlock.draw();

        // Sun position: reuse `sunPos` computed earlier for the depth pass
        // (keeps the emissive sun object and the light source in sync)

		// Build sun model matrix
		glm::mat4 sunModel = glm::translate(glm::mat4(1.0f), sunPos);
		sunModel = glm::scale(sunModel, glm::vec3(2.0f));

		// Switch to emissive shader
		glUseProgram(emissiveShader);

		// Get uniform locations (you can cache these later too)
		glUniformMatrix4fv(glGetUniformLocation(emissiveShader, "model"), 1, GL_FALSE, glm::value_ptr(sunModel));
		glUniformMatrix4fv(glGetUniformLocation(emissiveShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(emissiveShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		// Sun color (yellow-ish glow)
		glUniform3f(glGetUniformLocation(emissiveShader, "color"), 1.0f, 0.9f, 0.3f);

		// Draw sun mesh
		sun.draw();

        // Draw enemy as a red emissive sphere
        for (const auto& enemy : enemies)
        {
            if (!enemy.active)
                continue;

            glUseProgram(emissiveShader);
            glm::mat4 enemyModel = glm::translate(glm::mat4(1.0f), enemy.position + glm::vec3(0.0f, 1.0f, 0.0f));
            enemyModel = glm::scale(enemyModel, glm::vec3(0.55f));
            glUniformMatrix4fv(glGetUniformLocation(emissiveShader, "model"), 1, GL_FALSE, glm::value_ptr(enemyModel));
            glUniformMatrix4fv(glGetUniformLocation(emissiveShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(emissiveShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniform3f(glGetUniformLocation(emissiveShader, "color"), 2.2f, 0.25f, 0.15f);
            enemyMesh.draw();
            glUseProgram(shader);
        }

            // Draw active fireballs as emissive spheres
        for (const auto& fireball : fireballs)
        {
            if (!fireball.active)
                continue;

            glUseProgram(emissiveShader);
            glm::mat4 ballModel = glm::translate(glm::mat4(1.0f), fireball.position);
            ballModel = glm::scale(ballModel, glm::vec3(0.35f));
            glUniformMatrix4fv(glGetUniformLocation(emissiveShader, "model"), 1, GL_FALSE, glm::value_ptr(ballModel));
            glUniformMatrix4fv(glGetUniformLocation(emissiveShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(emissiveShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniform3f(glGetUniformLocation(emissiveShader, "color"), 4.0f, 1.6f, 0.2f);
            sun.draw();
            glUseProgram(shader);
        }

        // Cartoon-style fire particles near the origin
        renderFireParticles(view, projection);

		// IMPORTANT: switch back to main shader if needed later
		glUseProgram(shader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Extract only bright areas for bloom
        glBindFramebuffer(GL_FRAMEBUFFER, brightFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glUseProgram(brightShader);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffer);
        glUniform1f(glGetUniformLocation(brightShader, "threshold"), brightThreshold);
        renderQuad();

        // Blur bright scene texture using ping-pong FBOs
        glUseProgram(blurShader);
        bool horizontal = true;
        bool first_iteration = true;
        int blurAmount = 10;

        for (int i = 0; i < blurAmount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            glUniform1i(glGetUniformLocation(blurShader, "horizontal"), horizontal);

            glBindTexture(GL_TEXTURE_2D, first_iteration ? brightBuffer : pingpongColorbuffers[!horizontal]);
            renderQuad();

            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(screenShader);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffer);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        glUniform1f(glGetUniformLocation(screenShader, "bloomStrength"), bloomEnabled ? bloomStrength : 0.0f);
        glUniform1i(glGetUniformLocation(screenShader, "vignetteEnabled"), vignetteEnabled ? 1 : 0);
        glUniform1f(glGetUniformLocation(screenShader, "manaRatio"), playerMana / playerMaxMana);
        glUniform1f(glGetUniformLocation(screenShader, "healthRatio"), playerHealth / playerMaxHealth);
        glUniform1i(glGetUniformLocation(screenShader, "scoreValue"), playerScore);
        glUniform1i(glGetUniformLocation(screenShader, "timeValue"), static_cast<int>(gameTime));
        glUniform1i(glGetUniformLocation(screenShader, "paused"), gamePaused ? 1 : 0);
        glUniform2f(glGetUniformLocation(screenShader, "mousePos"), mousePosition.x, mousePosition.y);

        // Bind normal and scene depth for edge detection
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, normalBuffer);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, sceneDepth);
        glUniform1i(glGetUniformLocation(screenShader, "normalMap"), 2);
        glUniform1i(glGetUniformLocation(screenShader, "depthMap"), 3);
        // Edge detection settings
        glUniform1i(glGetUniformLocation(screenShader, "edgeEnabled"), edgeEnabled ? 1 : 0);
        glUniform1f(glGetUniformLocation(screenShader, "edgeDepthThreshold"), 0.006);
        glUniform1f(glGetUniformLocation(screenShader, "edgeNormalThreshold"), 0.25);
        glUniform1f(glGetUniformLocation(screenShader, "edgeStrength"), 1.0);

        // Render fullscreen quad with HDR scene + blurred texture
        renderQuad();
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}