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
uniform sampler2D noise;

uniform float time; 
//ci sono 4 shader, 0= blinn phong, 1= reflection, 2= fresnel, 3= fresnel con un eta differente
uniform int current_shader;
uniform int camMov; 
uniform int dithering;
uniform int plane; 
uniform int second_pass; 
uniform int repetition; 


uniform vec3 cameraPosition;
uniform mat4 camera; 

uniform vec2 resolution;
uniform vec2 noise_resolution;

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
	float morph; 
	float newShape; 
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

//struttura dati che restituisce la distanza percorsa e cosa è stato colpito (mi serve per il colore principalmente)
struct RM{
    Hit hit;
    float travel;
};

mat2 Rotation(float a) {
    float s = sin(a);
    float c = cos(a);
    return mat2(c, -s, s, c);
}
//-------------------- Gli operatori
float SmoothUnion( float d1, float d2, float k ) {
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
	return mix( d2, d1, h ) - k*h*(1.0-h); 
}

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
Hit df_plane (vec3 rayPos)
{
	Hit hit;
	hit.subject = false; 
	//vec4 n = vec4(0., 1., 0., 0.);
   // hit.dist =  dot(rayPos,n.xyz) + n.w + 0.1*sin(rayPos.z*3.0)*sin(rayPos.x*3.0);

	hit.dist = 2+rayPos.y;
	return hit;
}

Hit df_Sphere(vec3 rayPos, vec3 spherePos, float size)
{	
	float d = length(rayPos - spherePos) - size;
    Hit hit;
    hit.dist = d;
	hit.subject = true; 
	return hit;
}

Hit df_Box(vec3 rayPos, vec3 pos, float s )
{
  pos = rayPos - pos; 
  vec3 size = vec3(s); 
  vec3 d = abs(pos) - size;
  float dist = length(max(d,0.0));
         + min(max(d.x,max(d.y,d.z)),0.0); // remove this line for an only partially signed sdf 

  Hit hit; 
  hit.subject = true; 
  hit.dist = dist; 
  return hit; 
}

Hit df_Torus( vec3 rayPos, vec3 pos, float rad) //radius have 2 radius, the main shape and the radius of the border
{
	pos = rayPos - pos; 
	vec2 radius = vec2(rad, rad*0.3); 
	vec2 q = vec2(length(pos.xz)-radius.x,pos.y);
	float dist = length(q)-radius.y;

	Hit hit; 
	hit.subject = true; 
	hit.dist = dist; 
	return hit; 
}
//------------

Hit ShapeDistance(vec3 rayPos, Blob blobs){
	if(repetition == 1.0)
	{//k deve essere più grande della metà dell'oggetto considerato
		float k = 10.0; 
		rayPos.z = mod((rayPos.z),k); 
		rayPos -=vec3(k * 0.5); 
	}

	if(blobs.shape == 0)
		return df_Sphere(rayPos, blobs.position, blobs.size);
	if(blobs.shape == 1)
		return df_Box(rayPos, blobs.position, blobs.size);
	if(blobs.shape == 2)
		return df_Torus(rayPos, blobs.position, blobs.size);
}

vec3 GetColor(vec3 rayPos){ 
	vec3 color = vec3(0.0); 
	if(plane == 1.0) 
	 color = max(0.0, 1.0 -df_plane(rayPos).dist) * vec3(0.0,0.4,0.0)*1.0 ; 
	for (int i = 0; i<10; i++)
		if(blobs[i].operator != 1.0)
			color += max(0.0 , 1.0 -ShapeDistance(rayPos, blobs[i]).dist) *blobs[i].color*1.0; 

	return color;
}

Hit ChangeShape(Blob blob, vec3 pos)
{
	float f = mod((blob.shape + 1),3);
	Blob b1 = blob;
	b1.shape = mod((blob.newShape),3);
			
	Hit v1 = ShapeDistance(pos, blob);
	Hit v2 = ShapeDistance(pos, b1); 
	Hit hit;
	hit.dist = mix(v1.dist,v2.dist, sin(time)*.5+.5); 

	return  hit; 
}

float GetBorder (vec3 rayPos){
	float border = 0.0;
	for(int i = 0; i<10; i++){
		Hit shape = blobs[i].morph == 0.0 ? ShapeDistance(rayPos, blobs[i]) : ChangeShape(blobs[i], rayPos); 
		if(shape.dist < 0.3 )
			border += blobs[i].selected; 
	}
	return clamp (border, 0.0,1.0); 
}


Hit GetDist(vec3 pos)
{
    float obj;
	vec4 sphere_center = vec4(0,1,6,1);

    Hit result;
    result.dist = 1e20; 
	if(plane == 1.0)
		result = df_plane (pos); //ipotizzo che esista un piano, la sua distanza è sempre la y della camera rispetto al mondo

   //operation 
   Hit shape;
	
	//Union
	for (int i = 0; i < 10; i++){
		if(blobs[i].operator == 0){
			shape = blobs[i].morph == 0.0 ? ShapeDistance(pos, blobs[i]) : ChangeShape(blobs[i], pos); 
			result.dist = SmoothUnion(result.dist, shape.dist, 0.5);
		}
	}

	//Subtraction
	for (int i = 0; i<10; i++)
	{
		if(blobs[i].operator == 1){
			shape = blobs[i].morph == 0.0 ? ShapeDistance(pos, blobs[i]) : ChangeShape(blobs[i], pos); 
			result.dist = SmoothSubtraction( shape.dist,result.dist, 0.5);
		}
	}

	//Intersection
	for (int i = 0; i<10; i++)
	{
		if(blobs[i].operator == 2){
			shape = blobs[i].morph == 0.0 ? ShapeDistance(pos, blobs[i]) : ChangeShape(blobs[i], pos);
			result.dist = SmoothIntersection( shape.dist,result.dist, 0.5);
		}
	}
	
	result.color = GetColor(pos); 
	result.selected = GetBorder(pos); 
    return result; 
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

float SoftShadow( vec3 ro, vec3 rd)
{
	float res = 1.0;
    float t = 0.1;
     
    for( int i=0; i<100; i++ )
    {
		float h = RayMarch( ro, rd ).travel;
        res = min( res, 5.0*h/t );
        t += h;
        
        if( res<0.0001 || t>100 ) break;
    }
    return clamp( res, 0.0, 1.0 );
}



float dither8x8(vec2 uv, float brightness) {  
	float limit = texture(noise, uv / noise_resolution[0]*10).b ;
	float result; 
	
	result = brightness < limit ? 0.0 : 1.0;
	return result; 
}


//////////////////////////////////////////////////////////////////////////
vec4 Blinnphong(vec3 normal, vec3 light, vec3 objColor, bool shadow){
			float diffuse = clamp(dot(normal, light),0.0,1.0); //faccio il clamp in modo da non aver un valore negativo
			vec3 diffuseColor = diffuse * objColor;
			float specular = diffuse;
			vec3 specularColor = diffuseColor; 

			if(!shadow)	
			{
				vec3 reflectedLight = normalize(reflect(-light, normal));
				specular= pow(clamp(dot(reflectedLight, light), 0.0,1.0),10.0);
				specular = min (diffuse, specular);

				specularColor = specular * vec3(1.0);
			    return vec4(diffuseColor+specularColor,1.0);
			}
			return vec4(diffuseColor,1.0);	
}

vec4 Stripes (vec3 normal, vec3 light, vec3 objColor, bool shadow ){
		float diffuse = clamp(dot(normal, light), 0.0,1.0);
		diffuse = RampCoeff(diffuse, 4);
		vec3 diffuseColor = diffuse * objColor;
			if(!shadow){
			vec3 reflectedLight = normalize (reflect(-light, normal));
		
			float specular = pow(clamp(dot(reflectedLight, light), 0.0,1.0), 10.);
			specular = RampCoeff(specular, 4);
			specular = min (diffuse, specular); 
			vec3 specularColor = specular * vec3(1.0);

			return vec4(diffuseColor + specularColor,1.0); 		
		}
		return vec4(diffuseColor,1.0); 		
}

vec4 Reflection(vec3 surfacePoint, vec3 cameraPosition, vec3 normal){
		vec3 V = normalize(surfacePoint - cameraPosition);
        vec3 R = reflect(V, normal);
		return texture (tCube, R);
}

vec4 Fresnel(vec3 surfacePoint, vec3 cameraPosition, vec3 lightPosition, vec3 normal){
		vec3 V = normalize (surfacePoint - cameraPosition);
		vec3 L = normalize (surfacePoint - lightPosition);
		vec3 H = normalize (L + V); 
		vec3 R = reflect (V, normal); 
		vec4 reflectedColor = texture(tCube, R); 

		//chromaticabb

		float Eta = current_shader == 3 ? 1.00/1.52 : 1.01; //frsnel or bubble

		vec3 refractDir[3];
		refractDir[0] = refract(V, normal, Eta *.99 );
		refractDir[1] = refract(V, normal, Eta *.98 );
		refractDir[2] = refract(V, normal, Eta *.97 );

		vec4 refractedColor = vec4(0.0); 
		refractedColor.r = texture(tCube, refractDir[0]).r;
		refractedColor.g = texture(tCube, refractDir[1]).g;
		refractedColor.b = texture(tCube, refractDir[2]).b;

		//fresnel
		float F0 = ((1.0-Eta)*(1.0-Eta)) / ((1.0+Eta)*(1.0+Eta));
   		float Ratio = F0 + (1.0-F0) * pow( 1.0 - max(dot(V, H),0.0), 5.0 );

		return mix(refractedColor, reflectedColor, clamp (Ratio, 0.0,1.0));
}
////////////////////////////////////////////////////////////////

vec4 Rendering(vec3 surfacePoint, vec3 cameraPosition, Hit target)
{
	vec3 lightPosition = vec3 (0,5,0);
	lightPosition.xz += vec2(sin(time*0.5)*4., cos(time*0.5)*4.);
	vec3 light = normalize (lightPosition - surfacePoint);
	vec3 normal = GetNormal(surfacePoint);
  	vec4 finalColor = vec4(4.,1.,1.,1.0); 
	vec3 toCamera = normalize(cameraPosition - surfacePoint); 
		
	float shadowHit; 
	bool shadow;
	//Shadow color
	if(current_shader != 3 && current_shader != 4 && !target.subject){
		shadowHit = RayMarch(surfacePoint + (normal*PRECISION*2.), light).travel;
		 shadow = shadowHit < length(surfacePoint-lightPosition);
		//finalColor *= SoftShadow(surfacePoint + (normal*PRECISION*2.), light);
	}

	//blinphong
	if(current_shader == 0)
			finalColor = Blinnphong(normal, light, target.color, shadow);
	
	//stripes color
	if(current_shader == 1) 
		finalColor = Stripes(normal, light, target.color, shadow);	
	
	//reflection
	if(current_shader == 2)
		finalColor = Reflection(surfacePoint, cameraPosition, normal); 

	//fresnel
	if(current_shader == 3 || current_shader == 4)
		finalColor = Fresnel(surfacePoint, cameraPosition, lightPosition, normal); 

	if(shadow)
		finalColor*=0.4;

	//selection shape
	if(target.selected==1.0) {
		float border = dot(toCamera, normal); 
		if(border > -0.3 && border <0.3)
			finalColor = vec4(1.0);
	}
	
	//dittering
	if(dithering == 1.0) 	{
		float brightness = clamp(dot(normal, light), 0.0, 1.0);
		if(shadowHit < length(lightPosition - surfacePoint)) brightness *= 0.3;
		
		finalColor *= dither8x8(gl_FragCoord.xy, brightness); 
	}

	return finalColor;
}

vec4 SecondPass(vec3 ray_direction, vec3 surfacePoint, vec3 camera){

	vec3 normal = GetNormal(surfacePoint); 
	vec3 new_direction = normalize(reflect (ray_direction, normal)); 

	//trovo un nuovo punto 
	RM refl_hit = RayMarch(surfacePoint+ (normal*PRECISION*2.), new_direction); 
	vec3 new_surface = surfacePoint + new_direction * refl_hit.travel;
	
	vec3 normal_new_surface = GetNormal(new_surface);
	
	vec3 lightPosition = vec3 (0,5,0);
	vec3 light_dir = normalize(lightPosition-new_surface); 
	float diffuse = clamp(dot(normal_new_surface , light_dir), 0.0, 1.0); 
	float specular = pow(clamp(dot(reflect(-light_dir, normal), -new_direction), 0.0,1.0),10.0);
	vec3 col = GetColor(new_surface); 

	col = (col * (diffuse ) + vec3(1.0)*specular*1.5); 


	vec4 color = vec4(col, 1.0); 
	if(dithering == 1.0) 	{
		float brightness = clamp(dot(normal_new_surface, light_dir), 0.0, 1.0);
		
		color *= dither8x8(gl_FragCoord.xy, brightness); 
	}
	return color;
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
//vec4 Background( vec2 uv )
vec4 Background(vec3 dir) 
{
	//BackGroundGradient(uv); 
   vec4 color = texture (tCube, dir);
   if(dithering == 1.0) {
	color.x *= dither8x8(gl_FragCoord.xy, color.r); 
	color.y *= dither8x8(gl_FragCoord.xy, color.g); 
	color.z *= dither8x8(gl_FragCoord.xy, color.b); 
   }
   
   return vec4(color  ); 
}

vec3 getDirection(vec2 uv, vec3 position, vec3 dest, float value){
	vec3 f = normalize(dest - position); 
	vec3 r = normalize (cross ( f,vec3(0.0,1.0,0.0)));	
	vec3 u = normalize(cross(r,f)); 
	
	vec3 i = uv.x*r + uv.y*u + value*f;
	
	return normalize(i); 
}

//------------------------------------------------------- MAIN
void main()
{
    //normalizzo la finestra in uno spazio da 1 a -1 e -0.5 per mettere al centro il riferimento (0.0)
	vec2 uv = gl_FragCoord.xy/resolution.xy; 

	uv -= 0.5;
	uv.x *= resolution.x/resolution.y; 
	vec4 col; 
	col = vec4(0.0);

	for (int i = 0; i < 10; i++)
    {
        blobs[i].position = blobsPos[i].xyz;
		blobs[i].size = blobsPos[i].w;
		blobs[i].color = info1[i].xyz;
		blobs[i].selected = info1[i].w;
		blobs[i].shape = info2[i].x;
		blobs[i].newShape = mod((blobs[i].shape+1),3);
		blobs[i].operator = info2[i].y;
		blobs[i].morph = info2[i].z;
    }

	vec3 ray_origin = vec3(0.0,1.0,-30.0);
	if (camMov == 1){
		ray_origin = vec3(0.0f, 10.0f, -30.0f) + (5.0*sin(0.3*time),0.0,-5.0*cos(0.3*time));
		ray_origin.xz *= Rotation(0.5*time);
	}

	vec3 ray_direction = getDirection(uv, ray_origin, vec3(0.0,1.0,0.0), 2); 
	
	RM raymarch = RayMarch(ray_origin, ray_direction);

	
	col = vec4(vec3(raymarch.travel/35.), 1.0);
	if(current_shader != 5){
		if (raymarch.hit.dist < PRECISION)
		{
    		vec3 point = ray_origin + ray_direction * raymarch.travel;
			
    		col = Rendering (point, ray_origin, raymarch.hit);
			//second pass
			if(second_pass == 1.0)
				col += SecondPass(ray_direction, point, cameraPosition)*0.2;
		}		
		else col = Background(ray_direction);
	}
		//if( >0.0)  col = vec4(1.0); 
		
	
    colorFrag = col;
}
