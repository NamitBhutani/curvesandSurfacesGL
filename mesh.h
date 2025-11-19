#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "shader.h"

#include <string>
#include <vector>
#include <fstream>  // Required for file output
#include <iostream> // Required for error printing

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
};

class Mesh
{
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int edgeEBO;
    unsigned int edgeCount;
    unsigned int VAO;

    // Default constructor
    Mesh() : edgeCount(0), VAO(0), VBO(0), EBO(0), edgeEBO(0) {}

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices)
    {
        this->vertices = vertices;
        this->indices = indices;

        setupMesh();
    }

    void draw(Shader &shader)
    {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void drawEdges(Shader &shader)
    {
        glBindVertexArray(VAO);

        // temporarily disable the vertex color attribute and set a constant color (Black)
        glDisableVertexAttribArray(2);
        glVertexAttrib3f(2, 0.0f, 0.0f, 0.0f);

        // bind edge EBO and draw
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edgeEBO);
        glLineWidth(1.5f);
        glDrawElements(GL_LINES, edgeCount, GL_UNSIGNED_INT, 0);

        // restore triangle EBO and color attribute
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
    }

    bool writeOFF(const std::string &filename) const
    {
        std::ofstream out(filename);
        if (!out)
        {
            std::cerr << "[ERROR] Failed to open file for writing: " << filename << std::endl;
            return false;
        }

        // Write OFF header
        out << "OFF\n";

        // Calculate number of faces (each 3 indices = 1 triangle)
        size_t numFaces = indices.size() / 3;

        // Write counts: vertices, faces, edges (0 for edges)
        out << vertices.size() << " " << numFaces << " 0\n";

        // Write vertices (position only)
        for (const auto &v : vertices)
        {
            out << v.position.x << " " << v.position.y << " " << v.position.z << "\n";
        }

        // Write faces (triangles)
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            out << "3 " << indices[i] << " " << indices[i + 1] << " " << indices[i + 2] << "\n";
        }

        out.close();
        std::cout << "[SUCCESS] Wrote mesh to " << filename << std::endl;
        return true;
    }

private:
    unsigned int VBO, EBO;

    void setupMesh()
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        // build edge index list (each triangle gives three edges)
        std::vector<unsigned int> edges;
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            if (i + 2 >= indices.size())
                break; // Safety check
            unsigned int a = indices[i];
            unsigned int b = indices[i + 1];
            unsigned int c = indices[i + 2];
            edges.push_back(a);
            edges.push_back(b);
            edges.push_back(b);
            edges.push_back(c);
            edges.push_back(c);
            edges.push_back(a);
        }
        edgeCount = static_cast<unsigned int>(edges.size());

        glGenBuffers(1, &edgeEBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edgeEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, edges.size() * sizeof(unsigned int), edges.data(), GL_STATIC_DRAW);

        // triangle EBO needs to be bound last
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
        // Normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, normal));
        // Color
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, color));

        glBindVertexArray(0);
    }
};

#endif