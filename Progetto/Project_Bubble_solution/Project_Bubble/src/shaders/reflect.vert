/*
21_reflect.vert: reflections using  environment mapping

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2018/2019
Master degree in Computer Science
Universita' degli Studi di Milano
*/


#version 330 core

// vertex position in world coordinates
layout (location = 0) in vec3 position;
// vertex normal in world coordinate
layout (location = 1) in vec3 normal;

// model matrix
uniform mat4 modelMatrix;
// view matrix
uniform mat4 viewMatrix;
// Projection matrix
uniform mat4 projectionMatrix;

// vertex position and normal in world coordinates, to be calculated per vertex and interpolated for each fragment, in order to calculate the reflection vector in the fragment shader
out vec4 worldPosition;
out vec3 worldNormal;

void main(){

  // vertex position in world coordinate (= we apply only trasformations)
  worldPosition = modelMatrix * vec4( position, 1.0 );

  // we calculate the normal in world coordinates: in this case we do not use the normalMatrix (= inverse of the transpose of the modelview matrix), but we use the inverse of the transpose of the model matrix
  // We can think to pass this matrix as an uniform like the normalMatrix, if we do not want to calculate here
  worldNormal = mat3(transpose(inverse(modelMatrix))) * normal;

  // we apply the projection transformation
  gl_Position = projectionMatrix * viewMatrix * worldPosition;

}
