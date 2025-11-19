#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include <mesh.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <vector>
#include <cmath>

Mesh createCube(float size = 1.0f, glm::vec3 color = glm::vec3(1.0f))
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float h = size / 2.0f;

    //  8 corners of cube
    glm::vec3 corners[8] = {
        {-h, -h, -h}, {h, -h, -h}, {h, h, -h}, {-h, h, -h}, // back face
        {-h, -h, h},
        {h, -h, h},
        {h, h, h},
        {-h, h, h} // front face
    };

    //  6 faces with normals
    struct Face
    {
        int v[4];
        glm::vec3 normal;
    };

    Face faces[6] = {
        {{0, 1, 2, 3}, {0, 0, -1}}, // back
        {{4, 7, 6, 5}, {0, 0, 1}},  // front
        {{0, 3, 7, 4}, {-1, 0, 0}}, // left
        {{1, 5, 6, 2}, {1, 0, 0}},  // right
        {{0, 4, 5, 1}, {0, -1, 0}}, // bottom
        {{3, 2, 6, 7}, {0, 1, 0}}   // top
    };

    for (int f = 0; f < 6; f++)
    {
        unsigned int baseIdx = vertices.size();

        for (int i = 0; i < 4; i++)
        {
            Vertex v;
            v.position = corners[faces[f].v[i]];
            v.normal = faces[f].normal;
            v.color = color;
            vertices.push_back(v);
        }

        // 2 triangles per face
        indices.push_back(baseIdx);
        indices.push_back(baseIdx + 1);
        indices.push_back(baseIdx + 2);

        indices.push_back(baseIdx);
        indices.push_back(baseIdx + 2);
        indices.push_back(baseIdx + 3);
    }

    return Mesh(vertices, indices);
}

Mesh createCylinder(float radius = 0.5f, float height = 1.0f, int segments = 32, glm::vec3 color = glm::vec3(1.0f))
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float h = height / 2.0f;
    float angleStep = (2.0f * glm::pi<float>()) / segments;

    // gen side vertices
    for (int ring = 0; ring < 2; ring++)
    {
        float y = (ring == 0) ? -h : h;

        for (int i = 0; i < segments; i++)
        {
            float angle = i * angleStep;
            float x = radius * cos(angle);
            float z = radius * sin(angle);

            Vertex v;
            v.position = glm::vec3(x, y, z);
            v.normal = glm::normalize(glm::vec3(x, 0, z));
            v.color = color;
            vertices.push_back(v);
        }
    }

    // side faces
    for (int i = 0; i < segments; i++)
    {
        int next = (i + 1) % segments;

        indices.push_back(i);
        indices.push_back(i + segments);
        indices.push_back(next + segments);

        indices.push_back(i);
        indices.push_back(next + segments);
        indices.push_back(next);
    }

    int baseIdx = vertices.size();

    // Bottom cap center
    Vertex bottomCenter;
    bottomCenter.position = glm::vec3(0, -h, 0);
    bottomCenter.normal = glm::vec3(0, -1, 0);
    bottomCenter.color = color;
    vertices.push_back(bottomCenter);

    for (int i = 0; i < segments; i++)
    {
        int next = (i + 1) % segments;
        indices.push_back(baseIdx);
        indices.push_back(i);
        indices.push_back(next);
    }

    // Top cap center
    baseIdx = vertices.size();
    Vertex topCenter;
    topCenter.position = glm::vec3(0, h, 0);
    topCenter.normal = glm::vec3(0, 1, 0);
    topCenter.color = color;
    vertices.push_back(topCenter);

    for (int i = 0; i < segments; i++)
    {
        int next = (i + 1) % segments;
        indices.push_back(baseIdx);
        indices.push_back(next + segments);
        indices.push_back(i + segments);
    }

    return Mesh(vertices, indices);
}

Mesh createCone(float radius = 0.5f, float height = 1.0f, int segments = 32, glm::vec3 color = glm::vec3(1.0f))
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float angleStep = (2.0f * glm::pi<float>()) / segments;

    Vertex apex;
    apex.position = glm::vec3(0, height, 0);
    apex.color = color;
    vertices.push_back(apex);

    // base vertices
    for (int i = 0; i < segments; i++)
    {
        float angle = i * angleStep;
        float x = radius * cos(angle);
        float z = radius * sin(angle);

        Vertex v;
        v.position = glm::vec3(x, 0, z);
        v.normal = glm::normalize(glm::vec3(x, radius, z));
        v.color = color;
        vertices.push_back(v);
    }

    // Side faces
    for (int i = 0; i < segments; i++)
    {
        int next = (i + 1) % segments;
        indices.push_back(0);
        indices.push_back(i + 1);
        indices.push_back(next + 1);
    }

    // Base cap
    int baseIdx = vertices.size();
    Vertex baseCenter;
    baseCenter.position = glm::vec3(0, 0, 0);
    baseCenter.normal = glm::vec3(0, -1, 0);
    baseCenter.color = color;
    vertices.push_back(baseCenter);

    for (int i = 0; i < segments; i++)
    {
        int next = (i + 1) % segments;
        indices.push_back(baseIdx);
        indices.push_back(next + 1);
        indices.push_back(i + 1);
    }

    return Mesh(vertices, indices);
}

Mesh createSphere(float radius = 0.5f, int stacks = 16, int slices = 32, glm::vec3 color = glm::vec3(1.0f))
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= stacks; i++)
    {
        float phi = glm::pi<float>() * float(i) / float(stacks);
        float y = radius * cos(phi);
        float r = radius * sin(phi);

        for (int j = 0; j <= slices; j++)
        {
            float theta = 2.0f * glm::pi<float>() * float(j) / float(slices);
            float x = r * cos(theta);
            float z = r * sin(theta);

            Vertex v;
            v.position = glm::vec3(x, y, z);
            v.normal = glm::normalize(v.position);
            v.color = color;
            vertices.push_back(v);
        }
    }

    for (int i = 0; i < stacks; i++)
    {
        for (int j = 0; j < slices; j++)
        {
            int p0 = i * (slices + 1) + j;
            int p1 = p0 + slices + 1;
            int p2 = p1 + 1;
            int p3 = p0 + 1;

            indices.push_back(p0);
            indices.push_back(p1);
            indices.push_back(p2);

            indices.push_back(p0);
            indices.push_back(p2);
            indices.push_back(p3);
        }
    }

    return Mesh(vertices, indices);
}

Mesh createPlane(float width = 1.0f, float depth = 1.0f, glm::vec3 color = glm::vec3(1.0f))
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float w = width / 2.0f;
    float d = depth / 2.0f;

    Vertex v1, v2, v3, v4;
    v1.position = glm::vec3(-w, 0, -d);
    v2.position = glm::vec3(w, 0, -d);
    v3.position = glm::vec3(w, 0, d);
    v4.position = glm::vec3(-w, 0, d);

    v1.normal = v2.normal = v3.normal = v4.normal = glm::vec3(0, 1, 0);
    v1.color = v2.color = v3.color = v4.color = color;

    vertices.push_back(v1);
    vertices.push_back(v2);
    vertices.push_back(v3);
    vertices.push_back(v4);

    indices = {0, 1, 2, 0, 2, 3};

    return Mesh(vertices, indices);
}

// mainRadius: dist from center to the middle of the tube
// tubeRadius: radius of the tube itself
Mesh createTorus(float mainRadius = 1.0f, float tubeRadius = 0.2f, int mainSegments = 32, int tubeSegments = 16, glm::vec3 color = glm::vec3(1.0f))
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= mainSegments; i++)
    {
        float phi = glm::two_pi<float>() * (float)i / mainSegments;
        float cosPhi = cos(phi);
        float sinPhi = sin(phi);

        for (int j = 0; j <= tubeSegments; j++)
        {
            float theta = glm::two_pi<float>() * (float)j / tubeSegments;
            float cosTheta = cos(theta);
            float sinTheta = sin(theta);

            Vertex v;
            float x = (mainRadius + tubeRadius * cosTheta) * cosPhi;
            float z = (mainRadius + tubeRadius * cosTheta) * sinPhi;
            float y = tubeRadius * sinTheta;

            v.position = glm::vec3(x, y, z);

            // normals calculation
            glm::vec3 centerToTube = glm::vec3(mainRadius * cosPhi, 0.0f, mainRadius * sinPhi);
            v.normal = glm::normalize(v.position - centerToTube);
            v.color = color;
            vertices.push_back(v);
        }
    }

    for (int i = 0; i < mainSegments; i++)
    {
        for (int j = 0; j < tubeSegments; j++)
        {
            int current = i * (tubeSegments + 1) + j;
            int next = current + tubeSegments + 1;

            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(next);
            indices.push_back(next + 1);
            indices.push_back(current + 1);
        }
    }

    return Mesh(vertices, indices);
}

Mesh createCylinderSector(float radius = 0.5f, float height = 1.0f, float sweepAngle = 360.0f, int segments = 32, glm::vec3 color = glm::vec3(1.0f))
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float h = height / 2.0f;
    // convert sweep to radians
    float totalAngle = glm::radians(sweepAngle);
    float angleStep = totalAngle / segments;

    Vertex bottomCenter, topCenter;
    bottomCenter.position = glm::vec3(0, -h, 0);
    bottomCenter.normal = glm::vec3(0, -1, 0);
    bottomCenter.color = color;

    topCenter.position = glm::vec3(0, h, 0);
    topCenter.normal = glm::vec3(0, 1, 0);
    topCenter.color = color;

    vertices.push_back(bottomCenter);
    vertices.push_back(topCenter);

    // we need vertices for side, top cap, bottom cap, and the two flat faces
    // To simplify, we will duplicate vertices for sharp normals at the edges

    int rimStart = vertices.size();

    for (int i = 0; i <= segments; i++)
    {
        float angle = i * angleStep;
        float x = radius * cos(angle);
        float z = radius * sin(angle);

        // Top rim vertex
        Vertex vt;
        vt.position = glm::vec3(x, h, z);
        vt.normal = glm::normalize(glm::vec3(x, 0, z)); // Side normal
        vt.color = color;
        vertices.push_back(vt);

        // Bottom rim vertex
        Vertex vb;
        vb.position = glm::vec3(x, -h, z);
        vb.normal = glm::normalize(glm::vec3(x, 0, z)); // Side normal
        vb.color = color;
        vertices.push_back(vb);
    }

    for (int i = 0; i < segments; i++)
    {
        int top1 = rimStart + (i * 2);
        int bottom1 = top1 + 1;
        int top2 = top1 + 2;
        int bottom2 = bottom1 + 2;

        indices.push_back(bottom1);
        indices.push_back(top2);
        indices.push_back(top1);

        indices.push_back(bottom1);
        indices.push_back(bottom2);
        indices.push_back(top2);
    }

    // caps (simple fans)
    for (int i = 0; i < segments; i++)
    {
        int top1 = rimStart + (i * 2);
        int top2 = top1 + 2;
        // Top cap
        indices.push_back(1); // Top center
        indices.push_back(top1);
        indices.push_back(top2);

        int bottom1 = top1 + 1;
        int bottom2 = bottom1 + 2;
        // Bottom cap
        indices.push_back(0); // Bottom center
        indices.push_back(bottom2);
        indices.push_back(bottom1);
    }

    // If it's not a full circle, we need to close the flat faces
    if (sweepAngle < 360.0f)
    {
        // Close Start Face
        int startTop = rimStart;
        int startBottom = rimStart + 1;
        indices.push_back(0);
        indices.push_back(1);
        indices.push_back(startTop);
        indices.push_back(0);
        indices.push_back(startTop);
        indices.push_back(startBottom);

        // Close End Face
        int endTop = rimStart + (segments * 2);
        int endBottom = endTop + 1;
        indices.push_back(0);
        indices.push_back(endTop);
        indices.push_back(1);
        indices.push_back(0);
        indices.push_back(endBottom);
        indices.push_back(endTop);
    }

    return Mesh(vertices, indices);
}

#endif
