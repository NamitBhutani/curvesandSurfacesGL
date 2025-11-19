#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

// Smoothly interpolates between two values
float lerp(float start, float end, float t)
{
    return start + t * (end - start);
}

// Animates swing back and forth
class SwingAnimator
{
public:
    SwingAnimator(float amp = 25.0f, float freq = 0.8f)
        : amplitude(amp), frequency(freq) {}

    // Returns current swing angle in degrees
    float getAngle(double time) const
    {
        return amplitude * sin(2.0f * 3.14 * frequency * time);
    }

    void setAmplitude(float amp) { amplitude = amp; }
    void setFrequency(float freq) { frequency = freq; }

private:
    float amplitude; // Max swing angle in degrees
    float frequency; // Swings per second
};

// Animates continuous rotation
class RotationAnimator
{
public:
    RotationAnimator(float speed = 15.0f)
        : rotationSpeed(speed) {}

    // Returns current rotation angle in degrees
    float getRotation(double time) const
    {
        return rotationSpeed * time;
    }

    void setSpeed(float speed) { rotationSpeed = speed; }

private:
    float rotationSpeed; // Degrees per second
};

// Animates smooth transitions between positions (for bezier editing)
class PositionAnimator
{
public:
    PositionAnimator() : animating(false), duration(0.4) {}

    // Start animation from current to target positions
    void startAnimation(const std::vector<glm::vec2> &start_poses,
                        const std::vector<glm::vec2> &end_poses)
    {
        start_positions = start_poses;
        end_positions = end_poses;
        startTime = glfwGetTime();
        animating = true;
    }

    // Update current positions based on animation progress
    void update(std::vector<glm::vec2> &current_positions)
    {
        if (!animating)
            return;

        double currentTime = glfwGetTime();
        double elapsedTime = currentTime - startTime;
        float progress = static_cast<float>(elapsedTime / duration);

        if (progress >= 1.0f)
        {
            progress = 1.0f;
            animating = false;
        }

        // Interpolate each position
        for (size_t i = 0; i < current_positions.size(); ++i)
        {
            current_positions[i].x = lerp(start_positions[i].x, end_positions[i].x, progress);
            current_positions[i].y = lerp(start_positions[i].y, end_positions[i].y, progress);
        }

        // Ensure final positions are exact
        if (!animating)
        {
            current_positions = end_positions;
        }
    }

    bool isAnimating() const { return animating; }
    void setDuration(double dur) { duration = dur; }

private:
    bool animating;
    double startTime;
    double duration;
    std::vector<glm::vec2> start_positions;
    std::vector<glm::vec2> end_positions;
};

// Bobbing animation (up and down)
class BobAnimator
{
public:
    BobAnimator(float amp = 0.2f, float freq = 0.5f)
        : amplitude(amp), frequency(freq) {}

    // Returns vertical offset
    float getOffset(double time) const
    {
        return amplitude * sin(2.0f * 3.14 * frequency * time);
    }

private:
    float amplitude; // Max vertical offset
    float frequency; // Bobs per second
};
