/*
24_phong_tex_dirlight.vert: vertex shader for GGX, with texturing.
It consider a single directional light.

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
// UV coordinates
layout (location = 2) in vec2 UV;

// model matrix
uniform mat4 modelMatrix;
// view matrix
uniform mat4 viewMatrix;
// Projection matrix
uniform mat4 projectionMatrix;


// normals transformation matrix (= transpose of the inverse of the model-view matrix)
uniform mat3 normalMatrix;

// the light incidence direction of the directional light (passed as uniform)
uniform vec3 lightVector;
uniform mat4 lightSpaceMatrix;

// light incidence direction (in view coordinate)
out vec3 lightDir;

// the transformed normal (in view coordinate) is set as an output variable, to be "passed" to the fragment shader
// this means that the normal values in each vertex will be interpolated on each fragment created during rasterization between two vertices
out vec3 vNormal;

// in the fragment shader, we need to calculate also the reflection vector for each fragment
// to do this, we need to calculate in the vertex shader the view direction (in view coordinates) for each vertex, and to have it interpolated for each fragment by the rasterization stage
out vec3 vViewPosition;

// the output variable for UV coordinates
out vec2 interp_UV;
out vec4 posLightSpace;

void main(){

  // vertex position in ModelView coordinate (see the last line for the application of projection)
  // when I need to use coordinates in camera coordinates, I need to split the application of model and view transformations from the projection transformations
  vec4 mPosition = modelMatrix * vec4(position, 1.0);
  vec4 mvPosition = viewMatrix * mPosition;

  // view direction, negated to have vector from the vertex to the camera
  vViewPosition = -mvPosition.xyz;

  // transformations are applied to the normal
  vNormal = normalize( normalMatrix * normal );

  // we consider a directional light. The direction of light has been passed as an uniform. We apply the view transformation in order to have the direction in camera coordinates
  lightDir = vec3(viewMatrix  * vec4(lightVector, 0.0));

  // we apply the projection transformation
  gl_Position = projectionMatrix * mvPosition;

  // I assign the values to a variable with "out" qualifier so to use the per-fragment interpolated values in the Fragment shader
  interp_UV = UV;

  posLightSpace = lightSpaceMatrix * mPosition
}
