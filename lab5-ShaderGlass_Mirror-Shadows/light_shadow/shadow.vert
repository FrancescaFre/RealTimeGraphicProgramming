#version 330 core

layout(location=0) in vec3 position;

uniform mat4 modelMatrix;
uniform mat4 lightSpaceMatrix;

void main(){
  vec4 pos = lightSpaceMatrix*modelMatrix*vec4(position, 1.0);

  gl_Position = pos; 


}
