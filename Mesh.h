#pragma once
#include <vector>
#include <glad/glad.h>

class Mesh {
public:
    std::vector<float> vertices;
    unsigned int VAO, VBO;

    Mesh(const std::vector<float>& verts);
    void draw();
};