#version 330 core

in vec3 interp_UVW;

out vec4 colorFrag;

uniform samplerCube tCube;

void main (){
  colorFrag = texture (tCube, interp_UVW);

}
