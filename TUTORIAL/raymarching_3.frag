//TUTORIAL 3 http://xdpixel.com/ray-marching-101-part-3/
#define MAX_STEP 100
#define MAX_DIST 100.
#define PRECISION 0.01

float obj = 0.;


float GetDist(vec3 pos)
{
     
        
	vec4 sphere_center = vec4(0,1,6,1);
    
    float sphereDist = length(pos - sphere_center.xyz) - sphere_center.w; //dove w è il raggio
    
   	float planeDist = pos.y; //ipotizzo che esista un piano, la sua distanza è sempre la y della camera rispetto al mondo
    
    float closest = min(sphereDist, planeDist);
    if (closest == sphereDist) obj = 0.0;
    else obj = 1.0; 
    return closest;
}

float RayMarch(vec3 ray_origin, vec3 ray_direction)
{
	float travel = 0.0; 
    float obj = 0.0; 
    for (int i=0; i<MAX_STEP; i++)
    {
    	vec3 point = ray_origin + ray_direction * travel; 
        float dist = GetDist(point); 
        travel += dist;
        obj = dist;
        if (travel > MAX_DIST || dist < PRECISION) break;
    }
    return travel;
}

vec3 GetNormal(vec3 point)
{
	float dist = GetDist(point);
	
    vec2 epsilon = vec2 (0.01, 0.0);
    
    vec3 normal = dist - vec3( 
        GetDist(point-epsilon.xyy), //per capire lo slope, comparo i punti vicini al punto su cui calcolare la norm
		GetDist(point-epsilon.yxy),
        GetDist(point-epsilon.yyx) );
    
    return normalize (normal);
  }

float rampCoefficient(float t, int stripeCount)
{
	float fstripeCount = float (stripeCount);
    float modifiedT = mod(floor(t*fstripeCount),fstripeCount);
    float rampCoef = mix(0.1,1.0,modifiedT/(fstripeCount-1.0));
           
    return rampCoef;
}

vec3 GetLight(vec3 surfacePoint, vec3 cameraPosition, vec3 typeColor) 
{
	vec3 lightPosition = vec3 (0,4,10); 
    lightPosition.xz += vec2(sin(iTime)*10., cos(iTime)*10.);
    vec3 light = normalize (lightPosition - surfacePoint); 
    vec3 normal = GetNormal(surfacePoint); 
    
    float diffuse = clamp(dot(normal, light),0.0,1.0); //faccio il clamp in modo da non aver un valore negativo
    diffuse = rampCoefficient(diffuse, 4);
    vec3 diffuseColor = diffuse * typeColor;
    
    
    vec3 reflectedLight = normalize(reflect(-light, normal));
    float specular = pow(clamp(dot(reflectedLight, light), 0.0,1.0),10.0);                         
    specular = rampCoefficient(specular, 4);
    specular = min (diffuse, specular); 
    vec3 specularColor = specular * vec3(1.0);
                                    
                                    
     //per ottenere un'ombra posso fare il reymarch dal punto colpito alla posizione della luce, 
    //se la distanza è minore della distanza tra punto e luce, è stato colpito qualcosa
    vec3 finalColor = diffuseColor+specularColor;
    
    float hit = RayMarch(surfacePoint + (normal*PRECISION*2.), light);
    if (hit < length(surfacePoint-lightPosition))
 		finalColor *= 0.1;
    
   	return finalColor;
    
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    //normalizzo la finestra in uno spazio da 1 a -1 e cib -0.5 metto al centro il riferimento (0.0)
    vec2 uv = (fragCoord - 0.5 * iResolution.xy)/iResolution.y;

    
    vec3 col = vec3(0); 
    
    vec3 ray_origin = vec3(0,1,0);
    
    vec3 ray_direction = normalize(vec3(uv.x, uv.y,1));
    
    float dist = RayMarch(ray_origin, ray_direction); 
   
    vec3 point = ray_origin + ray_direction * dist;
    vec3 colType; 
    if (obj == 0.) colType = vec3(0.0,0.0,1.0);
        else colType = vec3(0.0,1.0,0.0);
    col = GetLight (point, ray_origin, colType);
    
    fragColor = vec4(col,1.0);
}