#ifndef __artd_ResourceBundle_h
#define __artd_ResourceBundle_h

#include "artd/ResourceManager.h"

ARTD_BEGIN

#define INL ARTD_ALWAYS_INLINE

class GraphicsContext;
class Font;

class ARTD_API_GPU_ENGINE ResourceBundle
    : public EngineResource
{
protected:
    ARTD_OBJECT_DECL

    GraphicsContext &gc_;
    ResourceBundle(GraphicsContext &gc) : gc_(gc) {
    }

    virtual void *getLoadedResource_(ResourceType type, int id) = 0;

public:
    ~ResourceBundle() override;
    ResourceType getType() const override {
        return(RES_BUNDLE);
    }

    INL GraphicsContext &myContext() {
        return(gc_);
    }
    /**
     * @param type  type of resource
     * @param name  name of resource
     * @return  "id" index of resource to be used for retrieval after loading.
     *          < 0  if failed to add the spec
     */
    virtual int addSpec(ResourceType type, StringArg name) = 0;

    template<ResourceType RetType>
    struct loaded_return;

    template<ResourceType Type>
    typename loaded_return<Type>::type getLoadedResource(int id);
};


template<>
struct ResourceBundle::loaded_return<RES_FONT> {
    typedef Font * type;
    INL static Font *castRef(void *ret) { return((Font *)ret); }
};


template<ResourceType Type>
typename ResourceBundle::loaded_return<Type>::type ResourceBundle::getLoadedResource(int id) {
    return(ResourceBundle::loaded_return<Type>::castRef(getLoadedResource_(Type,id)));
}


#undef INL

ARTD_END


#endif // __artd_ResourceBundle_h
