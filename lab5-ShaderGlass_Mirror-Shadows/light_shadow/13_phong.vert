/*
13_phong.vert: Vertex shader for the Phong and Blinn-Phong illumination model

N. B.) the shader treats a simplified situation, with a single point light.
For more point lights, a for cycle is needed to sum the contribution of each light
For different kind of lights, the computation must be changed (for example, a directional light is defined by the direction of incident light, so the lightDir is passed as uniform and not calculated in the shader like in this case with a point light).

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

out vec4 worldPosition;
out vec3 worldNormal;

void main(){
  worldPosition = modelMatrix*vec4(position,1.0);

//il world nomal è la trasposta dell'inverso del model Matrix per la normale
  worldNormal = mat3(transpose(inverse(modelMatrix))) * normal;

  gl_Position = projectionMatrix * viewMetrix * worldPosition;

}
