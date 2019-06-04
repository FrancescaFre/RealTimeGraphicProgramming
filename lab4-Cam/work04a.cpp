/*
work04a

N.B. 1) The presented shader swapping method is used in OpenGL 3.3 .
        For OpenGL 4.x, shader subroutines can be used : http://www.geeks3d.com/20140701/opengl-4-shader-subroutines-introduction-3d-programming-tutorial/

N.B. 2) we have considered point lights only, the code must be modifies in case of lights of different nature

N.B. 3) there are other methods (more efficient) to pass multiple data to the shaders (search for Uniform Buffer Objects)

N.B. 4) with last versions of OpenGL, using structures like the one cited above, it is possible to pass a dynamic number of lights

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2018/2019
Master degree in Computer Science
Universita' degli Studi di Milano
*/

/*
OpenGL coordinate system (right-handed)
positive X axis points right
positive Y axis points up
positive Z axis points "outside" the screen


                              Y
                              |
                              |
                              |________X
                             /
                            /
                           /
                          Z
*/

#ifdef _WIN32
    #define __USE_MINGW_ANSI_STDIO 0
#endif
// Std. Includes
#include <string>

// Loader for OpenGL extensions
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

// classes developed during lab lectures to manage shaders and to load models
#include <utils/shader_v1.h>
#include <utils/model_v1.h>
#include <utils/camera.h>

// we load the GLM classes used in the application
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stb_image/stb_image.h>

// number of lights in the scene
#define NR_LIGHTS 3


// dimensions of application's window
GLuint screenWidth = 800, screenHeight = 600;

// callback functions for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback (GLFWwindow* window, double xpos, double ypos);
void apply_camera_movements();

// setup of Shader Programs for the 5 shaders used in the application
void SetupShaders();
// delete Shader Programs whan application ends
void DeleteShaders();
// print on console the name of current shader
void PrintCurrentShader(int shader);

GLint LoadTexture(const char* path);

//global variable
bool keys[1024];
//setta la posizione iniziale del mouse
GLfloat lastX = 400.0f, lastY = 300.0f;
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
enum available_ShaderPrograms{ BLINN_PHONG_ML, GGX_ML};
// strings with shaders names to print the name of the current one on console
const char * print_available_ShaderPrograms[] = { "BLINN-PHONG-ML", "GGX-ML"};

Camera camera (glm::vec3 (0.0f, 0.0f, 7.0f), GL_TRUE);
// index of the current shader (= 0 in the beginning)
GLuint current_program = BLINN_PHONG_ML;
// a vector for all the Shader Programs used and swapped in the application
vector<Shader> shaders;

// Uniforms to pass to shaders
// pointlights positions
glm::vec3 lightPositions[] = {
    glm::vec3(5.0f, 10.0f, 10.0f),
    glm::vec3(-5.0f, 10.0f, 10.0f),
    glm::vec3(5.0f, 10.0f, -10.0f),
};

// diffusive, specular and ambient components
// GLfloat diffuseColor[] = {1.0f,0.0f,0.0f};
GLfloat specularColor[] = {1.0f,1.0f,1.0f};
GLfloat ambientColor[] = {0.1f,0.1f,0.1f};
// weights for the diffusive, specular and ambient components
GLfloat Kd = 0.8f;
GLfloat Ks = 0.5f;
GLfloat Ka = 0.1f;
// shininess coefficient for Blinn-Phong shader
GLfloat shininess = 25.0f;

// attenuation parameters for Blinn-Phong shader
GLfloat constant = 1.0f;
GLfloat linear = 0.02f;
GLfloat quadratic = 0.001f;

// roughness index for Cook-Torrance shader
GLfloat alpha = 0.2f;
// Fresnel reflectance at 0 degree (Schlik's approximation)
GLfloat F0 = 0.9f;

// color to be passed as uniform to the shader of the plane
// GLfloat planeMaterial[] = {0.0f,0.5f,0.0f};
vector<GLint> textureID; //GL usa un id per le texture, quindi se ho bisogno di una txure, faccio texture-id activate
GLfloat repeat = 1.0f; 

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
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "RGP_lecture03c", nullptr, nullptr);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // we put in relation the window and the callbacks
    glfwSetKeyCallback(window, key_callback);

    // GLAD tries to load the context set by GLFW
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
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

    // we create the Shader Program used for the plane (fixed)
    Shader plane_shader("15_phong_multiplelights.vert", "16_blinnphong_multiplelights.frag");

    // we load the model(s) (code of Model class is in include/utils/model_v2.h)
    Model cubeModel("../../../models/cube.obj");
    Model sphereModel("../../../models/sphere.obj");
    Model bunnyModel("../../../models/bunny_lp.obj");
    Model planeModel("../../../models/plane.obj");

//devo caricare le texture
    textureID.push_back(LoadTexture("../../../texture/UV_Grid_Sm.png")); //local path rispetto alla cartella di esecuzione
    textureID.push_back(LoadTexture("../../../texture/SoilCracked.png")); //local path rispetto alla cartella di esecuzione
  

    // we print on console the name of the first shader used
    PrintCurrentShader(current_program);

    // Projection matrix: FOV angle, aspect ratio, near and far planes
    glm::mat4 projection = glm::perspective(45.0f, (float)screenWidth/(float)screenHeight, 0.1f, 10000.0f);
    // View matrix (=camera): position, view direction, camera "up" vector
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 7.0f), glm::vec3(0.0f, 0.0f, -7.0f), glm::vec3(0.0f, 1.0f, 7.0f));


    // Rendering loop: this code is executed at each frame
    while(!glfwWindowShouldClose(window))
    {
        // we determine the time passed from the beginning
        // and we calculate time difference between current frame rendering and the previous one
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Check is an I/O event is happening
        glfwPollEvents();

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
            orientationY+=(deltaTime*spin_speed);

        /////////////////// PIANO ////////////////////////////////////////////////
        // We render a plane under the objects. We apply the fullcolor shader to the plane, and we do not apply the rotation applied to the other objects.
        plane_shader.Use();
        glActiveTexture(GL_TEXTURE1); //1 è per il soilcracked
        glBindTexture(GL_TEXTURE_2D, textureID[1]);
        
        // we pass projection and view matrices to the Shader Program of the plane
        glUniformMatrix4fv(glGetUniformLocation(plane_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(plane_shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));

        // we determine the position in the Shader Program of the uniform variables
        GLint textureLocation = glGetUniformLocation(plane_shader.Program, "tex");
        GLint repeatLocation = glGetUniformLocation(plane_shader.Program, "repeat");
        
        
        GLint matAmbientLocation = glGetUniformLocation(plane_shader.Program, "ambientColor");
        GLint matSpecularLocation = glGetUniformLocation(plane_shader.Program, "specularColor");
      //  GLint matDiffuseLocation = glGetUniformLocation(plane_shader.Program, "diffuseColor");
        GLint kaLocation = glGetUniformLocation(plane_shader.Program, "Ka");
        GLint kdLocation = glGetUniformLocation(plane_shader.Program, "Kd");
        GLint ksLocation = glGetUniformLocation(plane_shader.Program, "Ks");
        GLint shineLocation = glGetUniformLocation(plane_shader.Program, "shininess");
        GLint constantLocation = glGetUniformLocation(plane_shader.Program, "constant");
        GLint linearLocation = glGetUniformLocation(plane_shader.Program, "linear");
        GLint quadraticLocation = glGetUniformLocation(plane_shader.Program, "quadratic");

        // we pass each light position to the shader
        for (GLuint i = 0; i < NR_LIGHTS; i++)
        {
            string number = to_string(i);
            glUniform3fv(glGetUniformLocation(plane_shader.Program, ("lights[" + number + "]").c_str()), 1, glm::value_ptr(lightPositions[i]));
        }

        // we assign the value to the uniform variables
        glUniform1i(textureLocation, 1);
        glUniform1f(repeatLocation, 80.0f);
       
        glUniform3fv(matAmbientLocation, 1, ambientColor);        
        //glUniform3fv(matDiffuseLocation, 1, planeMaterial);
        glUniform3fv(matSpecularLocation, 1, specularColor);
        glUniform1f(kaLocation, Ka);
        glUniform1f(ksLocation, 0.0f);
        glUniform1f(kdLocation, Kd);
        glUniform1f(shineLocation, 1.0f);
        glUniform1f(constantLocation, constant);
        glUniform1f(linearLocation, linear);
        glUniform1f(quadraticLocation, quadratic);

        // we create the transformation matrix by defining the Euler's matrices, and the matrix for normals transformation
        glm::mat4 planeModelMatrix;
        glm::mat3 planeNormalMatrix;
        planeModelMatrix = glm::translate(planeModelMatrix, glm::vec3(0.0f, -1.0f, 0.0f));
        planeModelMatrix = glm::scale(planeModelMatrix, glm::vec3(10.0f, 1.0f, 10.0f));
        planeNormalMatrix = glm::inverseTranspose(glm::mat3(view*planeModelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(plane_shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(planeModelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(plane_shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(planeNormalMatrix));

        // we render the plane
        planeModel.Draw();


        /////////////////// OBJECTS ////////////////////////////////////////////////
        // We "install" the selected Shader Program as part of the current rendering process
        shaders[current_program].Use();
        glActiveTexture(GL_TEXTURE0); //1 è per il soilcracked
        glBindTexture(GL_TEXTURE_2D, textureID[0]);
       
       textureLocation = glGetUniformLocation(shaders[current_program].Program, "tex"); 
       repeatLocation = glGetUniformLocation(shaders[current_program].Program, "repeat"); 
    
        glUniform1i(textureLocation, 0);
        glUniform1f(repeatLocation, repeat); 
        
        // we determine the position in the Shader Program of the uniform variable
        kdLocation = glGetUniformLocation(shaders[current_program].Program, "Kd");
      //  matDiffuseLocation = glGetUniformLocation(shaders[current_program].Program, "diffuseColor");
        kdLocation = glGetUniformLocation(shaders[current_program].Program, "Kd");

        // we assign the value to the uniform variable
        glUniform1f(kdLocation, Kd);
       // glUniform3fv(matDiffuseLocation, 1, diffuseColor);
        glUniform1f(kdLocation, Kd);


        // we pass each light position to the shader
        for (GLuint i = 0; i < NR_LIGHTS; i++)
        {
            string number = to_string(i);
            glUniform3fv(glGetUniformLocation(shaders[current_program].Program, ("lights[" + number + "]").c_str()), 1, glm::value_ptr(lightPositions[i]));
        }

        // the other uniforms are passed only to the corresponding shader
        if (current_program == BLINN_PHONG_ML)
        {
            // we determine the position in the Shader Program of the uniform variable
            GLint matAmbientLocation = glGetUniformLocation(shaders[current_program].Program, "ambientColor");
            GLint matSpecularLocation = glGetUniformLocation(shaders[current_program].Program, "specularColor");
            GLint kaLocation = glGetUniformLocation(shaders[current_program].Program, "Ka");
            GLint ksLocation = glGetUniformLocation(shaders[current_program].Program, "Ks");
            GLint shineLocation = glGetUniformLocation(shaders[current_program].Program, "shininess");
            GLint constantLocation = glGetUniformLocation(shaders[current_program].Program, "constant");
            GLint linearLocation = glGetUniformLocation(shaders[current_program].Program, "linear");
            GLint quadraticLocation = glGetUniformLocation(shaders[current_program].Program, "quadratic");

            // we assign the value to the uniform variable
            glUniform3fv(matAmbientLocation, 1, ambientColor);
            glUniform3fv(matSpecularLocation, 1, specularColor);
            glUniform1f(kaLocation, Ka);
            glUniform1f(ksLocation, Ks);
            glUniform1f(shineLocation, shininess);
            glUniform1f(constantLocation, constant);
            glUniform1f(linearLocation, linear);
            glUniform1f(quadraticLocation, quadratic);

        }
        if (current_program == GGX_ML)
        {
            // we determine the position in the Shader Program of the uniform variable
            GLint alphaLocation = glGetUniformLocation(shaders[current_program].Program, "alpha");
            GLint f0Location = glGetUniformLocation(shaders[current_program].Program, "F0");
            // we assign the value to the uniform variable
            glUniform1f(alphaLocation, alpha);
            glUniform1f(f0Location, F0);
        }

        // we pass projection and view matrices to the Shader Program
        glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));

        //SPHERE
        /*
          we create the transformation matrix by defining the Euler's matrices

          N.B.) the last defined is the first applied

          We need also the matrix for normals transformation, which is the inverse of the transpose of the 3x3 submatrix (upper left) of the modelview. We do not consider the 4th column because we do not need translations for normals.
          An explanation (where XT means the transpose of X, etc):
            "Two column vectors X and Y are perpendicular if and only if XT.Y=0. If We're going to transform X by a matrix M, we need to transform Y by some matrix N so that (M.X)T.(N.Y)=0. Using the identity (A.B)T=BT.AT, this becomes (XT.MT).(N.Y)=0 => XT.(MT.N).Y=0. If MT.N is the identity matrix then this reduces to XT.Y=0. And MT.N is the identity matrix if and only if N=(MT)-1, i.e. N is the inverse of the transpose of M.
        */
        glm::mat4 sphereModelMatrix;
        glm::mat3 sphereNormalMatrix;
        sphereModelMatrix = glm::translate(sphereModelMatrix, glm::vec3(-3.0f, 0.0f, 0.0f));
        sphereModelMatrix = glm::rotate(sphereModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
        sphereModelMatrix = glm::scale(sphereModelMatrix, glm::vec3(0.8f, 0.8f, 0.8f));
        // if we cast a mat4 to a mat3, we are automatically considering the upper left 3x3 submatrix
        sphereNormalMatrix = glm::inverseTranspose(glm::mat3(view*sphereModelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(sphereModelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(shaders[current_program].Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(sphereNormalMatrix));

        // we render the sphere
        sphereModel.Draw();

        //CUBE
        // we create the transformation matrix by defining the Euler's matrices, and the normals transformation matrix
        glm::mat4 cubeModelMatrix;
        glm::mat3 cubeNormalMatrix;
        cubeModelMatrix = glm::translate(cubeModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
        cubeModelMatrix = glm::rotate(cubeModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
        cubeModelMatrix = glm::scale(cubeModelMatrix, glm::vec3(0.8f, 0.8f, 0.8f));
        cubeNormalMatrix = glm::inverseTranspose(glm::mat3(view*cubeModelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(cubeModelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(shaders[current_program].Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(cubeNormalMatrix));

        // we render the cube
        cubeModel.Draw();

        //BUNNY
        // we create the transformation matrix by defining the Euler's matrices, and the normals transformation matrix
        glm::mat4 bunnyModelMatrix;
        glm::mat3 bunnyNormalMatrix;
        bunnyModelMatrix = glm::translate(bunnyModelMatrix, glm::vec3(3.0f, 0.0f, 0.0f));
        bunnyModelMatrix = glm::rotate(bunnyModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
        bunnyModelMatrix = glm::scale(bunnyModelMatrix, glm::vec3(0.3f, 0.3f, 0.3f));
        bunnyNormalMatrix = glm::inverseTranspose(glm::mat3(view*bunnyModelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(bunnyModelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(shaders[current_program].Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(bunnyNormalMatrix));

        // we render the bunny
        bunnyModel.Draw();

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

GLint LoadTexture(const char*path)
{
    GLuint textureImage; 
    int w,h,channels; 
    unsigned char *image; 
    
    image=stdi_load(path, &w, &h, &channels, STBI_rgb); //w= righe dell'immagine, h= colonne dell'immagine, channel=rgb
    if(image == nullptr)
        std::cout<<"WRONG PATH "<<std::endl;
    
    glGenTextures(1,&textureImage); //1 è il texture id
    glBindTexture(GL_TEXTURE_2D, textureImage); //ho fatto il bind di un id (il primo, 1), ad un nome textureImage
    if(channels == 3)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        //crea un'immagine di tipo gltexture2d (target), level of detail (level), gl_rgb(internal format), size (width), size (height), (border), gl_rgb(format), unsigned byte(type), (*data)
    else if(channels == 4)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); //minification
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST); //magnification
    
    stbi_image_free(image); //elimino la struttura dati perchè già caricata
    
    return textureImage; 
    
}


//////////////////////////////////////////
// we create and compile shaders (code of Shader class is in include/utils/shader_v1.h), and we add them to the list of available shaders
void SetupShaders()
{
  // we create the Shader Programs (code in shader_v1.h)
    Shader shader1("15_phong_multiplelights.vert", "16_blinnphong_multiplelights.frag");
    shaders.push_back(shader1);
    Shader shader2("15_phong_multiplelights.vert", "17_ggx_multiplelights.frag");
    shaders.push_back(shader2);
}

/////////////////////////////////////////
// we delete all the Shaders Programs
void DeleteShaders()
{
    for(GLuint i = 0; i < shaders.size(); i++)
        shaders[i].Delete();
}

//////////////////////////////////////////
// we print on console the name of the currently used shader
void PrintCurrentShader(int shader)
{
    std::cout << "Current shader:" << print_available_ShaderPrograms[shader]  << std::endl;

}

//////////////////////////////////////////
// callback for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    // if ESC is pressed, we close the application
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // if P is pressed, we start/stop the animated rotation of models
    if(key == GLFW_KEY_P && action == GLFW_PRESS)
        spinning=!spinning;

    // if L is pressed, we activate/deactivate wireframe rendering of models
    if(key == GLFW_KEY_L && action == GLFW_PRESS)
        wireframe=!wireframe;

    // pressing a key between 1 and 2, we change the shader applied to the models
    if((key >= GLFW_KEY_1 && key <= GLFW_KEY_2) && action == GLFW_PRESS)
    {
        // "1" to "2" -> ASCII codes from 49 to 50
        // we subtract 48 (= ASCII CODE of "0") to have integers from 1 to 2
        // we subtract 1 to have indices from 0 to 1 in the shaders list
        current_program = (key-'0'-1);
        PrintCurrentShader(current_program);
   //serve per gestire se i tasti sono premuti e rilsciati
    if (action == GLFW_PRESS)
      keys[key] = true; 
      
    if (action == GLFW_RELEASE)
      keys[key] = false;      
    }
}
    //aggiungo i tasi wasd come tasti da rilevare, non si supporta la diagonale
void aplly_camera_movements()
{
  if (keys[GLFW_KEY_W])
    camera.ProcessKeyBoard(FORWARD, deltaTime);

  if (keys[GLFW_KEY_S])
    camera.ProcessKeyBoard(BACKWARD, deltaTime);

  if (keys[GLFW_KEY_A])
    camera.ProcessKeyBoard(LEFT, deltaTime);

  if (keys[GLFW_KEY_S])
    camera.ProcessKeyBoard(RIGHT, deltaTime);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
  if (firstMouse)
  { //inizializzo correttamente al primo frame la Camera
    lastX = xpos; 
    lastY = ypos; 
    firstMouse = false; 
  }
  
  GLfloat xoffset = xpos-lastX; 
  GLfloat yoffset = lastY-ypos; 

  lastX = xpos; 
  lastY = ypos; 

  camera.ProcessMouseMovement(xoffset, yoffset);
}

