#pragma once

#include "artd/gpu_engine.h"
#include "artd/Matrix4f.h"
#include "artd/TypedPropertyMap.h"
#include <functional>

ARTD_BEGIN


#define INL ARTD_ALWAYS_INLINE

// this is the base for all "addressable" scene objects
// That contain a property map
// virtual destructor

class ARTD_API_GPU_ENGINE SceneObject {
protected:

    int32_t flags_ = 0;
    int32_t id_ = -1;
    TypedPropertyMap properties_;

    INL void setFlags(uint32_t flags) {
        flags_ |= flags;
    }
    INL void clearFlags(uint32_t flags) {
        flags_ &= ~flags;
    }

    INL int32_t testFlags(int flags) const {
        return(flags_ & flags);
    }

    INL SceneObject()
    {}

public:

    static const int32_t fHasChildren  = 0x01;
    static const int32_t fHasTransform = 0x02;
    static const int32_t fHasParent    = 0x04;
    static const int32_t fIsDrawable   = 0x08;

    TypedPropertyMap &properties() {
        return(properties_);
    }

    template<class T>
    INL TypedPropertyKey<T> registerPropertyKey(StringArg name) {
        return(TypedPropertyKey<T>(name));
    }

    template<class T>
    INL T *getProperty(TypedPropertyKey<T> key) {
        return(properties_.getProperty(key));
    }

    template<class T>
    INL void setProperty(TypedPropertyKey<T> key, T &&prop) {
        return(properties_.setProperty<T>(key, std::move(prop)));
    }

    template<class T>
    INL void setProperty(TypedPropertyKey<T> key, const T &prop) {
        return(properties_.setProperty<T>(key, prop));
    }

    template<class T>
    INL void setProperty(TypedPropertyKey<T> key, ObjectPtr<T> &prop) {
        return(properties_.setSharedProperty<T>(key, prop));
    }

    virtual ~SceneObject();

    // TODO: have IDs come from a registrar to keep ordered and searchable
    INL void setId(int32_t id) {
        id_ = id;
    }
    INL int32_t getId() const {
        return(id_);
    }

    INL bool hasChildren() const {
        return(flags_ & fHasChildren);
    }
    INL bool hasParent() const {
        return(flags_ & fHasParent);
    }
    INL bool hasTransform() const {
        return(flags_ & fHasTransform);
    }
    INL bool isDrawable() const {
        return(flags_ & fIsDrawable);
    }
};

#undef INL

ARTD_END