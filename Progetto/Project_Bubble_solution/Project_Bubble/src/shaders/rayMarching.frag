#version 330 core

#define numballs 8

//Il numero di max_step � abbastanza alto in modo da evitare una brutta incurvatura vicino alle sfere
//un numero come 100 fa incurvare l'orizzonte
#define MAX_STEP 400
//Questo invece va bene a 100, la distanza e gli step non hanno la stessa unit� di misura, 
//questo mi determina il punto pi� lontano rappresentato 
#define MAX_DIST 100.
//livello di precisione, cio� quanto deve esser vicino il raggio da potersi fermare
#define PRECISION 0.01

//colori per lo sfondo
#define TopColor            vec3( 0.35, 0.4, 0.8 )
#define MiddleColor         vec3( 0.8, 0.8, 0.8 )
#define BottomColor         vec3( 0.55, 0.2, 0.6 )

out vec4 colorFrag;

uniform samplerCube tCube;
uniform float time; 
//ci sono 4 shader, 0= blinn phong, 1= reflection, 2= fresnel, 3= fresnel con un eta differente
uniform int current_shader;

uniform vec3 cameraPosition;
uniform mat4 camera; 

vec2 resolution = vec2(800,600);
vec4 fragCoord = gl_FragCoord;

//---------------------------------------------------------------------------------------
//struttura dati che raccoglie le info per una singola bolla
struct Blob
{
	float shape;//sphere, cube, tourus
	vec3 position;
    vec3 color;
    float size;
	float selected; //bool
	float operator; //union, substraction, intersection
	float spinning; 
};

Blob blobs[10];
uniform vec4 blobsPos[10]; 

uniform vec4 info1[10];
uniform vec3 info2[10];

//struttura dati che raccoglie informazioni sul punto colpito
struct Hit
{
	float dist;
    vec3 color;
    vec3 hitPos;
	bool subject; 
	float selected; 
};

//struttura dati che restituisce la distanza percorsa e cosa � stato colpito (mi serve per il colore principalmente)
struct RM{
    Hit hit;
    float travel;
};

//-------------------- Gli operatori
float SmoothUnion( float d1, float d2, float k ) {
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h); }

float Union (float obj1, float obj2) 
{
	return min(obj1, obj2); 
}

float SmoothSubtraction( float d1, float d2, float k ) {
    float h = clamp( 0.5 - 0.5*(d2+d1)/k, 0.0, 1.0 );
    return mix( d2, -d1, h ) + k*h*(1.0-h); }

float Subtraction (float obj1, float obj2)
{
	return max(-obj1, obj2);
}

float SmoothIntersection( float d1, float d2, float k ) {
    float h = clamp( 0.5 - 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) + k*h*(1.0-h); }

float Intersection (float obj1, float obj2)
   {
  	return max(obj1, obj2);  
   }


//---------------------------------------------------------Funzioni che calcolano la distanza
Hit df_Sphere(vec3 rayPos, vec3 spherePos, float size, vec3 color)
{
	float d = length(rayPos - spherePos) - size;

    Hit hit;
    hit.dist = d;
    hit.color = color;
	hit.subject = true; 
	return hit;
}

Hit df_plane (vec3 rayPos, vec3 color)
{
	Hit hit;
	hit.subject = false; 
    hit.dist = 2+rayPos.y;
    hit.color = color;
	return hit;
}

Hit df_Box(vec3 rayPos, vec3 pos, float s, vec3 color )
{
  pos = rayPos - pos; 
  vec3 size = vec3(s); 
  vec3 d = abs(pos) - size;
  float dist = length(max(d,0.0))
         + min(max(d.x,max(d.y,d.z)),0.0); // remove this line for an only partially signed sdf 

  Hit hit; 
  hit.subject = true; 
  hit.dist = dist; 
  hit.color = color; 
  return hit; 
}

Hit df_Torus( vec3 rayPos, vec3 pos, float rad, vec3 color) //radius have 2 radius, the main shape and the radius of the border
{
	pos = rayPos - pos; 
	vec2 radius = vec2(rad, rad*0.3); 
	vec2 q = vec2(length(pos.xz)-radius.x,pos.y);
	float dist = length(q)-radius.y;

	Hit hit; 
	hit.subject = true; 
	hit.dist = dist; 
	hit.color = color;
	return hit; 
}
//------------
Hit Minimum (Hit obj1, Hit obj2){

	Hit hit = obj1.dist<obj2.dist ? obj1 : obj2;
	return hit; 
}
Hit ShapeDistance(vec3 rayPos, Blob blobs){

	if(blobs.shape == 0)
		return df_Sphere(rayPos, blobs.position, blobs.size, blobs.color);
	if(blobs.shape == 1)
		return df_Box(rayPos, blobs.position, blobs.size, blobs.color);
	if(blobs.shape == 2)
		return df_Torus(rayPos, blobs.position, blobs.size, blobs.color);
}

Hit GetDist(vec3 pos)
{
    float obj;
	vec4 sphere_center = vec4(0,1,6,1);

    Hit result;
    result.dist = 1e20; 

   //operation 
	Hit sphere0 = ShapeDistance(pos, blobs[0]);
	
	result = sphere0;

	Hit cube = df_Box(pos, blobs[1].position, blobs[1].size, blobs[1].color);
	cube.selected = blobs[1].selected;
	result = Minimum(result, cube); 

	Hit torus = df_Torus(pos, blobs[2].position, blobs[2].size, blobs[2].color);
	torus.selected = blobs[2].selected;
	result = Minimum(result, torus); 

    //--------------LASCIARE NON COMMENTATO UN OPERATORE SOLO
   	//float op = Intersection(sphere0.dist, sphere1.dist);
    //float op = Subtraction(sphere0.dist, sphere1.dist);
    //float op = Union(sphere0.dist, sphere1.dist);

   //operation smooth
  
    //--------------LASCIARE NON COMMENTATO UN OPERATORE SOLO
    //float ops = SmoothIntersection(sphere2.dist, sphere3.dist, 0.2);
    //float ops = SmoothSubtraction(sphere2.dist, sphere3.dist,0.2);
    

   //-----------------------Fine controllo sui blob
  	Hit planeDist = df_plane (pos, vec3(0.0,0.4,0.0)); //ipotizzo che esista un piano, la sua distanza � sempre la y della camera rispetto al mondo


    return Minimum(result, planeDist);
}

//---------------------------- Funzione per fare il raymarching 
RM RayMarch(vec3 ray_origin, vec3 ray_direction)
{
	float travel = 0.0;
    Hit hit;
    float obj = 0.0;
    for (int i=0; i<MAX_STEP; i++)
    {
    	vec3 point = ray_origin + ray_direction * travel;
        hit = GetDist(point);
        travel += hit.dist;

        if (travel > MAX_DIST || hit.dist < PRECISION) break;
    }
    RM result;
    result.travel = travel;
    result.hit = hit;
    return result;
}
//-----------------------------------Rendering della superficie 
vec3 GetNormal(vec3 point)
{
	float dist = GetDist(point).dist;

    vec2 epsilon = vec2 (0.01, 0.0);

    vec3 normal = dist - vec3(
        GetDist(point-epsilon.xyy).dist, //per capire lo slope, comparo i punti vicini al punto su cui calcolare la norm
		GetDist(point-epsilon.yxy).dist,
        GetDist(point-epsilon.yyx).dist );

    return normalize (normal);
  }

float RampCoeff(float t, float stripes)
{
	float modifiedT = mod(floor (t*stripes), stripes); 
	float coeff = mix (0.1, 1.0, modifiedT /(stripes-1.0));
	return coeff; 
}

float SoftShadow(vec3 surfacePosition, vec3 normal, vec3 lightPosition)
{
	vec3 rayDirection = normalize(-vec3(lightPosition - surfacePosition)); 


return 0.0;
}

vec4 GetLight(vec3 surfacePoint, vec3 cameraPosition, Hit target)
{
	vec3 lightPosition = vec3 (0,5,0);
	lightPosition.xz += vec2(sin(time)*4., cos(time)*4.);
	vec3 light = normalize (lightPosition - surfacePoint);
	vec3 normal = GetNormal(surfacePoint);
  	vec4 finalColor = vec4(4.,1.,1.,1.0); 
	vec3 toCamera = normalize(cameraPosition - surfacePoint); 
		
	//blinphong
	if(current_shader == 0){
			float diffuse = clamp(dot(normal, light),0.0,1.0); //faccio il clamp in modo da non aver un valore negativo
			vec3 diffuseColor = diffuse * target.color;

			vec3 reflectedLight = normalize(reflect(-light, normal));
			float specular = pow(clamp(dot(reflectedLight, light), 0.0,1.0),10.0);

			specular = min (diffuse, specular);
			vec3 specularColor = specular * vec3(1.0);

			finalColor = vec4(diffuseColor+specularColor,1.0);
		}
	
	//reflection
	if(current_shader == 1){
        vec3 V = normalize(surfacePoint - cameraPosition);
        vec3 R = reflect(V, normal);
		finalColor = texture (tCube, R);		  
	}

	//fresnel
	if(current_shader == 2 || current_shader == 3){
		vec3 V = normalize (surfacePoint - cameraPosition);
		vec3 L = normalize (surfacePoint - lightPosition);
		vec3 H = normalize (L + V); 
		vec3 R = reflect (V, normal); 
		vec4 reflectedColor = texture(tCube, R); 

		//chromaticabb

		float Eta = current_shader == 3 ? 1.00/1.52 : 1.010; //frsnel or bubble

		vec3 refractDir[3];
		refractDir[0] = refract(V, normal, Eta);
		refractDir[1] = refract(V, normal, Eta * 0.99);
		refractDir[2] = refract(V, normal, Eta * 0.99);

		vec4 refractedColor = vec4(0.0); 
		refractedColor.r = texture(tCube, refractDir[0]).r;
		refractedColor.g = texture(tCube, refractDir[1]).g;
		refractedColor.b = texture(tCube, refractDir[2]).b;

		//fresnel
		float F0 = ((1.0-Eta)*(1.0-Eta)) / ((1.0+Eta)*(1.0+Eta));
   		float Ratio = F0 + (1.0-F0) * pow( 1.0 - max(dot(V, H),0.0), 5.0 );

		finalColor = mix(refractedColor, reflectedColor, clamp (Ratio, 0.0,1.0));
    }

	//stripes color
	if(current_shader == 4) {
		float diffuse = clamp(dot(normal, light), 0.0,1.0);
		diffuse = RampCoeff(diffuse, 4);
		vec3 diffuseColor = diffuse * target.color;

		vec3 reflectedLight = normalize (reflect(-light, normal));
		
		float specular = pow(clamp(dot(reflectedLight, light), 0.0,1.0), 10.);
		specular = RampCoeff(specular, 4);
		specular = min (diffuse, specular); 
		vec3 specularColor = specular * vec3(1.0);

		finalColor = vec4(diffuseColor + specularColor,1.0); 		
	}

		float hit = RayMarch(surfacePoint + (normal*PRECISION*2.), light).travel;
		if (hit < length(surfacePoint-lightPosition))
		finalColor *= 0.4;
		

		if (target.subject && target.selected == 1.0){
			float border = dot(toCamera, normal); 
			if(border > -0.3 && border <0.3)
				finalColor = vec4(1.0);
		}
		
		
		return finalColor;
}

//----------- BG
vec3 BackGroundGradient( vec2 uv )
{
   vec3 color = vec3(0.0);
   if( uv.y >= 1.0 )
       color = mix( TopColor, MiddleColor, uv.y + 1.0 );
   
   else
       color = mix( MiddleColor, BottomColor, uv.y );
   
   return color;
}

//--------------------------------------------------------------------------------
vec4 Background( vec2 uv )
{
   vec4 color = texture(tCube, vec3(uv,1.0)); //BackGroundGradient(uv); 
   return color;
}
//AIUTO: COME SI INTERPRETA UNA MAT4X4?
vec3 getPosition (mat4 camera){
	return vec3(camera[3][0],camera[3][1],camera[3][2]);
	//return vec3(0.0,1.0,6.0);
}

vec3 getRight(mat4 camera){
	return vec3(camera[0][0],camera[0][1],camera[0][2]);
	//return vec3(1.0,0.0,0.0);
}

vec3 getUp(mat4 camera){
	return vec3(camera[1][0],camera[1][1],camera[1][2]);
	//return vec3(0.0,1.0,0.0);
}

vec3 getForward(mat4 camera){
	return vec3(camera[2][0],camera[2][1],camera[2][2]);
	//return vec3(0.0,0.0,1.0);
}

vec3 getDirection(vec2 uv, vec3 position, vec3 dest, float value){
	vec3 f = normalize(dest - position); 
	vec3 r = normalize (cross ( f,vec3(0.0,1.0,0.0)));	
	vec3 u = normalize(cross(r,f)); 
	
	vec3 c = position + f * value; 
	vec3 i = c + uv.x*r + uv.y*u;
	return normalize (i-position);  
}

//------------------------------------------------------- MAIN
void main()
{
    //normalizzo la finestra in uno spazio da 1 a -1 e -0.5 per mettere al centro il riferimento (0.0)
	vec2 uv = gl_FragCoord.xy/resolution.xy; 
	uv -= 0.5;
	uv.x *= resolution.x/resolution.y; 
	
	vec4 col = vec4(0.0);

	for (int i = 0; i < 10; i++)
    {
        blobs[i].position = blobsPos[i].xyz;
		blobs[i].size = blobsPos[i].w;
		blobs[i].color = info1[i].xyz;
		blobs[i].selected = info1[i].w;
		blobs[i].shape = info2[i].x;
		blobs[i].operator = info2[i].y;
		blobs[i].spinning = info2[i].z;
       
	   // float m = mod(float(i),2.);
       // blobs[i].position.xz += vec2(sin(time)*(1.-m), cos(time)*(1.-m));
    }

    vec3 ray_origin = vec3(0.0,0.0,0.0) +getPosition(camera);

    //vec3 ray_direction = normalize(vec3(uv.x, uv.y,1));
	vec3 ray_direction = getDirection(uv, ray_origin, vec3(0.0,0.0,-5.), 0.7); 
	
    RM raymarch = RayMarch(ray_origin, ray_direction);

		if (raymarch.hit.dist < PRECISION)
		{
    		vec3 point = ray_origin + ray_direction * raymarch.travel;

    		col = GetLight (point, ray_origin, raymarch.hit);
		}
		else col = Background(uv);

		//if(blobs[2].selected == 1)  col = vec4(1.0); 

    colorFrag = col;
}
