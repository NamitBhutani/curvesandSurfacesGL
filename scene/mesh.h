#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <shader.h>

#include <string>
#include <vector>

struct Vertex
{
    glm::vec3 position;
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

        // temporarily disable the vertex color attribute and set a constant color
        glDisableVertexAttribArray(1);
        glVertexAttrib3f(1, 1.0f, 0.0f, 1.0f);

        // bind edge EBO and draw
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edgeEBO);
        glLineWidth(2.0f);
        glDrawElements(GL_LINES, edgeCount, GL_UNSIGNED_INT, 0);

        // restore triangle EBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
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
        for (int i = 0; i < indices.size() - 2; i += 3)
        {
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
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, edges.size() * sizeof(unsigned int), &edges[0], GL_STATIC_DRAW);

        // triangle EBO needs to be bound last since only one EBO can be bound to a VAO at any given draw call
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, color));

        glBindVertexArray(0);
    }
};
#endif