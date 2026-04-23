#include "helper/scene.h"
#include "helper/scenerunner.h"
#include "sceneSpeedrunner.h"


int main(int argc, char* argv[])
{
	SceneRunner runner("Speedrunner - Bloom & Particles");

	std::unique_ptr<Scene> scene;

	scene = std::unique_ptr<Scene>(new SceneSpeedRuner());


	return runner.run(*scene);
}