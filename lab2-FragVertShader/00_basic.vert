/*

00_basic.vert : basic Vertex shader

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2018/2019
Master degree in Computer Science
Universita' degli Studi di Milano

*/


#version 330 core

// vertex position in world coordinates
layout (location = 0) in vec3 position;
//layout (location = 1) in vec3 normal; //riga 120 del model.h, sono i location di VAO
layout (location = 2) in vec2 UV;

// model matrix
uniform mat4 modelMatrix;
// view matrix
uniform mat4 viewMatrix;
// Projection matrix
uniform mat4 projectionMatrix;
//nome usato nell'application, variabile per le normali
uniform mat3 normalMatrix;

uniform float timer;
uniform float weight;

//per ogni vertice ho una normale, per le normali all'interno vengono interpolate quelle dei vertici
out vec3 N;//variabile prodotta per i fragment
out vec2 interp_UV; //uv interpolati

void main()
{
  //---- SHADER 2 ------
  //allora praticamente sposto la posizione dei vertici in base alla direzione della normale di ogni poligono
  //quindi con un valore da 0 a 1 applico la modifica fino a creare un effetto "bump"
  //il peso e il timer li ottengo dalla main application.
//  float disp = weight * sin(timer) + weight;
//  vec3 newPos = position + disp * normal;
//  gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(newPos, 1.0f);

  /** ---- SHADER 1 ------
  si appiattisce la mesh, come se fosse una proiezione ortografica, quindi senza z
  vec3 flattened = position;
  flattened.z = 0.0f;
  gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(flattened, 1.0f);
**/






  // transformations are applied to each vertex
  gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1.0f);
                //vec4 è il casting per avere la posizione (3 valori) e 1, per avere l'omogeneus (rotazione quaternione), GLPOSITION

  //posso lavorare con i gradient usando le normali:
//  N = normalize(normalMatrix * normal);

  //---- SHADER 3 ------
  interp_UV = UV; //serve per trasportare valori interpolati nella pipeline



}
