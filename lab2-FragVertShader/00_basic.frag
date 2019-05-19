/*

00_basic.frag : (VERY) basic Fragment shader

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2018/2019
Master degree in Computer Science
Universita' degli Studi di Milano

*/



#version 330 core

// output variable for the fragment shader. Usually, it is the final color of the fragment
out vec4 color;

//devo usare colorIn, variabie usata anche nel main application
//uniform vec3 colorIn;
uniform vec3 color1;
uniform vec3 color2;
uniform float repeat;

//"in" indica quello che viene messo come input al fragment shader.
in vec3 N;
in vec2 interp_UV;

float aastep (float threshold, float value) {
//prendo i valori del fragment sopra e a lato destra (dfdx) poi calcola la distanza tra quei due fragment, si prende e si mette in un vec2 e si calolca la dimension
//infine si prende il modulo, più grande è la distanza...? Infine uso questo modulo come parametro per la smoothstep 

  float afwidth = 0.7 * length (vec2(dFdx(value), dFdy(value)));
  return smoothstep(threshold-afwidth, threshold+afwidth, value);

}

void main()
{
  //color = vec4(1.0,0.0,0.0,1.0);  //applico il rosso e basta

  //color = vec4(colorIn, 1.0f); //aggiungo i 3 componenti del colore MyColor e l'alpha a 1.0

  //color = vec4(N, 1.0f);    //si vedono i colori in base alla direzione delle normali

  //color = vec4(interp_UV, 0.0f, 1.0f) //le uv sono bidimensionali, però i valori devono essere 4, quindi si mette un dalso colore blu

  //voglio creare un pattern di strisce ripetute repeat volte (nell'esempio 5 righe verticali con i colori color1 e color2)
  //lavoriamo sull'asse X perccè vogliamo righe verticali. X == U, ma in openGL U == S (mentre Y(cartesiano) == V(UV) == T(openGL))
  float k = fract(interp_UV.s * repeat); //s va da 0 a 1
  k = abs(2.0f*k-1.0f); //ho aumentato il range, da 1 a -1, poi ci faccio il modulo per avere solo le parti positive
//  float f  = step (0.5f, k); //step: tutto quello che è minore di 0.5 diventa uguale a 0, tutto quello che è maggiore diventa 1, quindi le strisce non sono più gradient, ma flat
//  float f = smoothstep (0.45f, 0.5f, k); //per migliorare l'aliasing, infatti mi permette di creare un po' di antialiasing
float f = aastep(0.5, k); //migliorare ancora l'antialiasing
  color = vec4 (mix(color1, color2, f), 1.0f); //mix: fornisce l interpolazione lineare tra due colori con un k che definisce come

}
