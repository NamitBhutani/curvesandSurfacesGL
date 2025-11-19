#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <mesh.h>
#include <shader.h>

#include <string>
#include <vector>
#include <iostream>

class Model
{
public:
    std::vector<Mesh> meshes;

    Model(const std::string &path, glm::vec3 color = glm::vec3(1.0f))
    {
        if (!loadOFF(path, color))
        {
            std::cerr << "[ERROR] (MODEL) Failed to load OFF: " << path << std::endl;
        }
    }

    bool loadOFF(const std::string &path, glm::vec3 color)
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        std::ifstream in(path);
        if (!in)
        {
            std::cerr << "[ERROR] (MODEL) Failed to open file: " << path << std::endl;
            return false;
        }

        std::string header;
        if (!(in >> header))
        {
            std::cerr << "[ERROR] (MODEL) Empty or invalid file: " << path << std::endl;
            return false;
        }

        if (header != "OFF")
        {
            if (header.rfind("OFF", 0) != 0)
            {
                std::cerr << "[ERROR] (MODEL) Not an OFF file (header='" << header << "'): " << path << std::endl;
                return false;
            }
        }

        size_t numVertices = 0, numFaces = 0, numEdges = 0;
        while (true)
        {
            if (!in.good())
            {
                std::cerr << "[ERROR] (MODEL) Unexpected EOF while reading counts" << std::endl;
                return false;
            }
            std::string line;
            std::getline(in, line);
            if (line.size() == 0)
                continue;
            size_t p = line.find_first_not_of(" \t\r\n");
            if (p == std::string::npos)
                continue;
            if (line[p] == '#')
                continue;
            std::istringstream ss(line);
            if (ss >> numVertices >> numFaces >> numEdges)
                break;
        }

        for (int i = 0; i < numVertices; i++)
        {
            std::string line;
            do
            {
                if (!std::getline(in, line))
                {
                    std::cerr << "[ERROR] (MODEL) Unexpected EOF while reading vertices" << std::endl;
                    return false;
                }
                size_t p = line.find_first_not_of(" \t\r\n");
                if (p == std::string::npos)
                    continue;
                if (line[p] == '#')
                    continue;
                break;
            } while (true);

            std::istringstream ss(line);
            float x, y, z;
            if (!(ss >> x >> y >> z))
            {
                std::cerr << "[ERROR] (MODEL) Malformed vertex on line " << i + 1 << std::endl;
                return false;
            }

            Vertex v;
            // rotate model from Z-up to Y-up
            v.position = glm::vec3(x, y, z);
            v.color = color;

            float r, g, b, a;
            if (ss >> r >> g >> b)
            {
                if (r > 1.0f || g > 1.0f || b > 1.0f)
                {
                    r /= 255.0f;
                    g /= 255.0f;
                    b /= 255.0f;
                }
                v.color = glm::vec3(r, g, b);
            }

            vertices.push_back(v);
        }

        for (int i = 0; i < numFaces; i++)
        {
            std::string line;
            do
            {
                if (!std::getline(in, line))
                {
                    std::cerr << "[ERROR] (MODEL) Unexpected EOF while reading faces" << std::endl;
                    return false;
                }
                size_t p = line.find_first_not_of(" \t\r\n");
                if (p == std::string::npos)
                    continue;
                if (line[p] == '#')
                    continue;
                break;
            } while (true);

            std::istringstream ss(line);
            int vn;
            if (!(ss >> vn))
            {
                std::cerr << "[ERROR] (MODEL) Malformed face on line " << i + 1 << std::endl;
                return false;
            }
            if (vn < 3)
            {
                std::cerr << "[WARN] (MODEL) Face with less than 3 vertices; skipping" << std::endl;
                for (int k = 0; k < vn; ++k)
                {
                    int tmp;
                    ss >> tmp;
                }
                continue;
            }
            std::vector<int> faceIdx(vn);
            for (int k = 0; k < vn; ++k)
            {
                if (!(ss >> faceIdx[k]))
                {
                    std::cerr << "[ERROR] (MODEL) Malformed face indices" << std::endl;
                    return false;
                }
            }

            glm::vec3 normal = glm::normalize(glm::cross(vertices[faceIdx[2]].position - vertices[faceIdx[0]].position, vertices[faceIdx[1]].position - vertices[faceIdx[0]].position));

            glm::vec3 faceColor = color;
            float r, g, b;
            if (ss >> r >> g >> b)
            {
                if (r > 1.0f || g > 1.0f || b > 1.0f)
                {
                    r /= 255.0f;
                    g /= 255.0f;
                    b /= 255.0f;
                }
                faceColor = glm::vec3(r, g, b);
            }

            // vertex duplication for face coloring
            unsigned int baseVertexIndex = static_cast<unsigned int>(vertices.size());
            for (int k = 0; k < vn; ++k)
            {
                Vertex v = vertices[faceIdx[k]];
                v.color = faceColor;
                v.normal = normal;
                vertices.push_back(v);
            }

            // triangulate with new vertex indices
            for (int k = 1; k < vn - 1; ++k)
            {
                indices.push_back(baseVertexIndex);
                indices.push_back(baseVertexIndex + k);
                indices.push_back(baseVertexIndex + k + 1);
            }
        }

        meshes.clear();
        meshes.emplace_back(vertices, indices);
        return true;
    }

    void draw(Shader &shader, bool drawEdges = false)
    {
        for (Mesh &m : meshes)
        {
            m.draw(shader);
            if (drawEdges)
                m.drawEdges(shader);
        }
    }
};

#endif