#ifndef __artd_ResourceManagerImpl_h
#define __artd_ResourceManagerImpl_h

#include "artd/ResourceManager.h"
#include "artd/JPath.h"

#define USE_TEXT_INDEX32

ARTD_BEGIN

class ResourceManagerImpl
    : public ResourceManager
{

    JPath resourcePath_;

public:
    ResourceManagerImpl();
    ~ResourceManagerImpl();
    void _asyncLoadBinaryResource(StringArg path, const std::function<void (ByteArray &)> &onDone,bool onUIThread) override;
    void _asyncLoadStringResource(StringArg path, const std::function<void (RcString &)> &onDone,bool onUIThread) override;
    void shutdown();
};

ARTD_END

#endif // __artd_ResourceManagerImpl_h
