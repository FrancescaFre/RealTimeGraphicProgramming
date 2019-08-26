
// Std. Includes
#include <string>

// Loader estensioni OpenGL
// http://glad.dav1d.de/
// THIS IS OPTIONAL AND NOT REQUIRED, ONLY USE THIS IF YOU DON'T WANT GLAD TO INCLUDE windows.h
// GLAD will include windows.h for APIENTRY if it was not previously defined.
// Make sure you have the correct definition for APIENTRY for platforms which define _WIN32 but don't use __stdcall
#ifdef _WIN32
#define APIENTRY __stdcall
#endif

#include <glad/glad.h>

// GLFW library to create window and to manage I/O
#include <glfw/glfw3.h>

// another check related to OpenGL loader
// confirm that GLAD didn't include windows.h
#ifdef _WINDOWS_
#error windows.h was included!
#endif

// classes developed during lab lectures to manage shaders, to load models, and for FPS camera
// in this example, the Model and Mesh classes support texturing
#include <utils/shader_v1.h>
#include <utils/model_v2.h>
#include <utils/camera.h>

// we load the GLM classes used in the application
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

// we include the library for images loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

// dimensions of application's window
GLuint screenWidth = 800, screenHeight = 600;

// callback functions for keyboard and mouse events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
// if one of the WASD keys is pressed, we call the corresponding method of the Camera class
void apply_camera_movements();

// setup of Shader Programs for the 5 shaders used in the application
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
Camera camera(glm::vec3(0.0f, 0.0f, 7.0f), GL_FALSE);

// Uniforms to be passed to shaders
// point light positions
glm::vec3 lightPos0 = glm::vec3(0.0f, 0.0f, 10.0f);

// ratio between refraction indices (Fresnel shader)
GLfloat Eta = 1.00 / 1.52;
// exponent for Fresnel equation
// = 5 -> "physically correct" value
// < 5 -> technically not physically correct,
// but it gives more "artistic" results
GLfloat mFresnelPower = 5;

// texture unit for the cube map
GLuint textureCube;


/////////////////// MAIN function ///////////////////////
int main()
{
	// Initialization of OpenGL context using GLFW
	glfwInit();
	// We set OpenGL specifications required for this application
	// In this case: 3.3 Core
	// It is possible to raise the values, in order to use functionalities of OpenGL 4.x
	// If not supported by your graphics HW, the context will not be created and the application will close
	// N.B.) creating GLAD code to load extensions, try to take into account the specifications and any extensions you want to use,
	// in relation also to the values indicated in these GLFW commands
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	// we set if the window is resizable
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// we create the application's window
	GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "RGP_lecture05a", nullptr, nullptr);
	if (!window)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// we put in relation the window and the callbacks
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);

	// we disable the mouse cursor
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// GLAD tries to load the context set by GLFW
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

	// we create the Shader Programs used in the application for the bunny
	SetupShaders();

	// we create the Shader Program used for the environment map
	Shader skybox_shader("src/shaders/skybox.vert", "src/shaders/skybox.frag");

	// we load the cube map (we pass the path to the folder containing the 6 views)
	textureCube = LoadTextureCube("src/texture/cube/colored/");

	// we load the model(s) (code of Model class is in include/utils/model_v2.h)
	Model cubeModel("src/models/cube.obj"); // used for the environment map
	Model bunnyModel("src/models/bunny_lp.obj");
	Model sphereModel("src/models/sphere.obj");

	// we print on console the name of the first shader used
	PrintCurrentShader(current_program);

	// Projection matrix: FOV angle, aspect ratio, near and far planes
	glm::mat4 projection = glm::perspective(45.0f, (float)screenWidth / (float)screenHeight, 0.1f, 10000.0f);

	// Rendering loop: this code is executed at each frame
	while (!glfwWindowShouldClose(window))
	{
		// we determine the time passed from the beginning
		// and we calculate time difference between current frame rendering and the previous one
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

		/////////////////// BUNNY ////////////////////////////////////////////////
		// We "install" the selected Shader Program as part of the current rendering process
		shaders[current_program].Use();
		// we activate the cube map
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureCube);

		// we determine the position in the Shader Program of the uniform variable
		GLint textureLocation = glGetUniformLocation(shaders[current_program].Program, "tCube");
		// the shaders for reflection and refraction need the camera position in world coordinates
		GLint cameraLocation = glGetUniformLocation(shaders[current_program].Program, "cameraPosition");

		// we assign the value to the uniform variable
		glUniform1i(textureLocation, 0);
		glUniform3fv(cameraLocation, 1, glm::value_ptr(camera.Position));

		if (current_program == FRESNEL)
		{
			// we determine the position in the Shader Program of the uniform variable
			GLint etaLocation = glGetUniformLocation(shaders[current_program].Program, "Eta");
			GLint powerLocation = glGetUniformLocation(shaders[current_program].Program, "mFresnelPower");
			GLint pointLightLocation = glGetUniformLocation(shaders[current_program].Program, "pointLightPosition");

			// we assign the value to the uniform variable
			glUniform1f(etaLocation, Eta);
			glUniform1f(powerLocation, mFresnelPower);
			glUniform3fv(pointLightLocation, 1, glm::value_ptr(lightPos0));

		}

		// we pass projection and view matrices to the Shader Program
		glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));

		/*
		  we create the transformation matrix by defining the Euler's matrices

		  N.B.) the last defined is the first applied

		  We need also the matrix for normals transformation, which is the inverse of the transpose of the 3x3 submatrix (upper left) of the modelview. We do not consider the 4th column because we do not need translations for normals.
		  An explanation (where XT means the transpose of X, etc):
			"Two column vectors X and Y are perpendicular if and only if XT.Y=0. If We're going to transform X by a matrix M, we need to transform Y by some matrix N so that (M.X)T.(N.Y)=0. Using the identity (A.B)T=BT.AT, this becomes (XT.MT).(N.Y)=0 => XT.(MT.N).Y=0. If MT.N is the identity matrix then this reduces to XT.Y=0. And MT.N is the identity matrix if and only if N=(MT)-1, i.e. N is the inverse of the transpose of M.
		*/

		/* glm::mat4 bunnyModelMatrix;
				glm::mat3 bunnyNormalMatrix;
				bunnyModelMatrix = glm::rotate(bunnyModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
				bunnyModelMatrix = glm::scale(bunnyModelMatrix, glm::vec3(0.3f, 0.3f, 0.3f));
				bunnyNormalMatrix = glm::inverseTranspose(glm::mat3(view*bunnyModelMatrix));
				glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(bunnyModelMatrix));
				glUniformMatrix3fv(glGetUniformLocation(shaders[current_program].Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(bunnyNormalMatrix));

				// we render the bunny
				bunnyModel.Draw(shaders[current_program]);

		*/
		glm::mat4 sphereModelMatrix;
		glm::mat3 sphereNormalMatrix;
		//sphereModelMatrix = glm::translate(sphereModelMatrix, glm::vec3(0.0f, 1.0f, 0.0f));
		sphereModelMatrix = glm::rotate(sphereModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
		sphereModelMatrix = glm::scale(sphereModelMatrix, glm::vec3(0.8f, 0.8f, 0.8f));
		sphereNormalMatrix = glm::inverseTranspose(glm::mat3(view * sphereModelMatrix));
		glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(sphereModelMatrix));
		glUniformMatrix3fv(glGetUniformLocation(shaders[current_program].Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(sphereNormalMatrix));

		// we render the sphere
		sphereModel.Draw(shaders[current_program]);
		/////////////////// SKYBOX ////////////////////////////////////////////////
		// we use the cube to attach the 6 textures of the environment map.
		// we render it after all the other objects, in order to avoid the depth tests as much as possible.
		// we will set, in the vertex shader for the skybox, all the values to the maximum depth. Thus, the environment map is rendered only where there are no other objects in the image (so, only on the background). Thus, we set the depth test to GL_LEQUAL, in order to let the fragments of the background pass the depth test (because they have the maximum depth possible, and the default setting is GL_LESS)
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

		// Swapping back and front buffers
		glfwSwapBuffers(window);
	}

	// when I exit from the graphics loop, it is because the application is closing
	// we delete the Shader Programs
	DeleteShaders();
	// we close and delete the created context
	glfwTerminate();
	return 0;
}

//////////////////////////////////////////
// we load the 6 images from disk and we create an OpenGL cube map
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
	if (keys[GLFW_KEY_W])
		camera.ProcessKeyboard(FORWARD, deltaTime);
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
