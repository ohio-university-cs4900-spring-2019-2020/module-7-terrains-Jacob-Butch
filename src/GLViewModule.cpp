#include "GLViewModule.h"

#include "WorldList.h" //This is where we place all of our WOs
#include "ManagerOpenGLState.h" //We can change OpenGL State attributes with this
#include "Axes.h" //We can set Axes to on/off with this
#include "PhysicsEngineODE.h"
#include "AftrGLRendererBase.h"
#include "NetMessengerServer.h"

using namespace Aftr;
using namespace irrklang;
using namespace physx;

const std::string sharedMM = ManagerEnvironmentConfiguration::getSMM();

GLViewModule* GLViewModule::New(const std::vector<std::string>& args){
    GLViewModule* glv = new GLViewModule(args);
    glv->init(Aftr::GRAVITY, Vector(0.0f, 0.0f, -1.0f), "aftr.conf", PHYSICS_ENGINE_TYPE::petODE);
    glv->onCreate();
    return glv;
}

GLViewModule::GLViewModule(const std::vector<std::string>& args) : GLView(args) {
    this->progressBar = nullptr;
    this->warthog = nullptr;
    this->banner = nullptr;
    this->grid = nullptr;
    this->city = nullptr;
    this->track_sphere = nullptr;
    this->citySound = nullptr;
    this->client = nullptr;
    this->groundPlane = nullptr;
    this->centerOfWorld = Vector(0.f, 0.f, 0.f);
    this->gravityDirection = Vector(0.f, 0.f, 0.f);
    this->pauseMenu = nullptr;
    this->camera = nullptr;
}

void GLViewModule::init(float gravityScalar, Vector gravityNormalizedVector, std::string configFileName, const PHYSICS_ENGINE_TYPE& physicsEngineType) {
    // setup sound and physics managers
    ISoundManager::init();
    IPhysicsManager::init();
    GLView::init(gravityScalar, gravityNormalizedVector, configFileName, physicsEngineType);
}

void GLViewModule::onCreate(){
    //GLViewModule::onCreate() is invoked after this module's LoadMap() is completed.
    if( this->pe != NULL ){
        //optionally, change gravity direction and magnitude here. The user could load these values from the module's aftr.conf
        this->pe->setGravityNormalizedVector(Vector(0.f, 0.f, -1.f));
        this->pe->setGravityScalar(GRAVITY); 
    }
    this->setActorChaseType(STANDARDEZNAV); //Default is STANDARDEZNAV mode
    // setup Cam
    this->camera = Cam::New(this->cam);

    // create world grid after physics is setup
    this->grid = WorldGrid::New(this->centerOfWorld, this->gravityDirection);
    this->worldLst->push_back(this->grid);

    //this->city = WOCity::New();
    //this->worldLst->push_back(this->city);
    // setup background city noises
    //this->citySound = ISoundManager::engine->play2D("../mm/sounds/citysounds.ogg", true);  

    // make progressbar
    this->progressBar = ProgressBar::New();
    this->worldLst->push_back(this->progressBar->bar);
    this->worldLst->push_back(this->progressBar->progress);

    // make pause menu
    this->pauseMenu = PauseMenu::New();
    this->worldLst->push_back(this->pauseMenu->exit);
    this->worldLst->push_back(this->pauseMenu->resume);
    //this->worldLst->push_back(this->pauseMenu->restart);

    // make banner
    this->banner = GUI::New(nullptr, 0.3f, 0.1f, "../mm/images/banner.png");
    this->banner->setPosition(Vector(0.5f, 0.9f, 0.f));
    this->banner->setTextFont(sharedMM + "/fonts/arial.ttf");
    this->banner->setText("Press 't' to summon a toilet!");
    this->banner->setLabel("Banner");
    this->banner->isFocusable(false);
    this->banner->receivesEvents(false);
    this->worldLst->push_back(this->banner);
    
    // setup messenger client
    std::string port = ManagerEnvironmentConfiguration::getVariableValue("NetServerListenPort");
    this->client = port == "12683" ? NetMessengerClient::New("127.0.0.1", "12682") : NetMessengerClient::New("127.0.0.1", "12683");
}

void GLViewModule::updateWorld(){
    if (!this->pauseMenu->isPaused()) {
        GLView::updateWorld();
        ISoundManager::setListenerPosition(this->cam->getPosition(), this->cam->getLookDirection(), Vector(0.f, 0.f, 0.f), this->cam->getNormalDirection());
        // update warthog
        if (this->isDriving()) {
            this->warthog->update();
            if (this->client->isTCPSocketOpen()) {
                NetMsgWO netMsg = NetMsgWO::makeNetMsgWO(this->warthog->getPosition(), this->warthog->getDisplayMatrix(), this->getIndex(this->warthog));
                this->client->sendNetMsgSynchronousTCP(netMsg);
            }
        }
        //this->warthog->updateLabel();
        // progress bar
        if (this->client->isTCPSocketOpen()) {
            if (this->progressBar != nullptr) {
                this->client->sendNetMsgSynchronousTCP(NetMsgProgressBar(this->progressBar->getProgressWidth()));
            }
        }
        // update progress bar
        if (this->progressBar != nullptr) {
            if (this->progressBar->fill) {
                this->progressBar->fillBar();
            }
            else if (this->progressBar->empty) {
                this->progressBar->emptyBar();
            }
            if (this->progressBar->isFull()) {
                this->makeToilet();
                this->progressBar->reset();
            }
        }
        //for (SDL_Keycode key : this->keysPressed) {
        //    this->processKeyPress(key);
        //}
        this->camera->update();
        // update physics
        IPhysicsManager::simulate(this->client, this->worldObjects);
    }
}

void GLViewModule::updateOrAdd(WO* wo) {
    int index = this->getIndex(wo);
    if (index != -1) {
        this->worldLst->at(index)->setPosition(wo->getPosition());
        this->worldLst->at(index)->getModel()->setDisplayMatrix(wo->getDisplayMatrix());
        return;
    }
    index = this->getIndexFromLabel(wo->getLabel());
    if (index != -1) {
        this->worldLst->at(index)->setPosition(wo->getPosition());
        this->worldLst->at(index)->getModel()->setDisplayMatrix(wo->getDisplayMatrix());
        return;
    }
    this->worldLst->push_back(wo);
}

void GLViewModule::removeWO(WO* wo) {
    this->worldLst->eraseViaWOptr(wo);
}

void GLViewModule::toggleWarthog() {
    if (this->warthog == nullptr) {
        this->warthog = WOWarthog::New();
        this->warthog->setPosition(this->cam->getPosition());
        this->worldLst->push_back(this->warthog);
        this->worldLst->push_back(this->warthog->getSpeedometer());
        this->warthog->setDriver(this->cam);
    } else {
        this->warthog->setDriver(nullptr);
        this->removeWO(this->warthog);
        this->removeWO(this->warthog->getSpeedometer());
        this->warthog = nullptr;
    }
}

void GLViewModule::toggleBanner(bool toggle) {
    if (toggle) {
        this->banner->setWidthGUI(0.3f);
        if (this->numToilets <= 0) {
            this->banner->setText("Press 't' to summon a toilet!");
        } else {
            this->banner->setText(std::to_string(this->numToilets) + (this->numToilets == 1 ? " toilet" : " toilets"));
        }
    } else {
        this->banner->setWidthGUI(0.0f);
        this->banner->setText("");
    }
}


void GLViewModule::onKeyDown(const SDL_KeyboardEvent& key){
    GLView::onKeyDown(key);
    if (this->isDriving()) this->warthog->onKeyDown(key);
    else if (this->isMovementKey(key.keysym.sym)) this->camera->onKeyDown(key);
    this->processKeyPress(key.keysym.sym);
}

void GLViewModule::onKeyUp(const SDL_KeyboardEvent& key) {
    GLView::onKeyUp(key);
    if (this->isDriving()) this->warthog->onKeyUp(key);  
    if (this->isMovementKey(key.keysym.sym)) this->camera->onKeyUp(key);
}

void GLViewModule::onMouseDown(const SDL_MouseButtonEvent& e) {
    GLView::onMouseDown(e);
    if (this->pauseMenu->isPaused()) {
        if (this->pauseMenu->exit->pixelResidesInBoundingRect(e.x, e.y)) {
            //this->pauseMenu->exitGame();
            exit(1);
        }
        if (this->pauseMenu->resume->pixelResidesInBoundingRect(e.x, e.y)) {
            this->pauseMenu->resumeGame();
            if (this->client != nullptr && this->client->isTCPSocketOpen()) {
                NetMsgPause netMsgPause;
                netMsgPause.paused = false;
                this->client->sendNetMsgSynchronousTCP(netMsgPause);
            }
            if (this->isDriving()) {
                this->warthog->pauseWarthog(false);
            }
        }
        //if (this->pauseMenu->restart->pixelResidesInBoundingRect(e.x, e.y)) {
            //this->resetEngine();
        //}
    }
    if (this->isDriving()) { this->warthog->onMouseDown(e); }
}

void GLViewModule::onMouseUp(const SDL_MouseButtonEvent& e) {
    GLView::onMouseUp(e);
    if (this->isDriving()) { this->warthog->onMouseUp(e); }
}

void GLViewModule::onMouseMove(const SDL_MouseMotionEvent& e) {
    GLView::onMouseMove(e);
    if (this->isDriving()) { this->warthog->onMouseMove(e); }
}

int GLViewModule::handleWindowEvent(SDL_WindowEvent& e) {
    return GLView::handleWindowEvent(e);
}

int GLViewModule::handleEvent(SDL_Event& sdlEvent) {
    Uint32 type = sdlEvent.type;
    if (type == SDL_KEYDOWN && sdlEvent.key.keysym.sym == SDLK_ESCAPE) {
        NetMsgPause netMsgPause;
        if (this->pauseMenu->isPaused()) {
            this->pauseMenu->resumeGame();
            netMsgPause.paused = false;
        } else {
            this->pauseMenu->pauseGame();
            netMsgPause.paused = true;
        }
        if (this->client != nullptr && this->client->isTCPSocketOpen()) {
            this->client->sendNetMsgSynchronousTCP(netMsgPause);
        }
        if (this->isDriving()) {
            this->warthog->pauseWarthog(true);
        }
        this->camera->clear();
        return 1;
    }
    if(this->pauseMenu->isPaused()){
        if (type == SDL_QUIT || type == 256) {
            return 0;
        } else if (type != SDL_MOUSEBUTTONDOWN) {
            return 1;
        } 
    }
    return GLView::handleEvent(sdlEvent);
}

void GLViewModule::onMouseWheelScroll(const SDL_MouseWheelEvent& e) {
    GLView::onMouseWheelScroll(e);
}

void GLViewModule::loadMap(){
    this->worldLst = new WorldList(); //WorldList is a 'smart' vector that is used to store WO*'s
    this->actorLst = new WorldList();
    this->netLst = new WorldList();

    ManagerOpenGLState::GL_CLIPPING_PLANE = 10000.f;
    ManagerOpenGLState::GL_NEAR_PLANE = 0.1f;
    ManagerOpenGLState::enableFrustumCulling = false;
    Axes::isVisible = false;
    this->glRenderer->isUsingShadowMapping(false); //set to TRUE to enable shadow mapping, must be using GL 3.2+
    this->cam->setPosition(0.f, 0.f, 0.f);  
    
    float ga = 0.1f; //Global Ambient Light level for this module
    ManagerLight::setGlobalAmbientLight(aftrColor4f(ga, ga, ga, 1.f));
    WOLight* light = WOLight::New();
    light->isDirectionalLight(true);
    light->setPosition(Vector(0.f, 0.f, 100.f));
    //Set the light's display matrix such that it casts light in a direction parallel to the -z axis (ie, downwards as though it was "high noon")
    //for shadow mapping to work, this->glRenderer->isUsingShadowMapping(true), must be invoked.
    light->getModel()->setDisplayMatrix(Mat4::rotateIdentityMat({0.f, 1.f, 0.f}, 90.f * Aftr::DEGtoRAD));
    light->setLabel("Light");
    worldLst->push_back(light);
    
    //Create the SkyBox
    WO* wo = WOSkyBox::New(sharedMM + "/images/skyboxes/sky_mountains+6.jpg", this->getCameraPtrPtr());
    wo->setPosition(Vector(0.f, 0.f, 0.f));
    wo->setLabel("Sky Box");
    wo->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
    worldLst->push_back(wo);
    createNewModuleWayPoints();
}

void GLViewModule::createNewModuleWayPoints() {
    // Create a waypoint with a radius of 3, a frequency of 5 seconds, activated by GLView's camera, and is visible.
    WayPointParametersBase params(this);
    params.frequency = 5000;
    params.useCamera = true;
    params.visible = false;
    WOWayPointSpherical* wayPt = WOWP1::New(params, 3.f);
    wayPt->setPosition(Vector(50.f, 0.f, 3.f));
    worldLst->push_back(wayPt);
}

int GLViewModule::getIndex(WO* wo) {
    return this->worldLst->getIndexOfWO(wo);
}

void GLViewModule::processKeyPress(const SDL_Keycode& key) {
    if (key == SDLK_f) this->toggleWarthog();
    if (key == SDLK_t) this->makeToilet();
    if (key == SDLK_o) ISoundManager::engine->play2D("../mm/sounds/oof.mp3");
    if (key == SDLK_1 && this->progressBar != nullptr) this->progressBar->toggleEmpty();
    if (key == SDLK_2 && this->progressBar != nullptr) this->progressBar->toggleFill();
    if (key == SDLK_h) for (int i = 0; i < 50; ++i) { this->makeToilet(); }
}

bool GLViewModule::isDriving() { return this->warthog != nullptr && this->warthog->hasDriver(); }

void GLViewModule::makeToilet(Vector pos) {
    // Add the item to the physx world
    WOPhysx* wo = WOPhysx::New();
    worldLst->push_back(wo);
    wo->setPosition(pos != Vector(0.f, 0.f, 0.f) ? pos : this->cam != nullptr ? this->cam->getPosition() : Vector(0.f, 0.f, 0.f));
    wo->setLabel("Toilet #" + std::to_string(this->numToilets));
    // Play toilet flush sound :)
    ISoundManager::engine->play2D("../mm/sounds/toilet.wav");

    // Send new object net message to spawn a new object
    if (this->client != nullptr && this->client->isTCPSocketOpen()) {
        NetMsgToilet msg;
        msg.pos = wo->getPosition();
        this->client->sendNetMsgSynchronousTCP(msg);
    }
    
    // Register it to the local storage
    this->worldObjects.insert(std::pair(wo, numToilets));
    numToilets++;
    this->banner->setText(std::to_string(this->numToilets) + (this->numToilets == 1 ? " toilet" : " toilets"));
}

WO* GLViewModule::getFromLabel(std::string label) {
    for (int i = 0; i < this->worldLst->size(); ++i) {
        WO* wo = this->worldLst->at(i);
        if (wo != nullptr && wo->getLabel() == label) {
            return wo;
        }
    }
    return nullptr;
}

int GLViewModule::getIndexFromLabel(std::string label) {
    for (int i = 0; i < this->worldLst->size(); ++i) {
        WO* wo = this->worldLst->at(i);
        if (wo != nullptr && wo->getLabel() == label) {
            return i;
        }
    }
    return -1;
}

bool GLViewModule::isMovementKey(SDL_Keycode key) {
    return (key == SDLK_UP || key == SDLK_LEFT || key == SDLK_DOWN || key == SDLK_RIGHT ||
        key == SDLK_w || key == SDLK_a || key == SDLK_s || key == SDLK_d || key == SDLK_LSHIFT);
}


////Create the infinite grass plane (the floor)
/*
wo = WO::New(grass, Vector(1.f, 1.f, 1.f), MESH_SHADING_TYPE::mstFLAT);
wo->setPosition(Vector(0.f, 0.f, 0.f));
wo->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
ModelMeshSkin& grassSkin = wo->getModel()->getModelDataShared()->getModelMeshes().at(0)->getSkins().at(0);
grassSkin.getMultiTextureSet().at(0)->setTextureRepeats(5.f);
grassSkin.setAmbient(aftrColor4f(0.4f, 0.4f, 0.4f, 1.f)); //Color of object when it is not in any light
grassSkin.setDiffuse(aftrColor4f(1.f, 1.f, 1.f, 1.f)); //Diffuse color components (ie, matte shading color of this object)
grassSkin.setSpecular(aftrColor4f(0.4f, 0.4f, 0.4f, 1.f)); //Specular color component (ie, how "shiny" it is)
grassSkin.setSpecularCoefficient(10); // How "sharp" are the specular highlights (bigger is sharper, 1000 is very sharp, 10 is very dull)
wo->setLabel("Grass");
worldLst->push_back(wo);
*/