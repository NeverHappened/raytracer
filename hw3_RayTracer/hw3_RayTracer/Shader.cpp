#include "Shader.h"

Shader::Shader(vector<Light> lights, int maxReflectionDepth) : lights(lights), maxReflectionDepth(maxReflectionDepth)
{}

Shader::~Shader()
{}

void Shader::addLight(Light light) {
	lights.push_back(light);
}

vec3 computeReflections(int currentDepth) {
	
}

RGB Shader::shade(Camera camera, Intersection intersection, Ray ray, PixelSample sample, vector<GameObject*> objects, int currentDepth = 0) {
	if (intersection.isIntersection()) {
		if (currentDepth != maxReflectionDepth) {
			vec4 reflection = computeReflections(currentDepth + 1);
			return RGB(computeLightOnHit(camera, intersection, ray, objects) + reflection);
		}
		else {
			return RGB(computeLightOnHit(camera, intersection, ray, objects));
		}
		
	}
	else {
		return RGB(0, 0, 0);
	}
}

double distance(vec3 vect) {
	return sqrt(vect.x * vect.x +
		vect.y * vect.y +
		vect.z * vect.z);
}

bool Shader::lightVisibleFromHere(Intersection intersection, Light light, vector<GameObject*> objects) {
	float epsilon = 0.005; // too big just to be sure
	vec3 lookToLightSource = light.getPosition() - intersection.getPosition();
	vec3 correctedStart = intersection.getPosition() + epsilon * normalize(lookToLightSource);
	Ray rayToLightSource(correctedStart, normalize(lookToLightSource));

	double distanceToLightSource = distance(lookToLightSource);
	Intersection onThePathToLight = IntersectionHelper::findClosestIntersection(rayToLightSource, objects);

	if (!onThePathToLight.isIntersection()) {
		return true;
	}
	else if (onThePathToLight.getDistance() > distanceToLightSource) {
		return true;
	}
	else {
		return false;
	}
}


double Shader::computeDistanceFromTo(Intersection intersection, Light light) {
	vec3 lookToLightSource = light.getPosition() - intersection.getPosition();
	return distance(lookToLightSource);
}

vec4 Shader::computeLightOnHit(Camera camera, Intersection intersection, Ray ray, vector<GameObject*> objects) {
	GameObject* obj = intersection.getObject();
	vec4 resultLight = obj->getAmbient() + obj->getEmission(); // no support for emission for now
	
	for (Light& light : lights) {
		if (!lightVisibleFromHere(intersection, light, objects)) {// TURN OFF SHADOWS FOR NOW
			continue;
		}
		
		resultLight += standardShadingFormula(camera, light, intersection);
	}
	
	return resultLight;
}

vec4 Shader::standardShadingFormula(Camera camera, Light light, Intersection intersection) {

	GameObject* obj = intersection.getObject();

	vec3 lightDirection;
	double attenuation = 1.0f;
	if (light.isDirectional()) {
		lightDirection = normalize(light.getPosition());
	}
	else {
		vec3 _light_direction = light.getPosition() - intersection.getPosition();
		lightDirection = normalize(_light_direction);
		float d = computeDistanceFromTo(intersection, light);
		float constant = light.getAttenuation().x;
		float linear = light.getAttenuation().y;
		float quadratic = light.getAttenuation().z;
		float attenuationFactor = constant + linear * d + quadratic * d * d;
		attenuation = 1.0f / attenuationFactor;
	}

	vec3 eyeDirection = normalize(camera.getEye() - intersection.getPosition());
	vec3 halfAngle = normalize(lightDirection + eyeDirection);
	vec3 _surfaceNormal = normalize(obj->getNormal(intersection.getPosition())); // TODO!: convert to real coordinates!
	vec3 surfaceNormal = normalize(vec3(transpose(inverse(obj->getTransform())) * vec4(_surfaceNormal, 0.0)));

	float dotL = dot(surfaceNormal, lightDirection);
	float dotH = dot(surfaceNormal, halfAngle);

	float diffuse_coeff = max(0.0f, dotL);
	float specular_coeff = pow(max(0.0f, dotH), obj->getShininess());

	vec4 diffuse_part = obj->getDiffuse() * light.getColor() * diffuse_coeff;
	vec4 specular_part = obj->getSpecular() * light.getColor() * specular_coeff; // phong illumination

	return attenuation * (diffuse_part + specular_part);
}
