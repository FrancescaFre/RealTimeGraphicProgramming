
#version 330 core

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

uniform float iTime; 

/** VARIABLES **/

const vec3 light_pos = vec3(-2.0, 4.0, -1.0);
const vec3 c_light_pos = vec3(2.0, -4.0, -0.0);

const int 	TRACE_STEPS 		= 248;
const float TRACE_EPSILON 		= 0.00000000001; // 0.00000001;
const float REFLECT_EPSILON 	= 100.0;
const float TRACE_DISTANCE		= 50.0;
const float NORMAL_EPSILON		= 1.0;
const int   REFLECT_DEPTH		= 50;
const int 	NUM_BALLS			= 10;

const vec2 iResolution = vec2 (800.0f, 600.0f); 


vec3 balls[NUM_BALLS];




/* FUNCTIONS */
void update_balls_position() {
	float t = iTime * 0.1;
	for (int i = 0; i < NUM_BALLS; ++i) {
		balls[i] = 3. * vec3(
			sin(2.3+float(i+2)*t),
			cos(1.7+float(-5+i)*t),
			1.1*sin(3.0+float(i+7)*t));
	}
}


float metaballs_field(in vec3 at) {
	float sum = 0.;
	for (int i = 0; i < NUM_BALLS; ++i) {
		float r = length(balls[i] - at);
		
		sum += 1.0 / ( r * r * r * (r * (r * 6.0 - 15.0) + 10.0));
	    // sum += 1.0 / ( r * r * r * r - r * r + 0.25);
	}
	return 1. - sum;
}



vec3 normal(in vec3 at) {
	vec2 e = vec2(0.0, NORMAL_EPSILON);
	return normalize(vec3(metaballs_field(at+e.yxx)-metaballs_field(at), 
						  metaballs_field(at+e.xyx)-metaballs_field(at),
						  metaballs_field(at+e.xxy)-metaballs_field(at)));
}

vec4 raymarch(in vec3 pos, in vec3 dir, in float maxL) {
	float l = 0.;
	for (int i = 0; i < TRACE_STEPS; ++i) {
		float d = metaballs_field(pos + dir * l);
		if (d < TRACE_EPSILON*l)
			break;
		l += d;
		if (l > maxL) break;
	}
	return vec4(pos + dir * l, l);
}

vec3 lookAtDir(in vec3 dir, in vec3 pos, in vec3 at) {
	vec3 f = normalize(at - pos);
	vec3 r = cross(f, vec3(0.,1.,0.));
	vec3 u = cross(r, f);
	return normalize(dir.x * r + dir.y * u + dir.z * f);
}


void main ( ) {
//	update_balls_position();
	
	float t = iTime * 0.1;
	float aspect = iResolution.x / iResolution.y;
	//vec2 uv = (fragCoord.xy / iResolution.xy * 2. - 1.) * vec2(aspect, 1.);
	vec2 uv = (iResolution.xy / iResolution.xy * 2. - 1.) * vec2(aspect, 1.);

	
	vec3 pos = vec3(cos(2.+4.*cos(t))*10., 2.+8.*cos(t*.8), 10.*sin(2.+3.*cos(t)));
	vec3 dir = lookAtDir(normalize(vec3(uv, 2.)), pos.xyz, vec3(balls[0]));
	
	vec4 color = vec4(0.0, 0.0, 0.0, 1.0);

	
	for (int reflections = 0; reflections < REFLECT_DEPTH; ++reflections) {
		vec4 tpos = raymarch(pos, dir, TRACE_DISTANCE);
		if (tpos.w >= TRACE_DISTANCE) {
			color += vec4(0.0, 0.0, 0.0, 0.0);
			break;
		}
		
		// color 
		vec3 norm = normal(tpos.xyz);
		vec3 light_norm = normalize(light_pos - tpos.xyz); 
		vec4 diffuse = max(dot(norm, light_norm), 0.0) * vec4(0.5, 0.0, 0.0, 1.0); 
		vec3 r_light = normalize(reflect(light_norm, norm));
		vec4 specular = vec4(1.0) * pow(max(dot(r_light, -dir), 0.0), 4.0);
		vec3 c_light_normal = normalize(c_light_pos - tpos.xyz);
		vec3 c_r_light = normalize(reflect(c_light_normal, norm));
		vec4 c_specular = vec4(0.0, 0.0, 0.7, 1.0) * pow(max(dot(c_r_light, -dir), 0.0), 4.0);
	   
		color = vec4(0.1, 0.0, 0.0, 0.0) + diffuse + specular + c_specular;
		
		color -= color.w * 0.004;

		dir = normalize(reflect(dir, normal(tpos.xyz)));
		pos = tpos.xyz + dir * REFLECT_EPSILON;
	}
	colorFrag = color;
}