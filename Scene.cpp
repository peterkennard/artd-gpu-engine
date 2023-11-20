#include "./GpuEngineImpl.h"
#include "artd/Scene.h"
#include "artd/LightNode.h"
#include "artd/Mutex.h"

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
        
        TaskEntry(AnimationFunction f, SceneNode *owner)
            : task_(f)
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
        AnimationFunction task_;
        SceneNode *owner_;
        TaskEntry *nextOwned_ = nullptr;

        INL bool tick(AnimationTaskContext &tc) {
            tc.timing().isDebugFrame();
            return(task_(tc));
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
                    *pPrior = destroying->nextOwned_;
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

        void onAttach(TaskEntry *te) {
            AD_LOG(print) << "attaching " << (void *)te;
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
    
    void addTask(SceneNode *owner, AnimationFunction task) {
        list_.addHead(new TaskEntry(task,owner));
    }
    
//    void test() {
//
//        TaskList test1;
//
//        for(int i = 0; i < 10; ++i) {
//            TaskEntry *te = new TaskEntry();
//            test1.addTail(te);
//        }
//
//        int i = 0;
//        for(auto it = test1.begin(); it != test1.end(); ++it) {
//            ++i;
//           // TaskEntry &task = *it;
//            if(i >= 5) {
//                it.remove();
//            }
//        }
//        test1.clear();
//    }

};

const TypedPropertyKey<AnimationTaskList::OwnedEntryList>  AnimationTaskList::OwnedEntriesKey("AnimationTaskList::OwnedEntriesKey");


Scene::Scene(GpuEngine *e)
    : owner_(e)
{
    rootNode_ = ObjectBase::make<SceneRoot>(this);
    animationTasks_ = ObjectBase::make<AnimationTaskList>();
    
//    animationTasks_->test();
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
Scene::addDrawable(SceneNode *l) {

    for(int i = (int)drawables_.size(); i > 0;) {
        --i;
        if(((SceneNode*)(drawables_[i])) == l) {
            return;
        }
    }
    drawables_.push_back((MeshNode*)l);
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

void Scene::addAnimationTask(SceneNode *owner, AnimationFunction task) {
    animationTasks_->addTask(owner, task);
}


ARTD_END


