#pragma once

using namespace std;

#include<vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

struct Vertex{
  glm::vec3 Position;
  glm::vec3 Normal;

  glm::vec3 Tangent;
  glm::vec3 Bitangent;

  glm::vec2 TexCoords;
}

class Mesh{
  public:
    vector <Vertex> verticies;
    vector <GLuint> indices;

    GLuint VAO;

    Mesh(vector <Vertex> verticies, vector <GLuint> indices)
    {
      this-> verticies = verticies;
      this-> indices = inces;

      this->setupMesh();
    }

    void Draw(Shader shader)
    {

      //VAO sono buffer, VAO è quello che gestisce gli atri glDeleteBuffers
      //bing vertex array rende attivo un buffers

      glBindVertexArray(this->VAO);
      //con questo comando, gldraw elements, disegno ciò che è presente e attivo
      glDrawElements(GL_TRIANGLES, this-> indices.size(), GL_UNSIGNED_INT, 0);
      //ora rimuovo l'oggetto dallo stato di rendering, infatti se avessi n oggetti
      glBindVertexArray(0);

      //manca una roba con gli shader
    }

    void Delete()
    {
      glDeleteVertexArrays(1, &this->VAO);  //sono locali
      glDeleteBuffers(1, &VBO);
      glDeleteBuffers(1, &EBO);
    }

  private:
    //questi due sono nascosti
    GLuint VBO, EBO;

    void setupMesh()
    {
      glDeleteVertexArrays(1, &this->VAO);  //sono locali
      glDeleteBuffers(1, &this->VBO);
      glDeleteBuffers(1, &this->EBO);

      //serve per renderlo attivo
      glBindVertexArray(this->VAO);

      //tutto quello che accade dopo il bind
      glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
      glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * izeof(Vertex), &this->vertices[0], GL_STATIC_DRAW);

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(GLuint), &this->indices[0], GL_STATIC_DRAW);

      //Serve per scrivere tutto nel VAO e
      glEnableVertexAttribArray(0);
                            //0 = indice del componente
                            //3 = quanti float Sono
                            //(GLvoid*)0 = il primo elemento è nel posto zero
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,sizeof(Vertex), (GLvoid*)0);

      //serve per determinare da dove leggere la prima normale (?????????????????????????)
      glEnableVertexAttribArray(1);
                          //offesetof = indica dove si trova il primo elemento 
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,sizeof(Vertex), (GLvoid*)offsetof(Vertex,Normal));

      //Qui si parla di UV coordinate
      glEnableVertexAttribArray(2);
      glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,sizeof(Vertex), (GLvoid*)offsetof(Vertex,TexCoords));

      glEnableVertexAttribArray(3);
      glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE,sizeof(Vertex), (GLvoid*)offsetof(Vertex,Tangent));

      glEnableVertexAttribArray(4);
      glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE,sizeof(Vertex), (GLvoid*)offsetof(Vertex,Bitangent));

      glBindVertexArray(0);
    }
}
