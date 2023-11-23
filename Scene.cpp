#include "./GpuEngineImpl.h"
#include "artd/Scene.h"
#include "artd/LightNode.h"
#include "artd/Mutex.h"
#include "artd/Material.h"

#include "./MeshNode.h"


ARTD_BEGIN

#define INL ARTD_ALWAYS_INLINE

class SceneRoot
    : public TransformNode
{
public:
    SceneRoot(Scene *owner) {
        parent_ = reinterpret_cast<TransformNode *>(owner);
        clearFlags(fHasTransform | fHasParent);
        setId(-1);
    }
    void clearParent() {
        parent_ = nullptr;
        clearFlags(fHasTransform | fHasParent);
    }
};


class OwnedAnimList {
public:
    ~OwnedAnimList() {
        AD_LOG(info) << "######## destroy owned AnimList";
    }
};


class AnimationTaskList
{
public:
    
    class OwnedEntryList;
    
    static const TypedPropertyKey<OwnedEntryList>  OwnedEntriesKey;

    class TaskEntry
        : public DlNode
    {
    public:
        TaskEntry(ObjectPtr<AnimationTask> task, SceneNode *owner)
            : task_(task)
            , owner_(owner)
        {
            if(owner) {
                OwnedEntryList *list = owner->getProperty(OwnedEntriesKey);
                if(list) {
                    list->addTaskEntry(this);
                } else {
                    owner->setProperty(OwnedEntriesKey, OwnedEntryList(this));
                }
            }
        }
        ~TaskEntry() {
            if(owner_) {
                OwnedEntryList *list = owner_->getProperty(OwnedEntriesKey);
                if(list) {
                    list->onEntryDestroy(this);
                }
            }
        }
        void onOwnerDestroyed() {
            owner_ = nullptr;
            if(!detached()) {
                detach();
            }
        }
        ObjectPtr<AnimationTask> task_;
        SceneNode *owner_;
        TaskEntry *nextOwned_ = nullptr;

        INL bool tick(AnimationTaskContext &tc) {
            tc.owner_ = owner_;
            return(task_->onAnimationTick(tc));
        }
    };

    // TODO: maybe use the doubly linked list !!
    // maybe needs a more general implementation
    class OwnedEntryList {
    public:
        TaskEntry *list_ = nullptr;
        OwnedEntryList(TaskEntry *list)
            : list_(list)
        {
        }
        // NOTE: we only will add the first one if there is a chain.
        void addTaskEntry(TaskEntry *entry) {
            entry->nextOwned_ = list_;
            list_ = entry;
        }
        void removeEntry(TaskEntry *toRemove) {
            TaskEntry **pPrior = &list_;
            for(auto entry = list_; entry != nullptr; entry = entry->nextOwned_) {
                if(entry == toRemove) {
                    *pPrior = toRemove->nextOwned_;
                    break;
                }
                pPrior = &entry->nextOwned_;
            }
        }
        
        void onEntryDestroy(TaskEntry *destroying) {
            removeEntry(destroying);
        }
        OwnedEntryList(OwnedEntryList &&from)
            : list_(from.list_)
        {
            from.list_ = nullptr;
        }
        int size() {
            int sz = 0;
            for(auto entry = list_; entry != nullptr; entry = entry->nextOwned_) {
                ++sz;
            }
            return(sz);
        }
        ~OwnedEntryList() {
            while(list_) {
                auto entry = list_;
                list_ = entry->nextOwned_;
                entry->onOwnerDestroyed();
            }
            if(list_) {
                AD_LOG(print) << "destroying OwnedEntryList of: " << size();
            }
        }
    };

    class TaskList
        : public IntrusiveList<TaskEntry,TaskList>
    {
    public:
        ~TaskList() {
        }

        void onAttach(TaskEntry */*te*/) {
//            AD_LOG(print) << "attaching " << (void *)te;
        }

        void onDetach(TaskEntry *te) {
//            AD_LOG(print) << "detaching " << (void *)te;
            delete(te);
        }

    };

    TaskList list_;
    Mutex listLock_;
    
    void tickAnimations(TimingContext &timing) {
        
        AnimationTaskContext context;
        context.setTiming(timing);
                
        TaskList toRun;
        {
        synchronized(listLock_);
            toRun.appendFrom(list_);
            if (toRun.empty()) {
                return;
            }
        }
        
        try {
            TaskList doneWith;

            for(auto it = toRun.begin(); it != toRun.end(); ++it) {
                TaskEntry &task = *it;
                auto ret = task.tick(context);
                if(!ret) {
                    it.remove();
                }
            }

            // release all the executed events inside a list lock
            // schedule all the do-again events to be executed again.
            {
            synchronized(listLock_)
                list_.appendFrom(toRun); // add to end of list if anything was added to it it stays there
            }
        }
        catch (...) {
        }
    }
    
//    void addTask(SceneNode *owner, AnimationFunction task) {
//        list_.addHead(new TaskEntry(task,owner));
//    }
    void addTask(SceneNode *owner, ObjectPtr<AnimationTask> task) {
        list_.addHead(new TaskEntry(task,owner));
    }
};

const TypedPropertyKey<AnimationTaskList::OwnedEntryList>  AnimationTaskList::OwnedEntriesKey("AnimationTaskList::OwnedEntriesKey");


Scene::Scene(GpuEngine *e)
    : owner_(e)
{
    backgroundColor_ = Color4f{ 0.2f, 0.2f, 0.2f, 1.0f };
    currentCamera_ = ((GpuEngineImpl*)e)->defaultCamera_;
    rootNode_ = ObjectPtr<SceneRoot>::make(this);
    animationTasks_ = ObjectPtr<AnimationTaskList>::make();
    activeMaterials_ = ObjectPtr<MaterialList>::make();
}

Scene::~Scene()
{
    lights_.clear();
    drawables_.clear();
    
   // ((SceneRoot *)rootNode_.get())->clearParent(); // prevents onDetach from being called
}

SceneNode *
Scene::addChild(ObjectPtr<SceneNode> child) {
    return(rootNode_->addChild(child));
}

void
Scene::removeChild(SceneNode *child) {
    
    // TODO: we need to update the light list
    // when a light is removed - even if attached to a
    // lower down the tree node.
    
    rootNode_->removeChild(child);
}

void
Scene::tickAnimations(TimingContext &timing) {
    animationTasks_->tickAnimations(timing);
}

void
Scene::removeActiveLight(LightNode *l) {
    for(auto it = lights_.begin(); it != lights_.end(); ++it) {
        if(*it == l) {
            lights_.erase(it);
            return;
        }
    }
}

void
Scene::setBackgroundColor(Color4f &color) {
    backgroundColor_ = color;
}

void
Scene::setCurrentCamera(ObjectPtr<CameraNode> &camera) {
    if(!camera) {
        camera = GpuEngineImpl::getInstance().defaultCamera_;
    }
    camera->getCamera()->setViewport(GpuEngineImpl::getInstance().viewport_);
    currentCamera_ = camera;    
}
void
Scene::addActiveLight(LightNode *l) {

    for(int i = (int)lights_.size(); i > 0;) {
        --i;
        if(lights_[i] == l) {
            return;
        }
    }
    lights_.push_back(l);
}

void
Scene::addActiveMaterial(Material *mat) {
    if(mat && ((DlNode *)mat)->detached()) {
        activeMaterials_->addTail(mat);
    };
}

void
Scene::addDrawable(SceneNode *l) {

    for(int i = (int)drawables_.size(); i > 0;) {
        --i;
        if(((SceneNode*)(drawables_[i])) == l) {
            return;
        }
    }
    MeshNode *mn = (MeshNode*)l;
    Material *mat = mn->getMaterial().get();
    addActiveMaterial(mat);
    drawables_.push_back(mn);
}
void
Scene::removeDrawable(SceneNode *l) {
    for(auto it = drawables_.begin(); it != drawables_.end(); ++it) {
        if(*it == (MeshNode *)l) {
            drawables_.erase(it);
            return;
        }
    }
}

void
Scene::onNodeAttached(SceneNode *n) {
    AD_LOG(print) << "attached " << (void *)n;
    if(n->isDrawable()) {
        addDrawable(n);
    }
    if(dynamic_cast<LightNode*>(n)) {
        addActiveLight((LightNode *)n);
    }
}

void
Scene::onNodeDetached(SceneNode *n) {
    AD_LOG(print) << "detached " << (void *)n;
    if(n == nullptr) {
        return;
    }
    if(n->isDrawable()) {
        removeDrawable(n);
    }
    else if(dynamic_cast<LightNode*>(n)) {
        removeActiveLight((LightNode *)n);
    }
}

void Scene::addAnimationTask(SceneNode *owner, ObjectPtr<AnimationTask> task) {
    animationTasks_->addTask(owner, task);
}


ARTD_END


