#version 330 core

#define numballs 8

//Il numero di max_step è abbastanza alto in modo da evitare una brutta incurvatura vicino alle sfere
//un numero come 100 fa incurvare l'orizzonte
#define MAX_STEP 400
//Questo invece va bene a 100, la distanza e gli step non hanno la stessa unità di misura, 
//questo mi determina il punto più lontano rappresentato 
#define MAX_DIST 100.
//livello di precisione, cioè quanto deve esser vicino il raggio da potersi fermare
#define PRECISION 0.01

//colori per lo sfondo
#define TopColor            vec3( 0.35, 0.4, 0.8 )
#define MiddleColor         vec3( 0.8, 0.8, 0.8 )
#define BottomColor         vec3( 0.55, 0.2, 0.6 )

out vec4 colorFrag;

uniform samplerCube tCube;
uniform float time; 
//ci sono 4 shader, 0= blinn phong, 1= reflection, 2= fresnel, 3= fresnel con un eta differente
uniform float current_shader;

uniform vec3 cameraPosition;
uniform mat4 camera; 

vec2 resolution = vec2(800,600);
vec4 fragCoord = gl_FragCoord;

//---------------------------------------------------------------------------------------
//struttura dati che raccoglie informazioni sul punto colpito
struct Hit
{
	float dist;
    vec3 color;
    vec3 hitPos;
};

//struttura dati che restituisce la distanza percorsa e cosa è stato colpito (mi serve per il colore principalmente)
struct RM{
    Hit hit;
    float travel;
};

//struttura dati che raccoglie le info per una singola bolla
struct Blob
{
	vec3 position;
    vec3 color;
    float size;
};

Blob blobs[10];
uniform vec4 blobsPos[10]; //{blob0,blob1,blob2,blob3,blob4,blob5,blob6,blob7,blob8,blob9};


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
	return hit;
}

Hit df_plane (vec3 pos, vec3 color)
{
	Hit hit;
    hit.dist = pos.y;
    hit.color = color;
	return hit;
}

Hit GetDist(vec3 pos)
{
    float obj;

	vec4 sphere_center = vec4(0,1,6,1);

    Hit result;
    result.dist = 1e20;

   //operation  
    Hit sphere0 = df_Sphere(pos, blobs[0].position, blobs[0].size, blobs[0].color);
	Hit sphere1 = df_Sphere(pos, blobs[1].position, blobs[1].size, blobs[1].color);
     
    //--------------LASCIARE NON COMMENTATO UN OPERATORE SOLO
   	//float op = Intersection(sphere0.dist, sphere1.dist);
    //float op = Subtraction(sphere0.dist, sphere1.dist);
    float op = Union(sphere0.dist, sphere1.dist);

    if (op < result.dist)
    {
        sphere1.dist = op;
        result = sphere1;
    }
    
   //operation smooth
  	Hit sphere2 = df_Sphere(pos, blobs[2].position, blobs[2].size, blobs[2].color);
   	Hit sphere3 = df_Sphere(pos, blobs[3].position, blobs[3].size, blobs[3].color);

    //--------------LASCIARE NON COMMENTATO UN OPERATORE SOLO
    //float ops = SmoothIntersection(sphere2.dist, sphere3.dist, 0.2);
    //float ops = SmoothSubtraction(sphere2.dist, sphere3.dist,0.2);
    float ops = SmoothUnion(sphere2.dist, sphere3.dist, 0.2);

    if (ops < result.dist)
	{
        sphere3.dist = ops;
        result = sphere3;
    }
	//-----------------------Fine controllo sui blob


  //	Hit planeDist = df_plane (pos, vec3(0.0,0.5,0.0)); //ipotizzo che esista un piano, la sua distanza è sempre la y della camera rispetto al mondo

  //  if (result.dist < planeDist.dist)
        return result;
    //else return planeDist;

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

vec4 GetLight(vec3 surfacePoint, vec3 cameraPosition, vec3 typeColor)
{
	vec3 lightPosition = vec3 (0,6,0);
	//lightPosition.xz += vec2(sin(time)*2., cos(time)*2.);
	vec3 light = normalize (lightPosition - surfacePoint);
	vec3 normal = GetNormal(surfacePoint);
  	vec4 finalColor = vec4(4.,1.,1.,1.0); 

	//blinphong
	if(current_shader == 0){
			float diffuse = clamp(dot(normal, light),0.0,1.0); //faccio il clamp in modo da non aver un valore negativo
			vec3 diffuseColor = diffuse * typeColor;

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


		float hit = RayMarch(surfacePoint + (normal*PRECISION*2.), light).travel;
		if (hit < length(surfacePoint-lightPosition))
		finalColor *= 0.3;

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

  // float sunStrength = mix( 2.8, 3.0, sin( time ) * 0.5 + 0.5 );
  
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

vec3 getDirection(vec2 uv, vec3 position, vec3 right, vec3 up, vec3 forward, vec3 dest, float value){
	vec3 f = normalize(dest - position); 
	vec3 r = normalize (cross ( vec3(0.0,1.0,0.0),f));	//AIUTO: PERCHè SE INVERTO UP E FORWARD SI RIBALTA TUTTO? Sulle slide era questo il lookAt
	vec3 u = normalize(cross(r,f)); 
	
	vec3 c = position + f * value; 
	vec3 i = c + uv.x*r + uv.y*u;
	return normalize (i-position);  
}

//------------------------------------------------------- MAIN
void main()
{
    //normalizzo la finestra in uno spazio da 1 a -1 e -0.5 per mettere al centro il riferimento (0.0)
	//vec2 uv = vec2(fragCoord.xy * resolution.xy)/resolution.y; 
	//vec2 uv =( -1.0 + 2.0*(fragCoord.xy/resolution.xy)) * vec2(resolution.x/resolution.y); 
	  vec2 uv = vec2(gl_FragCoord.x / resolution.x, gl_FragCoord.y/resolution.y) -0.5;	//AIUTO: VIENE SCHIACCIATO

	  vec4 col = vec4(0.0);

	for (int i = 0; i < 10; i++)
    {
        blobs[i].position = blobsPos[i].xyz;
		blobs[i].size = blobsPos[i].w;
		blobs[i].color = vec3(float(3)*0.2,float(3)*0.2,float(3)*0.9)*i;
       // float m = mod(float(i),2.);
       // blobs[i].position.xz += vec2(sin(time)*(1.-m), cos(time)*(1.-m));
    }

    vec3 ray_origin = getPosition(camera); 

    //vec3 ray_direction = normalize(vec3(uv.x, uv.y,1));
	vec3 ray_direction = getDirection(uv, ray_origin, getRight(camera), getUp(camera), getForward(camera), vec3(0.0,0.0,0.0), 0.7); 
	
    RM raymarch = RayMarch(ray_origin, ray_direction);

		if (raymarch.hit.dist < PRECISION)
		{
    		vec3 point = ray_origin + ray_direction * raymarch.travel;

    		col = GetLight (point, ray_origin, raymarch.hit.color);
		}
		else col = Background(uv);

    colorFrag = col;
}
