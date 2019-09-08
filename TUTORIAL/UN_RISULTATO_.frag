#define MAX_STEP 300
#define MAX_DIST 100.
#define PRECISION 0.01

#define TopColor            vec3( 0.35, 0.4, 0.8 )
#define MiddleColor         vec3( 0.8, 0.8, 0.8 )
#define BottomColor         vec3( 0.55, 0.2, 0.6 )

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
//
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

  //operation  
    Hit sphere0 = df_Sphere(pos, blobs[0].position, blobs[0].size, blobs[0].color);
	Hit sphere1 = df_Sphere(pos, blobs[1].position, blobs[1].size, blobs[1].color);
	
   // float op = Intersection(sphere0.dist, sphere1.dist);
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

    //float ops = SmoothIntersection(sphere2.dist, sphere3.dist, 0.2);
    //float ops = SmoothSubtraction(sphere2.dist, sphere3.dist,0.2);
    float ops = SmoothUnion(sphere2.dist, sphere3.dist, 0.2);

    if (ops < result.dist)
	{
        sphere3.dist = ops;
        result = sphere3;
    }
	//-----------------------


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
	vec3 lightPosition = vec3 (0,5,5);
    lightPosition.xz += vec2(sin(iTime)*2., cos(iTime)*2.);
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

//----------- BG
vec3 BackGroundGradient( vec2 uv )
{
   uv.y *= -1.0;
   vec3 color = vec3(0.0);
   if( uv.y <= 0.0 )
   {
       color = mix( TopColor, MiddleColor, uv.y + 1.0 );
   }
   else
   {
       color = mix( MiddleColor, BottomColor, uv.y );
   }
   return color;
}

vec3 Sun( vec2 uv, vec2 pos, float size, float strength )
{
   float t = pow( abs( 1.0/(length( uv + pos) * size) ), strength );
   return t * vec3( 5.0, 3.0, 2.0 );
}

//--------------------------------------------------------------------------------
vec3 Background( vec2 uv )
{
   vec3 color = BackGroundGradient(uv);

   float sunStrength = mix( 2.8, 3.0, sin( iTime ) * 0.5 + 0.5 );
   color += Sun( uv, vec2( -1.4, -0.6 ), 10.0, sunStrength );

   return color;
}

//------------------------------------------------------- MAIN
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    //normalizzo la finestra in uno spazio da 1 a -1 e cib -0.5 metto al centro il riferimento (0.0)
    vec2 uv = (fragCoord - 0.5 * iResolution.xy)/iResolution.y;

    for (int i = 0; i < 4; i++)
    {
        blobs[i].position = vec3((float(i)-1.5)*2., 1.0, 6.0);
        float m = mod(float(i),2.);
        blobs[i].position.xz += vec2(sin(iTime)*(1.-m), cos(iTime)*(1.-m));
        blobs[i].size = 1.0;
    	blobs[i].color = vec3(float(i)*0.2,float(i)*0.4,float(i)*0.9);
    }

    vec3 col = vec3(0);

    vec3 ray_origin = vec3(0,1,0);

    vec3 ray_direction = normalize(vec3(uv.x, uv.y,1));

    RM raymarch = RayMarch(ray_origin, ray_direction);

		if (raymarch.hit.dist < PRECISION)
		{
    	vec3 point = ray_origin + ray_direction * raymarch.travel;

    	col = GetLight (point, ray_origin, raymarch.hit.color);
		}
		else col = Background(uv);

    fragColor = vec4(col,1.0);
}
