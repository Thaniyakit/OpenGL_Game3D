#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <set>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void updateCars(float deltaTime);
void checkCollisions();
void resetGame();
bool checkGameOver();
void spawnNewRow();
void renderCube(); // Function to render a simple cube for road markings
bool canMoveTo(glm::vec3 newPosition); // Function to check tree collisions

// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// Game constants
const float MOVE_DISTANCE = 2.0f;
const float CAR_SPEED = 10.0f;
const int GRID_WIDTH = 11;
const int VISIBLE_ROWS = 15;

// camera - positioned for crossy road perspective
Camera camera(glm::vec3(0.0f, 8.0f, 8.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Game state
struct Car {
    glm::vec3 position;
    float speed;
    int lane;
    bool movingRight;
    int rowIndex; // Track which row this car belongs to
};

struct Tree {
    glm::vec3 position;
};

// Game variables
glm::vec3 playerPosition(0.0f, 0.0f, -10.0f);
float playerRotation = 0.0f; // Duck rotation angle - start facing forward (0 degrees)
std::vector<Car> cars;
std::vector<Tree> trees;
std::vector<int> roadRows; // stores which rows are roads (1) vs grass/safe (0)
int playerScore = 0;
bool gameOver = false;
float gameSpeed = 1.0f;
int furthestRow = 0;
std::default_random_engine generator;
std::uniform_real_distribution<float> distribution(0.0f, 1.0f);

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D Crossy Road", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse - DISABLED for debugging
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader("1.model_loading.vs", "1.model_loading.fs");

    // load models
    // -----------
    Model duckModel(FileSystem::getPath("resources/objects/mallard-crossy-road/source/Mallard_crossy_road.obj"));
    Model carModel(FileSystem::getPath("resources/objects/pixel-car-city/source/model.obj"));
    Model treeModel(FileSystem::getPath("resources/objects/elm-tree-low-poly/source/tree-elm-low-poly.obj"));
    Model roadModel(FileSystem::getPath("resources/objects/road/road.obj")); // Load road model

    // Initialize game
    resetGame();

    // Set up initial camera position with smooth following setup
    camera.Position = glm::vec3(playerPosition.x, playerPosition.y + 10.0f, playerPosition.z - 6.0f);
    camera.Yaw = 90.0f;   // Face forward
    camera.Pitch = -30.0f; // Look down at the duck
    camera.ProcessMouseMovement(0, 0);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // Update game state
        if (!gameOver) {
            updateCars(deltaTime);
            checkCollisions();
            
            // Check if player moved forward
            int currentRow = (int)((playerPosition.z + 10.0f) / MOVE_DISTANCE);
            if (currentRow > furthestRow) {
                furthestRow = currentRow;
                playerScore = furthestRow;
                gameSpeed += 0.01f; // Gradually increase difficulty
            }
            
            // Continuously spawn new rows to create endless gameplay
            while (currentRow + VISIBLE_ROWS >= roadRows.size()) {
                spawnNewRow();
            }
        }

        // Update camera to follow the duck - smoother approach
        // Target camera position behind and above the duck
        glm::vec3 targetCameraPos = glm::vec3(
            playerPosition.x,
            playerPosition.y + 10.0f, // 10 units above
            playerPosition.z - 6.0f   // 6 units behind
        );
        // Prevent the camera from moving backwards (only allow non-decreasing Z)
        if (targetCameraPos.z < camera.Position.z) {
            targetCameraPos.z = camera.Position.z;
        }
        
        // Smooth camera following with interpolation
        float cameraFollowSpeed = 2.5f * deltaTime; // Smoother follow speed
        camera.Position = glm::mix(camera.Position, targetCameraPos, cameraFollowSpeed);
        
        // Target look direction - where camera should look
        glm::vec3 targetLookAt = playerPosition;
        glm::vec3 direction = glm::normalize(targetLookAt - camera.Position);
        
        // Calculate target yaw and pitch
        float targetYaw = glm::degrees(atan2(direction.z, direction.x));
        float targetPitch = glm::degrees(asin(direction.y));
        
        // Clamp target pitch
        targetPitch = glm::clamp(targetPitch, -89.0f, 89.0f);
        
        // Smooth camera rotation interpolation
        float cameraRotationSpeed = 3.0f * deltaTime;
        
        // Handle yaw wrapping (shortest rotation path)
        float yawDiff = targetYaw - camera.Yaw;
        if (yawDiff > 180.0f) yawDiff -= 360.0f;
        if (yawDiff < -180.0f) yawDiff += 360.0f;
        
        camera.Yaw += yawDiff * cameraRotationSpeed;
        camera.Pitch = glm::mix(camera.Pitch, targetPitch, cameraRotationSpeed);
        
        // Update camera vectors
        camera.ProcessMouseMovement(0, 0);

        // render
        // ------
        glClearColor(0.3f, 0.7f, 0.3f, 1.0f); // Green background for mixed environment
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        ourShader.use();

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // Draw player (duck) - properly sized and rotated, positioned at same level as cars
        glm::mat4 model = glm::mat4(1.0f);
        glm::vec3 duckCarPosition = glm::vec3(playerPosition.x, 0.0f, playerPosition.z); // Same level as cars
        model = glm::translate(model, duckCarPosition);
        model = glm::rotate(model, glm::radians(playerRotation - 90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Apply -90 degree offset
        model = glm::scale(model, glm::vec3(0.3f, 0.3f, 0.3f)); // Scale down to fit within one lane
        if (gameOver) {
            model = glm::scale(model, glm::vec3(1.2f, 0.3f, 1.2f));
        }
        ourShader.setMat4("model", model);
        
        // Debug output with camera info
        static int frameCounter = 0;
        if (frameCounter % 60 == 0) {
            std::cout << "Duck Position: (" << playerPosition.x << ", " << playerPosition.y << ", " << playerPosition.z << ")" << std::endl;
            std::cout << "Duck Rotation: " << playerRotation << " degrees" << std::endl;
            std::cout << "Camera Position: (" << camera.Position.x << ", " << camera.Position.y << ", " << camera.Position.z << ")" << std::endl;
            std::cout << "Camera Yaw: " << camera.Yaw << ", Pitch: " << camera.Pitch << std::endl;
            std::cout << "Distance to duck: " << glm::length(camera.Position - playerPosition) << std::endl;
            std::cout << "---" << std::endl;
        }
        frameCounter++;
        
        duckModel.Draw(ourShader);

        // Draw cars - properly sized and positioned on road surface
        for (const auto& car : cars) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, car.position);
            // Rotate car to face forward/backward along the road (0 degrees = forward)
            if (car.movingRight) {
                model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Face forward (right direction)
            } else {
                model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Face backward (left direction)
            }
            model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f)); // Make cars larger and more visible
            ourShader.setMat4("model", model);
            carModel.Draw(ourShader);
        }

        // Draw trees as obstacles on safe lanes
        for (const auto& tree : trees) {
            model = glm::mat4(1.0f);
            glm::vec3 treeGroundPosition = glm::vec3(tree.position.x, -0.5f, tree.position.z); // Ensure trees are at ground level
            model = glm::translate(model, treeGroundPosition);
            model = glm::scale(model, glm::vec3(0.003f, 0.003f, 0.003f)); // Same size as duck (30% scale)
            ourShader.setMat4("model", model);
            treeModel.Draw(ourShader);
        }

        // Draw road models for all rows that have been designated as car lanes (endless roads)
        int playerRow = (int)((playerPosition.z + 10.0f ) / MOVE_DISTANCE);

        // ?????????? ??????? (???????????????????)
        int startRow = std::max(0, playerRow - 5);
        int endRow = std::min((int)roadRows.size() - 1, playerRow + VISIBLE_ROWS);

        for (int row = startRow; row <= endRow; ++row) {
            if (roadRows[row] == 1) {
                glm::mat4 model = glm::mat4(1.0f);

 
                float rowZ = (row - playerRow) * MOVE_DISTANCE + playerPosition.z;
                glm::vec3 roadPos(0.0f, -1.2f, rowZ); // Same level as trees

                model = glm::translate(model, roadPos);
                float roadWidthX = (GRID_WIDTH * MOVE_DISTANCE) + 10.0f;
                float roadLengthZ = 0.5f; // decreased road length (depth)
                model = glm::scale(model, glm::vec3(roadWidthX, 1.0f, roadLengthZ));
                ourShader.setMat4("model", model);
                roadModel.Draw(ourShader);
            }
        }

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Game controls
    static bool wPressed = false;
    static bool aPressed = false;
    static bool sPressed = false;
    static bool dPressed = false;
    static bool rPressed = false;

    if (!gameOver) {
        // Player movement - only allow one move per key press
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && !wPressed) {
            // Allow forward, but keep the player ahead of the camera (never behind)
            float minZ = camera.Position.z + 2.0f;
            float nextZ = playerPosition.z + MOVE_DISTANCE;
            glm::vec3 newPos = glm::vec3(playerPosition.x, playerPosition.y, nextZ);
            if (nextZ >= minZ && canMoveTo(newPos)) {
                playerPosition.z = nextZ; // Move forward
                playerRotation = 0.0f; // Face forward (0 degrees)
            }
            wPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE) {
            wPressed = false;
        }

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && !sPressed) {
            // Constrain moving backward: do not allow player to go behind camera's forward edge
            float minZ = camera.Position.z + 2.0f; // small margin in front of camera
            glm::vec3 newPos = glm::vec3(playerPosition.x, playerPosition.y, playerPosition.z - MOVE_DISTANCE);
            if (playerPosition.z - MOVE_DISTANCE >= minZ && canMoveTo(newPos)) {
                playerPosition.z -= MOVE_DISTANCE; // Move backward within camera bounds
                playerRotation = 180.0f; // Face backward (180 degrees)
            }
            sPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE) {
            sPressed = false;
        }

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && !aPressed) {
            // Constrain left movement: keep player inside camera horizontal frustum
            float halfViewWidth = 8.0f; // tune to your FOV and distance
            float minX = camera.Position.x - halfViewWidth;
            glm::vec3 newPos = glm::vec3(playerPosition.x - MOVE_DISTANCE, playerPosition.y, playerPosition.z);
            if (playerPosition.x - MOVE_DISTANCE >= minX && playerPosition.x > -GRID_WIDTH * MOVE_DISTANCE && canMoveTo(newPos)) {
                playerPosition.x += MOVE_DISTANCE; // Move left
                playerRotation = 90.0f; // Face left (-90 degrees)
            }
            aPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE) {
            aPressed = false;
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS && !dPressed) {
            // Constrain right movement: keep player inside camera horizontal frustum
            float halfViewWidth = 8.0f; // tune to your FOV and distance
            float maxX = camera.Position.x + halfViewWidth;
            glm::vec3 newPos = glm::vec3(playerPosition.x + MOVE_DISTANCE, playerPosition.y, playerPosition.z);
            if (playerPosition.x + MOVE_DISTANCE <= maxX && playerPosition.x < GRID_WIDTH * MOVE_DISTANCE && canMoveTo(newPos)) {
                playerPosition.x -= MOVE_DISTANCE; // Move right
                playerRotation = -90.0f; // Face right (90 degrees)
            }
            dPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE) {
            dPressed = false;
        }
    }

    // Reset game
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !rPressed) {
        resetGame();
        rPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) {
        rPressed = false;
    }

    // Disable manual camera movement to keep constraints consistent
    // (Camera is fully driven by follow logic above)
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    //camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// Game logic functions
// ---------------------
void updateCars(float deltaTime) {
    for (auto& car : cars) {
        if (car.movingRight) {
            car.position.x += car.speed * gameSpeed * deltaTime;
            if (car.position.x > GRID_WIDTH * MOVE_DISTANCE + 20) {
                car.position.x = -GRID_WIDTH * MOVE_DISTANCE - 20;
            }
        } else {
            car.position.x -= car.speed * gameSpeed * deltaTime;
            if (car.position.x < -GRID_WIDTH * MOVE_DISTANCE - 20) {
                car.position.x = GRID_WIDTH * MOVE_DISTANCE + 20;
            }
        }
    }
}

void checkCollisions() {
    const float COLLISION_DISTANCE = 1.0f;
    
    for (const auto& car : cars) {
        float distance = glm::length(playerPosition - car.position);
        if (distance < COLLISION_DISTANCE) {
            gameOver = true;
            std::cout << "Game Over! Score: " << playerScore << " - Press R to restart" << std::endl;
            return;
        }
    }
    
    // Check tree collisions if needed
    // Currently no trees, but could be added to grass areas
}

void resetGame() {
    playerPosition = glm::vec3(0.0f, 0.0f, -10.0f);
    playerRotation = 0.0f; // Reset duck rotation to face forward (0 degrees)
    cars.clear();
    trees.clear();
    roadRows.clear();
    playerScore = 0;
    gameOver = false;
    gameSpeed = 1.0f;
    furthestRow = 0;
    
    // Reset camera position to match the new player position
    camera.Position = glm::vec3(playerPosition.x, playerPosition.y + 10.0f, playerPosition.z - 6.0f);
    camera.Yaw = 90.0f;   // Face forward
    camera.Pitch = -30.0f; // Look down at the duck
    camera.ProcessMouseMovement(0, 0); // Update camera vectors
    
    // Initialize the game world with more rows for endless gameplay
    for (int row = 0; row < VISIBLE_ROWS * 2; ++row) {
        spawnNewRow();
    }
    
    std::cout << "Crossy Road Started! Use WASD to move, R to restart" << std::endl;
}

void spawnNewRow() {
    int rowIndex = roadRows.size();
    float rowZ = (rowIndex * MOVE_DISTANCE) - 10.0f;
    
    // Initially mark as no road (0)
    bool hasRoad = false;
    
    // Decide whether to spawn cars (and thus create a road)
    if (distribution(generator) < 0.5f) { // 50% chance to spawn car lanes
        hasRoad = true; // This row will have a road because it has cars
        
        int numLanes = 1; // Single lane to match car width
        bool movingRight = distribution(generator) < 0.5f; // Random direction
        
        int numCarsPerLane = 1 + (distribution(generator) * 2); // 1-3 cars per lane
        
        for (int i = 0; i < numCarsPerLane; ++i) {
            Car car;
            car.position.y = 0.0f; // Position cars on road surface
            car.rowIndex = rowIndex; // Track which row this car belongs to
            
            car.position.z = rowZ; // Cars stay in the center of the road row
            
            car.speed = CAR_SPEED * (0.7f + distribution(generator) * 0.6f); // Speed variation
            car.movingRight = movingRight;
            car.lane = rowIndex * 10; // Unique lane identifier
            
            // Space cars out along the road with proper gaps
            float carSpacing = 6.0f + distribution(generator) * 8.0f; // 6-14 units apart
            
            if (movingRight) {
                car.position.x = -GRID_WIDTH * MOVE_DISTANCE - 20 - (i * carSpacing); // Start off-screen left
            } else {
                car.position.x = GRID_WIDTH * MOVE_DISTANCE + 20 + (i * carSpacing); // Start off-screen right
            }
            
            cars.push_back(car);
        }
    } else {
        // This is a safe lane (no cars), potentially spawn trees as obstacles
        if (distribution(generator) < 0.4f) { // 40% chance to spawn trees on safe lanes
            int numTrees = 1 + static_cast<int>(distribution(generator) * 3); // 1-3 trees per safe lane
            
            for (int i = 0; i < numTrees; ++i) {
                Tree tree;
                tree.position.y = -0.5f; // Position trees at ground level
                tree.position.z = rowZ; // Same Z as the row
                
                // Random X position within the lane, but not too close to edges
                float minX = -GRID_WIDTH * MOVE_DISTANCE * 0.8f;
                float maxX = GRID_WIDTH * MOVE_DISTANCE * 0.8f;
                tree.position.x = minX + distribution(generator) * (maxX - minX);
                
                trees.push_back(tree);
            }
        }
    }
    
    // Add to roadRows: 1 if has road (with cars), 0 if no road (safe area)
    roadRows.push_back(hasRoad ? 1 : 0);
}

// renderCube() renders a 1x1 3D cube for road markings
// ----------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            // front face
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
            // left face
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            // right face
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            // bottom face
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            // top face
            -1.0f,  1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

// Function to check if player can move to a position (tree collision)
bool canMoveTo(glm::vec3 newPosition) {
    const float TREE_COLLISION_DISTANCE = 1.5f; // Trees have larger collision radius
    
    for (const auto& tree : trees) {
        float distance = glm::length(glm::vec2(newPosition.x - tree.position.x, newPosition.z - tree.position.z));
        if (distance < TREE_COLLISION_DISTANCE) {
            return false; // Cannot move, tree is blocking
        }
    }
    return true; // Can move, no trees blocking
}

