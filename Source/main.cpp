#include "Globals.hpp"
#include "Kinect.hpp"
#include "Model.hpp"

//Delete console if not in debug mode


Kinect K;
Model M;
bool KinectOn;

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
	glEnable(GL_CULL_FACE);

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

	KinectOn = true;

	return true;
}

int main() {
	sf::ContextSettings Settings;
	Settings.depthBits = 24; // Request a 24 bits depth buffer
	Settings.stencilBits = 8;  // Request a 8 bits stencil buffer
	Settings.antialiasingLevel = 2;  // Request 2 levels of antialiasing
	sf::Window window(sf::VideoMode(WNDW_WIDTH, WNDW_HEIGHT, 32), APP_NAME, sf::Style::Close, Settings);
	window.setKeyRepeatEnabled(false); // not count key holding press

	if (!init()) exit(EXIT_FAILURE);

	if (!K.init()) KinectOn = false;
	M.init();

	std::cout << "LOADING OK" << std::endl;

	sf::Clock clock;
	sf::Time elapsed;

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			switch (event.type)
			{
				case sf::Event::Closed:
					window.close();
					break;
				case sf::Event::KeyPressed:
					switch (event.key.code) {
						case sf::Keyboard::P: K.playRecord(); break;
						case sf::Keyboard::R: K.record(); break;
						case sf::Keyboard::S: K.stopRecord(); break;
						case sf::Keyboard::Z: M.setShocked(); break;
						case sf::Keyboard::X: M.undoShocked(); break;
						case sf::Keyboard::Escape: window.close(); break;
						default: break;
					}
					break;
				default:
					break;
			}				
		}

		elapsed = clock.getElapsedTime();
		if (elapsed.asSeconds() > 1.0 / 60) { // 60 fps = 1.0/60
			window.setActive();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			if (KinectOn) {
				K.update();
				if (K.IsTracked()) {
					FLOAT* sc = K.GetScale();
					FLOAT* rot = K.GetRotation();
					FLOAT* trans = K.GetTranslation();
					M.registerResult(sc, rot, trans);
				}
				else M.stopAnimation();
			}
			M.update();
			K.render();
			M.render();
			window.display();
		}
	}

	return 0;
}