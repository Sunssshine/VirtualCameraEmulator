#include "CameraDriver.h"
#include "Camera.h"

CameraDriver::CameraDriver(Camera* camera) : camera(camera) {}

void CameraDriver::addRoutePoint(const CameraRoutePoint& point) {
	currentRoute.push_back(point);
}

void CameraDriver::update(float delta) {
	if (currentRoute.empty()) {
		return;
	}

	auto & currentRoutePoint = currentRoute.front();
	auto deltaPos = currentRoutePoint.destinationPos - camera->Position;
	
	if (!isActive) {
		currentCameraVelocity = deltaPos / currentRoutePoint.duration;
		isActive = true;
	}

	if (currentRoutePoint.duration <= 0.0f) {
		currentRoute.erase(currentRoute.begin());
		isActive = false;
		update(delta);
		return;
	}	
	camera->setCameraDirection(currentRoutePoint.cameraDirection);
	auto newCameraPos = camera->Position + delta * currentCameraVelocity;
	currentRoutePoint.duration -= delta;
	camera->setCameraPosition(newCameraPos);
}

std::list<CameraRoutePoint> CameraDriver::getCurrentRoute() const {
	return currentRoute;
}
