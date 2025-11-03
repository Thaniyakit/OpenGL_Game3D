#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/animator.h>
#include <learnopengl/model_animation.h>



#include <iostream>


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 800;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Character movement and rotation
glm::vec3 characterPosition(0.0f, 0.0f, 0.0f);
float characterRotation = 0.0f; // Y-axis rotation in degrees
float moveSpeed = 2.0f;

enum AnimState {
	IDLE = 1,
	IDLE_PUNCH,
	PUNCH_IDLE,
	IDLE_KICK,
	KICK_IDLE,
	IDLE_TALK,
	TALK_IDLE,
	IDLE_WALK,
	WALK_IDLE,
	WALK
};

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
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
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

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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
	Shader ourShader("anim_model.vs", "anim_model.fs");


	// load models
	// -----------
	// idle 3.3, walk 2.06, run 0.83, punch 1.03, kick 1.6
	Model ourModel(FileSystem::getPath("resources/objects/pleasant_girl/Peasant Girl.dae"));
	Animation idleAnimation(FileSystem::getPath("resources/objects/pleasant_girl/Idle.dae"), &ourModel);
	Animation walkAnimation(FileSystem::getPath("resources/objects/pleasant_girl/Walking.dae"), &ourModel);
	Animation runAnimation(FileSystem::getPath("resources/objects/pleasant_girl/Fast Run.dae"), &ourModel);
	Animation punchAnimation(FileSystem::getPath("resources/objects/pleasant_girl/Quad Punch.dae"), &ourModel);
	Animation kickAnimation(FileSystem::getPath("resources/objects/pleasant_girl/Mma Kick.dae"), &ourModel);
	Animation talkAnimation(FileSystem::getPath("resources/objects/pleasant_girl/Talking.dae"), &ourModel);
	Animator animator(&idleAnimation);
	enum AnimState charState = IDLE;
	float blendAmount = 0.0f;
	float blendRate = 0.055f;

	// draw in wireframe
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		// -----
		processInput(window);
		if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
			animator.PlayAnimation(&idleAnimation, NULL, 0.0f, 0.0f, 0.0f);
		if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
			animator.PlayAnimation(&walkAnimation, NULL, 0.0f, 0.0f, 0.0f);
		if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
			animator.PlayAnimation(&punchAnimation, NULL, 0.0f, 0.0f, 0.0f);
		if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
			animator.PlayAnimation(&kickAnimation, NULL, 0.0f, 0.0f, 0.0f);
		if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
			animator.PlayAnimation(&talkAnimation, NULL, 0.0f, 0.0f, 0.0f);


		switch (charState) {
		case IDLE:
			// Check for any movement input to trigger walk
			if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS || 
				glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS ||
				glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS ||
				glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
				blendAmount = 0.0f;
				animator.PlayAnimation(&idleAnimation, &walkAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				bool isMoving = false;
				glm::vec3 moveDirection(0.0f);

				// Check for movement input and set rotation
				if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) { // Forward
					moveDirection.z = 1.0f;
					characterRotation = 180.0f;
					isMoving = true;
				}
				if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) { // Backward
					moveDirection.z = -1.0f;
					characterRotation = 0.0f;
					isMoving = true;
				}
				if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) { // Left
					moveDirection.x = 1.0f;
					characterRotation = -90.0f;
					isMoving = true;
				}
				if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) { // Right
					moveDirection.x = -1.0f;
					characterRotation = 90.0f;
					isMoving = true;
				}

				// Move character if input detected
				if (isMoving && glm::length(moveDirection) > 0.0f) {
					moveDirection = glm::normalize(moveDirection);
					characterPosition += moveDirection * moveSpeed * deltaTime;
				}

				// Exit walk state when no movement keys are pressed
				if (!isMoving) {
					blendAmount = 0.0f;
					charState = WALK_IDLE;
				}
				charState = IDLE_WALK;
			}
			else if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
				blendAmount = 0.0f;
				animator.PlayAnimation(&idleAnimation, &punchAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				charState = IDLE_PUNCH;
			}
			else if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
				blendAmount = 0.0f;
				animator.PlayAnimation(&idleAnimation, &kickAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				charState = IDLE_KICK;
			}
			else if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
				blendAmount = 0.0f;
				animator.PlayAnimation(&idleAnimation, &talkAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				charState = IDLE_TALK;
			}
			printf("idle \n");
			break;
		case IDLE_WALK:
			blendAmount += blendRate;
			blendAmount = fmod(blendAmount, 1.0f);
			animator.PlayAnimation(&idleAnimation, &walkAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if (blendAmount > 0.9f) {
				blendAmount = 0.0f;
				float startTime = animator.m_CurrentTime2;
				animator.PlayAnimation(&walkAnimation, NULL, startTime, 0.0f, blendAmount);
				charState = WALK;
			}
			printf("idle_walk \n");
			break;
		case WALK: {
			animator.PlayAnimation(&walkAnimation, NULL, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			
			// Character movement during walk state
			bool isMoving = false;
			glm::vec3 moveDirection(0.0f);
			
			// Check for movement input and set rotation
			if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) { // Forward
				moveDirection.z = 0.0f;
				characterRotation = 180.0f;
				isMoving = true;
			}
			if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) { // Backward
				moveDirection.z = 0.0f;
				characterRotation = 0.0f;
				isMoving = true;
			}
			if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) { // Left
				moveDirection.x = 0.0f;
				characterRotation = -90.0f;
				isMoving = true;
			}
			if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) { // Right
				moveDirection.x = 0.0f;
				characterRotation = 90.0f;
				isMoving = true;
			}
			
			// Move character if input detected
			if (isMoving && glm::length(moveDirection) > 0.0f) {
				moveDirection = glm::normalize(moveDirection);
				characterPosition += moveDirection * moveSpeed * deltaTime;
			}
			
			// Exit walk state when no movement keys are pressed
			if (!isMoving) {
				blendAmount = 0.0f;
				charState = WALK_IDLE;
			}
			printf("walking\n");
			break;
		}
		case WALK_IDLE:
			blendAmount += blendRate;
			blendAmount = fmod(blendAmount, 1.0f);
			animator.PlayAnimation(&walkAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if (blendAmount > 0.9f) {
				blendAmount = 0.0f;
				float startTime = animator.m_CurrentTime2;
				animator.PlayAnimation(&idleAnimation, NULL, startTime, 0.0f, blendAmount);
				charState = IDLE;
			}
			printf("walk_idle \n");
			break;
		case IDLE_PUNCH:
			blendAmount += blendRate;
			blendAmount = fmod(blendAmount, 1.0f);
			animator.PlayAnimation(&idleAnimation, &punchAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if (blendAmount > 0.9f) {
				blendAmount = 0.0f;
				float startTime = animator.m_CurrentTime2;
				animator.PlayAnimation(&punchAnimation, NULL, startTime, 0.0f, blendAmount);
				charState = PUNCH_IDLE;
			}
			printf("idle_punch\n");
			break;
		case PUNCH_IDLE:
			if (animator.m_CurrentTime > 0.7f) {
				blendAmount += blendRate;
				blendAmount = fmod(blendAmount, 1.0f);
				animator.PlayAnimation(&punchAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				if (blendAmount > 0.9f) {
					blendAmount = 0.0f;
					float startTime = animator.m_CurrentTime2;
					animator.PlayAnimation(&idleAnimation, NULL, startTime, 0.0f, blendAmount);
					charState = IDLE;
				}
				printf("punch_idle \n");
			}
			else {
				// punching
				printf("punching \n");
			}
			break;
		case IDLE_KICK:
			blendAmount += blendRate;
			blendAmount = fmod(blendAmount, 1.0f);
			animator.PlayAnimation(&idleAnimation, &kickAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if (blendAmount > 0.9f) {
				blendAmount = 0.0f;
				float startTime = animator.m_CurrentTime2;
				animator.PlayAnimation(&kickAnimation, NULL, startTime, 0.0f, blendAmount);
				charState = KICK_IDLE;
			}
			printf("idle_kick\n");
			break;
		case KICK_IDLE:
			if (animator.m_CurrentTime > 1.0f) {
				blendAmount += blendRate;
				blendAmount = fmod(blendAmount, 1.0f);
				animator.PlayAnimation(&kickAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				if (blendAmount > 0.9f) {
					blendAmount = 0.0f;
					float startTime = animator.m_CurrentTime2;
					animator.PlayAnimation(&idleAnimation, NULL, startTime, 0.0f, blendAmount);
					charState = IDLE;
				}
				printf("kick_idle \n");
			}
			else {
				// punching
				printf("kicking \n");
			}
			break;
		case IDLE_TALK:
			blendAmount += blendRate;
			blendAmount = fmod(blendAmount, 1.0f);
			animator.PlayAnimation(&idleAnimation, &talkAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if (blendAmount > 0.9f) {
				blendAmount = 0.0f;
				float startTime = animator.m_CurrentTime2;
				animator.PlayAnimation(&talkAnimation, NULL, startTime, 0.0f, blendAmount);
				charState = TALK_IDLE;
			}
			printf("idle_talk\n");
			break;
		case TALK_IDLE:
			 if (animator.m_CurrentTime > 3.0f) { // Talk animation duration
				blendAmount += blendRate;
				blendAmount = fmod(blendAmount, 1.0f);
				animator.PlayAnimation(&talkAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				if (blendAmount > 0.9f) {
					blendAmount = 0.0f;
					float startTime = animator.m_CurrentTime2;
					animator.PlayAnimation(&idleAnimation, NULL, startTime, 0.0f, blendAmount);
					charState = IDLE;
				}
				printf("talk_idle \n");
			}
			else {
				// talking
				printf("talking \n");
			}
			break;
		}



		animator.UpdateAnimation(deltaTime);

		// render
		// ------
		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// don't forget to enable shader before setting uniforms
		ourShader.use();

		// view/projection transformations
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		ourShader.setMat4("projection", projection);
		ourShader.setMat4("view", view);

		auto transforms = animator.GetFinalBoneMatrices();
		for (int i = 0; i < transforms.size(); ++i)
			ourShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);


		// render the loaded model
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, characterPosition); // Use character position
		model = glm::rotate(model, glm::radians(characterRotation), glm::vec3(0.0f, 1.0f, 0.0f)); // Apply character rotation
		model = glm::translate(model, glm::vec3(0.0f, -0.4f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::scale(model, glm::vec3(.5f, .5f, .5f));	// it's a bit too big for our scene, so scale it down
		ourShader.setMat4("model", model);
		ourModel.Draw(ourShader);


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

	// Camera controls disabled to prevent interference with character movement
	// Character movement controls (Arrow Keys):
	// UP = Forward, DOWN = Backward, LEFT = Left, RIGHT = Right
	// Movement automatically triggers walk animation with blending
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
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = static_cast<float>(xpos);
		lastY = static_cast<float>(ypos);
		firstMouse = false;
	}

	float xoffset = static_cast<float>(xpos) - lastX;
	float yoffset = lastY - static_cast<float>(ypos); // reversed since y-coordinates go from bottom to top

	lastX = static_cast<float>(xpos);
	lastY = static_cast<float>(ypos);

	// Mouse camera movement disabled to focus on character control
	// camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// Keep scroll for camera zoom
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
