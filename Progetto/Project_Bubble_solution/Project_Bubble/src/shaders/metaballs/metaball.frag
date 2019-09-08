

#version 330 core
//https://medium.com/@avseoul/ray-marching-metaball-in-unity3d-fc6f83766c5d
#define AA = 2
// output shader variable
out vec4 colorFrag;

// vertex position and normal in world coordinates
in vec4 worldPosition;
in vec3 worldNormal;

// texture sampler: we are using a cube map, so we need a samplerCube
uniform samplerCube tCube;

// camera position
uniform vec3 cameraPosition;

// point light position
uniform vec3 pointLightPosition;

// Eta (= ratio between the 2 refraction indices)
uniform float Eta;
// exponent of Fresnel equation (=5 in literature, but we can change its value to have more artistic control on the final effect)
uniform float mFresnelPower;

uniform float time; 

//other objects in scene
uniform vec4 blob0;
uniform vec4 blob1; 
uniform vec4 blob2; 
uniform vec4 blob3; 
uniform vec4 blob4; 
uniform vec4 blob5; 
uniform vec4 blob6; 
uniform vec4 blob7; 
uniform vec4 blob8; 
uniform vec4 blob9; 

vec4 fragCoord = gl_FragCoord;

//------------------- Variabili per il Setup
vec2 resolution = vec2(800,600);
vec2 pixel = -1.0+2.0*fragCoord.xy / resolution.xy;

vec3 world_up = vec3(0.0, 1.0, 0.0);
vec3 cam_forward;
vec3 cam_left;
vec3 cam_up;

float focal_length = 2.0;
//aggiungo al pixel, il dato che mi allontana il plane dalla cam
vec3 image_plane = vec3(pixel, focal_length);

vec3 ray_direction;

//------------------- Variabili per il March Rays
//numero che indica la precisione, cioè quanto deve essere vicino il raggio alla superficie (è approssimato) 
const float precis= 0.01;
//distanza massima del raggio, se il raggio è così lungo, si assume che non ci sia nulla
float maxd = 20.0;
//marching step, quanto avanza il ray 
float marchingStep; //h
//distanza percorsa
float travel; //t
//valore che fa da boolean integer
float out_of_range; 

//------------------- Variabili per il distance field
//vec4 blobs[10] = {blob0,blob1,blob2,blob3,blob4,blob5,blob6, blob7, blob8, blob9}; //posizione + raggio
vec4 blobs[10];
float m; 
float p;
//distanza minima tra il raggio corrente e la superficie più vicina
float dmin = 1e20;
float h; 

float db;

//------------------- Funzioni


//----------------------------------------------------------------

float hash1( float n )
{
    return fract(sin(n)*43758.5453123);
}

vec2 hash2( float n )
{
    return fract(sin(vec2(n,n+1.0))*vec2(43758.5453123,22578.1459123));
}

vec3 hash3( float n )
{
    return fract(sin(vec3(n,n+1.0,n+2.0))*vec3(43758.5453123,22578.1459123,19642.3490423));
}

//----------------------------------------------------------------


//----- distance of field
float sdMetaballs(vec3 pos){
	m = 0.0;
	p = 0.0;
	dmin = 1e20;
	h = 1.0; //lipshitz

	for (int i = 0; i < 10; i++)
	{
		//---bounding sphere
		//distanza dal raggio e la posizione del blob
		db = length(blobs[i].xyz - pos); 
		//se la distanza tra cam e centro del blob è minore rispetto al raggio del blob
		if (db < blobs[i].w) 
		{
		//il raggio è nel blob, quindi è da calcolare il field
		    float x = db/blobs[i].w;	//si normalizza la distanza tra 0 e 1
            p += 1.0 - x*x*x*(x*(x*6.0-15.0)+10.0);
            m += 1.0;
            h = max( h, 0.5333*blobs[i].w );
		}
		else //bounching sphere distance
		{
		//altrimenti il raggio è ancora fuori dal blob, aggiorno dmin con il valore nuovo 
		//la distanza minore tra dmin o tra raggio e blob
			dmin = min (dmin, db - blobs[i].w);			
		}
	}

	//si aggiunge un margine di errore per entrare nel bounding sphere
	float d  = dmin +0.1;

	if(m>0.5)
	{
		float th = 0.2;
		d = h*(th-p); //th è la thresholder
	}
	return d;
}

//-----MAP
float map(in vec3 p)
{
	return sdMetaballs(p);
}

//-----Normal calc
vec3 norMetaBalls( vec3 pos )
{
	vec3 nor = vec3( 0.0, 0.0001, 0.0 );
		
	for( int i=0; i<10; i++ )
	{
        float db = length( blobs[i].xyz - pos );
		float x = clamp( db/blobs[i].w, 0.0, 1.0 );
		float p = x*x*(30.0*x*x - 60.0*x + 30.0);
		nor += normalize( pos - blobs[i].xyz ) * p / blobs[i].w;
	}
	
	return normalize( nor );
}

vec3 calcNormal( in vec3 pos )
{

#ifdef ANALYTIC_NORMALS	
	return norMetaBalls( pos );
#else	
    vec3 eps = vec3(precis,0.0,0.0);
	return normalize( vec3(
           map(pos+eps.xyy) - map(pos-eps.xyy),
           map(pos+eps.yxy) - map(pos-eps.yxy),
           map(pos+eps.yyx) - map(pos-eps.yyx) ) );
#endif

}
//----- Intersection
//ro= origine del raggio, rd= direzione del raggio 
vec2 intersect (in vec3 ro, in vec3 rd) 
{
	marchingStep = precis*2.0;	
	travel = 0.0;
	out_of_range = 1.0;

	//ok, ora si itera finchè non si raggiunge la superficie di qualcosa
	for (int i = 0; i<75; i++)
	{
	//se ho fatto abbastanza passi da arrivare vicino alla superificie (primo) 
	//o se ne ho fatti troppi e sono andata troppo in la, stop 
		if (marchingStep<precis || travel > maxd) break;
		
		//aggiorno la distanza "percorsa"
		travel += marchingStep;

		//controllo la distance field, la map restituisce la distanza minima tra il raggio e la scena
		marchingStep = map(ro+rd*travel); //ro+rd*travel è la posisione corrente del raggio)
	}

	if (travel > maxd)
		out_of_range = -1.0;

	return vec2 (travel,out_of_range);
}



void main(){
//------------------
	//--------------------------------- Setup
	//---InitPlane
	
	//vec2 pixel = -1.0+2.0*fragCoord.xy / resolution.xy;
	//pixel.x *=resolution.x/resolution.y; //per avere il pixel largo giusto
	vec2 pixel = fragCoord.xy / resolution.xy;
	
	for( int i=0; i<10; i++ )
        {
            float h = float(i)/8.0;
            blobs[i].xyz = 2.0*sin( 6.2831*hash3(h*1.17) + hash3(h*13.7)*time );
            blobs[i].w = 1.7 + 0.9*sin(6.28*hash1(h*23.13));
		}

	//---LookAt
	cam_forward = normalize( vec3(0.0,0.0,0.0) - cameraPosition );
	cam_left = normalize( cross(cam_forward, world_up) );
	cam_up = normalize( cross(cam_left, cam_forward));

	//--- Field of view
	//vec3 ray_direction = normalize( pixel.x*cam_left + pixel.y*cam_up + 2.0*cam_forward );
	//moltiplico per il lookat, così il plane "guarda" nella camera (non c'è il backface)
	image_plane = vec3(
				image_plane.x * cam_left +
				image_plane.y * cam_up + 
				image_plane.z * cam_forward);
	//sommo la posizione così si muovoo insieme (cam + plane)
	image_plane = cameraPosition + image_plane;
	//creo il raggio facendo la differenza tra il piano e la camera, poi normalizzo
	ray_direction = normalize(image_plane - cameraPosition);

	//--------------------------------- March rays
	vec2 distanceField = intersect(cameraPosition,ray_direction);

	//--------------------------------- Rendering
	vec3 col = vec3 (1.0,1.0,1.0);
	vec3 tot = vec3(0.0,0.0,0.0);

        if( distanceField.y>-0.5 )
        {
            // geometry: surface_geometry = ray_origin + distance_field * ray_direction
            vec3 pos = cameraPosition + distanceField.x*ray_direction;
            vec3 nor = calcNormal(pos);
    		vec3 ref = reflect( ray_direction, nor );

            // materials
    		vec3 mate = vec3(0.0);
    		float w = 0.01;
    		
			for( int i=0; i<10; i++ )
    		{
    			float h = float(i)/8.0;

                // metaball color
    			vec3 ccc = vec3(1.0);
    			ccc = mix( ccc, vec3(1.0,0.60,0.05), smoothstep(0.65,0.66,sin(30.0*h)));
    			ccc = mix( ccc, vec3(0.3,0.45,0.25), smoothstep(0.65,0.66,sin(15.0*h)));
			
                float x = clamp( length( blobs[i].xyz - pos )/blobs[i].w, 0.0, 1.0 );
                float p = 1.0 - x*x*(3.0-2.0*x);
                mate += p*ccc;
                w += p;
            }
            mate /= w;

            // lighting
    		vec3 lin = vec3(0.0);
			lin += mix( vec3(0.05,0.02,0.0), 1.2*vec3(0.8,0.9,1.0), 0.5 + 0.5*nor.y );
            lin *= 1.0 + 1.5*vec3(0.7,0.5,0.3)*pow( clamp( 1.0 + dot(nor,ray_direction), 0.0, 1.0 ), 2.0 );
    		lin += 1.5*clamp(0.3+2.0*nor.y,0.0,1.0)*pow(texture( tCube, ref ).xyz,vec3(2.2))*(0.04+0.96*pow( clamp( 1.0 + dot(nor,ray_direction), 0.0, 1.0 ), 4.0 ));

	    	// surface-light interacion
    		col = lin * mate;
	    }
        // gamma
		
	    col = pow( clamp(col,0.0,1.0), vec3(0.45) );
		tot += col;

	colorFrag = vec4(tot,1.0);
	/*

//------------------
    vec3 N = normalize(worldNormal);

    // vector from vertex to camera in world coordinates
    vec3 V = normalize(worldPosition.xyz - cameraPosition);

    // incidence light direction in world coordinates
    vec3 L = normalize(worldPosition.xyz - pointLightPosition);

    // half vector in world coordinates
    vec3 H = normalize(L + V);

    // reflection vector of V with respect to normal
    vec3 R = reflect(V, N);

    // we sample the texture cube using the components of the reflection vector as texture coordinates.
    vec4 reflectedColor = texture( tCube, R);

    // refraction vectors
    // we use 3 vectors, one for each RGB channel, to simulate chromatic aberration:
    // we use a slightly different value of Eta in the second and third vector, in order to have 3 slightly different colors
    vec3 refractDir[3];
    refractDir[0] = refract( V , N, Eta );
    refractDir[1] = refract( V , N, Eta * 0.99 );
    refractDir[2] = refract( V , N, Eta * 0.98 );

    // we sample 3 times the cube map for the refraction, and we use only 1 channel for each for each sample
    vec4 refractedColor = vec4( 1.0 );
    refractedColor.r = texture( tCube,refractDir[0]).r;
    refractedColor.g = texture( tCube,refractDir[1]).g;
    refractedColor.b = texture( tCube,refractDir[2]).b;

    // Fresnel equation with Schlik's approximation
    // F(0°) factor
    float F0 = ((1.0-Eta)*(1.0-Eta)) / ((1.0+Eta)*(1.0+Eta));
    // ratio between reflection and refraction
    float Ratio = F0 + (1.0-F0) * pow( 1.0 - max(dot(V, H),0.0), mFresnelPower );

    // we merge the 2 colors, using the ratio calculated with the Fresnel equation
    colorFrag = mix( refractedColor, reflectedColor, clamp( Ratio, 0.0, 1.0 ) );*/
}
