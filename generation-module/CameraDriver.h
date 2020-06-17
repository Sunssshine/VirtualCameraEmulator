#pragma once
#include "glm/vec3.hpp"

#include <list>

class Camera;

struct CameraRoutePoint {
	glm::vec3 destinationPos = {0.0f, 0.0f, 0.0f};
	glm::vec3 cameraDirection = {1.0f, 0.0f, 0.0f};
	float duration = 0.0f;
};

class CameraDriver {
public:
	CameraDriver(Camera* camera);
	void addRoutePoint(const CameraRoutePoint& point);
	void update(float delta);

	std::list<CameraRoutePoint> getCurrentRoute() const;
	
private:
	std::list<CameraRoutePoint> currentRoute;
	Camera* camera = nullptr;
	glm::vec3 currentCameraVelocity;
	bool isActive = false;
};