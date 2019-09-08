//viene un effetto figo

#define MAX_STEP 100
#define MAX_DIST 100.
#define PRECISION 0.01

struct Hit
{
	float dist;
    vec3 color; 
    vec3 hitPos; 
};

struct RM{
    Hit hit; 
    float travel; 
};
    
struct Blob
{
	vec3 position; 
    vec3 color; 
    float size; 
};

Blob blobs[4];
//---------------------------------------------------------
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
    
    
    for (int i = 0; i<4; i++)
    {
    	Hit sphere = df_Sphere(pos, blobs[i].position, blobs[i].size, blobs[i].color);
        if (sphere.dist < result.dist) 
            result = sphere; 
         
    }
    
  //  Hit sphereDist = df_Sphere(pos, sphere_center.xyz, sphere_center.w, vec3(0.0,0.0,1.0)); //dove w è il raggio
   // Hit sphereDist = df_Sphere(pos, blobs[1].position, blobs[1].size, blobs[1].color);
    //Hit sphereDist1 = df_Sphere(pos, sphere_center.xyz+vec3(2.0,0.0,0.0), sphere_center.w, vec3(1.0,0.0,0.0)); //dove w è il raggio
    //Hit sphereDist2 = df_Sphere(pos, sphere_center.xyz+vec3(-2.0,0.0,0.0), sphere_center.w, vec3(0.5,0.,0.5)); //dove w è il raggio
   
   	Hit planeDist = df_plane (pos, vec3(0.0,0.5,0.0)); //ipotizzo che esista un piano, la sua distanza è sempre la y della camera rispetto al mondo
   
    if (result.dist < planeDist.dist)
        return result; 
    else return planeDist; 
  
}

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
///-----------------------------------
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

vec3 GetLight(vec3 surfacePoint, vec3 cameraPosition, vec3 typeColor) 
{
	vec3 lightPosition = vec3 (0,5,6); 
    lightPosition.xz += vec2(sin(iTime), cos(iTime)*2.);
    vec3 light = normalize (lightPosition - surfacePoint); 
    vec3 normal = GetNormal(surfacePoint); 
    
    float diffuse = clamp(dot(normal, light),0.0,1.0); //faccio il clamp in modo da non aver un valore negativo
    vec3 diffuseColor = diffuse * typeColor;
    
    
    vec3 reflectedLight = normalize(reflect(-light, normal));
    float specular = pow(clamp(dot(reflectedLight, light), 0.0,1.0),10.0);                         
    
    specular = min (diffuse, specular); 
    vec3 specularColor = specular * vec3(1.0);
                                    
                                    
     //per ottenere un'ombra posso fare il reymarch dal punto colpito alla posizione della luce, 
    //se la distanza è minore della distanza tra punto e luce, è stato colpito qualcosa
    vec3 finalColor = diffuseColor+specularColor;
    
    float hit = RayMarch(surfacePoint + (normal*PRECISION*2.), light).travel;
    if (hit < length(surfacePoint-lightPosition))
 		finalColor *= 0.1;
    
   	return finalColor;
    
}

//-------------------------------------------------------
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    //normalizzo la finestra in uno spazio da 1 a -1 e cib -0.5 metto al centro il riferimento (0.0)
    vec2 uv = (fragCoord - 0.5 * iResolution.xy)/iResolution.y;
	
    for (int i = 0; i < 4; i++)
    {
        blobs[i].position = vec3((float(i)-1.5)*2., 1.0, 6.0);
        blobs[i].size = 1.0; 
    	blobs[i].color = vec3(float(i)*2.,float(i)*3.,float(i)*4.);
    }
    
    vec3 col = vec3(0); 
    
    vec3 ray_origin = vec3(0,1,0);
    
    vec3 ray_direction = normalize(vec3(uv.x, uv.y,1));
    
    RM raymarch = RayMarch(ray_origin, ray_direction); 
   
    vec3 point = ray_origin + ray_direction * raymarch.travel;
   
    col = GetLight (point, ray_origin, raymarch.hit.color);
    
    fragColor = vec4(col,1.0);
}