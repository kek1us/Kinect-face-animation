#include "Globals.hpp"
#include "Kinect.hpp"
#include "Model.hpp"

//Delete console if not in debug mode
#if (!DEBUG_MODE)
	#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#endif

Kinect K;
Model M;

bool init() {
	// Initialize GLEW

	GLenum glewinit = glewInit();
	if (glewinit != GLEW_OK) {
		std::cout << "Glew not okay! " << glewinit;
		exit(EXIT_FAILURE);
	}

	// Clear color screen
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);

	// Accept fragment if it closer to the camera than the former one
	//glDepthFunc(GL_LEQUAL);
	//glDepthRange(0.1f, 1.0f);

	// Cull triangles which normal is not towards the camera
	//glCullFace(GL_BACK);

	// Enable blending
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Camera setup
	glViewport(0, 0, WNDW_WIDTH, WNDW_HEIGHT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, WNDW_WIDTH, WNDW_HEIGHT, 0, 1, -1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	return true;
}

int main() {
	sf::ContextSettings Settings;
	Settings.depthBits = 24; // Request a 24 bits depth buffer
	Settings.stencilBits = 8;  // Request a 8 bits stencil buffer
	Settings.antialiasingLevel = 2;  // Request 2 levels of antialiasing
	sf::Window window(sf::VideoMode(WNDW_WIDTH, WNDW_HEIGHT, 32), APP_NAME, sf::Style::Close, Settings);

	if (!init()) return 1;

	K.init();
	M.init();

	std::cout << "LOADING OK" << std::endl;

	sf::Clock clock;
	sf::Time elapsed;

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}

		elapsed = clock.getElapsedTime();
		if (elapsed.asSeconds() > 1.0 / 60) { // 60 fps = 1.0/60
			window.setActive();
			K.render();
			M.render();
			window.display();
		}
	}

	return 0;
}