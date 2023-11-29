#include "artd/GpuEngine.h"
#include "artd/Scene.h"
#include "artd/CameraNode.h"
#include "artd/Camera.h"
#include "artd/LightNode.h"
#include "artd/MeshNode.h"

int createScene(artd::GpuEngine *engine)  {
    
using namespace artd;
    
    ObjectPtr<Scene>  scene = ObjectPtr<Scene>::make(engine);
 
    ObjectPtr<TransformNode> ringGroup = ObjectPtr<TransformNode>::make();

    ObjectPtr<CameraNode> cameraNode = ObjectPtr<CameraNode>::make();
    {
        auto camera = ObjectBase::make<Camera>();
        // this could be cleaned up and done more automatically, but we will have diffent types of cameras !
        cameraNode->setCamera(camera);

        glm::mat4 camPose(1.0);
        camPose = glm::rotate(camPose, glm::pi<float>()/8, glm::vec3(1.0,0,0)); // glm::vec4(1.0,0,-3,1);
        camPose = glm::translate(camPose, glm::vec3(0,1.0,-5.1));

        cameraNode->setLocalTransform(camPose);

        camera->setNearClip(0.01f);
        camera->setFarClip(100.0f);
        camera->setFocalLength(2.0);
        scene->setCurrentCamera(cameraNode);
    }

    scene->addChild(ringGroup);
    {
        class AnimTask
            : public AnimationTask
        {
        public:
            bool onAnimationTick(AnimationTaskContext &ac) override {

                TransformNode *owner = (TransformNode *)ac.owner();

                // Matrix4f lt = ringGroup_->getLocalTransform();
                Matrix4f lt = owner->getLocalTransform();

                Matrix4f rot;
                angle += ac.timing().lastFrameDt() * .01;
                if( angle > (glm::pi<float>()*2)) {
                    angle -= (glm::pi<float>()*2);
                }

                rot = glm::rotate(rot, -angle, glm::vec3(0,1.0,0));
                lt[0] = rot[0];
                lt[1] = rot[1];
                lt[2] = rot[2];

                owner->setLocalTransform(lt);

                return(true);

            }
            float angle = 0.0;
        };
        scene->addAnimationTask(ringGroup, ObjectPtr<AnimTask>::make());
    }

    // this holds references until we assign them to drawable nodes;
    // TODO: store by name in cache ? optional ?
    // Currently unless referenced, when all objects with materials in them
    // are deleted the material goes away too.
    
    std::vector<ObjectPtr<Material>> materials;

    // initialize a scene (big hack)
    {
        // test colors
        Color3f colors[] = {
            { 36,133,234 },
            { 177,20,31 },
            { 203,205,195 },
            { 152, 35, 24 },
            { 154, 169, 153 },
            { 179, 126, 64 },
            { 50, 74, 84 },
            { 226, 175, 105 }
        };

        for(int i = 0; i < (int)ARTD_ARRAY_LENGTH(colors); ++i) {
            materials.push_back(ObjectPtr<Material>::make(engine));
            auto pMat = materials[materials.size()-1].get();
            pMat->setDiffuse(colors[i]);
            pMat->setShininess(.001);
        }

        // create/load two meshes cone and sphere
        
        const char *coneMeshPath = "cone";
        const char *sphereMeshPath = "sphere";

        // lay out a bunch of instances

        Matrix4f lt(1.0);
        
        lt = glm::translate(lt, glm::vec3(0.0, 0.0, 3.0));
        ringGroup->setLocalTransform(lt);
        
        AD_LOG(info) << lt;

        
        glm::mat4 drot(1.0);
        drot = glm::rotate(drot, -glm::pi<float>()/6, glm::vec3(0,1.0,0)); // rotate about Y
        
        Vec3f trans = glm::vec3(0.0, 0.0, 3.5);

        AD_LOG(info) << drot;

        // Create some lights
        
        {
            ObjectPtr<LightNode> light = ObjectPtr<LightNode>::make();
            light->setLightType(LightNode::directional);
            light->setDirection(Vec3f(0.5, .5, 0.1));
            light->setDiffuse(Color3f(1.f,1.f,1.f));
            light->setAreaWrap(.25);
            scene->addChild(light);

            // this rotates the light direction around the center of ths scene
            class AnimTask
                : public AnimationTask
            {
            public:
                bool onAnimationTick(AnimationTaskContext &ac) override {

                    TransformNode *owner = (TransformNode *)ac.owner();

                    // Matrix4f lt = ringGroup_->getLocalTransform();
                    Matrix4f lt = owner->getLocalTransform();

                    Matrix4f rot;
                    angle += ac.timing().lastFrameDt() * .2;
                    if( angle > (glm::pi<float>()*2)) {
                        angle -= (glm::pi<float>()*2);
                    }

                    rot = glm::rotate(rot, -angle, glm::vec3(0,1.0,0));
                    lt[0] = rot[0];
                    lt[1] = rot[1];
                    lt[2] = rot[2];

                    owner->setLocalTransform(lt);

                    return(true);

                }
                float angle = 0.0;
            };
            scene->addAnimationTask(light, ObjectPtr<AnimTask>::make());
        }

        // layout some objects in a ring around the ringGroup_ node
        // assign one of the test materials to it.
        uint32_t materialId = 0;
        uint32_t maxI = 12;
        for(uint32_t i = 0; i < maxI; ++i)  {
        
            MeshNode *node = (MeshNode *)ringGroup->addChild(ObjectPtr<MeshNode>::make());
            node->setId(i + 10);

            lt = glm::mat4(1.0);
            lt[3] = glm::vec4(trans,1.0);
            node->setLocalTransform(lt);
            trans = glm::mat3(drot) * trans;
            
            node->setMaterial(materials[materialId]);
            if(++materialId >= materials.size()) {
                materialId = 0;
            }
            
            if((i & 1) != 0) {
                node->setMesh(coneMeshPath);
            } else {
                node->setMesh(sphereMeshPath);
            }
        }


        {
            materials.push_back(ObjectBase::make<Material>(engine));
            auto pMat = materials[materials.size()-1];
            pMat->setDiffuse(Color3f(180,180,180));
            pMat->setShininess(.001);
            pMat->setDiffuseTexture("test0");

            lt = glm::mat4(1.0);
            MeshNode *node = (MeshNode *)ringGroup->addChild(ObjectPtr<MeshNode>::make());
            node->setLocalTransform(lt);
            node->setId(maxI + 10);
            node->setMesh("cube");
            node->setMaterial(pMat);

            {
                class AnimTask
                    : public AnimationTask
                {
                public:

                    bool highlit = false;
                    float toggleTime = 2.1;

                    bool onAnimationTick(AnimationTaskContext &ac) override {

                        MeshNode *owner = (MeshNode *)ac.owner();
                        double dt = ac.timing().lastFrameDt();

                        Matrix4f rot;
                        angle += dt * .1;
                        if( angle > (glm::pi<float>()*2)) {
                            angle -= (glm::pi<float>()*2);
                        }

                        rot = glm::rotate(rot, angle, glm::vec3(0,1.0,0));
                        rot = glm::rotate(rot, angle*2.5f, glm::normalize(glm::vec3(0,1.0,1.0)));

                        owner->setLocalTransform(rot);

                        toggleTime -= dt;
                        if(toggleTime < 0)  {
                            Material *mat = owner->getMaterial().get();
                            highlit = !highlit;
                            if(highlit)  {
                                mat->setEmissive(Color3f(1.f,1.f,1.f));
                                toggleTime = .2;
                            } else {
                                mat->setEmissive(Color3f(0,0,0));
                                toggleTime += 2.1;
                            }
                        }
                        return(true);

                    }
                    float angle = 0.0;
                };

                scene->addAnimationTask(node, ObjectPtr<AnimTask>::make());
            }
        }
    }
    
    engine->setCurrentScene(scene);
    return(0);
}

int runGpuTest(int argc, char **argv)
{
using namespace artd;

    ObjectPtr<GpuEngine> engine = GpuEngine::createInstance(false,1920,1080);

    if(!engine) {
        return(1);
    }
    
    createScene(engine.get());
    
    return(engine->run());
}

int main (int argc, char**argv) {
    return(runGpuTest(argc,argv));
}

