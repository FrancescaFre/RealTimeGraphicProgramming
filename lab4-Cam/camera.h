#pragma once

#include <vector>

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//enum per le direzioni della camera
enum Camera_Movement
{
  FORWARD,
  BACKWARD,
  LEFT,
  RIGHT
};

//costanti per la rotazione della camera
const GLfloat YAW = -90.0f;       //YAW = attorno all'asse Y,
const GLfloat PITCH = -90.0f;    //PITCH=rotazione attorno all'asse x
                                //ROLL= rotazione attorno all'asse z
const GLfloat SPEED = 3.0f;        //velocità di rotazione
const GLfloat SENSITIVITY = 0.25f; //quanto pesa il movimento del mouse

class Camera
{
  public:

  //attributi
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 WorldFront;
    glm::vec3 Up;
    glm::vec3 WorldUp;
    glm::vec3 Right;
    glm::vec3 WorldRigth;

    GLboolean onGround;
    GLfloat Yaw;
    GLfloat Pitch;
    GLfloat MovementSpeed;
    GLfloat MouseSensitivity;

    //costruttore
    Camera(glm::vec3 position, GLboolean on_ground)
    {
      this-> Position = position;
      this-> WorldUp = glm::vec3 (0.0f, 1.0f, 0.0f); //l'asse Y

      this-> Yaw = YAW; //la costante
      this-> Pitch = PITCH; //la costante

      this-> onGround = on_ground;

      this-> MovementSpeed = SPEED;
      this-> MouseSensitivity = SENSITIVITY;
      this->updateCameraVectors();
    }

    //tre metodi pubblici che verranno chamati dal main application
    glm::mat4 GetViewMatrix() //
    {
      return glm::lookAt(this->Position, this->Position+Front, this->Up); //ritorna un mat4, il look at vuole: posizione, dove si sta guardando e il vettore up
                                                                        //il FRONT viene dato dal YAW rotation(??)
    }

    void ProcessKeyBoard(Camera_Movement direction, GLfloat deltaTime)
    {
      GLfloat velocity = this-> MovementSpeed*deltaTime; //per aver un movimento costante e smooth
      if (direction == FORWARD) //forward dell'enum
          this->Position+=  (this->onGround ? this->WorldFront : this->Front) * velocity;//funziona per le superfici piatte

      if (direction == BACKWARD) //forward dell'enum
          this->Position-=  (this->onGround ? this->WorldFront : this->Front) * velocity;//funziona per le superfici piatte

      if (direction == LEFT)
          this->Position-= this->Right * velocity;

      if (direction == RIGHT)
          this->Position+= this->Right * velocity;
    }

    void ProcessMouseMovement(GLfloat xoffset, GLfloat yoffset, GLboolean constraintPitch = GL_TRUE)
    {
      //offset: è la differenza della posizione a tempo t-1 e al tempo t (attuale) sia sull'asse x, sia sull'asse y

      xoffset *= this->MouseSensitivity;
      yoffset *= this->MouseSensitivity;

      this-> Yaw += xoffset;
      this-> Pitch += yoffset;

      if(constraintPitch)
      {
        if(this->Pitch > 89.0f) //serve per evitare il gimbal lock
          this->Pitch = 89.0f;

        if(this->Pitch < -89.0f) //serve per evitare il gimbal lock
          this->Pitch = -89.0f;
      }

      this->updateCameraVectors();
    }


  private:
    //solo un metodo, è l' updateCameraVectors: serve per aggiornare i local coordinate, non cambia quando si muove, ma cambia quando viene ruotata
    void updateCameraVectors()
    {
      glm::vec3 front;

      // PITCH
      // y
      // |  /             x = cos(Pitch)
      // | / PITCH        y = sen(Pitch)
      // |/------x/z      z = cos(Pitch)
      //
      // YAW
      // z
      // |  /         x = cos(YAW)
      // | / YAW      y = 1
      // |/------x    z = sin(YAW)

      //cos e sen vogliono i radianti
      front.x = cos(glm::radians(this->Yaw)) * cos(glm::radians(this->Pitch));
      front.y = sin(glm::radians(this->Pitch));
      front.x = sin(glm::radians(this->Yaw)) * cos(glm::radians(this->Pitch));

      //per il word front, se ho il onground e non voglio volare, setto la y a 0
      this->WorldFront=this->Front = glm::normalize(front);
      this->WorldFront.y = 0.0f;
//      this->Front = glm::normalize(front);

      //per avere il local up, per creare il piano con la direzione del front,
      //quindi per trovare l'ortogonale al front, uso il cross prod tra front e world up
      this->Right = glm::normalize(glm::cross(this->Front, this->WorldUp));
      this->Up = glm::normalize(glm::cross(this->Right, this->Front));
    }
};
