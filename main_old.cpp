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
#include "shader.h"
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

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void createParkScene(std::vector<SceneObject> &objects);

const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;

bool drawEdges = false;
bool lightingEnabled = true;

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
const float POINT_RADIUS = 0.5f;       // In world space
const float SPHERE_RADIUS = 0.3f;      // Visual sphere size
BezierCurve slideCurve;                // Global slide curve
glm::vec3 slidePos(-5.0f, 0.0f, 5.0f); // Global slide position
float platformHeight = 4.0f;
float slideWidth = 1.2f;
int slideObjectIndex = -1; // Track which object is the slide

GLFWwindow *g_window = nullptr;                     // Global window pointer
std::vector<SceneObject> *g_sceneObjects = nullptr; // Global scene objects pointer

glm::vec3 screenToWorld(double xpos, double ypos, const glm::mat4 &view, const glm::mat4 &projection)
{
    int width, height;
    glfwGetFramebufferSize(g_window, &width, &height); // Use framebuffer size, not constant

    // Convert to NDC (-1 to 1)
    float x = (2.0f * (float)xpos) / (float)width - 1.0f;
    float y = 1.0f - (2.0f * (float)ypos) / (float)height;

    // Raycast from camera
    glm::vec4 rayClip(x, y, -1.0f, 1.0f);
    glm::vec4 rayEye = glm::inverse(projection) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));

    // Project onto XY plane at slide position Z
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

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Replicated Scene Viewer", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    g_window = window; // Store global reference

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // capture mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Shader shader("shader.vert", "shader.frag");

    std::vector<SceneObject> sceneObjects;
    g_sceneObjects = &sceneObjects; // Store global reference
    createParkScene(sceneObjects);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.53f, 0.80f, 0.91f, 1.0f); // Sky blue background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        // Use dynamic window size for projection
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glm::mat4 projection = glm::perspective(glm::radians(camera.zoom), (float)width / (float)height, 0.1f, 100.0f);
        glm::mat4 view = camera.getViewMatrix();

        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setBool("lightingEnabled", lightingEnabled);
        shader.setFloat("ambientStrength", 0.3f); // Brighter for outdoor look
        shader.setFloat("lightStrength", 0.8f);
        shader.setFloat("shininess", 32.0f);
        shader.setVec3("lightPos", glm::vec3(50.0f, 100.0f, 50.0f)); // Sun position
        shader.setVec3("viewPos", camera.position);

        // Regenerate slide mesh if in edit mode and slide has enough points
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
                // Determine color based on selection
                glm::vec3 pointColor = (i == selectedPointIndex) ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

                // Create sphere mesh with appropriate color
                Mesh sphereMesh = createSphere(SPHERE_RADIUS, 16, 32, pointColor);

                // Position sphere at control point location
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

    // slide
    {
        // platform
        // Centered at height 4.0
        glm::mat4 platMat = glm::translate(glm::mat4(1.0f), slidePos + glm::vec3(0.0f, platformHeight, 0.0f));
        platMat = glm::scale(platMat, glm::vec3(2.0f, 0.2f, slideWidth + 0.2f));
        SceneObject plat{createCube(1.0f, COLOR_RED), platMat};
        objects.push_back(plat);

        // ladder
        // Calculate vector from ground to platform edge
        glm::vec3 ladderTop(-1.0f, platformHeight, 0.0f); // Edge of platform
        glm::vec3 ladderBase(-2.5f, 0.0f, 0.0f);          // Ground point
        glm::vec3 ladderVec = ladderTop - ladderBase;
        float ladderLen = glm::length(ladderVec);

        // Calculate angle from vertical Y-axis
        float ladderAngle = -glm::degrees(atan2(ladderVec.x, ladderVec.y));

        // ladder rails
        for (int s = -1; s <= 1; s += 2)
        {
            float z = s * (slideWidth * 0.35f);
            // Midpoint of the rail
            glm::vec3 mid = (ladderTop + ladderBase) * 0.5f;
            mid.z = z;
            glm::mat4 railMat = glm::translate(glm::mat4(1.0f), slidePos + mid);
            railMat = glm::rotate(railMat, glm::radians(ladderAngle), glm::vec3(0, 0, 1));
            SceneObject rail{createCylinder(0.1f, ladderLen, 12, COLOR_YELLOW), railMat};
            objects.push_back(rail);
        }

        // ladder rungs
        int numRungs = 6;
        for (int i = 1; i < numRungs; i++)
        {
            float t = (float)i / numRungs;
            // interpolate pos along the ladder vector
            glm::vec3 pos = ladderBase + ladderVec * t;
            glm::mat4 rungMat = glm::translate(glm::mat4(1.0f), slidePos + pos);
            rungMat = glm::rotate(rungMat, glm::radians(90.0f), glm::vec3(1, 0, 0)); // Rotate to lay flat in Z
            // Height of cylinder becomes the width of the rung
            SceneObject rung{createCylinder(0.08f, slideWidth * 0.8f, 8, COLOR_YELLOW), rungMat};
            objects.push_back(rung);
        }

        // slide using bezier curve
        slideCurve.controlPoints.clear();
        // P0: Start (Top)
        slideCurve.controlPoints.push_back(glm::vec2(0.5f, platformHeight));
        // P1: Control Point (Push out horizontally first)
        slideCurve.controlPoints.push_back(glm::vec2(2.5f, platformHeight));
        // P2: Control Point (Steep drop)
        slideCurve.controlPoints.push_back(glm::vec2(4.0f, 0.5f));
        // P3: End (Level out at bottom)
        slideCurve.controlPoints.push_back(glm::vec2(5.5f, 0.5f));

        // Generate the mesh (Width 1.2, Wall Height 0.3, 20 segments)
        Mesh bezierMesh = slideCurve.createSlideExtrusion(slideWidth, 0.3f, 20, COLOR_RED);
        glm::mat4 slideMat = glm::translate(glm::mat4(1.0f), slidePos);
        SceneObject slide{bezierMesh, slideMat};
        slideObjectIndex = objects.size(); // Remember the slide's index
        objects.push_back(slide);

        // vertical supports
        for (int s = -1; s <= 1; s += 2)
        {
            float z = s * (slideWidth * 0.35f);
            // Place legs under the side of the platform opposite the ladder
            glm::vec3 postPos(0.8f, platformHeight / 2.0f, z);
            glm::mat4 pMat = glm::translate(glm::mat4(1.0f), slidePos + postPos);
            SceneObject post{createCylinder(0.1f, platformHeight, 12, COLOR_YELLOW), pMat};
            objects.push_back(post);
        }
    }

    // swing
    {
        glm::vec3 swingPos(5.0f, 0.0f, -5.0f);
        float swingHeight = 5.0f;
        float groundSpreadZ = 3.5f; // Distance between front and back feet
        float frameWidthX = 3.5f;   // Distance between left and right frames

        glm::vec3 topPoint(0.0f, swingHeight, 0.0f);           // Top center
        glm::vec3 footFront(0.0f, 0.0f, groundSpreadZ / 2.0f); // Front foot (+Z)
        glm::vec3 footBack(0.0f, 0.0f, -groundSpreadZ / 2.0f); // Back foot (-Z)

        // We use the vector from Foot to Top to determine the lean
        glm::vec3 vecFront = topPoint - footFront;
        glm::vec3 vecBack = topPoint - footBack;
        float legLen = glm::length(vecFront);

        // Calculate rotation angle around X-axis to lean the legs
        // atan2(z, y) gives the angle of the vector in the YZ plane
        float angleFront = atan2(vecFront.z, vecFront.y);
        float angleBack = atan2(vecBack.z, vecBack.y);

        for (int side = -1; side <= 1; side += 2)
        {
            float x = side * frameWidthX / 2.0f;

            // -- Front Leg --
            // cylinder center is the midpoint between foot and top
            glm::vec3 midF = (footFront + topPoint) * 0.5f;
            glm::mat4 matF = glm::translate(glm::mat4(1.0f), swingPos + glm::vec3(x, midF.y, midF.z));
            // Rotate to match the vector angle
            matF = glm::rotate(matF, angleFront, glm::vec3(1, 0, 0));
            SceneObject legF{createCylinder(0.15f, legLen, 12, COLOR_BLUE), matF};
            objects.push_back(legF);

            // -- Back Leg --
            glm::vec3 midB = (footBack + topPoint) * 0.5f;
            glm::mat4 matB = glm::translate(glm::mat4(1.0f), swingPos + glm::vec3(x, midB.y, midB.z));
            matB = glm::rotate(matB, angleBack, glm::vec3(1, 0, 0));
            SceneObject legB{createCylinder(0.15f, legLen, 12, COLOR_BLUE), matB};
            objects.push_back(legB);
        }

        // Top Bar
        glm::mat4 barMat = glm::translate(glm::mat4(1.0f), swingPos + topPoint);
        barMat = glm::rotate(barMat, glm::radians(90.0f), glm::vec3(0, 0, 1));
        SceneObject topBar{createCylinder(0.15f, frameWidthX + 1.0f, 12, COLOR_BLUE), barMat};
        objects.push_back(topBar);

        // seats and chains
        for (int i = 0; i < 2; i++)
        {
            float seatX = (i == 0) ? -0.8f : 0.8f; // Position seats slightly apart
            float seatY = 1.0f;

            // seat
            glm::mat4 seatMat = glm::translate(glm::mat4(1.0f), swingPos + glm::vec3(seatX, seatY, 0.0f));
            seatMat = glm::scale(seatMat, glm::vec3(0.7f, 0.1f, 0.6f));
            SceneObject seat{createCube(1.0f, COLOR_RED), seatMat};
            objects.push_back(seat);

            // chains
            float chainLen = swingHeight - seatY;
            float chainMidY = seatY + (chainLen / 2.0f);

            // 2 chains per seat
            for (int c = -1; c <= 1; c += 2)
            {
                float chainX = seatX + (c * 0.3f);
                glm::mat4 chainMat = glm::translate(glm::mat4(1.0f), swingPos + glm::vec3(chainX, chainMidY, 0.0f));
                SceneObject chain{createCylinder(0.02f, chainLen, 6, glm::vec3(0.2f)), chainMat};
                objects.push_back(chain);
            }
        }
    }

    // merry go round
    {
        glm::vec3 mgrPos(-8.0f, 0.2f, -8.0f);
        int segments = 12;
        float radius = 3.0f;
        float thickness = 0.2f;
        float sectorAngle = 360.0f / segments;

        // central rod
        glm::mat4 hubMat = glm::translate(glm::mat4(1.0f), mgrPos + glm::vec3(0, 0.5f, 0));
        SceneObject hub{createCylinder(0.5f, 1.2f, 16, COLOR_YELLOW), hubMat};
        objects.push_back(hub);

        // disc
        for (int i = 0; i < segments; i++)
        {
            glm::vec3 color = (i % 2 == 0) ? COLOR_PINK : COLOR_DEEP_BLUE;
            glm::mat4 segMat = glm::translate(glm::mat4(1.0f), mgrPos);
            segMat = glm::rotate(segMat, glm::radians(i * sectorAngle), glm::vec3(0, 1, 0));
            SceneObject seg{createCylinderSector(radius, thickness, sectorAngle, 8, color), segMat};
            objects.push_back(seg);
        }

        // hand railing
        float ringRadius = radius * 0.8f; // Distance from center to tube center
        float ringHeight = 1.0f;          // Height above the base
        glm::mat4 railMat = glm::translate(glm::mat4(1.0f), mgrPos + glm::vec3(0.0f, ringHeight, 0.0f));
        SceneObject rail{createTorus(ringRadius, 0.05f, 32, 12, glm::vec3(0.7f)), railMat};
        objects.push_back(rail);

        // connecting rods
        // Connects the base (y=0 relative) to the ring (y=1.0 relative)
        for (int i = 0; i < segments; i += 2)
        { // Place a rod every 2 segments
            float angle = glm::radians((float)i * sectorAngle);
            // Calculate X/Z position on the ring's radius
            float x = sin(angle) * ringRadius;
            float z = cos(angle) * ringRadius;
            // Cylinder is centered at origin, so we move it up by half its height (0.5)
            // to span from 0.0 to 1.0
            glm::mat4 rodMat = glm::translate(glm::mat4(1.0f), mgrPos + glm::vec3(x, ringHeight / 2.0f, z));
            SceneObject rod{createCylinder(0.04f, ringHeight, 8, glm::vec3(0.7f)), rodMat};
            objects.push_back(rod);
        }
    }

    // bench
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

    // Edit mode toggle
    static bool eKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && !eKeyPressed)
    {
        editMode = !editMode;
        eKeyPressed = true;

        // Toggle cursor mode
        if (editMode)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            std::cout << "Edit Mode: ON - Left click to select/add, Right click to delete, E to exit" << std::endl;
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            isDragging = false;
            selectedPointIndex = -1;
            std::cout << "Edit Mode: OFF" << std::endl;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_RELEASE)
    {
        eKeyPressed = false;
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
        // In edit mode, handle dragging
        if (isDragging && selectedPointIndex != -1)
        {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            glm::mat4 projection = glm::perspective(glm::radians(camera.zoom),
                                                    (float)width / (float)height, 0.1f, 100.0f);
            glm::mat4 view = camera.getViewMatrix();
            glm::vec3 worldPos = screenToWorld(xpos, ypos, view, projection);

            // Update control point relative to slide position
            slideCurve.controlPoints[selectedPointIndex] = glm::vec2(
                worldPos.x - slidePos.x,
                worldPos.y - slidePos.y);
        }
        return;
    }

    // Original camera movement code
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
                // Add new point
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