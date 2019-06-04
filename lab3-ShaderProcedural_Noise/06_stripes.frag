/*

06_stripes.frag: it creates a stripes procedural pattern

N.B.)  "06_procedural_base.vert" must be used as vertex shader

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2018/2019
Master degree in Computer Science
Universita' degli Studi di Milano

*/

#version 330 core
//mi serve per il FDG
const float PI = 3.141592;

// output shader variable
out vec4 colorFrag;

// UV texture coordinates, interpolated in each fragment by the rasterization process
in vec2 interp_UV;

// texture repetitions
uniform float frequency;
uniform float power;
uniform float harmonics;
uniform float timer;

//altre variabili per aggiungere la luce
uniform vec3 diffuseColor;
uniform vec3 specularColor;
uniform vec3 ambientColor;

uniform float Kd;
uniform float Ks;
uniform float Ka;

uniform float alpha;
uniform float F0;

uniform float shininess;
uniform float linear;
uniform float constant;
uniform float quadratic;

in vec3 lightDir;
in vec3 vNormal;

in vec3 vViewPosition;

// stripes colors
//uniform vec3 color1;
//uniform vec3 color2;

////////////////////////////////////////////////////////////////////
// Description : Array and textureless GLSL 2D/3D/4D simplex
//               noise functions.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : ijm
//     Lastmod : 20110822 (ijm)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise
//

vec3 mod289(vec3 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x) {
     return mod289(((x*34.0)+1.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

float snoise(vec3 v)
  {
  const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

// First corner
  vec3 i  = floor(v + dot(v, C.yyy) );
  vec3 x0 =   v - i + dot(i, C.xxx) ;

// Other corners
  vec3 g = step(x0.yzx, x0.xyz);
  vec3 l = 1.0 - g;
  vec3 i1 = min( g.xyz, l.zxy );
  vec3 i2 = max( g.xyz, l.zxy );

  //   x0 = x0 - 0.0 + 0.0 * C.xxx;
  //   x1 = x0 - i1  + 1.0 * C.xxx;
  //   x2 = x0 - i2  + 2.0 * C.xxx;
  //   x3 = x0 - 1.0 + 3.0 * C.xxx;
  vec3 x1 = x0 - i1 + C.xxx;
  vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
  vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

// Permutations
  i = mod289(i);
  vec4 p = permute( permute( permute(
             i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0 ))
           + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

// Gradients: 7x7 points over a square, mapped onto an octahedron.
// The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
  float n_ = 0.142857142857; // 1.0/7.0
  vec3  ns = n_ * D.wyz - D.xzx;

  vec4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)

  vec4 x_ = floor(j * ns.z);
  vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

  vec4 x = x_ *ns.x + ns.yyyy;
  vec4 y = y_ *ns.x + ns.yyyy;
  vec4 h = 1.0 - abs(x) - abs(y);

  vec4 b0 = vec4( x.xy, y.xy );
  vec4 b1 = vec4( x.zw, y.zw );

  //vec4 s0 = vec4(lessThan(b0,0.0))*2.0 - 1.0;
  //vec4 s1 = vec4(lessThan(b1,0.0))*2.0 - 1.0;
  vec4 s0 = floor(b0)*2.0 + 1.0;
  vec4 s1 = floor(b1)*2.0 + 1.0;
  vec4 sh = -step(h, vec4(0.0));

  vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

  vec3 p0 = vec3(a0.xy,h.x);
  vec3 p1 = vec3(a0.zw,h.y);
  vec3 p2 = vec3(a1.xy,h.z);
  vec3 p3 = vec3(a1.zw,h.w);

//Normalise gradients
  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;

// Mix final noise value
  vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
  m = m * m;
  return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1),
                                dot(p2,x2), dot(p3,x3) ) );
  }
// ------------------------------------------------------------ FINE noise

//Definisco l'aastep per usarlo, è quello con l'antialiasing per i regular patterns
  float aastep(float threshold, float value) {
    float afwidth = 0.7 * length(vec2(dFdx(value), dFdy(value)));

    return smoothstep(threshold-afwidth, threshold+afwidth, value);
  }

  //definisco una funzione per il turbolence, cioè la funzione delle ottave per modificare il noise
  float turbulence()
  {
    float p = power;
    float f = frequency;

    float value = 0.0;

    for(int i = 0; i<harmonics; i++)
    {
      value += p*snoise(vec3(interp_UV*f, 0.0));
//per ottenere un effetto differente, direi marmoreo, si usano solo i valori positivi applicando l'absolute
//    value += p*abs(snoise(vec3(interp_UV*f, 0.0)));
      p*=0.5; //lo dimezzo
      f*=2.0; //lo raddoppio
      //(è il funzionamento delle ottave)
    }
    return value;
  }

  float turbulence1(float v)
  {
    float p = power;
    float f = frequency;

    float value = 0.0;

    for(int i = 0; i<harmonics; i++)
    {
      value += p*snoise(vec3(interp_UV*f, v*timer));
//per ottenere un effetto differente, direi marmoreo, si usano solo i valori positivi applicando l'absolute
//    value += p*abs(snoise(vec3(interp_UV*f, 0.0)));
      p*=0.5; //lo dimezzo
      f*=2.0; //lo raddoppio
      //(è il funzionamento delle ottave)
    }
    return value;
  }


  float G1(float angle, float alpha)
  {
    float r = (alpha+1.0);
    float k = (r*r)/8; //uniform color

    float num = angle;
    float denom = angle*(1.0-k) + k;

    return num/denom;

  }

  ////////////////////////////////////////////////////////////////////


void main(){
/**
    // s coordinates -> from 0.0 to 1.0
    // multiplied for "repeat" -> from 0.0 to "repeat"
    // fractional part gives the repetition of a gradient from 0.0 to 1.0 for "repeat" times
    float k = fract(interp_UV.s * repeat);
    // from [0.0,1.0] to [-1.0,1.0] -> 2.0*k -1.0
    // taking the absolute value, we "translate" the pattern to the left
    // -> try to save in f the value of k to see what happens to the colors mix
    k = abs(2.0*k -1.0);
    // step function forces half pattern to white and the other half to black
    float f = step(0.5,k);
    // we use f to mix the colors, to obtain a colored stripes pattern
    // A LOT OF ALIASING!
    colorFrag = vec4(mix(color1, color2, f),1.0);
**/

//per ogni frame, cambia, perchè timer è un parametro che cambia ad ogni frame

  //---1
  //usando i colori bianco e nero
  //colorFrag = vec4(vec3(color), 1.0);

  //---2
  //usando i colori definiti
  /**
    float r = power*snoise(vec3(interp_UV*frequency, 0.1*timer));
    float g = power*snoise(vec3(interp_UV*frequency, 0.7*timer));
    float b = power*snoise(vec3(interp_UV*frequency, 0.4*timer));
  **/
  //colorFrag = vec4(r,g,b, 1.0);

  //---3
  //   //con la turbolence
  // float color = turbulence();
  // color = aastep(0.05, color);
  // colorFrag = vec4(vec3(color),1.0);

  //---4
  // float color = turbulence();
  // color = aastep(0.05, color);
  // if (color <0.5)
  //   discard;
  // colorFrag = vec4(vec3(color), 1.0);

/*
  //---5
  float r = turbulence1(0.3);
  float g = turbulence1(0.7);
  float b = turbulence1(0.4);

  r = aastep(0.5, r);
  g = aastep(0.5, g);
  b = aastep(0.5, b);
  colorFrag = vec4(r,g,b,1.0);
*/

/*
//--- 6
  vec3 N = normalize(vNormal);
  vec3 L = normalize(lightDir);

  //lambert illumination models --> Kd*L_i*(N*L)
  float lambertian = max(dot(L,N),0.0); //faccio la dot prod tra L e N, ottengo il coseno tra i due vettori

  vec3 color = vec3(Kd*lambertian*diffuseColor); //si moltiplicano i valori rgb qui

  //riflesso tra RV --> Ks*specular(R dot V)^alpha


  colorFrag = vec4(color, 1.0); //qui applico l'alpha in un momento successivo
*/

/*
  //---7
  vec3 N = normalize(vNormal);
  vec3 L = normalize(lightDir);
  vec3 color = Ka*ambientColor;

  //per prendere la distanza dalla luce dalla camera si deve fare la length del vettore tra camera e luce (light dir?)
  float distanceL = length(L);
  float attenuation = 1.0/(constant + linear*distanceL + quadratic*(distanceL*distanceL));

  float lambertian = max(dot(L,N),0.0);
  if (lambertian > 0.0)
  {
    vec3 V = normalize(vViewPosition);
    //ho bisogno del vettore di riflesso
    vec3 R = reflect(-L,N); //la luce rispetto alla normale, ma devo menttere un -L perchè il vettore senza meno punta "fuori",
                            //con il meno punta verso l'origine, cioè il punto in cui colpisce la superficie
    float specAngle = max(dot(R,V), 0.0);
    float specular = pow(specAngle, shininess); //elevation di alpha nella formula --> riflesso tra RV --> Ks*specular(R dot V)^alpha
    color += vec3 (Kd*lambertian*diffuseColor + Ks*specular*specularColor); //è sommato alla luce ambientale già inserita
    color*=attenuation;
  }
  colorFrag = vec4(color, 1.0);
*/

/*
//---8 Per il phong con il vettore H
vec3 N = normalize(vNormal);
vec3 L = normalize(lightDir);
vec3 color = Ka*ambientColor;

//per prendere la distanza dalla luce dalla camera si deve fare la length del vettore tra camera e luce (light dir?)
float distanceL = length(L);
float attenuation = 1.0/(constant + linear*distanceL + quadratic*(distanceL*distanceL));

float lambertian = max(dot(L,N),0.0);
if (lambertian > 0.0)
{
  vec3 V = normalize(vViewPosition);
  //ho bisogno del vettore di riflesso
  vec3 H = normalize(L+V); //ottengo il vettore H prendendo il vettore in mezzo L e V

  float specAngle = max(dot(H,N), 0.0);
  float specular = pow(specAngle, shininess); //elevation di alpha nella formula --> riflesso tra RV --> Ks*specular(R dot V)^alpha
  color += vec3 (Kd*lambertian*diffuseColor + Ks*specular*specularColor); //è sommato alla luce ambientale già inserita
  color*=attenuation;
}
colorFrag = vec4(color, 1.0);
*/

  //---9 GGX model --> si va di formula con l'integrale grosso
  vec3 N = normalize(vNormal);
  vec3 L = normalize(lightDir);
  float NdotL = max(dot(L,N),0.0);
  vec3 specular = vec3(0.0);
  vec3 lambert = (Kd*diffuseColor)/PI;

  if (NdotL > 0.0)
  {
    vec3 V = normalize(vViewPosition);
    vec3 H = normalize(L+V);

    float NdotH = max(dot(N,H),0.0);
    float NdotV = max(dot(N,V),0.0);
    float VdotH = max(dot(V,H),0.0);

    float NdotH_squared = NdotH*NdotH;

    float alpha_squared = alpha*alpha;

    //Fresnel con la shilk approssimation
    vec3 F = vec3(pow(1-VdotH,5.0));
    F*=(1.0-F0);
    F+=F0;

    //D
    float D = alpha_squared;
    float denom = (NdotH_squared*(alpha_squared-1.0)+1.0);
    D/=PI*denom*denom;

    //Geometry
    float G2 = G1(NdotV, alpha) * G1(NdotL, alpha);

    specular = (F*G2*D) /(4.0*NdotV*NdotL);
}

vec3 finalColor = (lambert+specular)*NdotL;
colorFrag = vec4(finalColor,1.0);


}
