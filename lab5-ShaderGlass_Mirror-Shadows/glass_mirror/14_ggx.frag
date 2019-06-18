#version 330 core

// output shader variable
out vec4 colorFrag;


in vec4 worldPosition;
in vec3 worldNormal;

uniform samplerCube tCube;
uniform vec3 cameraPosition;

//
// in vec3 vNormal;
// // vector from fragment to camera (in view coordinate)
// in vec3 vViewPosition;
//
// // diffusive component (passed from the application)
// uniform vec3 diffuseColor;
uniform vec3 pointLightLocation
uniform float Eta; // fresnel reflectance at normal incidence
uniform float mFresnelPower; // weight of diffuse reflection

void main()
{

  //----------------- VETRO
    vec3 N = normalize (worldNormal);
    vec3 V = normalize (worldPosition.xyz - cameraPosition);
    vec3 R = reflect(V, N));
    vec3 L = normalize(worldPosition.xyz-pointLightLocation);
    vec3 H = normalize(L+V);
    vec4 reflectedColor = texture(tCube, R);

//refraction
  //vec3 refractionDir = refract(V,N,Eta); //direzione della view, normale e il ratio
//per aggiungere il chromatic aberration
  vec3 refractDir[3];
  vec3 refractDir[0] = refract(V,N,Eta);
  vec3 refractDir[0] = refract(V,N,Eta*0.99);
  vec3 refractDir[0] = refract(V,N,Eta*0.98);


  vec4 refractedColor = vec4(1.0);

  //refractedColor = texture(tCube, refractDir);
  refractedColor.r = texture(tCube, refractDir[0].r);
  refractedColor.g = texture(tCube, refractDir[0].g);
  refractedColor.b = texture(tCube, refractDir[0].b);

  //serve un parametro per mischiare riflazione e riflessi
  float F0 = ((1.0-Eta)*(1.0-Eta))/((1.0+Eta)*(1.0+Eta));
  float Ratio = F0 + (1.0-F0)*pow(1.0-max(dot(V,H),0.0), mFresnelPower);

  colorFrag = mix(refractedColor, reflectedColor, Clamp (Ratio, 0.0, 1.0));

  }
