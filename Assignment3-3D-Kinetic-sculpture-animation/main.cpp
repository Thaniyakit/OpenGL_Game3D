// multiple_lights_sun_earth_moon.cpp
// Built from src/2.lighting/6.multiple_lights/multiple_lights.cpp
// Sun (point light) -> Earth (orbit + spin) -> Moon (orbit Earth + spin)
// Uses the same dependencies as the LearnOpenGL example.

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>

#include <iostream>
#include <vector>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int createSphereVAO(int sectorCount, int stackCount, unsigned int& vertexCount);
unsigned int loadTexture(const char* path);

// settings
const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(0.0f, 3.0f, 12.0f)); // pulled back so we see orbits
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
    // glfw: initialize
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Sun-Earth-Moon (multiple lights)", NULL, NULL);
    if (!window) { std::cout << "Failed to create GLFW window\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    // capture mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n"; return -1;
    }
    glEnable(GL_DEPTH_TEST);

    // shaders (these are the updated shader sources below)
    Shader lightingShader("6.multiple_lights.vs", "6.multiple_lights.fs");


    // cube data (positions, normals, texcoords)


    // VBO + VAOs

    // Load textures for Sun, Earth and Moon
    unsigned int sunDiffuse = loadTexture(FileSystem::getPath("resources/textures/sun.jpg").c_str());
    unsigned int sunSpecular = loadTexture(FileSystem::getPath("resources/textures/sun.jpg").c_str()); // Optional

    unsigned int earthDiffuse = loadTexture(FileSystem::getPath("resources/textures/earth.jpg").c_str());
    unsigned int earthSpecular = loadTexture(FileSystem::getPath("resources/textures/earth_specular.jpg").c_str()); // Optional

    unsigned int moonDiffuse = loadTexture(FileSystem::getPath("resources/textures/moon.jpg").c_str());
    unsigned int moonSpecular = loadTexture(FileSystem::getPath("resources/textures/moon.jpg").c_str()); // Optional

    // shader config
    lightingShader.use();
    lightingShader.setInt("material.diffuse", 0);
    lightingShader.setInt("material.specular", 1);
    lightingShader.setFloat("material.shininess", 32.0f);

    unsigned int sphereVAO;
    unsigned int sphereVertexCount;
    sphereVAO = createSphereVAO(64, 32, sphereVertexCount); // 64 sectors, 32 stacks for smooth sphere

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        // time
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // clear
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- lighting shader (applied to Sun, Earth, Moon objects)
        lightingShader.use();
        lightingShader.setVec3("viewPos", camera.Position);

        // Remove or set to zero if you want only the Sun
        lightingShader.setVec3("dirLight.ambient", 0.0f, 0.0f, 0.0f);
        lightingShader.setVec3("dirLight.diffuse", 0.0f, 0.0f, 0.0f);
        lightingShader.setVec3("dirLight.specular", 0.0f, 0.0f, 0.0f);

        // Only one point light: the Sun
        glm::vec3 sunPos = glm::vec3(0.0f, 0.0f, 0.0f);
        lightingShader.setVec3("pointLights[0].position", sunPos);
        lightingShader.setVec3("pointLights[0].ambient", 1.0f, 0.9f, 0.6f);   // brighter ambient
        lightingShader.setVec3("pointLights[0].diffuse", 2.0f, 1.8f, 1.2f);   // very bright
        lightingShader.setVec3("pointLights[0].specular", 3.0f, 2.7f, 1.8f);
        lightingShader.setFloat("pointLights[0].constant", 1.0f);
        lightingShader.setFloat("pointLights[0].linear", 0.022f);
        lightingShader.setFloat("pointLights[0].quadratic", 0.0019f);

        // spot light (flashlight from camera) - set to zero
        lightingShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
        lightingShader.setVec3("spotLight.diffuse", 0.0f, 0.0f, 0.0f);
        lightingShader.setVec3("spotLight.specular", 0.0f, 0.0f, 0.0f);

        // projection + view
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 200.0f);
        glm::mat4 view = camera.GetViewMatrix();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        // ---- Draw SUN
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, sunPos);
        model = glm::scale(model, glm::vec3(1.8f));
        lightingShader.setMat4("model", model);
        lightingShader.setFloat("overrideColor", 0.0f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sunDiffuse);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, sunSpecular);
        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES, sphereVertexCount, GL_UNSIGNED_INT, 0);

        // ---- Draw EARTH (orbits Sun)
        float earthOrbitRadius = 6.0f;
        float earthOrbitSpeed = 0.3f;
        glm::vec3 earthPos;
        earthPos.x = sunPos.x + earthOrbitRadius * sin(currentFrame * earthOrbitSpeed);
        earthPos.y = 0.0f;
        earthPos.z = sunPos.z + earthOrbitRadius * cos(currentFrame * earthOrbitSpeed);

        model = glm::mat4(1.0f);
        model = glm::translate(model, earthPos);
        model = glm::rotate(model, glm::radians(23.5f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, currentFrame * 1.5f, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.9f));
        lightingShader.setMat4("model", model);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, earthDiffuse);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, earthSpecular);
        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES, sphereVertexCount, GL_UNSIGNED_INT, 0);

        // ---- Draw MOON (orbits Earth)
        float moonOrbitRadius = 1.8f;
        float moonOrbitSpeed = 1.0f;
        glm::vec3 moonPos;
        moonPos.x = earthPos.x + moonOrbitRadius * sin(currentFrame * moonOrbitSpeed);
        moonPos.y = earthPos.y + 0.15f * sin(currentFrame * 1.2f);
        moonPos.z = earthPos.z + moonOrbitRadius * cos(currentFrame * moonOrbitSpeed);

        model = glm::mat4(1.0f);
        model = glm::translate(model, moonPos);
        model = glm::rotate(model, currentFrame * 3.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.35f));
        lightingShader.setMat4("model", model);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, moonDiffuse);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, moonSpecular);
        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES, sphereVertexCount, GL_UNSIGNED_INT, 0);



        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // cleanup


    glfwTerminate();
    return 0;
}

// Generates a sphere mesh (positions, normals, texcoords) and returns VAO, vertex count
unsigned int createSphereVAO(int sectorCount, int stackCount, unsigned int& vertexCount)
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    float radius = 0.5f;
    for (int i = 0; i <= stackCount; ++i)
    {
        float stackAngle = glm::pi<float>() / 2 - i * glm::pi<float>() / stackCount; // from pi/2 to -pi/2
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);

        for (int j = 0; j <= sectorCount; ++j)
        {
            float sectorAngle = j * 2 * glm::pi<float>() / sectorCount; // 0 to 2pi

            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);

            float nx = x / radius;
            float ny = y / radius;
            float nz = z / radius;
            float s = (float)j / sectorCount;
            float t = (float)i / stackCount;

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);
            vertices.push_back(s);
            vertices.push_back(t);
        }
    }

    for (int i = 0; i < stackCount; ++i)
    {
        int k1 = i * (sectorCount + 1);
        int k2 = k1 + sectorCount + 1;
        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2)
        {
            if (i != 0)
            {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }
            if (i != (stackCount - 1))
            {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }

    vertexCount = (unsigned int)indices.size();

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texcoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return VAO;
}

// input
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos; lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

unsigned int loadTexture(char const* path)
{
    unsigned int textureID; glGenTextures(1, &textureID);
    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, (GLint)format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    }
    else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}