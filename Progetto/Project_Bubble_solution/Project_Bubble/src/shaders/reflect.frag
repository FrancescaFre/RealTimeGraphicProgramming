/*
21_reflect.frag: reflections using  environment mapping

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2018/2019
Master degree in Computer Science
Universita' degli Studi di Milano
*/


#version 330 core

// output shader variable
out vec4 colorFrag;


// vertex position and normal in world coordinates
in vec4 worldPosition;
in vec3 worldNormal;

// texture sampler: we are using a cube map, so we need a samplerCube
uniform samplerCube tCube;

// camera position
uniform vec3 cameraPosition;

void main(){

    // vector from vertex to camera in world coordinates
    vec3 V = normalize(worldPosition.xyz - cameraPosition);

    // reflection vector of I with respect to normal
    vec3 R = reflect( V, normalize(worldNormal));

    // we sample the texture cube using the components of the reflection vector as texture coordinates. 
    colorFrag = texture( tCube, R);
}
