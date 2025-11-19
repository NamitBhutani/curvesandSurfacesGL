#ifndef BEZIER_H
#define BEZIER_H

#include <iostream>
#include <vector>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "../mesh.h"

class BezierCurve
{
public:
    std::vector<glm::vec2> controlPoints;

    BezierCurve() {}

    glm::vec2 evaluate(float t) const
    {
        if (controlPoints.empty())
        {
            return glm::vec2(0.0f);
        }
        if (controlPoints.size() == 1)
        {
            return controlPoints[0];
        }

        // clamp t to the valid range
        t = glm::clamp(t, 0.0f, 1.0f);

        std::vector<glm::vec2> tempPoints = controlPoints;
        int n = tempPoints.size();

        // iterative De Casteljau's algorithm
        for (int k = 1; k < n; ++k)
        {
            for (int i = 0; i < n - k; ++i)
            {
                tempPoints[i] = (1.0f - t) * tempPoints[i] + t * tempPoints[i + 1];
            }
        }

        return tempPoints[0];
    }

    std::vector<glm::vec2> sampleCurve(float step = 0.1f) const
    {
        std::vector<glm::vec2> sampledPoints;
        if (controlPoints.empty())
        {
            return sampledPoints;
        }

        for (float t = 0.0f; t < 1.0f; t += step)
        {
            sampledPoints.push_back(evaluate(t));
        }

        // always include the last point
        sampledPoints.push_back(evaluate(1.0f));

        return sampledPoints;
    }

    Mesh createSurfaceOfRevolution(int segments, float sampleStep = 0.1f) const
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        std::vector<glm::vec2> samples = sampleCurve(sampleStep);
        int numSamples = samples.size();

        if (numSamples < 2 || segments < 3)
        {
            // Return empty mesh if invalid
            return Mesh(vertices, indices);
        }

        // calc tangents for normals
        std::vector<glm::vec2> tangents(numSamples);
        if (numSamples > 1)
        {
            tangents[0] = glm::normalize(samples[1] - samples[0]);
            for (int i = 1; i < numSamples - 1; ++i)
            {
                tangents[i] = glm::normalize(samples[i + 1] - samples[i - 1]);
            }
            tangents[numSamples - 1] = glm::normalize(samples[numSamples - 1] - samples[numSamples - 2]);
        }

        float angleStep = (2.0f * glm::pi<float>()) / static_cast<float>(segments);

        // gen vertices from sampled points
        for (int i = 0; i < numSamples; ++i)
        {
            float r = samples[i].x;
            float y = samples[i].y;

            float tx = tangents[i].x;
            float ty = tangents[i].y;

            for (int j = 0; j < segments; ++j)
            {
                float angle = static_cast<float>(j) * angleStep;
                float cosA = cos(angle);
                float sinA = sin(angle);

                glm::vec3 pos = {r * cosA, y, r * sinA};

                // Calculate 3D normal based on 2D tangent rotated around Y
                glm::vec3 norm = glm::normalize(glm::vec3(ty * cosA, -tx, ty * sinA));

                Vertex v;
                v.position = pos;
                v.normal = norm;
                v.color = glm::vec3(1.0f); // Default white

                vertices.push_back(v);
            }
        }

        // make quads (as 2 tris) to connect the rings
        for (int i = 0; i < numSamples - 1; ++i)
        {
            for (int j = 0; j < segments; ++j)
            {
                unsigned int p0 = i * segments + j;
                unsigned int p1 = (i + 1) * segments + j;
                unsigned int p2 = (i + 1) * segments + ((j + 1) % segments);
                unsigned int p3 = i * segments + ((j + 1) % segments);

                // tri 1
                indices.push_back(p0);
                indices.push_back(p1);
                indices.push_back(p2);

                // tri 2
                indices.push_back(p0);
                indices.push_back(p2);
                indices.push_back(p3);
            }
        }

        return Mesh(vertices, indices);
    }

    // Creates the Slide geometry by extruding a U-profile along the curve
    Mesh createSlideExtrusion(float width, float wallHeight, int segments, glm::vec3 color = glm::vec3(1.0f)) const
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        // Sample the curve points
        std::vector<glm::vec2> samples = sampleCurve(1.0f / segments);
        int numSamples = samples.size();

        if (numSamples < 2)
            return Mesh(vertices, indices);

        // 1. Define the U-shaped profile (relative to the curve center)
        // 4 points: Left Wall Top -> Left Floor -> Right Floor -> Right Wall Top
        std::vector<glm::vec3> profile;
        profile.push_back(glm::vec3(-width / 2.0f, wallHeight, 0.0f));
        profile.push_back(glm::vec3(-width / 2.0f, 0.0f, 0.0f));
        profile.push_back(glm::vec3(width / 2.0f, 0.0f, 0.0f));
        profile.push_back(glm::vec3(width / 2.0f, wallHeight, 0.0f));

        // 2. Generate Vertices by moving the profile along the curve
        for (int i = 0; i < numSamples; ++i)
        {
            // Current position on the curve (mapped to X, Y, 0)
            glm::vec3 P(samples[i].x, samples[i].y, 0.0f);

            // Calculate Tangent (Forward vector)
            glm::vec3 forward;
            if (i < numSamples - 1)
                forward = glm::normalize(glm::vec3(samples[i + 1].x - samples[i].x, samples[i + 1].y - samples[i].y, 0.0f));
            else
                forward = glm::normalize(glm::vec3(samples[i].x - samples[i - 1].x, samples[i].y - samples[i - 1].y, 0.0f));

            // Calculate Orientation Vectors (Frenet Frame approximation)
            glm::vec3 globalUp = glm::vec3(0.0f, 1.0f, 0.0f);
            // Prevent gimbal lock if curve goes vertical
            if (glm::abs(glm::dot(forward, globalUp)) > 0.99f)
                globalUp = glm::vec3(0.0f, 0.0f, 1.0f);

            glm::vec3 right = glm::normalize(glm::cross(forward, globalUp)); // Profile X axis
            glm::vec3 up = glm::normalize(glm::cross(right, forward));       // Profile Y axis

            // Place profile vertices
            for (const auto &p : profile)
            {
                Vertex v;

                // Transform profile point to world space
                // p.x moves along 'right', p.y moves along 'up'
                v.position = P + (right * p.x) + (up * p.y);

                // Calculate Normals
                // Floor normal is 'up', Wall normals are 'right' or '-right'
                if (p.y > 0.01f)
                {
                    v.normal = (p.x > 0) ? right : -right; // Walls
                }
                else
                {
                    v.normal = up; // Floor
                }

                v.color = color;
                vertices.push_back(v);
            }
        }

        // 3. Generate Indices to stitch the segments
        int vertsPerRing = profile.size();
        for (int i = 0; i < numSamples - 1; ++i)
        {
            for (int j = 0; j < vertsPerRing - 1; ++j)
            {
                unsigned int current = i * vertsPerRing + j;
                unsigned int next = (i + 1) * vertsPerRing + j;

                // Quad formed by two triangles
                indices.push_back(current);
                indices.push_back(next);
                indices.push_back(current + 1);

                indices.push_back(current + 1);
                indices.push_back(next);
                indices.push_back(next + 1);
            }
        }

        return Mesh(vertices, indices);
    }

    // to hold de Casteljau intermediate points for anim
    struct DeCasteljauSteps
    {
        std::vector<std::vector<glm::vec2>> levels; // Each level is one iteration
        glm::vec2 finalPoint;                       // The point on the curve
    };

    DeCasteljauSteps evaluateWithSteps(float t) const
    {
        DeCasteljauSteps steps;

        if (controlPoints.empty())
        {
            return steps;
        }

        if (controlPoints.size() == 1)
        {
            steps.finalPoint = controlPoints[0];
            steps.levels.push_back({controlPoints[0]});
            return steps;
        }

        t = glm::clamp(t, 0.0f, 1.0f);
        std::vector<glm::vec2> tempPoints = controlPoints;
        int n = tempPoints.size();

        // store the initial control points
        steps.levels.push_back(tempPoints);

        // iterative algo -> store each iteration
        for (int k = 1; k < n; ++k)
        {
            std::vector<glm::vec2> currentLevel;
            for (int i = 0; i < n - k; ++i)
            {
                tempPoints[i] = (1.0f - t) * tempPoints[i] + t * tempPoints[i + 1];
                currentLevel.push_back(tempPoints[i]);
            }
            steps.levels.push_back(currentLevel);
        }

        steps.finalPoint = tempPoints[0];
        return steps;
    }
};

#endif