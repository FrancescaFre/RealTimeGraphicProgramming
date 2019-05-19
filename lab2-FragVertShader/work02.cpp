/*
Es01: Opengl basic application v1
- class for OBJ models loading using Assimp library
- creation and initialization VBO, VAO and EBO
  - VBO : Vertex Buffer Object
  - VAO : Vertex Array Object
  - EBO : Element Buffer Object
- class for loading shaders source code and Program Shader creation

N.B.) no texturing in this version of the classes

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

// we load the GLM classes used in the application
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp> //per la trasformazione delle normal
#include <glm/gtc/type_ptr.hpp>

// dimensions of application's window
GLuint screenWidth = 800, screenHeight = 600;

// callback function for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

GLfloat deltaTime = 0.0f;   //var globali
GLfloat lastFrame = 0.0f;

GLfloat orientationY = 0.0f;
GLfloat spin_speed = 30.0f;

GLboolean spinning = GL_TRUE;
GLboolean wireframe = GL_FALSE; //sono i true o false settati da openGL, come convenzione si usano questi per evitare problemi

GLfloat myColor[] = {1.0f, 0.0f, 1.0f};
GLfloat myColor1[] = {1.0f, 0.0f, 1.0f};
GLfloat myColor2[] = {0.0f, 0.0f, 1.0f};
GLfloat repeat = 5.0f;
GLfloat weight = 0.2f;
GLfloat speed = 0.5f; //parametri per lo shader


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
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "RTGP_lecture01", nullptr, nullptr);
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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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

    // we create and compile shaders (code of Shader class is in include/utils/shader_v1.h)
    Shader shader("00_basic.vert", "00_basic.frag");

    // we load the model(s) (code of Model class is in include/utils/model_v1.h)
    Model cubeModel("../../../models/cube.obj");  //quesi sono costruttori
    Model sphereModel("../../../models/sphere.obj");
    Model bunnyModel("../../../models/bunny_lp.obj");

    // We "install" the Shader Program as part of the current rendering process
    // with only one Shader Program, we can do this call now, and not inside the main loop:
    // we will use this Shader Program for everything is rendered after this call
    shader.Use();   // <-- Don't forget this one!

    // we set projection and view matrices
     //++++ la camera viene messa fuori dal rendering loop se è fissata,
     //++++ attenzione che nei casi in cui la telecamera è da fps,
     //++++ è da mettere all'interno del rendering loop perchè i valori cambiano di continuo
    // Projection matrix: FOV angle, aspect ratio, near and far planes
    glm::mat4 projection = glm::perspective(45.0f, (float)screenWidth/(float)screenHeight, 0.1f, 10000.0f);
    // View matrix (=camera): position, view direction, camera "up" vector --> il lookAt genera un vettore di 4 elementi prendendo in input pos, view e up
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 7.0f), glm::vec3(0.0f, 0.0f, -7.0f), glm::vec3(0.0f, 1.0f, 7.0f));

    // Rendering loop: this code is executed at each frame
  //  glClearColor(0.05f, 0.05f, 0.05f, 1.0f); //pulisce dal buffer il colore (????)
    glClearColor(0.26f, 0.45f, 0.98f, 1.0f); //pulisce dal buffer il colore (????)

    while(!glfwWindowShouldClose(window)) //questo è il rendering loop
    {
        //glfw sono le lib che gestiscono i frame di openGL
        GLfloat currentFrame = glfwGetTime(); //metodo di glfw che restituisce il tempo corrente, tempo passato dall'inizio dell'esecuzione del programma
        //sono le var dichiarate prima
        deltaTime = currentFrame - lastFrame; //così al primo ciclo, è current frame - 0
        lastFrame = currentFrame;

        // Check is an I/O event is happening
        glfwPollEvents();

        // we "clear" the frame buffer, setting an initial color
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //settaggio di cose riguardanti lo spinning e il wireframe
        //spiego come deve essere renderizzato il poligono, con wireframe true = mostro le linee, con wireframe false = riempio la superficie
        if(wireframe)
          glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
          glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        if(spinning)
          orientationY += deltaTime*spin_speed;
        //se mettessi l'else, fermando la rotazione si azzererebbe la rotazione
        //else orientationY = 0.0f

        GLint fragColorLocation1 = glGetUniformLocation(shader.Program, "color1");
        glUniform3fv(fragColorLocation1, 1, myColor1); //f=float v=pointing to the variable
        GLint fragColorLocation2 = glGetUniformLocation(shader.Program, "color2");
        glUniform3fv(fragColorLocation2, 1, myColor2); //f=float v=pointing to the variable

        GLint repeatLocation = glGetUniformLocation(shader.Program, "repeat");
        glUniform1f(repeatLocation, repeat);

        GLint weightLocation = glGetUniformLocation(shader.Program, "weight");
        glUniform1f(weightLocation, weight);

        GLint timerLocation = glGetUniformLocation(shader.Program, "timer");
        glUniform1f(timerLocation, currentFrame*speed);

        // we pass the matrices as uniform variables to the shaders
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));

        // we create the transformation matrix by defining the Euler's matrices
        // N.B.) last defined is the first applied
        glm::mat4 cubeModelMatrix;
        cubeModelMatrix = glm::translate(cubeModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
        cubeModelMatrix = glm::rotate(cubeModelMatrix, glm::radians (orientationY), glm::vec3(0.0f, 1.0f, 0.0f));	//glm vuole i radianti, quindi si usa la funzione, infine si mette l'asse su cui deve girare, y
        cubeModelMatrix = glm::scale(cubeModelMatrix, glm::vec3(0.8f, 0.8f, 0.8f));	// It's a bit too big for our scene, so scale it down

        /**ho bisogno della gestione delle normali, dobbiamo aplicare alla mesh delle trasformazioni,
         la mesh ha delle normali, anche le normali devono cambiare insieme alla mesh, perchè devono essere sempre ortogonali alla mesh (angolo retto)
         Quindi per modificarla devo cambiare la rotazione, definendo la metrica della rotazione della mesh sulla normale, è linversal of traspose of 3x3 mat of 4x4 mat
        |* * * -|
        |* * * -| --> x^T*y = 0 --> (Mx)^T * (Ny) = 0
        |* * * -|            (A*B)^T = B^T * A^T (attenzione all'ordine perchè non è commutativo)
        |- - - -|    == (x^T*M^T)*(Ny) = 0 --> x^T (M^T*N) * y = 0 --> N=(M^T)^-1 //non ho capito perchè y e x spariscono
        **/
        glm::mat3 cubeNormalMatrix;
        cubeNormalMatrix = glm::inverseTranspose(glm::mat3(view*cubeModelMatrix)); //mi serve in view model coordinate, e faccio un cast per prendere la sub matrix 3x3 definita sorpra

        // we pass the matrix as uniform variable to the shaders
        //ho 3 modelli, quindi devo farlo più volte
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(cubeModelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(cubeNormalMatrix));
        //normalMatrix indica il nome della variabile dentro lo shader per indicare cosa usare --> si trova in basic.vert
        //il ptr è il puntatore all'applicazione, quindi serve per fare un collegamento tra shader e progrmama

        // we render the model
        //le trasformazioi aplicate: ricordarsi che l'ordine è: TRANSLATE - SCALE - ROTATION, cioè da destra a sinistra della matrice di trasformazione
        cubeModel.Draw();
 //
        //-------- copypaste per sphere
        glm::mat4 sphereModelMatrix;
        sphereModelMatrix = glm::translate(sphereModelMatrix, glm::vec3(3.0f, 0.0f, 0.0f));
        sphereModelMatrix = glm::rotate(sphereModelMatrix, glm::radians (orientationY), glm::vec3(0.0f, 1.0f, 0.0f));	//glm vuole i radianti, quindi si usa la funzione, infine si mette l'asse su cui deve girare, y
        sphereModelMatrix = glm::scale(sphereModelMatrix, glm::vec3(0.8f, 0.8f, 0.8f));	// It's a bit too big for our scene, so scale it down

        glm::mat3 sphereNormalMatrix;
        sphereNormalMatrix = glm::inverseTranspose(glm::mat3(view*sphereModelMatrix)); //mi serve in view model coordinate, e faccio un cast per prendere la sub matrix 3x3 definita sorpra

        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(sphereModelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(sphereNormalMatrix));
        sphereModel.Draw();
        //-------- copypaste per bunny
        glm::mat4 bunnyModelMatrix;
        bunnyModelMatrix = glm::translate(bunnyModelMatrix, glm::vec3(-3.0, 0.0f, 0.0f));
        bunnyModelMatrix = glm::rotate(bunnyModelMatrix, glm::radians (orientationY), glm::vec3(0.0f, 1.0f, 0.0f));	//glm vuole i radianti, quindi si usa la funzione, infine si mette l'asse su cui deve girare, y
        bunnyModelMatrix = glm::scale(bunnyModelMatrix, glm::vec3(0.3f, 0.3f, 0.3f));	// It's a bit too big for our scene, so scale it down

        glm::mat3 bunnyNormalMatrix;
        bunnyNormalMatrix = glm::inverseTranspose(glm::mat3(view*bunnyModelMatrix)); //mi serve in view model coordinate, e faccio un cast per prendere la sub matrix 3x3 definita sorpra

        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(bunnyModelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(bunnyNormalMatrix));
        bunnyModel.Draw();

        // Swapping back and front buffers
        glfwSwapBuffers(window);
    }

    // when I exit from the graphics loop, it is because the application is closing
    // we delete the Shader Program
    shader.Delete();
    // we close and delete the created context
    glfwTerminate();
    return 0;
}

//////////////////////////////////////////
// callback for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    // if ESC is pressed, we close the application
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // if P is pressed, we spinning the models in the application
    if(key == GLFW_KEY_P && action == GLFW_PRESS)
        spinning = !spinning;

    // if P is pressed, we show wire frame of the models in the application
    if(key == GLFW_KEY_L && action == GLFW_PRESS)
        wireframe = !wireframe;

}
