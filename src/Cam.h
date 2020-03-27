#pragma once

#include "AftrAPI.h"

namespace Aftr {

	class Cam {
		public:
			static Cam* New(Camera* cam) { return new Cam(cam); };

			~Cam(){};
			void onKeyDown(const SDL_KeyboardEvent& key);
			void onKeyUp(const SDL_KeyboardEvent& key);
			void update();
			void clear() { keysPressed.clear(); };

			Camera* getCamera() { return camera; };

		private:
			Camera* camera;
			Cam(Camera* cam) { camera = cam; };

			bool isMovementKey(SDL_Keycode key);

			std::set<SDL_Keycode> keysPressed;
	};
}

