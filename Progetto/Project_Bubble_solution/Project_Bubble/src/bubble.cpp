//-------------------------------------------
//					include					|========================================================
//-------------------------------------------
// basic
#include <string>
#include <iostream>

// imgui
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw_gl3.h"

// glad
#include <glad/glad.h>

// glfw
#include <glfw/glfw3.h>

// utils
#include <utils/shader_v1.h>
#include <utils/model_v2.h>
#include <utils/camera.h>

// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

// image/texture 
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

//-------------------------------------------
//			Variable/Callback setup			|========================================================
//-------------------------------------------
// dimensions of application's window
GLuint screenWidth = 800, screenHeight = 600;

// callback functions for keyboard and mouse events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
// if one of the WASD keys is pressed, we call the corresponding method of the Camera class
void apply_camera_movements();

// setup of Shader Programs for the shaders used in the application
void SetupShaders();
// delete Shader Programs whan application ends
void DeleteShaders();
// print on console the name of current shader
void PrintCurrentShader(int shader);

// load the 6 images from disk and create an OpenGL cubemap
GLint LoadTextureCube(string path);

// we initialize an array of booleans for each keybord key
bool keys[1024];

// we set the initial position of mouse cursor in the application window
GLfloat lastX = 400, lastY = 300;
// when rendering the first frame, we do not have a "previous state" for the mouse, so we need to manage this situation
bool firstMouse = true;

// parameters for time calculation (for animations)
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// rotation angle on Y axis
GLfloat orientationY = 0.0f;
// rotation speed on Y axis
GLfloat spin_speed = 30.0f;
// boolean to start/stop animated rotation on Y angle
GLboolean spinning = GL_TRUE;

// boolean to activate/deactivate wireframe rendering
GLboolean wireframe = GL_FALSE;
// enum data structure to manage indices for shaders swapping
enum available_ShaderPrograms { REFLECTION, FRESNEL };
// strings with shaders names to print the name of the current one on console
const char* print_available_ShaderPrograms[] = { "REFLECTION", "FRESNEL" };

// index of the current shader (= 0 in the beginning)
GLuint current_program = REFLECTION;
// a vector for all the Shader Programs used and swapped in the application
vector<Shader> shaders;
// we create a camera. We pass the initial position as a paramenter to the constructor. The last boolean tells that we want a camera "anchored" to the ground
Camera camera(glm::vec3(0.0f, 0.0f, 7.0f), GL_TRUE);

glm::vec3 lightPos0 = glm::vec3(0.0f, 0.0f, 10.0f);

// weight for the diffusive component
GLfloat Kd = 0.8f;
// roughness index for Cook-Torrance shader
GLfloat alpha = 0.2f;
// Fresnel reflectance at 0 degree (Schlik's approximation)
GLfloat F0 = 0.9f;

// ratio between refraction indices (Fresnel shader)
GLfloat Eta = 1.00 / 1.52;
// exponent for Fresnel equation
// = 5 -> "physically correct" value
// < 5 -> technically not physically correct,
// but it gives more "artistic" results
GLfloat mFresnelPower = 5;

// texture unit for the cube map
GLuint textureCube;


//-------------------------------------------
//			MAIN							|========================================================
//-------------------------------------------
int main() {
	// glfw init
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
		
	// glfw create window
	GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Bubble", nullptr, nullptr);
	if (!window)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// bind between window and callbacks
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);

	// mouse cursor disabled
	// glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad load opengl function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return -1;
	}
	// we define the viewport dimensions
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// we enable Z test
	glEnable(GL_DEPTH_TEST);

	//the "clear" color for the frame buffer
	glClearColor(0.26f, 0.46f, 0.98f, 1.0f);

	// we create the Shader Programs used in the application
	SetupShaders();

	// we create the Shader Program used for the environment map
	Shader skybox_shader("src/shaders/skybox.vert", "src/shaders/skybox.frag");

	// we load the cube map (we pass the path to the folder containing the 6 views)
	textureCube = LoadTextureCube("src/texture/skybox/");

	// we load the model(s) (code of Model class is in include/utils/model_v2.h)
	Model cubeModel("src/models/cube.obj");
	Model bunnyModel("src/models/bunny_lp.obj");
	Model sphereModel("src/models/sphere.obj");

	// we print on console the name of the first shader used
	PrintCurrentShader(current_program);

	// Projection matrix: FOV angle, aspect ratio, near and far planes
	glm::mat4 projection = glm::perspective(45.0f, (float)screenWidth / (float)screenHeight, 0.1f, 10000.0f);

	// imgui
	// Setup ImGui binding
	ImGui::CreateContext();
	ImGui_ImplGlfwGL3_Init(window, true);

	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// Setup style
	ImGui::StyleColorsDark();
	
//-------------------------------------------
//				Rendering LOOP 				|========================================================
//-------------------------------------------
	while (!glfwWindowShouldClose(window))
	{
		// we determine the time passed from the beginning
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Check is an I/O event is happening
		glfwPollEvents();
		// we apply FPS camera movements
		apply_camera_movements();
		// View matrix (=camera): position, view direction, camera "up" vector
		glm::mat4 view = camera.GetViewMatrix();

		// we "clear" the frame and z buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// we set the rendering mode
		if (wireframe)
			// Draw in wireframe
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		// if animated rotation is activated, than we increment the rotation angle using delta time and the rotation speed parameter
		if (spinning)
			orientationY += (deltaTime * spin_speed);

		//================================ OBJECT ===============================
		shaders[current_program].Use();

		// cube texture setup
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureCube);
		
		// we determine the position in the Shader Program of the uniform variable
		GLint textureLocation = glGetUniformLocation(shaders[current_program].Program, "tCube");
		// the shaders for reflection and refraction need the camera position in world coordinates
		GLint cameraLocation = glGetUniformLocation(shaders[current_program].Program, "cameraPosition");

		if (current_program == FRESNEL)
		{
			// si trovano i puntatori alle uniform nello shader
			GLint etaLocation = glGetUniformLocation(shaders[current_program].Program, "Eta");
			GLint powerLocation = glGetUniformLocation(shaders[current_program].Program, "mFresnelPower");
			GLint pointLightLocation = glGetUniformLocation(shaders[current_program].Program, "pointLightPosition");

			// si assegnano i dati alle uniform
			glUniform1f(etaLocation, Eta);
			glUniform1f(powerLocation, mFresnelPower);
			glUniform3fv(pointLightLocation, 1, glm::value_ptr(lightPos0));
		}

		//si cercano i puntatori delle uniform della projectione e della view, poi ci si assegnano i valori
		glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));

		//==============BUNNY
		glm::mat4 bunnyModelMatrix;
		glm::mat3 bunnyNormalMatrix;
		//bunnyModelMatrix = glm::translate(bunnyModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
		bunnyModelMatrix = glm::rotate(bunnyModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
		bunnyModelMatrix = glm::scale(bunnyModelMatrix, glm::vec3(0.3f, 0.3f, 0.3f));
		bunnyNormalMatrix = glm::inverseTranspose(glm::mat3(view * bunnyModelMatrix));
		glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(bunnyModelMatrix));
		glUniformMatrix3fv(glGetUniformLocation(shaders[current_program].Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(bunnyNormalMatrix));

		// render del modello
		bunnyModel.Draw(shaders[current_program]);
		//-------------Fine BUNNY


		//==============SKYBOX
		glDepthFunc(GL_LEQUAL);
		skybox_shader.Use();
		// we activate the cube map
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureCube);
		// we pass projection and view matrices to the Shader Program of the skybox
		glUniformMatrix4fv(glGetUniformLocation(skybox_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
		// to have the background fixed during camera movements, we have to remove the translations from the view matrix
		// thus, we consider only the top-left submatrix, and we create a new 4x4 matrix
		view = glm::mat4(glm::mat3(camera.GetViewMatrix()));    // Remove any translation component of the view matrix
		glUniformMatrix4fv(glGetUniformLocation(skybox_shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));

		// we determine the position in the Shader Program of the uniform variables
		textureLocation = glGetUniformLocation(skybox_shader.Program, "tCube");
		// we assign the value to the uniform variable
		glUniform1i(textureLocation, 0);

		// we render the cube with the environment map
		cubeModel.Draw(skybox_shader);
		// we set again the depth test to the default operation for the next frame
		glDepthFunc(GL_LESS);
		//-------------Fine SKYBOX
		
		// imgui render
	    // imgui new frame
	/*	ImGui_ImplGlfwGL3_NewFrame();
		{
			static float f = 0.0f;
			static int counter = 0;
			ImGui::Text("Hello, world!");                           // Display some text (you can use a format string too)
			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f    
			ImGui::ColorEdit3("clear color", (float*)& clear_color); // Edit 3 floats representing a color

			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our windows open/close state
			ImGui::Checkbox("Another Window", &show_another_window);

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (NB: most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			
		}
		ImGui::Render();
		ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
		*/
		//swap dei buffer
		glfwSwapBuffers(window);
	}
	// we delete the Shader Programs
	DeleteShaders();
	// we close and delete the created context
	glfwTerminate();
	return 0;
}

//-------------------------------------------
//	 Implementazione funzioni e callback     |========================================================
//-------------------------------------------
GLint LoadTextureCube(string path)
{
	GLuint textureImage;
	int w, h;
	unsigned char* image;
	string fullname;

	glGenTextures(1, &textureImage);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureImage);

	// we use as convention that the names of the 6 images are "posx, negx, posy, negy, posz, negz", placed at the path passed as parameter
	//POSX
	fullname = path + std::string("posx.jpg");
	image = stbi_load(fullname.c_str(), &w, &h, 0, STBI_rgb);
	if (image == nullptr)
		std::cout << "Failed to load texture!" << std::endl;
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	// we free the memory once we have created an OpenGL texture
	stbi_image_free(image);
	//NEGX
	fullname = path + std::string("negx.jpg");
	image = stbi_load(fullname.c_str(), &w, &h, 0, STBI_rgb);
	if (image == nullptr)
		std::cout << "Failed to load texture!" << std::endl;
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	// we free the memory once we have created an OpenGL texture
	stbi_image_free(image);
	//POSY
	fullname = path + std::string("posy.jpg");
	image = stbi_load(fullname.c_str(), &w, &h, 0, STBI_rgb);
	if (image == nullptr)
		std::cout << "Failed to load texture!" << std::endl;
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	// we free the memory once we have created an OpenGL texture
	stbi_image_free(image);
	//NEGY
	fullname = path + std::string("negy.jpg");
	image = stbi_load(fullname.c_str(), &w, &h, 0, STBI_rgb);
	if (image == nullptr)
		std::cout << "Failed to load texture!" << std::endl;
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	// we free the memory once we have created an OpenGL texture
	stbi_image_free(image);
	//POSZ
	fullname = path + std::string("posz.jpg");
	image = stbi_load(fullname.c_str(), &w, &h, 0, STBI_rgb);
	if (image == nullptr)
		std::cout << "Failed to load texture!" << std::endl;
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	// we free the memory once we have created an OpenGL texture
	stbi_image_free(image);
	//NEGZ
	fullname = path + std::string("negz.jpg");
	image = stbi_load(fullname.c_str(), &w, &h, 0, STBI_rgb);
	if (image == nullptr)
		std::cout << "Failed to load texture!" << std::endl;
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	// we free the memory once we have created an OpenGL texture
	stbi_image_free(image);

	// we set the filtering for minification and magnification
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// we set how to consider the texture coordinates outside [0,1] range
	// in this case we have a cube map, so
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	return textureImage;
}


//////////////////////////////////////////
// we create and compile shaders (code of Shader class is in include/utils/shader_v1.h), and we add them to the list of available shaders
void SetupShaders()
{
	// we create the Shader Programs (code in shader_v1.h)
	Shader shader1("src/shaders/reflect.vert", "src/shaders/reflect.frag");
	shaders.push_back(shader1);
	Shader shader2("src/shaders/reflect.vert", "src/shaders/fresnel.frag");
	shaders.push_back(shader2);
	
}

//////////////////////////////////////////
// we delete all the Shaders Programs
void DeleteShaders()
{
	for (GLuint i = 0; i < shaders.size(); i++)
		shaders[i].Delete();
}

//////////////////////////////////////////
// we print on console the name of the currently used shader
void PrintCurrentShader(int shader)
{
	std::cout << "Current shader:" << print_available_ShaderPrograms[shader] << std::endl;
}

//////////////////////////////////////////
// callback for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	std::cout << "keycallback " << key<< endl;
	// if ESC is pressed, we close the application
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	// if P is pressed, we start/stop the animated rotation of models
	if (key == GLFW_KEY_P && action == GLFW_PRESS)
		spinning = !spinning;

	// if L is pressed, we activate/deactivate wireframe rendering of models
	if (key == GLFW_KEY_L && action == GLFW_PRESS)
		wireframe = !wireframe;

	// pressing a key between 1 and 2, we change the shader applied to the models
	if ((key >= GLFW_KEY_1 && key <= GLFW_KEY_2) && action == GLFW_PRESS)
	{
		// "1" to "2" -> ASCII codes from 49 to 50
		// we subtract 48 (= ASCII CODE of "0") to have integers from 1 to 2
		// we subtract 1 to have indices from 0 to 1 in the shaders list
		current_program = (key - '0' - 1);
		PrintCurrentShader(current_program);
	}
	// we keep trace of the pressed keys
	// with this method, we can manage 2 keys pressed at the same time:
	// many I/O managers often consider only 1 key pressed at the time (the first pressed, until it is released)
	// using a boolean array, we can then check and manage all the keys pressed at the same time
	if (action == GLFW_PRESS)
		keys[key] = true;
	else if (action == GLFW_RELEASE)
		keys[key] = false;
}

//////////////////////////////////////////
// If one of the WASD keys is pressed, the camera is moved accordingly (the code is in utils/camera.h)
void apply_camera_movements()
{
	
	if (keys[GLFW_KEY_W]) {
		camera.ProcessKeyboard(FORWARD, deltaTime);
		std::cout << "wasd camera" << endl;
	}
	if (keys[GLFW_KEY_S])
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (keys[GLFW_KEY_A])
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (keys[GLFW_KEY_D])
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

//////////////////////////////////////////

  // callback for mouse events
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	// we move the camera view following the mouse cursor
	// we calculate the offset of the mouse cursor from the position in the last frame
	// when rendering the first frame, we do not have a "previous state" for the mouse, so we set the previous state equal to the initial values (thus, the offset will be = 0)
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	// offset of mouse cursor position
	GLfloat xoffset = xpos - lastX;
	GLfloat yoffset = lastY - ypos;

	// the new position will be the previous one for the next frame
	lastX = xpos;
	lastY = ypos;

	// we pass the offset to the Camera class instance in order to update the rendering
	camera.ProcessMouseMovement(xoffset, yoffset);
}
