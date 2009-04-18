// Main setup for the OpenEngine PostProcessingDemo project.
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
#include <Scene/PointLightNode.h>

// Utilities and logger
#include <Logging/Logger.h>
#include <Logging/StreamLogger.h>
#include <Scene/DotVisitor.h>

// OERacer utility files
#include <Utils/QuitHandler.h>
#include <Utils/RenderStateHandler.h>
#include <Utils/MoveHandler.h>

#include <Animation/MetaMorpher.h>
#include <Animation/TransformationNodeMorpher.h>
using namespace OpenEngine::Animation;

#include <Meta/OpenGL.h>

#include "TeaPotNode.h"
#include <EffectHandler.h>

// Post processing extension
#include <Renderers/OpenGL/PostProcessingRenderingView.h>

// Post processing effects extension
#include <Effects/SimpleDoF.h>
#include <Effects/DoF.h>
#include <Effects/Glow.h>
#include <Effects/VolumetricLightScattering.h>


// effects
#include <Effects/Wobble.h>
#include <Effects/Shadows.h>
#include <Effects/SimpleBlur.h>
#include <Effects/TwoPassBlur.h>
#include <Effects/GaussianBlur.h>
#include <Effects/Glow.h>
#include <Effects/SimpleMotionBlur.h>
#include <Effects/MotionBlur.h>
#include <Effects/EdgeDetection.h>
#include <Effects/Toon.h>
#include <Effects/SimpleDoF.h>
#include <Effects/VolumetricLightScattering.h>
#include <Effects/GrayScale.h>
#include <Effects/Pixelate.h>
#include <Effects/Saturate.h>
#include <Effects/ShowImage.h>
#include <Effects/DoF.h>
#include <Effects/SimpleExample.h>
#include <SunModule.h>

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

class Preprocessing : public RenderingView {
private:
	PostProcessingEffect* effect;

public:
	Preprocessing( Viewport& viewport, PostProcessingEffect* effect )
        : IRenderingView(viewport), RenderingView(viewport), effect(effect) {}
	
	void Handle(RenderingEventArg arg) {
		effect->PreRender();
        CHECK_FOR_GL_ERROR();
	}
};

class Postprocessing : public RenderingView {
private:
	PostProcessingEffect* effect;

public:
	Postprocessing( Viewport& viewport, PostProcessingEffect* effect )
        : IRenderingView(viewport), RenderingView(viewport), effect(effect) {}
	
	void Handle(RenderingEventArg arg) {
		effect->PostRender();
        CHECK_FOR_GL_ERROR();
	}
};

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
    string resources = "projects/PostProcessingDemo/data/";
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
    //renderer->SetSceneRoot(new SceneNode());
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


    // add post processing effects
    IEngine& engine = config.engine;
    Viewport* viewport = config.viewport;

	Wobble* wobble;
    Glow* glow;
    SimpleBlur* simpleBlur;
    TwoPassBlur* twoPassBlur;
    GaussianBlur* gaussianBlur;
    SimpleMotionBlur* simpleMotionBlur;
    MotionBlur* motionBlur;
    SimpleDoF* simpleDoF;
    EdgeDetection* edgeDetection;
    Toon* toon;
    GrayScale* grayscale;
    Saturate* saturate;
    Pixelate* pixelate;
    VolumetricLightScattering* volumetricLightScattering;
    Shadows* shadows;
    //ShowImage* showImage;

    vector<IPostProcessingEffect*> fullscreeneffects;
    vector<std::string> fullscreeneffectsNames;
	wobble                    = new Wobble(viewport,engine);
    glow                      = new Glow(viewport,engine);
    simpleBlur                = new SimpleBlur(viewport,engine);
    twoPassBlur               = new TwoPassBlur(viewport,engine);
    gaussianBlur              = new GaussianBlur(viewport,engine);
    simpleMotionBlur          = new SimpleMotionBlur(viewport,engine);
    motionBlur                = new MotionBlur(viewport,engine);
    simpleDoF                 = new SimpleDoF(viewport,engine);
    edgeDetection             = new EdgeDetection(viewport,engine);
    toon                      = new Toon(viewport,engine);
    grayscale                 = new GrayScale(viewport,engine);
    saturate                  = new Saturate(viewport,engine);
    pixelate                  = new Pixelate(viewport,engine);
    volumetricLightScattering = new VolumetricLightScattering(viewport,engine);
    shadows                   = new Shadows(viewport,engine);
    //this->showImage                 = new ShowImage(viewport,engine, texture);
	//missing: showImage, sunmodule 

    SimpleExample* simpleExample = new SimpleExample(viewport,engine);
    DoF* dof = new DoF(viewport, engine);

    // muligvis skal rækkefølgen laves om...
    wobble->Add(edgeDetection);
    wobble->Add(toon);
    wobble->Add(glow);
    wobble->Add(simpleBlur);
    //wobble->Add(twoPassBlur);
    wobble->Add(gaussianBlur);
    wobble->Add(simpleMotionBlur);
    wobble->Add(motionBlur);
    //wobble->Add(simpleDoF);
    wobble->Add(grayscale);
    //wobble->Add(saturate);
    //wobble->Add(volumetricLightScattering);
    //wobble->Add(shadows);
    //wobble->Add(pixelate);
    wobble->Add(simpleExample);
    //wobble->Add(dof);

    wobble->Enable(false);
    glow->Enable(false);
    simpleBlur->Enable(false);
    twoPassBlur->Enable(false);
    gaussianBlur->Enable(false);
    simpleMotionBlur->Enable(false);
    motionBlur->Enable(false);
    simpleDoF->Enable(false);
    edgeDetection->Enable(false);
    toon->Enable(false);
    grayscale->Enable(false);
    saturate->Enable(false);
    pixelate->Enable(false);
    volumetricLightScattering->Enable(false);
    shadows->Enable(false);
    simpleExample->Enable(false);
    dof->Enable(false);

    // add to effects
    fullscreeneffects.push_back(wobble);
    fullscreeneffects.push_back(glow);
    fullscreeneffects.push_back(simpleBlur);
    fullscreeneffects.push_back(twoPassBlur);
    fullscreeneffects.push_back(gaussianBlur);
    fullscreeneffects.push_back(simpleMotionBlur);
    fullscreeneffects.push_back(motionBlur);
    fullscreeneffects.push_back(simpleDoF);
    fullscreeneffects.push_back(edgeDetection);
    fullscreeneffects.push_back(toon);
    fullscreeneffects.push_back(grayscale);
    fullscreeneffects.push_back(saturate);
    fullscreeneffects.push_back(pixelate);
    fullscreeneffects.push_back(volumetricLightScattering);
    fullscreeneffects.push_back(shadows);
    fullscreeneffects.push_back(simpleExample);
    fullscreeneffects.push_back(dof);
	
    fullscreeneffectsNames.push_back("wobble");
    fullscreeneffectsNames.push_back("glow");
    fullscreeneffectsNames.push_back("simpleBlur");
    fullscreeneffectsNames.push_back("twoPassBlur");
    fullscreeneffectsNames.push_back("gaussianBlur");
    fullscreeneffectsNames.push_back("simpleMotionBlur");
    fullscreeneffectsNames.push_back("motionBlur");
    fullscreeneffectsNames.push_back("simpleDoF");
    fullscreeneffectsNames.push_back("edgeDetection");
    fullscreeneffectsNames.push_back("toon");
    fullscreeneffectsNames.push_back("grayscale");
    fullscreeneffectsNames.push_back("saturate");
    fullscreeneffectsNames.push_back("pixelate");
    fullscreeneffectsNames.push_back("volumetricLightScattering");
    fullscreeneffectsNames.push_back("shadows");
    fullscreeneffectsNames.push_back("simpleExample");
    fullscreeneffectsNames.push_back("dof");

    PostProcessingEffect* ppe = wobble;
        
    IRenderingView* rv2 = new Preprocessing(*viewport, ppe);
    IRenderingView* rv3 = new Postprocessing(*viewport, ppe);
    renderer->PreProcessEvent().Attach(*rv2);
    renderer->PostProcessEvent().Attach(*rv3);

    VolumetricLightScattering* sun = new
        VolumetricLightScattering(viewport,engine);
    TransformationNode* sunTrans = new TransformationNode();
    SunModule* sunModule = new SunModule(sun,
                                         sunTrans, config.camera);
    config.engine.ProcessEvent().Attach(*sunModule);
    sun->Enable(true);
    sunModule->SetFollowSun(false);
    //config.scene->AddNode(sunTrans);

    // Register effect handler to be able to toggle effects
    EffectHandler* effectHandler = 
        new EffectHandler(fullscreeneffects, NULL, sunModule);
    config.keyboard->KeyEvent().Attach(*effectHandler);
    effectHandler->SetNameList(fullscreeneffectsNames);
}

void SetupScene(Config& config) {
    if (config.scene  != NULL ||
        config.mouse  == NULL ||
        config.keyboard == NULL)
        throw Exception("Setup scene dependencies are not satisfied.");

    // Create a root scene node
    RenderStateNode* renderStateNode = new RenderStateNode();
    renderStateNode->EnableOption(RenderStateNode::LIGHTING);
    renderStateNode->DisableOption(RenderStateNode::WIREFRAME);

    config.scene = renderStateNode;

    // Supply the scene to the renderer
    config.renderer->SetSceneRoot(config.scene);

    //add point light
    PointLightNode* light1 = new PointLightNode();
    TransformationNode* light1Pos = new TransformationNode();
    light1Pos->SetPosition(Vector<3,float>(-100,0,0));
    light1Pos->AddNode(light1);
    config.scene->AddNode(light1Pos);

    //Bind renderstatenode handler: F1...
    RenderStateHandler* rs_h = new RenderStateHandler(*renderStateNode);
    config.keyboard->KeyEvent().Attach(*rs_h);
    

    TransformationNode* left = new TransformationNode();
    //left->SetPosition(Vector<3,float>(-10,0,0));

    TransformationNode* topCenter = new TransformationNode();
    //topCenter->SetPosition(Vector<3,float>(0,7,0));
    topCenter->SetRotation(Quaternion<float>(Math::PI/2,0,Math::PI/2));

    TransformationNode* right = new TransformationNode();
    //right->SetPosition(Vector<3,float>(10,0,0));
    right->SetRotation(Quaternion<float>(Math::PI,0,Math::PI));

    TransformationNode* bottomCenter = new TransformationNode();
    //bottomCenter->SetPosition(Vector<3,float>(0,-7,0));
    bottomCenter->SetRotation(Quaternion<float>(-Math::PI/2,0,-Math::PI/2));

    TransformationNodeMorpher* tmorpher =
      new TransformationNodeMorpher();
    
    MetaMorpher<TransformationNode>* metamorpher =
      new MetaMorpher<TransformationNode>
      (tmorpher,LOOP);
    config.engine.ProcessEvent().Attach(*metamorpher);
    metamorpher->Add(left, new Utils::Time(0));
    metamorpher->Add(topCenter, new Utils::Time(3000000));
    metamorpher->Add(right, new Utils::Time(6000000));
    metamorpher->Add(bottomCenter, new Utils::Time(9000000));
    metamorpher->Add(left, new Utils::Time(12000000));
    
    TransformationNode* trans = metamorpher->GetObject();
    trans->AddNode(new TeaPotNode(1.0));


    TransformationNode* tnode = new TransformationNode();
    tnode->Rotate(0,0,Math::PI);
    tnode->Rotate(0,Math::PI/2,0);
    config.scene->AddNode(tnode);

    tnode->AddNode(trans);
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

