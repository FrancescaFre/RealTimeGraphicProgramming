#version 330 core

layout(location=0) in vec3 position;

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

// le out interpolano le UV coordinate del cubo, W perchè è tridimensionale questo cubo
out vec3 interp_UVW;

void main(){
//nel mondo processo i 4 vertici del cubo, ogni Vertice è il UVW coordinate, la posizione dei vertici sono le coordinate da leggere per la texture
  interp_UVW = position;
  vec4 pos = projectionMatrix*viewMatrix*vec4(position, 1.0);

  //voglio posizionare il cubo alla massima depth
  gl_Position = pos.xyww; //dopo il prospection divide (fivido tutto per W per avere valori da 0 a 1,) la distanza massima è 1, quindi metto la z a w
//gl_Position è una var di openGl



}
