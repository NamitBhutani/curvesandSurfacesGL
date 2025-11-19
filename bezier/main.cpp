#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include "bezier1.h"
#include "../shader.h"
#include "../mesh.h"

const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;
const float POINT_radius = 0.03f;

BezierCurve curve;
int selectedPointIndex = -1;
bool isDragging = false;

// anim variables
bool animateCasteljau = false;
float animationT = 0.0f;
float animationSpeed = 0.3f; // Speed of t parameter animation
bool animationReverse = false;

unsigned int VAO_Curve, VBO_Curve;
unsigned int VAO_Points, VBO_Points;
unsigned int VAO_Lines, VBO_Lines;

GLFWwindow *g_window = nullptr;

glm::vec2 screenToNDC(double xpos, double ypos)
{
    int width, height;
    glfwGetFramebufferSize(g_window, &width, &height);
    float x = (2.0f * (float)xpos) / (float)width - 1.0f;
    float y = 1.0f - (2.0f * (float)ypos) / (float)height;
    return glm::vec2(x, y);
}

int getPointIndexAt(glm::vec2 mousePos)
{
    for (size_t i = 0; i < curve.controlPoints.size(); ++i)
    {
        float dist = glm::distance(mousePos, curve.controlPoints[i]);
        if (dist < POINT_radius)
        {
            return (int)i;
        }
    }
    return -1;
}

void setupBuffers()
{
    // Curve buffer
    glGenVertexArrays(1, &VAO_Curve);
    glGenBuffers(1, &VBO_Curve);
    glBindVertexArray(VAO_Curve);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Curve);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * 1000, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void *)0);
    glEnableVertexAttribArray(0);

    // Points buffer
    glGenVertexArrays(1, &VAO_Points);
    glGenBuffers(1, &VBO_Points);
    glBindVertexArray(VAO_Points);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Points);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * 100, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void *)0);
    glEnableVertexAttribArray(0);

    // Lines buffer for construction lines
    glGenVertexArrays(1, &VAO_Lines);
    glGenBuffers(1, &VBO_Lines);
    glBindVertexArray(VAO_Lines);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Lines);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * 500, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void *)0);
    glEnableVertexAttribArray(0);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    glm::vec2 mouseNDC = screenToNDC(xpos, ypos);

    if (action == GLFW_PRESS)
    {
        int hitIndex = getPointIndexAt(mouseNDC);
        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            if (hitIndex != -1)
            {
                selectedPointIndex = hitIndex;
                isDragging = true;
            }
            else
            {
                curve.controlPoints.push_back(mouseNDC);
                std::cout << "Added Point. Total: " << curve.controlPoints.size() << std::endl;
            }
        }
        else if (button == GLFW_MOUSE_BUTTON_RIGHT)
        {
            if (hitIndex != -1)
            {
                curve.controlPoints.erase(curve.controlPoints.begin() + hitIndex);
                std::cout << "Deleted Point. Total: " << curve.controlPoints.size() << std::endl;
                selectedPointIndex = -1;
                isDragging = false;
            }
        }
    }
    else if (action == GLFW_RELEASE)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            isDragging = false;
            selectedPointIndex = -1;
        }
    }
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (isDragging && selectedPointIndex != -1)
    {
        glm::vec2 mouseNDC = screenToNDC(xpos, ypos);
        curve.controlPoints[selectedPointIndex] = mouseNDC;
    }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void drawDeCasteljauSteps(const BezierCurve::DeCasteljauSteps &steps, Shader &shader)
{
    if (steps.levels.empty())
        return;

    // colors for different levels
    std::vector<glm::vec3> colors = {
        glm::vec3(1.0f, 0.2f, 0.2f), // Red for control points
        glm::vec3(1.0f, 0.8f, 0.2f), // Orange
        glm::vec3(0.2f, 1.0f, 0.8f), // Cyan
        glm::vec3(0.6f, 0.2f, 1.0f), // Purple
        glm::vec3(0.2f, 1.0f, 0.2f), // Green
    };

    glBindVertexArray(VAO_Lines);

    // Draw construction lines and points for each level
    for (size_t level = 0; level < steps.levels.size(); ++level)
    {
        const auto &points = steps.levels[level];
        if (points.empty())
            continue;

        glm::vec3 color = colors[level % colors.size()];

        // draw lines connecting points in this level
        if (points.size() > 1)
        {
            glBindBuffer(GL_ARRAY_BUFFER, VBO_Lines);
            glBufferSubData(GL_ARRAY_BUFFER, 0, points.size() * sizeof(glm::vec2), &points[0]);

            float alpha = 1.0f - (level * 0.15f); // Fade later levels slightly
            shader.setVec3("uColor", color.r * alpha, color.g * alpha, color.b * alpha);
            glLineWidth(2.0f - level * 0.2f);
            glDrawArrays(GL_LINE_STRIP, 0, points.size());
        }

        glBindVertexArray(VAO_Points);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_Points);
        glBufferSubData(GL_ARRAY_BUFFER, 0, points.size() * sizeof(glm::vec2), &points[0]);

        glPointSize(12.0f - level * 1.5f);
        shader.setVec3("uColor", color);
        glDrawArrays(GL_POINTS, 0, points.size());
    }

    // Draw the final point on the curve with a distinctive color
    glBindVertexArray(VAO_Points);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Points);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2), &steps.finalPoint);

    glPointSize(15.0f);
    shader.setVec3("uColor", 1.0f, 1.0f, 0.0f); // Yellow for final point
    glDrawArrays(GL_POINTS, 0, 1);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Bezier Curve - De Casteljau Animation", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    g_window = window;
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    Shader shader("curve.vert", "curve.frag");
    setupBuffers();
    glEnable(GL_PROGRAM_POINT_SIZE);

    // init points
    curve.controlPoints.push_back(glm::vec2(-0.6f, -0.4f));
    curve.controlPoints.push_back(glm::vec2(-0.2f, 0.6f));
    curve.controlPoints.push_back(glm::vec2(0.3f, 0.5f));
    curve.controlPoints.push_back(glm::vec2(0.6f, -0.3f));

    std::cout << "Controls:" << std::endl;
    std::cout << "  Left Click - Add/Select point" << std::endl;
    std::cout << "  Right Click - Delete point" << std::endl;
    std::cout << "  SPACE - Toggle De Casteljau animation" << std::endl;
    std::cout << "  UP/DOWN - Adjust animation speed" << std::endl;

    float lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // Input
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Toggle animation
        static bool spacePressed = false;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressed)
        {
            animateCasteljau = !animateCasteljau;
            spacePressed = true;
            std::cout << "Animation: " << (animateCasteljau ? "ON" : "OFF") << std::endl;
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
        {
            spacePressed = false;
        }

        static bool sPressed = false;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && !sPressed)
        {
            sPressed = true;

            std::cout << "[INFO] Generating Surface of Revolution..." << std::endl;

            Mesh sorMesh = curve.createSurfaceOfRevolution(36, 0.02f);

            // save to OFF file
            if (sorMesh.vertices.size() > 0)
            {
                sorMesh.writeOFF("../../surface.off");
            }
            else
            {
                std::cout << "[WARN] Curve invalid or too short to generate surface." << std::endl;
            }
        }

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE)
        {
            sPressed = false;
        }

        // Adjust speed
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        {
            animationSpeed += 0.1f * deltaTime;
            std::cout << "Speed: " << animationSpeed << std::endl;
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        {
            animationSpeed = glm::max(0.05f, animationSpeed - 0.1f * deltaTime);
            std::cout << "Speed: " << animationSpeed << std::endl;
        }

        // Update animation
        if (animateCasteljau && curve.controlPoints.size() >= 2)
        {
            if (animationReverse)
            {
                animationT -= animationSpeed * deltaTime;
                if (animationT <= 0.0f)
                {
                    animationT = 0.0f;
                    animationReverse = false;
                }
            }
            else
            {
                animationT += animationSpeed * deltaTime;
                if (animationT >= 1.0f)
                {
                    animationT = 1.0f;
                    animationReverse = true;
                }
            }
        }

        glClearColor(0.15f, 0.15f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        shader.use();

        // Draw full curve
        std::vector<glm::vec2> curvePoints = curve.sampleCurve(0.01f);
        if (curvePoints.size() > 1)
        {
            glBindVertexArray(VAO_Curve);
            glBindBuffer(GL_ARRAY_BUFFER, VBO_Curve);
            glBufferSubData(GL_ARRAY_BUFFER, 0, curvePoints.size() * sizeof(glm::vec2), &curvePoints[0]);
            glLineWidth(1.5f);
            shader.setVec3("uColor", 0.3f, 0.6f, 0.3f);
            glDrawArrays(GL_LINE_STRIP, 0, curvePoints.size());
        }

        //  control polygon
        if (curve.controlPoints.size() > 0)
        {
            glBindVertexArray(VAO_Points);
            glBindBuffer(GL_ARRAY_BUFFER, VBO_Points);
            glBufferSubData(GL_ARRAY_BUFFER, 0, curve.controlPoints.size() * sizeof(glm::vec2), &curve.controlPoints[0]);

            float dimFactor = animateCasteljau ? 0.3f : 1.0f;
            shader.setVec3("uColor", 0.5f * dimFactor, 0.5f * dimFactor, 0.5f * dimFactor);
            glLineWidth(1.0f);
            glDrawArrays(GL_LINE_STRIP, 0, curve.controlPoints.size());

            glPointSize(8.0f);
            shader.setVec3("uColor", 0.8f * dimFactor, 0.2f * dimFactor, 0.2f * dimFactor);
            glDrawArrays(GL_POINTS, 0, curve.controlPoints.size());
        }

        // draw de Casteljau steps if animating
        if (animateCasteljau && curve.controlPoints.size() >= 2)
        {
            BezierCurve::DeCasteljauSteps steps = curve.evaluateWithSteps(animationT);
            drawDeCasteljauSteps(steps, shader);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO_Curve);
    glDeleteBuffers(1, &VBO_Curve);
    glDeleteVertexArrays(1, &VAO_Points);
    glDeleteBuffers(1, &VBO_Points);
    glDeleteVertexArrays(1, &VAO_Lines);
    glDeleteBuffers(1, &VBO_Lines);

    glfwTerminate();
    return 0;
}