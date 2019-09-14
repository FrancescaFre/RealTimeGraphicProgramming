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


#define ASSERT(x) if(!(x))  debugbreak();
#define GLCall(x) GLClearError();\
    x;\
    ASSERT (GLLogCall(#x,__FILE,LINE))

void GLClearError() {
	while (glGetError() != GL_NO_ERROR);
}

bool GLLogCall(const char* function, const char* file, int line) {
	while (GLenum error = glGetError()) {
		std::cerr << "[OpenGL ERROR] (" << error << " ):" << function << " " << file << " " << line << std::endl;
		return false;
	}
	return true;
}

//per usare la scheda grafica dedicata anzichè l'integrata (povera intel <3) 
extern "C"
{
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}
// dimensions of application's window
GLuint screenWidth = 800, screenHeight = 800;

// callback functions for keyboard and mouse events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

//Functions
void apply_camera_movements();
void SetupShaders();
void DeleteShaders();
void PrintCurrentShader(int shader);
GLint LoadTextureCube(string path);
void CameraMovement(GLfloat frame);

// keyboard keys
bool keys[1024];

//Mouse
GLfloat lastX = 400, lastY = 300;
bool firstMouse = true;

// parameters for time calculation (for animations)
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

//Spinning values
GLfloat orientationY = 0.0f;
GLfloat spin_speed = 30.0f;

bool spinning, wireframe; 
//Camera end light
Camera camera(glm::vec3(0.0f,0.0f, 7.0f), GL_FALSE);

glm::vec3 lightPos0 = glm::vec3(0.0f, 0.0f, 10.0f);

//FRESNEL
GLfloat Eta = 1.0 / 1.52; 
GLfloat mFresnelPower = 5;

// CUBE MAP Texture
GLuint textureCube;

// SHADERS
enum available_ShaderPrograms { BUBBLE, RAYMARCHING};
const char* print_available_ShaderPrograms[] = {"BUBBLE", "RAYMARCHING"};
GLuint current_program = RAYMARCHING;
vector<Shader> shaders;


//MODELS
enum available_models { CUBE, SPHERE, BUNNY };
const char* print_available_Models[] = { "CUBE", "SPHERE", "BUNNY" };

struct object_on_scene {
	int shape; 
	float scale = 1.0;
	glm::vec3 position = glm::vec3(0.0f,0.0f,0.0f); 
	bool select = false;
	glm::vec3 color; 
	int op; 

	bool isMoving = false; 
	glm::vec3 movement = glm::vec3(0.1f, 1.0f, 1.1f);
	bool spinning = false; 
	float orientationY = 0.0f;
	bool wireframe = false;
	bool metashape = true; 
	
};
vector<object_on_scene> scene; 

int choosemodel=0;
bool cameraMovement = false; 
int rayMarchingShader = 0; 
//-------------------------------------------
//			MAIN							|========================================================
//-------------------------------------------
int main() {
	// glfw init
	glfwInit();

	// glfw create window
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
		
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
	
	GLint m_viewport[4];

	glGetIntegerv(GL_VIEWPORT, m_viewport);

	glEnable(GL_DEPTH_TEST); // Z test
	
	glClearColor(0.26f, 0.46f, 0.98f, 1.0f); //the "clear" color for the frame buffer

	//SETUP SHADERS
	SetupShaders();

	//Load skybox and skybox's shader
	Shader skybox_shader("src/shaders/skybox.vert", "src/shaders/skybox.frag");
	textureCube = LoadTextureCube("src/texture/cube/skybox/");

	// LOAD MODELs
	Model cubeModel("src/models/cube.obj");
	Model sphereModel("src/models/sphere.obj");
	Model bunnyModel("src/models/bunny_lp.obj");
	Model planeModel("src/models/plane.obj");


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
	
	ImGui::StyleColorsDark();
	

//-------------------------------------------
//				Rendering LOOP 				|========================================================
//-------------------------------------------
	while (!glfwWindowShouldClose(window))
	{
		// Current Frame and DeltaTime
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		
		//event functions
		glfwPollEvents(); // Check is an I/O event is happening
		apply_camera_movements();	// we apply FPS camera movements

		if (cameraMovement == true)
			CameraMovement(currentFrame);

		// View matrix (=camera): position, view direction, camera "up" vector
		glm::mat4 view = camera.GetViewMatrix();

		// we "clear" the frame and z buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		

		//================================ OBJECT ===============================
		shaders[current_program].Use();

		// cube texture setup
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureCube);
		
		// we determine the position in the Shader Program of the uniform variable
		GLint textureLocation = glGetUniformLocation(shaders[current_program].Program, "tCube");
		// the shaders for reflection and refraction need the camera position in world coordinates
		GLint cameraLocation = glGetUniformLocation(shaders[current_program].Program, "cameraPosition");
	
		// we assign the value to the uniform variable
		glUniform1i(textureLocation, 0);
		glUniform3fv(cameraLocation, 1, glm::value_ptr(camera.Position));

		//si cercano i puntatori delle uniform della projectione e della view, poi ci si assegnano i valori
		glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));

		if (current_program == BUBBLE)
			glUniform1f(glGetUniformLocation(shaders[current_program].Program, "mFresnelPower"),mFresnelPower);
		
		if (current_program == RAYMARCHING) {
			glUniform1f(glGetUniformLocation(shaders[current_program].Program, "time"), currentFrame);
			glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "camera"), 1, GL_FALSE, glm::value_ptr(camera.GetViewMatrix()));
			
			glm::mat4 modelMatrix;
			modelMatrix = glm::scale(modelMatrix, glm::vec3(10.));

			glm::mat3 normalMatrix;
			
			glUniform1i(glGetUniformLocation(shaders[current_program].Program, "current_shader"), rayMarchingShader);

			normalMatrix = glm::inverseTranspose(glm::mat3(view * modelMatrix));
			glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
			glUniformMatrix3fv(glGetUniformLocation(shaders[current_program].Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));


			glm::vec4 blob;
			glm::vec4 info1;
			glm::vec2 info2;
			bool s = false; 
			for (int i = 0; i < 10; i++)
			{

				blob = glm::vec4(0.0);
				info1 = glm::vec4(0.0);
				info2 = glm::vec3(0.0);
				s = false;

				float m = i % 4; 
				if (i < scene.size()) {
					glm::vec3 newPosition = scene[i].position; 
					if (scene[i].isMoving) 
						newPosition = glm::vec3(cos(currentFrame * scene[i].movement.x), cos(currentFrame * scene[i].movement.y), cos(currentFrame * scene[i].movement.z));
					//posizione + size
					blob = glm::vec4(newPosition, scene[i].scale);
					//colore + select
					info1 =glm::vec4(scene[i].color, scene[i].select );
					//forma + operatore
					info2 = glm::vec3(scene[i].shape, scene[i].op, scene[i].spinning);
				}
				
				string nameVar = "blobsPos[" + std::to_string(i)+"]";
				glUniform4fv(glGetUniformLocation(shaders[current_program].Program, nameVar.c_str()), 1, glm::value_ptr(blob));
				
				nameVar = "info1[" + std::to_string(i) + "]";
				glUniform4fv(glGetUniformLocation(shaders[current_program].Program, nameVar.c_str()), 1, glm::value_ptr(info1));

				nameVar = "info2[" + std::to_string(i) + "]";
				glUniform3fv(glGetUniformLocation(shaders[current_program].Program, nameVar.c_str()), 1, glm::value_ptr(info2));
				

			}

			cubeModel.Draw(shaders[current_program]);
		}
		//==============Draw models
		if(current_program != RAYMARCHING){
			for (int i = 0; i < scene.size(); i++) {
				glm::mat4 modelMatrix;
				glm::mat3 normalMatrix;
				glm::vec3 translate;

				if (scene[i].wireframe)
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				else
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

				if (scene[i].spinning) {
					scene[i].orientationY += (deltaTime * spin_speed);
				}
				if (scene[i].isMoving)
					translate = glm::vec3(cos(currentFrame * scene[i].movement.x), cos(currentFrame * scene[i].movement.y), cos(currentFrame * scene[i].movement.z)) + scene[i].position;
				else
					translate = +scene[i].position;

				modelMatrix = glm::translate(modelMatrix, translate);
				modelMatrix = glm::rotate(modelMatrix, glm::radians(scene[i].orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(scene[i].scale));

				normalMatrix = glm::inverseTranspose(glm::mat3(view * modelMatrix));
				glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
				glUniformMatrix3fv(glGetUniformLocation(shaders[current_program].Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

				Model* modeltoDraw = &cubeModel;
				switch (scene[i].shape) {
				case 0: modeltoDraw = &cubeModel;	break;
				case 1: modeltoDraw = &sphereModel;	break;
				case 2: modeltoDraw = &bunnyModel;	break;
				case 3: modeltoDraw = &planeModel;	break;
				}
				modeltoDraw->Draw(shaders[current_program]);
			}
		}
		
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
		

//-------------------------------------------
//				BORDELLO CON LA GUI			|========================================================
//-------------------------------------------
	// imgui render
	    // imgui new frame
		ImGui_ImplGlfwGL3_NewFrame();
		//begin of main gui
		ImGui::Begin("Main GUI");
		{
			if (ImGui::Button("Bubble"))
				current_program = 0;
			ImGui::SameLine();
			if (ImGui::Button("RayMarching"))
				current_program = 1;

			//-------------------------
			if (current_program == 0)
				ImGui::Text("Bubble");

			if (current_program == 1) {
				ImGui::Text("RayMarching");	
				if (ImGui::Button("Blin-Phonn"))
					rayMarchingShader = 0;
				ImGui::SameLine();
				if (ImGui::Button("Reflect"))                            // Buttons return true when clicked (NB: most widgets return true when edited/activated)
					rayMarchingShader = 1;
				ImGui::SameLine();
				if (ImGui::Button("Fresnel"))
					rayMarchingShader = 2;
				ImGui::SameLine();
				if (ImGui::Button("Bubble2"))
					rayMarchingShader = 3;
				ImGui::SameLine();
				if (ImGui::Button("Stripes"))
					rayMarchingShader = 4;
			}

			if (ImGui::CollapsingHeader("Background Texture")) {
				if (ImGui::Button("Park"))
					textureCube = LoadTextureCube("src/texture/cube/park/");
				ImGui::SameLine();
				if (ImGui::Button("Mountain"))
					textureCube = LoadTextureCube("src/texture/cube/mountain/");
				ImGui::SameLine();
				if (ImGui::Button("Uffizi"))
					textureCube = LoadTextureCube("src/texture/cube/uffizi/");
				ImGui::SameLine();
				if (ImGui::Button("Sky"))
					textureCube = LoadTextureCube("src/texture/cube/skybox/");
				ImGui::SameLine();
				if (ImGui::Button("Colored"))
					textureCube = LoadTextureCube("src/texture/cube/colored/");
			}

			ImGui::Text(" ");
			ImGui::Separator;
			ImGui::Text("%d object, MAX 10	", scene.size());
			ImGui::SameLine();
			if (scene.size() < 10) {
				if (ImGui::Button("Create object")) {
					object_on_scene newObject;

					newObject.shape = available_models(choosemodel);
					scene.push_back(newObject);
				}
			}
			else {
				ImGui::Text("Limit reached");
			}
			

			ImGui::Text(" ");
			ImGui::Checkbox("Move camera", &cameraMovement);
			if (cameraMovement == false)
				camera.Position = glm::vec3(0.0f, 0.0f, 7.0f);

			ImGui::SliderFloat("x", &camera.Position.x, -5.0f, 5.0f);
			ImGui::SliderFloat("y", &camera.Position.y, -5.0f, 5.0f);
			ImGui::SliderFloat("z", &camera.Position.z, -5.0f, 5.0f);

			if (ImGui::Button("Print values"))
			{
				std::cout << "-------------" << endl;

				PrintCurrentShader(current_program); 
			}
		
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		}
		ImGui::End();
		
		int choose;
		vector <int> toRemove;
		string label;
		//static ImVec4 color = ImColor(114, 144, 154, 200);
		glm::vec4 color = glm::vec4(1.0);
		//gui for each object
		for (int i = 0; i < scene.size(); i++) {
			label = "Object GUI - " + std::to_string(i);
			ImGui::Begin(label.c_str());

			ImGui::Text("Shape");
			ImGui::Combo("Shape", &scene[i].shape, "Sphere\0Cube\0Torus");
			

			ImGui::Checkbox("Moving", &scene[i].isMoving);
			if (scene[i].isMoving && ImGui::CollapsingHeader("Change movement"))
			{
				ImGui::SliderFloat("x", &scene[i].movement.x, -5.0f, 5.0f);
				ImGui::SliderFloat("y", &scene[i].movement.y, -5.0f, 5.0f);
				ImGui::SliderFloat("z", &scene[i].movement.z, -5.0f, 5.0f);
			}
			if (ImGui::CollapsingHeader("Scale & position")) {
				ImGui::Text("\nScale");
				ImGui::SliderFloat("Scale", &scene[i].scale, 0.0f, 5.0f);

				ImGui::Text("\nPosition");
				ImGui::SliderFloat("x", &scene[i].position.x, -5.0f, 5.0f);
				ImGui::SliderFloat("y", &scene[i].position.y, -5.0f, 5.0f);
				ImGui::SliderFloat("z", &scene[i].position.z, -5.0f, 5.0f);
			}

			ImGui::Checkbox("Select", &scene[i].select);
			ImGui::Text("Color:");
		
			int misc_flags = (ImGuiColorEditFlags_NoOptions);

			color = glm::vec4(scene[i].color, 1.0); 
			ImGui::ColorEdit3("MyColor##1", (float*)&color, misc_flags);
			scene[i].color.x = color.x;
			scene[i].color.y = color.y;
			scene[i].color.z = color.z;

			ImGui::Combo("Operation", &scene[i].op, "Union\0Subtraction\0Intersection");

			ImGui::Checkbox("Spinning", &scene[i].spinning);
			ImGui::Checkbox("Wireframe", &scene[i].wireframe);
			
			if (ImGui::Button("Delete Model"))
				toRemove.push_back(i);
			
			ImGui::End();
		}
		for (int i = 0; i < toRemove.size(); i++)
			scene.erase(scene.begin() + toRemove[i]);
		toRemove.clear();
	

		ImGui::Render();
		ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
	
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
	Shader shader0("src/shaders/reflect.vert", "src/shaders/bubble.frag");
	shaders.push_back(shader0);
	Shader shader1("src/shaders/reflect.vert", "src/shaders/rayMarching.frag");
	shaders.push_back(shader1);
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
	std::cout << "Current shader: " << print_available_ShaderPrograms[shader] << std::endl;
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
	//camera.ProcessMouseMovement(xoffset, yoffset);
}

/////////////////////////////////////////
void CameraMovement(GLfloat frame) {
	float angle = 0.5 * frame;

	camera.Position = glm::vec3(0.0f, 5.0f, 0.0f) + glm::vec3(10.0 * sin(angle), 1.0,
														    10.0 * cos( angle));
															//5 * cos(angle));

	//camera.Position += vec2(sin(time) * 2., cos(time) * 2.);
}
