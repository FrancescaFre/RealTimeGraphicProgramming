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
/*
#define TopColor            vec3( 0.35, 0.4, 0.8 )
#define MiddleColor         vec3( 0.8, 0.8, 0.8 )
#define BottomColor         vec3( 0.55, 0.2, 0.6 )
*/
#define TopColor            vec3( 1.0,0.0,0.0 )
#define MiddleColor         vec3( 0.0,1.0,0.0 )
#define BottomColor         vec3( 0.0,0.0,1.0 )


out vec4 colorFrag;

uniform float time; 

vec2 resolution = vec2(800,600);
//---------------------------------------------------------------------------------------
//struttura dati che raccoglie informazioni sul punto colpito
struct Hit
{
	float dist;
    vec3 color;
    vec3 hitPos;
};

//struttura dati che restituisce la distanza percorsa e cosa � stato colpito (mi serve per il colore principalmente)
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

int current_shader; //ci sono 4 shader, 0= blinn phong, 1= reflection, 2= fresnel, 3= fresnel con un eta differente
Blob blobs[4];


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


  	Hit planeDist = df_plane (pos, vec3(0.0,0.5,0.0)); //ipotizzo che esista un piano, la sua distanza � sempre la y della camera rispetto al mondo

    if (result.dist < planeDist.dist)
        return result;
    else return planeDist;

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
///-----------------------------------Rendering della superficie 
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
	vec3 lightPosition = vec3 (0,5,5);
	lightPosition.xz += vec2(sin(time)*2., cos(time)*2.);
	vec3 light = normalize (lightPosition - surfacePoint);
	vec3 normal = GetNormal(surfacePoint);
  	vec4 finalColor = vec4(4.,1.,1.,1.0); 

	//blinphong
	float diffuse = clamp(dot(normal, light),0.0,1.0); //faccio il clamp in modo da non aver un valore negativo
	vec3 diffuseColor = diffuse * typeColor;

	vec3 reflectedLight = normalize(reflect(-light, normal));
	float specular = pow(clamp(dot(reflectedLight, light), 0.0,1.0),10.0);

	specular = min (diffuse, specular);
	vec3 specularColor = specular * vec3(1.0);

	finalColor = vec4(diffuseColor+specularColor,1.0);
		
		//----shadows

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
vec3 Background( vec2 uv )
{
   vec3 color = BackGroundGradient(uv);

   return color;
}

//------------------------------------------------------- MAIN
void main()
{
vec4 fragCoord = gl_FragCoord;

    //normalizzo la finestra in uno spazio da 1 a -1 e -0.5 per mettere al centro il riferimento (0.0)
   
     vec2 uv = vec2(gl_FragCoord.x / resolution.x, gl_FragCoord.y/resolution.y);
	 uv -= 0.5;
    //qui modifico lo shader 
	current_shader = 0; 
    //aggiorno le blobs
    for (int i = 0; i < 4; i++)
    {
        blobs[i].position = vec3((float(i)-1.5)*2., 1.0, 6.0);
        float m = mod(float(i),2.);
        blobs[i].position.xz += vec2(sin(time)*(1.-m), cos(time)*(1.-m));
        blobs[i].size = 1.0;
    	blobs[i].color = vec3(float(3)*0.2,float(3)*0.2,float(3)*0.9);
    }

    vec4 col = vec4(0);

    vec3 ray_origin = vec3(0,1,0); 

    vec3 ray_direction = normalize(vec3(uv.x, uv.y,1));

    RM raymarch = RayMarch(ray_origin, ray_direction);

		if (raymarch.hit.dist < PRECISION)
		{
    	vec3 point = ray_origin + ray_direction * raymarch.travel;

    	col = GetLight (point, ray_origin, raymarch.hit.color);
		}
		else col = vec4(Background(uv),1.0);

	
//	col = vec4(uv.xy, 0.,1.0);

	colorFrag = vec4(col);
}
