#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include "bezier/bezier1.h"
#include "camera.h"
#include "mesh.h"
#include "model.h"
#include "shader.h"
#include "animator.h"
#include "primitives.h"

const glm::vec3 COLOR_RED = glm::vec3(204.0f / 255.0f, 50.0f / 255.0f, 6.0f / 255.0f);
const glm::vec3 COLOR_YELLOW = glm::vec3(204.0f / 255.0f, 190.0f / 255.0f, 2.0f / 255.0f);
const glm::vec3 COLOR_BLUE = glm::vec3(4.0f / 255.0f, 57.0f / 255.0f, 204.0f / 255.0f);
const glm::vec3 COLOR_PINK = glm::vec3(204.0f / 255.0f, 58.0f / 255.0f, 136.0f / 255.0f);
const glm::vec3 COLOR_DEEP_BLUE = glm::vec3(1.0f / 255.0f, 29.0f / 255.0f, 204.0f / 255.0f);
const glm::vec3 COLOR_GROUND = glm::vec3(0.2f, 0.6f, 0.2f);
const glm::vec3 COLOR_WOOD = glm::vec3(0.6f, 0.4f, 0.2f);

struct SceneObject
{
    Mesh mesh;
    glm::mat4 transform;
};

// Animation data structures
struct SwingData
{
    int seatIndex;
    std::vector<int> chainIndices;
    glm::vec3 pivotPoint;
    glm::vec3 seatBasePos;
    float seatX;
    SwingAnimator animator;
};

struct MerryGoRoundData
{
    std::vector<int> segmentIndices;
    int railIndex;
    std::vector<int> rodIndices;
    glm::vec3 centerPos;
    RotationAnimator animator;
};

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void createParkScene(std::vector<SceneObject> &objects);
void updateAnimations(std::vector<SceneObject> &objects, float time);

const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;

bool drawEdges = false;
bool lightingEnabled = true;
bool animationsEnabled = true;

// camera
Camera camera(glm::vec3(0.0f, 15.0f, 30.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Bezier editing mode
bool editMode = false;
int selectedPointIndex = -1;
bool isDragging = false;
const float POINT_RADIUS = 0.5f;
const float SPHERE_RADIUS = 0.3f;
BezierCurve slideCurve;
glm::vec3 slidePos(-5.0f, 0.0f, 5.0f);
float platformHeight = 4.0f;
float slideWidth = 1.2f;
int slideObjectIndex = -1;

GLFWwindow *g_window = nullptr;
std::vector<SceneObject> *g_sceneObjects = nullptr;

// Animation data
std::vector<SwingData> swings;
MerryGoRoundData merryGoRound;
PositionAnimator bezierAnimator; // For smooth bezier editing

glm::vec3 screenToWorld(double xpos, double ypos, const glm::mat4 &view, const glm::mat4 &projection)
{
    int width, height;
    glfwGetFramebufferSize(g_window, &width, &height);

    float x = (2.0f * (float)xpos) / (float)width - 1.0f;
    float y = 1.0f - (2.0f * (float)ypos) / (float)height;

    glm::vec4 rayClip(x, y, -1.0f, 1.0f);
    glm::vec4 rayEye = glm::inverse(projection) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));

    float t = (slidePos.z - camera.position.z) / rayWorld.z;
    glm::vec3 worldPos = camera.position + t * rayWorld;

    return worldPos;
}

int getSlidePointIndexAt(glm::vec3 worldPos)
{
    for (size_t i = 0; i < slideCurve.controlPoints.size(); ++i)
    {
        glm::vec3 pointWorld = slidePos + glm::vec3(slideCurve.controlPoints[i].x, slideCurve.controlPoints[i].y, 0.0f);
        float dist = glm::distance(glm::vec2(worldPos.x, worldPos.y), glm::vec2(pointWorld.x, pointWorld.y));
        if (dist < POINT_RADIUS)
        {
            return (int)i;
        }
    }
    return -1;
}

void updateAnimations(std::vector<SceneObject> &objects, float time)
{
    if (!animationsEnabled)
        return;

    // Animate swings using SwingAnimator
    for (auto &swing : swings)
    {
        float angle = swing.animator.getAngle(time);

        // Calculate seat position after rotation
        glm::vec3 seatLocalPos = swing.seatBasePos - swing.pivotPoint; // Position relative to pivot

        // Update seat with proper rotation
        glm::mat4 seatTransform = glm::translate(glm::mat4(1.0f), swing.pivotPoint);
        seatTransform = glm::rotate(seatTransform, glm::radians(angle), glm::vec3(1, 0, 0));
        seatTransform = glm::translate(seatTransform, seatLocalPos);
        seatTransform = glm::scale(seatTransform, glm::vec3(0.7f, 0.1f, 0.6f));
        objects[swing.seatIndex].transform = seatTransform;

        // Update chains
        float chainLen = swing.pivotPoint.y - swing.seatBasePos.y;

        for (int i = 0; i < swing.chainIndices.size(); i++)
        {
            int c = (i == 0) ? -1 : 1;
            float chainX = swing.seatX + (c * 0.3f);

            // Chain local position relative to pivot
            glm::vec3 chainLocalPos(chainX, -chainLen / 2.0f, 0.0f);

            glm::mat4 chainTransform = glm::translate(glm::mat4(1.0f), swing.pivotPoint);
            chainTransform = glm::rotate(chainTransform, glm::radians(angle), glm::vec3(1, 0, 0));
            chainTransform = glm::translate(chainTransform, chainLocalPos);
            objects[swing.chainIndices[i]].transform = chainTransform;
        }
    }

    // Animate merry-go-round using RotationAnimator
    float rotation = merryGoRound.animator.getRotation(time);

    int segments = merryGoRound.segmentIndices.size();
    float sectorAngle = 360.0f / segments;
    for (int i = 0; i < segments; i++)
    {
        glm::mat4 segMat = glm::translate(glm::mat4(1.0f), merryGoRound.centerPos);
        segMat = glm::rotate(segMat, glm::radians(rotation + i * sectorAngle), glm::vec3(0, 1, 0));
        objects[merryGoRound.segmentIndices[i]].transform = segMat;
    }

    float ringRadius = 3.0f * 0.8f;
    float ringHeight = 1.0f;
    glm::mat4 railMat = glm::translate(glm::mat4(1.0f), merryGoRound.centerPos + glm::vec3(0.0f, ringHeight, 0.0f));
    railMat = glm::rotate(railMat, glm::radians(rotation), glm::vec3(0, 1, 0));
    objects[merryGoRound.railIndex].transform = railMat;

    for (int i = 0; i < merryGoRound.rodIndices.size(); i++)
    {
        float angle = glm::radians(rotation + (float)(i * 2) * sectorAngle);
        float x = sin(angle) * ringRadius;
        float z = cos(angle) * ringRadius;

        glm::mat4 rodMat = glm::translate(glm::mat4(1.0f), merryGoRound.centerPos + glm::vec3(x, ringHeight / 2.0f, z));
        objects[merryGoRound.rodIndices[i]].transform = rodMat;
    }
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Animated Playground", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    g_window = window;

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Shader shader("shader.vert", "shader.frag");

    std::vector<SceneObject> sceneObjects;
    g_sceneObjects = &sceneObjects;
    createParkScene(sceneObjects);

    std::cout << "Controls:" << std::endl;
    std::cout << "  TAB - Toggle edit mode" << std::endl;
    std::cout << "  P - Toggle animations" << std::endl;
    std::cout << "  L - Toggle lighting" << std::endl;
    std::cout << "  B - Toggle edges" << std::endl;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Update bezier animation if active
        if (bezierAnimator.isAnimating())
        {
            bezierAnimator.update(slideCurve.controlPoints);
        }

        // Update playground animations
        updateAnimations(sceneObjects, currentFrame);

        glClearColor(0.53f, 0.80f, 0.91f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glm::mat4 projection = glm::perspective(glm::radians(camera.zoom), (float)width / (float)height, 0.1f, 100.0f);
        glm::mat4 view = camera.getViewMatrix();

        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setBool("lightingEnabled", lightingEnabled);
        shader.setFloat("ambientStrength", 0.3f);
        shader.setFloat("lightStrength", 0.8f);
        shader.setFloat("shininess", 32.0f);
        shader.setVec3("lightPos", glm::vec3(50.0f, 100.0f, 50.0f));
        shader.setVec3("viewPos", camera.position);

        // Regenerate slide mesh if in edit mode
        if (editMode && slideObjectIndex >= 0 && slideCurve.controlPoints.size() >= 2)
        {
            Mesh newSlideMesh = slideCurve.createSlideExtrusion(slideWidth, 0.3f, 20, COLOR_RED);
            sceneObjects[slideObjectIndex].mesh = newSlideMesh;
        }

        // Render scene objects
        for (auto &obj : sceneObjects)
        {
            shader.setMat4("model", obj.transform);
            obj.mesh.draw(shader);
            if (drawEdges)
                obj.mesh.drawEdges(shader);
        }

        // Render control point spheres in edit mode
        if (editMode)
        {
            for (size_t i = 0; i < slideCurve.controlPoints.size(); ++i)
            {
                glm::vec3 pointColor = (i == selectedPointIndex) ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
                Mesh sphereMesh = createSphere(SPHERE_RADIUS, 16, 32, pointColor);
                glm::vec3 pointWorld = slidePos + glm::vec3(slideCurve.controlPoints[i].x, slideCurve.controlPoints[i].y, 0.0f);
                glm::mat4 sphereTransform = glm::translate(glm::mat4(1.0f), pointWorld);
                shader.setMat4("model", sphereTransform);
                sphereMesh.draw(shader);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void createParkScene(std::vector<SceneObject> &objects)
{
    // ground plane
    {
        SceneObject ground{createPlane(60.0f, 60.0f, COLOR_GROUND), glm::mat4(1.0f)};
        objects.push_back(ground);
    }

    // slide (static objects)
    {
        glm::mat4 platMat = glm::translate(glm::mat4(1.0f), slidePos + glm::vec3(0.0f, platformHeight, 0.0f));
        platMat = glm::scale(platMat, glm::vec3(2.0f, 0.2f, slideWidth + 0.2f));
        SceneObject plat{createCube(1.0f, COLOR_RED), platMat};
        objects.push_back(plat);

        glm::vec3 ladderTop(-1.0f, platformHeight, 0.0f);
        glm::vec3 ladderBase(-2.5f, 0.0f, 0.0f);
        glm::vec3 ladderVec = ladderTop - ladderBase;
        float ladderLen = glm::length(ladderVec);
        float ladderAngle = -glm::degrees(atan2(ladderVec.x, ladderVec.y));

        for (int s = -1; s <= 1; s += 2)
        {
            float z = s * (slideWidth * 0.35f);
            glm::vec3 mid = (ladderTop + ladderBase) * 0.5f;
            mid.z = z;
            glm::mat4 railMat = glm::translate(glm::mat4(1.0f), slidePos + mid);
            railMat = glm::rotate(railMat, glm::radians(ladderAngle), glm::vec3(0, 0, 1));
            SceneObject rail{createCylinder(0.1f, ladderLen, 12, COLOR_YELLOW), railMat};
            objects.push_back(rail);
        }

        int numRungs = 6;
        for (int i = 1; i < numRungs; i++)
        {
            float t = (float)i / numRungs;
            glm::vec3 pos = ladderBase + ladderVec * t;
            glm::mat4 rungMat = glm::translate(glm::mat4(1.0f), slidePos + pos);
            rungMat = glm::rotate(rungMat, glm::radians(90.0f), glm::vec3(1, 0, 0));
            SceneObject rung{createCylinder(0.08f, slideWidth * 0.8f, 8, COLOR_YELLOW), rungMat};
            objects.push_back(rung);
        }

        slideCurve.controlPoints.clear();
        slideCurve.controlPoints.push_back(glm::vec2(0.5f, platformHeight));
        slideCurve.controlPoints.push_back(glm::vec2(2.5f, platformHeight));
        slideCurve.controlPoints.push_back(glm::vec2(4.0f, 0.5f));
        slideCurve.controlPoints.push_back(glm::vec2(5.5f, 0.5f));

        Mesh bezierMesh = slideCurve.createSlideExtrusion(slideWidth, 0.3f, 20, COLOR_RED);
        glm::mat4 slideMat = glm::translate(glm::mat4(1.0f), slidePos);
        SceneObject slide{bezierMesh, slideMat};
        slideObjectIndex = objects.size();
        objects.push_back(slide);

        for (int s = -1; s <= 1; s += 2)
        {
            float z = s * (slideWidth * 0.35f);
            glm::vec3 postPos(0.8f, platformHeight / 2.0f, z);
            glm::mat4 pMat = glm::translate(glm::mat4(1.0f), slidePos + postPos);
            SceneObject post{createCylinder(0.1f, platformHeight, 12, COLOR_YELLOW), pMat};
            objects.push_back(post);
        }
    }

    // swing (animated)
    {
        glm::vec3 swingPos(5.0f, 0.0f, -5.0f);
        float swingHeight = 5.0f;
        float groundSpreadZ = 3.5f;
        float frameWidthX = 3.5f;

        glm::vec3 topPoint(0.0f, swingHeight, 0.0f);
        glm::vec3 footFront(0.0f, 0.0f, groundSpreadZ / 2.0f);
        glm::vec3 footBack(0.0f, 0.0f, -groundSpreadZ / 2.0f);

        glm::vec3 vecFront = topPoint - footFront;
        glm::vec3 vecBack = topPoint - footBack;
        float legLen = glm::length(vecFront);

        float angleFront = atan2(vecFront.z, vecFront.y);
        float angleBack = atan2(vecBack.z, vecBack.y);

        // Frame legs (static)
        for (int side = -1; side <= 1; side += 2)
        {
            float x = side * frameWidthX / 2.0f;

            glm::vec3 midF = (footFront + topPoint) * 0.5f;
            glm::mat4 matF = glm::translate(glm::mat4(1.0f), swingPos + glm::vec3(x, midF.y, midF.z));
            matF = glm::rotate(matF, angleFront, glm::vec3(1, 0, 0));
            SceneObject legF{createCylinder(0.15f, legLen, 12, COLOR_BLUE), matF};
            objects.push_back(legF);

            glm::vec3 midB = (footBack + topPoint) * 0.5f;
            glm::mat4 matB = glm::translate(glm::mat4(1.0f), swingPos + glm::vec3(x, midB.y, midB.z));
            matB = glm::rotate(matB, angleBack, glm::vec3(1, 0, 0));
            SceneObject legB{createCylinder(0.15f, legLen, 12, COLOR_BLUE), matB};
            objects.push_back(legB);
        }

        // Top bar (static)
        glm::mat4 barMat = glm::translate(glm::mat4(1.0f), swingPos + topPoint);
        barMat = glm::rotate(barMat, glm::radians(90.0f), glm::vec3(0, 0, 1));
        SceneObject topBar{createCylinder(0.15f, frameWidthX + 1.0f, 12, COLOR_BLUE), barMat};
        objects.push_back(topBar);

        // Seats and chains (animated)
        for (int i = 0; i < 2; i++)
        {
            float seatX = (i == 0) ? -0.8f : 0.8f;
            float seatY = 1.0f;

            SwingData swing;
            swing.seatX = seatX;
            swing.pivotPoint = swingPos + topPoint;
            swing.seatBasePos = swingPos + glm::vec3(seatX, seatY, 0.0f);
            swing.animator = SwingAnimator(25.0f, 0.8f + i * 0.1f); // Slightly different frequencies

            // Seat
            glm::mat4 seatMat = glm::translate(glm::mat4(1.0f), swingPos + glm::vec3(seatX, seatY, 0.0f));
            seatMat = glm::scale(seatMat, glm::vec3(0.7f, 0.1f, 0.6f));
            SceneObject seat{createCube(1.0f, COLOR_RED), seatMat};
            swing.seatIndex = objects.size();
            objects.push_back(seat);

            // Chains
            float chainLen = swingHeight - seatY;
            for (int c = -1; c <= 1; c += 2)
            {
                float chainX = seatX + (c * 0.3f);
                float chainMidY = seatY + (chainLen / 2.0f);
                glm::mat4 chainMat = glm::translate(glm::mat4(1.0f), swingPos + glm::vec3(chainX, chainMidY, 0.0f));
                SceneObject chain{createCylinder(0.02f, chainLen, 6, glm::vec3(0.2f)), chainMat};
                swing.chainIndices.push_back(objects.size());
                objects.push_back(chain);
            }

            swings.push_back(swing);
        }
    }

    // merry go round (animated)
    {
        glm::vec3 mgrPos(-8.0f, 0.2f, -8.0f);
        int segments = 12;
        float radius = 3.0f;
        float thickness = 0.2f;
        float sectorAngle = 360.0f / segments;

        merryGoRound.centerPos = mgrPos;
        merryGoRound.animator = RotationAnimator(15.0f);

        // Central hub (static)
        glm::mat4 hubMat = glm::translate(glm::mat4(1.0f), mgrPos + glm::vec3(0, 0.5f, 0));
        SceneObject hub{createCylinder(0.5f, 1.2f, 16, COLOR_YELLOW), hubMat};
        objects.push_back(hub);

        // Disc segments (animated)
        for (int i = 0; i < segments; i++)
        {
            glm::vec3 color = (i % 2 == 0) ? COLOR_PINK : COLOR_DEEP_BLUE;
            glm::mat4 segMat = glm::translate(glm::mat4(1.0f), mgrPos);
            segMat = glm::rotate(segMat, glm::radians(i * sectorAngle), glm::vec3(0, 1, 0));
            SceneObject seg{createCylinderSector(radius, thickness, sectorAngle, 8, color), segMat};
            merryGoRound.segmentIndices.push_back(objects.size());
            objects.push_back(seg);
        }

        // Hand railing (animated)
        float ringRadius = radius * 0.8f;
        float ringHeight = 1.0f;
        glm::mat4 railMat = glm::translate(glm::mat4(1.0f), mgrPos + glm::vec3(0.0f, ringHeight, 0.0f));
        SceneObject rail{createTorus(ringRadius, 0.05f, 32, 12, glm::vec3(0.7f)), railMat};
        merryGoRound.railIndex = objects.size();
        objects.push_back(rail);

        // Connecting rods (animated)
        for (int i = 0; i < segments; i += 2)
        {
            float angle = glm::radians((float)i * sectorAngle);
            float x = sin(angle) * ringRadius;
            float z = cos(angle) * ringRadius;
            glm::mat4 rodMat = glm::translate(glm::mat4(1.0f), mgrPos + glm::vec3(x, ringHeight / 2.0f, z));
            SceneObject rod{createCylinder(0.04f, ringHeight, 8, glm::vec3(0.7f)), rodMat};
            merryGoRound.rodIndices.push_back(objects.size());
            objects.push_back(rod);
        }
    }

    // bench (static)
    {
        glm::vec3 benchPos(0.0f, 0.5f, 10.0f);
        glm::mat4 seatMat = glm::translate(glm::mat4(1.0f), benchPos);
        seatMat = glm::scale(seatMat, glm::vec3(4.0f, 0.2f, 1.2f));
        SceneObject seat{createCube(1.0f, COLOR_WOOD), seatMat};
        objects.push_back(seat);

        for (int i = 0; i < 2; i++)
        {
            float x = (i == 0) ? -1.5f : 1.5f;
            glm::mat4 legMat = glm::translate(glm::mat4(1.0f), benchPos + glm::vec3(x, -0.25f, 0.0f));
            legMat = glm::scale(legMat, glm::vec3(0.3f, 0.5f, 1.0f));
            SceneObject leg{createCube(1.0f, glm::vec3(0.2f)), legMat};
            objects.push_back(leg);
        }
    }
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.processKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camera.processKeyboard(DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.processRoll(1.0f * deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.processRoll(-1.0f * deltaTime);
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        camera.resetRoll();

    static bool bKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !bKeyPressed)
    {
        drawEdges = !drawEdges;
        bKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
    {
        bKeyPressed = false;
    }
    static bool oKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS && !oKeyPressed)
    {
        oKeyPressed = true;

        std::cout << "[INFO] Attempting to load model..." << std::endl;

        Model loadedModel("../surface.off", COLOR_RED);

        if (!loadedModel.meshes.empty())
        {
            SceneObject newObj;

            newObj.mesh = loadedModel.meshes[0];

            newObj.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, 0.0f));

            newObj.transform = glm::rotate(newObj.transform, glm::radians(-90.0f), glm::vec3(1, 0, 0));

            if (g_sceneObjects)
                g_sceneObjects->push_back(newObj);
            std::cout << "[SUCCESS] Added model to scene." << std::endl;
        }
        else
        {
            std::cout << "[ERROR] Model failed to load or contained no meshes." << std::endl;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_RELEASE)
    {
        oKeyPressed = false;
    }
    static bool lKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && !lKeyPressed)
    {
        lightingEnabled = !lightingEnabled;
        lKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_RELEASE)
    {
        lKeyPressed = false;
    }

    // Animation toggle
    static bool aKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !aKeyPressed)
    {
        animationsEnabled = !animationsEnabled;
        aKeyPressed = true;
        std::cout << "Animations: " << (animationsEnabled ? "ON" : "OFF") << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE)
    {
        aKeyPressed = false;
    }

    // Edit mode toggle
    static bool tabKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS && !tabKeyPressed)
    {
        editMode = !editMode;
        tabKeyPressed = true;
        if (editMode)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            std::cout << "Edit Mode: ON - Left click to select/add, Right click to delete" << std::endl;
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            isDragging = false;
            selectedPointIndex = -1;
            std::cout << "Edit Mode: OFF" << std::endl;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_RELEASE)
    {
        tabKeyPressed = false;
    }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (editMode)
    {
        if (isDragging && selectedPointIndex != -1)
        {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            glm::mat4 projection = glm::perspective(glm::radians(camera.zoom),
                                                    (float)width / (float)height, 0.1f, 100.0f);
            glm::mat4 view = camera.getViewMatrix();
            glm::vec3 worldPos = screenToWorld(xpos, ypos, view, projection);

            slideCurve.controlPoints[selectedPointIndex] = glm::vec2(
                worldPos.x - slidePos.x,
                worldPos.y - slidePos.y);
        }
        return;
    }

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    camera.processMouse(xoffset, yoffset);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (!editMode)
        return;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glm::mat4 projection = glm::perspective(glm::radians(camera.zoom),
                                            (float)width / (float)height, 0.1f, 100.0f);
    glm::mat4 view = camera.getViewMatrix();
    glm::vec3 worldPos = screenToWorld(xpos, ypos, view, projection);

    if (action == GLFW_PRESS)
    {
        int hitIndex = getSlidePointIndexAt(worldPos);

        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            if (hitIndex != -1)
            {
                selectedPointIndex = hitIndex;
                isDragging = true;
                std::cout << "Selected point " << hitIndex << std::endl;
            }
            else
            {
                glm::vec2 localPos(worldPos.x - slidePos.x, worldPos.y - slidePos.y);
                slideCurve.controlPoints.push_back(localPos);
                std::cout << "Added Point. Total: " << slideCurve.controlPoints.size() << std::endl;
            }
        }
        else if (button == GLFW_MOUSE_BUTTON_RIGHT)
        {
            if (hitIndex != -1 && slideCurve.controlPoints.size() > 2)
            {
                slideCurve.controlPoints.erase(slideCurve.controlPoints.begin() + hitIndex);
                std::cout << "Deleted Point. Total: " << slideCurve.controlPoints.size() << std::endl;
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

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    camera.processScroll(static_cast<float>(yoffset));
}
