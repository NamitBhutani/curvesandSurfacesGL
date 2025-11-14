#include <iostream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
};

// for full poly mesh
struct Mesh
{
    std::vector<Vertex> vertices;      // vertices to go into VBO
    std::vector<unsigned int> indices; // indices to go into EBO
};

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
            return controlPoints[0]; // ret the single point
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
        Mesh mesh;
        std::vector<glm::vec2> samples = sampleCurve(sampleStep);
        int numSamples = samples.size();

        if (numSamples < 2 || segments < 3)
        {
            return mesh;
        }

        // calc tangents for normals
        // need the tangent at each sample point to calc the 3D normal
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

        // gen vertices from sampled pointes
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

                glm::vec3 norm = glm::normalize(glm::vec3(ty * cosA, -tx, ty * sinA));

                mesh.vertices.push_back({pos, norm});
            }
        }

        // make quads (as 2 tris) to connect the rings
        for (int i = 0; i < numSamples - 1; ++i)
        {
            for (int j = 0; j < segments; ++j)
            {

                // indices of the 4 verts forming a quad
                unsigned int p0 = i * segments + j;
                unsigned int p1 = (i + 1) * segments + j;
                unsigned int p2 = (i + 1) * segments + ((j + 1) % segments);
                unsigned int p3 = i * segments + ((j + 1) % segments);

                // tri 1 (CCW )
                mesh.indices.push_back(p0);
                mesh.indices.push_back(p1);
                mesh.indices.push_back(p2);

                // tri 2 (CCW)
                mesh.indices.push_back(p0);
                mesh.indices.push_back(p2);
                mesh.indices.push_back(p3);
            }
        }

        return mesh;
    }
};
int main()
{
    return 0;
}