/*
Es02b: Procedural shaders 1 (regular patterns)
- procedural shaders for 2 regular patterns, with and without antialiasing, pressing keys from 1 to 6

N.B. 1) The presented shader swapping method is used in OpenGL 3.3 .
        For OpenGL 4.x, shader subroutines can be used : http://www.geeks3d.com/20140701/opengl-4-shader-subroutines-introduction-3d-programming-tutorial/

N.B. 2) no texturing in this version of the classes

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

// classes developed during lab lectures to manage shaders and to load models
#include <utils/shader_v1.h>
#include <utils/model_v1.h>

// we load the GLM classes used in the application
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

// dimensions of application's window
GLuint screenWidth = 800, screenHeight = 600;

// callback function for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// setup of Shader Programs for the 5 shaders used in the application
void SetupShaders();
// delete Shader Programs whan application ends
void DeleteShaders();
// print on console the name of current shader
void PrintCurrentShader(int shader);

// parametri per il calcolo del tempo
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
enum available_ShaderPrograms{ STRIPES, STRIPES_SMOOTHSTEP, STRIPES_AASTEP, CIRCLES, CIRCLES_SMOOTHSTEP, CIRCLES_AASTEP};
// strings with shaders names to print the name of the current one on console
const char * print_available_ShaderPrograms[] = { "STRIPES", "STRIPES-SMOOTHSTEP", "STRIPES-AASTEP", "CIRCLES", "CIRCLES-SMOOTHSTEP", "CIRCLES-AASTEP"};

// index of the current shader (= 0 in the beginning)
GLuint current_program = STRIPES;
// a vector for all the Shader Programs used and swapped in the application
vector<Shader> shaders;

// Uniforms to pass to shaders
// colors to be passed to Fullcolor and Flatten shaders
GLfloat myColor1[] = {1.0f,1.0f,0.0f};
GLfloat myColor2[] = {0.0f,0.0f,1.0f};
// number of pattern repetitions
GLfloat repeat = 5.0f;

// color to be passed as uniform to the shader of the plane
GLfloat planeColor[] = {0.0f,0.5f,0.0f};

//le unniform che servono per gli Shaders
GLfloat frequency = 10.0f;
GLfloat power = 1.0f;
GLfloat harmonics = 4.0f;

//illumination model con un singolo pointlights
//qui si definisce la posizione della luce
glm::vec3 lightPos0=glm::vec3(5.0f, 10.0f, 10.0f);
GLfloat diffuseColor[] = {1.0, 0.5, 0.0}; //rosso
GLfloat specularColor[] = {1.0, 1.0, 1.0};   //bianco
GLfloat ambientColor[] = {0.1, 0.1, 0.1};
//la somma dei pesi dovrebbe essere 1, sono i pesi che regolano la luce, peso per la diffuse, specular e ambient
GLfloat Kd = 0.8f;
GLfloat Ks = 0.5f;
GLfloat Ka = 0.1f;
//costanti
GLfloat shininess = 25.0f;
GLfloat constant = 1.0;
GLfloat linear = 0.09;
GLfloat quadratic = 0.032;

GLfloat alpha = 0.2f;
GLfloat F0 = 0.9f;
//bisogna settare il GL uniform per ogni valore della luce

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
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "RGP_work03", nullptr, nullptr);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // we put in relation the window and the callbacks
    glfwSetKeyCallback(window, key_callback);

    // we disable the mouse cursor
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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
    Shader plane_shader("00_basic.vert", "01_fullcolor.frag");

    // we load the model(s) (code of Model class is in include/utils/model_v1.h)
    Model cubeModel("../../../models/cube.obj");
    Model sphereModel("../../../models/sphere.obj");
    Model bunnyModel("../../../models/bunny_lp.obj");
    Model planeModel("../../../models/plane.obj");

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

        /////////////////// PLANE ////////////////////////////////////////////////
        // We render a plane under the objects. We apply the fullcolor shader to the plane, and we do not apply the rotation applied to the other objects.
        plane_shader.Use();
        // we pass projection and view matrices to the Shader Program of the plane
        glUniformMatrix4fv(glGetUniformLocation(plane_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(plane_shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));

        // we determine the position in the Shader Program of the uniform variables
        GLint planeColorLocation = glGetUniformLocation(plane_shader.Program, "colorIn");
        // we assign the value to the uniform variables
        glUniform3fv(planeColorLocation, 1, planeColor);

        // we create the transformation matrix by defining the Euler's matrices
        glm::mat4 planeModelMatrix;
        planeModelMatrix = glm::translate(planeModelMatrix, glm::vec3(0.0f, -1.0f, 0.0f));
        planeModelMatrix = glm::scale(planeModelMatrix, glm::vec3(10.0f, 1.0f, 10.0f));
        glUniformMatrix4fv(glGetUniformLocation(plane_shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(planeModelMatrix));

        // we render the plane
        planeModel.Draw();


        /////////////////// OBJECTS ////////////////////////////////////////////////
        // We "install" the selected Shader Program as part of the current rendering process
        shaders[current_program].Use();

        // we determine the position in the Shader Program of the uniform variable
        GLint pointLightsLocation = glGetUniformLocation(shaders[current_program].Program, "pointLightPosition");

        GLint fragColorLocation1 = glGetUniformLocation(shaders[current_program].Program, "color1");
        GLint fragColorLocation2 = glGetUniformLocation(shaders[current_program].Program, "color2");
        GLint repeatLocation = glGetUniformLocation(shaders[current_program].Program, "repeat");

        //abbiamo 4 nuove uniform, quindi 4 nuove location
        GLint frequencyLocation = glGetUniformLocation(shaders[current_program].Program, "frequency");
        GLint powerLocation = glGetUniformLocation(shaders[current_program].Program, "power");
        GLint timerLocation = glGetUniformLocation(shaders[current_program].Program, "timer");
        GLint harmonicsLocation = glGetUniformLocation(shaders[current_program].Program, "harmonics");

        //si sttano tutte le uniform della luce
        GLint matDiffuseLocation =  glGetUniformLocation(shaders[current_program].Program, "diffuseColor");
        GLint matSpecularLocation =  glGetUniformLocation(shaders[current_program].Program, "specularColor");
        GLint matAmbientLocation =  glGetUniformLocation(shaders[current_program].Program, "ambientColor");

        GLint kdLocation =  glGetUniformLocation(shaders[current_program].Program, "Kd");
        GLint ksLocation =  glGetUniformLocation(shaders[current_program].Program, "Ks");
        GLint kaLocation =  glGetUniformLocation(shaders[current_program].Program, "Ka");

        GLint shineLocation =  glGetUniformLocation(shaders[current_program].Program, "shininess");
        GLint constantLocation =  glGetUniformLocation(shaders[current_program].Program, "constant");
        GLint linearLocation =  glGetUniformLocation(shaders[current_program].Program, "linear");
        GLint quadraticLocation =  glGetUniformLocation(shaders[current_program].Program, "quadratic");

        GLint alphaLocation =  glGetUniformLocation(shaders[current_program].Program, "alpha");
        GLint F0Location =  glGetUniformLocation(shaders[current_program].Program, "F0");



        // we assign the value to the uniform variable
        glUniform3fv(fragColorLocation1, 1, myColor1);
        glUniform3fv(fragColorLocation2, 1, myColor2);
        glUniform1f(repeatLocation, repeat);

        glUniform1f(powerLocation, power);
        glUniform1f(frequencyLocation, frequency);
        glUniform1f(harmonicsLocation, harmonics);
        glUniform1f(timerLocation, currentFrame);

        //per le luci
        glUniform3fv(fragColorLocation1, 1, glm::value_ptr(lightPos0));
//anche i colori sono vect 3, non solo la posizione
        glUniform3fv(matDiffuseLocation, 1, diffuseColor);
        glUniform3fv(matSpecularLocation, 1, specularColor);
        glUniform3fv(matSpecularLocation, 1, ambientColor);

        glUniform1f(kdLocation, Kd);
        glUniform1f(ksLocation, Ks);
        glUniform1f(kaLocation, Ka);

        glUniform1f(shineLocation, shininess);
        glUniform1f(constantLocation, constant);
        glUniform1f(linearLocation, linear);
        glUniform1f(quadraticLocation, quadratic);

        glUniform1f(alphaLocation, alpha);
        glUniform1f(F0Location, F0);




        // we pass projection and view matrices to the Shader Program
        glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));

        // SPHERE
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

        // we render the model
        sphereModel.Draw();

        //CUBE
        // we create the transformation matrix by defining the Euler's matrices, and the normals transformation matrix
        glm::mat4 cubeModelMatrix;
        glm::mat3 cubeNormalMatrix;
        cubeModelMatrix = glm::translate(cubeModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
        cubeModelMatrix = glm::rotate(cubeModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
        cubeModelMatrix = glm::scale(cubeModelMatrix, glm::vec3(0.8f, 0.8f, 0.8f));	// It's a bit too big for our scene, so scale it down
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
        bunnyModelMatrix = glm::scale(bunnyModelMatrix, glm::vec3(0.3f, 0.3f, 0.3f));	// It's a bit too big for our scene, so scale it down
        bunnyNormalMatrix = glm::inverseTranspose(glm::mat3(view*bunnyModelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(bunnyModelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(shaders[current_program].Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(bunnyNormalMatrix));

        // Swapping back and front buffers
        bunnyModel.Draw();

        // Faccio lo swap tra back e front buffer
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
// we create and compile shaders (code of Shader class is in include/utils/shader_v1.h), and we add them to the list of available shaders
void SetupShaders()
{
    Shader shader1("06_procedural_base.vert", "06_stripes.frag");
    shaders.push_back(shader1);
    Shader shader2("06_procedural_base.vert", "06a_stripes_smoothstep.frag");
    shaders.push_back(shader2);
    Shader shader3("06_procedural_base.vert", "06b_stripes_aastep.frag");
    shaders.push_back(shader3);
    Shader shader4("06_procedural_base.vert", "07_circles.frag");
    shaders.push_back(shader4);
    Shader shader5("06_procedural_base.vert", "07a_circles_smoothstep.frag");
    shaders.push_back(shader5);
    Shader shader6("06_procedural_base.vert", "07b_circles_aastep.frag");
    shaders.push_back(shader6);
}

//////////////////////////////////////////
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

    // pressing a key between 1 and 6, we change the shader applied to the models
    if((key >= GLFW_KEY_1 && key <= GLFW_KEY_6) && action == GLFW_PRESS)
    {
        // "1" to "6" -> ASCII codes from 49 to 58
        // we subtract 48 (= ASCII CODE of "0") to have integers from 1 to 6
        // we subtract 1 to have indices from 0 to 4 in the shaders list
        current_program = (key-'0'-1);
        PrintCurrentShader(current_program);
    }
}
