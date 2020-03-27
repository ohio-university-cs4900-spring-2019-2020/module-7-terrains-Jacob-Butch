#pragma once

#include "GLView.h"
#include "AftrAPI.h"
#include "GameModuleAPI.h"
#include "ISoundManager.h"
#include "PxPhysicsAPI.h"
#include <irrKlang.h>

namespace Aftr {
    class Camera;
    class NetMessengerClient;
    class IPhysicsManager;

    class GLViewModule : public GLView {
        public:
            static GLViewModule* New(const std::vector< std::string >& outArgs);
            virtual ~GLViewModule() { ISoundManager::drop(); IPhysicsManager::drop(); };
            virtual void init(float gravityScalar, Vector gravityNormalizedVector, std::string confFileName, const PHYSICS_ENGINE_TYPE& physicsEngineType);
            virtual void updateWorld(); ///< Called once per frame
            virtual void loadMap(); ///< Called once at startup to build this module's scene
            virtual void createNewModuleWayPoints();
            virtual void onResizeWindow(GLsizei width, GLsizei height) { GLView::onResizeWindow(width, height); };
            virtual void onMouseDown(const SDL_MouseButtonEvent& e);
            virtual void onMouseUp(const SDL_MouseButtonEvent& e);
            virtual void onMouseMove(const SDL_MouseMotionEvent& e);
            virtual void onMouseWheelScroll(const SDL_MouseWheelEvent& e);
            virtual void onKeyDown(const SDL_KeyboardEvent& key);
            virtual void onKeyUp(const SDL_KeyboardEvent& key);
            virtual int handleWindowEvent(SDL_WindowEvent& e);
            virtual int handleEvent(SDL_Event& sdlEvent);

            void processKeyPress(const SDL_Keycode& key);
            void makeToilet(Vector pos = Vector(0, 0, 0));

            void updateOrAdd(WO* wo);
            void removeWO(WO* wo);
            void toggleWarthog();
            void toggleBanner(bool toggle);

            int getIndex(WO* wo);
            int getIndexFromLabel(std::string label);
            WO* getFromLabel(std::string label);
            Camera* getCam() { return camera->getCamera(); };

            ProgressBar* progressBar;
            WOWarthog* warthog;
            GUI* banner;
            WorldGrid* grid;
            WOCity* city;
            WO* track_sphere;
            PauseMenu* pauseMenu;

            Vector centerOfWorld;
            Vector gravityDirection;

            std::map<WOPhysx*, int> worldObjects;
            int numToilets = 0;

        protected:
            GLViewModule(const std::vector<std::string>& args);
            virtual void onCreate();
            bool isDriving();
            bool isMovementKey(SDL_Keycode key);
            
            Cam* camera;
            NetMessengerClient* client;
            irrklang::ISound* citySound;
            physx::PxRigidStatic* groundPlane;
            std::set<SDL_Keycode> keysPressed;
    };
}

