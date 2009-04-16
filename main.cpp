// Main setup for the OpenEngine MetaMopher project.
// -------------------------------------------------------------------
// Copyright (C) 2008 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

// OpenEngine stuff
#include <Meta/Config.h>

// Core structures
#include <Core/Engine.h>

// Display structures
#include <Display/IFrame.h>
#include <Display/FollowCamera.h>
#include <Display/Frustum.h>
#include <Display/InterpolatedViewingVolume.h>
#include <Display/ViewingVolume.h>
// SDL implementation
#include <Display/HUD.h>
#include <Display/SDLFrame.h>
#include <Devices/SDLInput.h>
#include <Display/SDLEnvironment.h>

// OpenGL rendering implementation
#include <Renderers/OpenGL/LightRenderer.h>
#include <Renderers/OpenGL/Renderer.h>
#include <Renderers/OpenGL/RenderingView.h>
#include <Renderers/TextureLoader.h>

// Resources
#include <Resources/IModelResource.h>
#include <Resources/File.h>
#include <Resources/DirectoryManager.h>
#include <Resources/ResourceManager.h>
// OBJ and TGA plugins
#include <Resources/ITextureResource.h>
#include <Resources/OBJResource.h>

// Scene structures
#include <Scene/ISceneNode.h>
#include <Scene/SceneNode.h>
#include <Scene/GeometryNode.h>
#include <Scene/TransformationNode.h>

// Utilities and logger
#include <Logging/Logger.h>
#include <Logging/StreamLogger.h>
#include <Scene/DotVisitor.h>

// OERacer utility files
#include <Utils/QuitHandler.h>
#include <Utils/MoveHandler.h>

#include "TeaPotNode.h"

// Additional namespaces
using namespace OpenEngine;
using namespace OpenEngine::Core;
using namespace OpenEngine::Logging;
using namespace OpenEngine::Devices;
using namespace OpenEngine::Display;
using namespace OpenEngine::Renderers::OpenGL;
using namespace OpenEngine::Renderers;
using namespace OpenEngine::Resources;
using namespace OpenEngine::Utils;

// Configuration structure to pass around to the setup methods
struct Config {
    IEngine&              engine;
    IFrame*               frame;
    Viewport*             viewport;
    IViewingVolume*       viewingvolume;
    Camera*               camera;
    Frustum*              frustum;
    IRenderer*            renderer;
    IMouse*               mouse;
    IKeyboard*            keyboard;
    ISceneNode*           scene;
    TextureLoader*        textureLoader;
    Config(IEngine& engine)
        : engine(engine)
        , frame(NULL)
        , viewport(NULL)
        , viewingvolume(NULL)
        , camera(NULL)
        , frustum(NULL)
        , renderer(NULL)
        , mouse(NULL)
        , keyboard(NULL)
        , scene(NULL)
        , textureLoader(NULL)
    {}
};

// Forward declaration of the setup methods
void SetupResources(Config&);
void SetupDevices(Config&);
void SetupDisplay(Config&);
void SetupRendering(Config&);
void SetupScene(Config&);
void SetupDebugging(Config&);
void SetupSound(Config&);

int main(int argc, char** argv) {

    // Setup logging facilities.
    Logger::AddLogger(new StreamLogger(&std::cout));

    // Create an engine and config object
    Engine* engine = new Engine();
    Config config(*engine);

    // Setup the engine
    SetupResources(config);
    SetupDisplay(config);
    SetupDevices(config);
    SetupRendering(config);
    SetupScene(config);

    // Possibly add some debugging stuff
    SetupDebugging(config);

    // Start up the engine.
    engine->Start();

    // release event system
    // post condition: scene and modules are not processed
    delete engine;

    delete config.scene;

    // Return when the engine stops.
    return EXIT_SUCCESS;
}

void SetupResources(Config& config) {
    // set the resources directory
    // @todo we should check that this path exists
    // set the resources directory
    string resources = "projects/MetaMorpher/data/";
    DirectoryManager::AppendPath(resources);

    // load resource plug-ins
    ResourceManager<IModelResource>::AddPlugin(new OBJPlugin());
}

void SetupDisplay(Config& config) {
    if (config.frame         != NULL ||
        config.viewingvolume != NULL ||
        config.camera        != NULL ||
        config.frustum       != NULL ||
        config.viewport      != NULL)
        throw Exception("Setup display dependencies are not satisfied.");

    //config.frame         = new SDLFrame(1440, 900, 32, FRAME_FULLSCREEN);
    config.frame         = new SDLFrame(800, 600, 32);
    config.viewingvolume = new ViewingVolume();
    config.camera        = new Camera( *config.viewingvolume );
    //config.frustum       = new Frustum(*config.camera, 20, 3000);

    config.camera->SetPosition(Vector<3,float>(0,0,10));
    config.camera->LookAt(Vector<3,float>(0,0,0));

    config.viewport      = new Viewport(*config.frame);
    config.viewport->SetViewingVolume(config.camera);

    config.engine.InitializeEvent().Attach(*config.frame);
    config.engine.ProcessEvent().Attach(*config.frame);
    config.engine.DeinitializeEvent().Attach(*config.frame);
}

void SetupDevices(Config& config) {
    if (config.keyboard != NULL ||
        config.mouse    != NULL)
        throw Exception("Setup devices dependencies are not satisfied.");
    // Create the mouse and keyboard input modules
    SDLEnvironment* input = new SDLEnvironment();
    config.keyboard = input->GetKeyboard();
    config.mouse = input->GetMouse();

    // Bind the quit handler
    QuitHandler* quit_h = new QuitHandler(config.engine);
    config.keyboard->KeyEvent().Attach(*quit_h);

    // Bind to the engine for processing time
    config.engine.InitializeEvent().Attach(*input);
    config.engine.ProcessEvent().Attach(*input);
    config.engine.DeinitializeEvent().Attach(*input);

    MoveHandler* move_h = 
        new MoveHandler(*config.camera, *input->GetMouse());
    move_h->SetObjectMove(false);
    config.keyboard->KeyEvent().Attach(*move_h);
    config.engine.InitializeEvent().Attach(*move_h);
    config.engine.ProcessEvent().Attach(*move_h);
    config.engine.DeinitializeEvent().Attach(*move_h);
    
}

void SetupRendering(Config& config) {
    if (config.viewport == NULL ||
        config.renderer != NULL ||
        config.camera == NULL )
        throw Exception("Setup renderer dependencies are not satisfied.");

    // Setup a rendering view for both renderers
    // this only holds perspective
    RenderingView* rv = new RenderingView(*config.viewport);

    Renderer* renderer = new Renderer(config.viewport);
    renderer->SetBackgroundColor(Vector<4,float>(0,0,0,1));
    renderer->SetSceneRoot(new SceneNode());
    config.engine.InitializeEvent().Attach(*renderer);
    config.engine.ProcessEvent().Attach(*renderer);
    config.engine.DeinitializeEvent().Attach(*renderer);
    renderer->ProcessEvent().Attach(*rv);
    config.renderer = renderer;

    // Add rendering initialization tasks
    config.textureLoader = new TextureLoader(*renderer);
    renderer->PreProcessEvent().Attach(*config.textureLoader);

    renderer->PreProcessEvent()
      .Attach( *(new LightRenderer(*config.camera)) );

}

void SetupScene(Config& config) {
    if (config.scene  != NULL ||
        config.mouse  == NULL ||
        config.keyboard == NULL)
        throw Exception("Setup scene dependencies are not satisfied.");

    // Create a root scene node
    RenderStateNode* renderStateNode = new RenderStateNode();
    renderStateNode->DisableOption(RenderStateNode::LIGHTING);
    config.scene = renderStateNode;

    // Supply the scene to the renderer
    config.renderer->SetSceneRoot(config.scene);

    TransformationNode* tnode = new TransformationNode();
    tnode->Rotate(0,0,Math::PI);
    tnode->Rotate(0,Math::PI/2,0);
    
    tnode->AddNode( new TeaPotNode(1.0) );
    config.scene->AddNode(tnode);
}

void SetupDebugging(Config& config) {
    // Visualization of the frustum
    if (config.frustum != NULL) {
        config.frustum->VisualizeClipping(true);
        config.scene->AddNode(config.frustum->GetFrustumNode());
    }

    ofstream dotfile("scene.dot", ofstream::out);
    if (!dotfile.good()) {
        logger.error << "Can not open 'scene.dot' for output"
                     << logger.end;
    } else {
        DotVisitor dot;
        dot.Write(*config.scene, &dotfile);
        logger.info << "Saved scene graph to 'scene.dot'"
                    << logger.end
                    << "To create a SVG image run: "
                    << "dot -Tsvg scene.dot > scene.svg"
                    << logger.end;
    }
}

