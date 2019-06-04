/*

06_procedural_base.vert: Vertex shader for the examples on procedural texturing. It is equal to 05_uv2color.vert

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2018/2019
Master degree in Computer Science
Universita' degli Studi di Milano

*/

#version 330 core

// vertex position in world coordinates
layout (location = 0) in vec3 position;
// UV texture coordinates
layout (location = 1) in vec3 normal;

// model matrix
uniform mat4 modelMatrix;
// view matrix
uniform mat4 viewMatrix;
// Projection matrix
uniform mat4 projectionMatrix;

uniform mat3 normalMatrix;
uniform vec3 pointLightPosition;

// the output variable for UV coordinates
out vec2 interp_UV;

//in questo vertex shader si applica la trasfomazione ai vertici
//trasformando le normali, passando in view coordinates
out vec3 vNormal;
out vec3 lightDir;
out vec3 vViewPosition;

void main()
{
	/*---1
	// I assign the values to a variable with "out" qualifier so to use the per-fragment interpolated values in the Fragment shader
		interp_UV = UV;

		// transformations are applied to each vertex
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1.0f);
*/
		//mv: model view. Quindi traduco le posizioni in model view
		vec4 mvPosition = viewMatrix * modelMatrix * vec4(position,1.0);

		//computazione dell'incidenza della luce
		vec4 lightPos = viewMatrix*vec4(pointLightPosition,1.0f); //è una point light, perchè è un punto nello spazio, quindi devo mettere 1.
		 																													//Se fosse una direzione (directional light) sarebbe stato uno 0
		//ora sto esprimendo la posizione della luce (5, 10, 10) rispetto alla camera, quindi usando la camera come origine

		//Creo il vettore che va tra le due posizioni, quindi ottengo un vettore con un modulo e una rotazione
		lightDir = lightPos.xyz - mvPosition.xyz; //lightPos.xyz --> prendo i 3 componenti.

		//Mi serve per la questione del view position. Ottengo il v vector in world coordinate
		vViewPosition = -mvPosition.xyz;

		//normal viene dalla mesh, in layout
		vNormal = normalize(normalMatrix*normal);

		//faccio la rpoiezione, .... ???
		gl_Position = projectionMatrix * mvPosition;

}
