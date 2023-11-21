#include <sys/stat.h>
#include "ResourceManagerImpl.h"
#include "GpuEngineImpl.h"
#include "artd/Logger.h"

#include <cstdio>
#include <cstdlib>

ARTD_BEGIN

ResourceManager *
ResourceManager::instance_ = 0;

ResourceManagerImpl::ResourceManagerImpl() {
    instance_ = this;

    resourcePath_ = ".";
    auto cpath = resourcePath_.getCanonicalPath(); // current directory
    
    std::string ssp = cpath.c_str();
 
    // TODO: A BIT OF A HACK FOR NOW - to handle running from the debuggers.
    
    int pos = (int)ssp.find("/build/bin/");
    if(pos > 0) {
        ssp = ssp.substr(0,pos+7);
        ssp = ssp + "production/.didi/thePool/";  // for debugging and testing !!
        resourcePath_ = ssp;
    } else {
        ssp = ssp + "/build/production/.didi/thePool/";
        resourcePath_ = ssp;
    }
    return;
}
ResourceManagerImpl::~ResourceManagerImpl() {
    instance_ = nullptr;
}
void
ResourceManagerImpl::shutdown() {
}

ObjectPtr<ResourceManager>
ResourceManager::create()  {
    return(ObjectPtr<ResourceManagerImpl>::make());
}

#ifdef __EMSCRIPTEN__

ARTD_END

#include <emscripten/fetch.h>

ARTD_BEGIN

class LoadBinaryArg {
public:
    LoadBinaryArg(const std::function<void (ByteArray &)> &f, bool inUI)
        : onDone(f), onUIThread(inUI) {
    }
    ByteArray data;
    std::function<void (ByteArray &)> onDone;
    bool onUIThread;
};


extern "C" {

    static void downloadByteArraySucceeded(emscripten_fetch_t *fetch) {
        printf("Finished downloading %llu bytes from URL %s.\n", fetch->numBytes, fetch->url);

        auto *parg = (LoadBinaryArg *)(fetch->userData);
        parg->data = ByteArray((int)(fetch->numBytes));
        memcpy(parg->data.elements(), fetch->data, (size_t)fetch->numBytes);

        if (parg->onUIThread) {
            std::function<void (ByteArray &)> onDone = parg->onDone;
            ByteArray data(std::move(parg->data));

            std::function<bool(void *arg)> f = [data, onDone](void *arg) {
                onDone(const_cast<ByteArray &>(data));
                return(false);
            };
            UIEngineImpl::instance()->runOnMainThread(nullptr,f);
        }
        else {
            ByteArray ba(parg->data);
            parg->onDone(ba);
        }
        delete(parg);
        emscripten_fetch_close(fetch); // Free data associated with the fetch.
    }


    static void downloadFailed(emscripten_fetch_t *fetch) {
        printf("Downloading %s failed, HTTP failure status code: %d.\n", fetch->url, fetch->status);
        auto *parg = (LoadBinaryArg *)(fetch->userData);

        if (parg->onUIThread) {
            auto onDone = parg->onDone;
            std::function<bool(void *)> f = [onDone](void *) {
                ByteArray empty;
                onDone(empty);
                return(false);
            };
            UIEngineImpl::instance()->runOnMainThread(nullptr,f);
        }
        else {
            ByteArray empty;
            parg->onDone(empty);
        }
        delete(parg);
        emscripten_fetch_close(fetch); // Also free data on failure.
    }

};

void
ResourceManagerImpl::_asyncLoadBinaryResource(StringArg path, const std::function<void (ByteArray &)> &onDone, bool onUIThread) {

    // TODO: rather than copy a big buffer do an incremental read and copy one chunk at a time. ???
    LOGDEBUG("starting async fetch %s", path.c_str());

    auto *parg = new LoadBinaryArg(onDone,onUIThread);

    emscripten_fetch_attr_t arg;
    emscripten_fetch_attr_init(&arg);

    strcpy(arg.requestMethod, "GET");
    arg.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    arg.onsuccess = downloadByteArraySucceeded;
    arg.onerror = downloadFailed;
    arg.userData = parg;
    emscripten_fetch(&arg, path);
}

void
ResourceManagerImpl::_asyncLoadStringResource(StringArg path, const std::function<void (RcString &)> &onDone, bool onUIThread) {

    LOGDEBUG("starting async fetch %s", path.c_str());

    auto *parg = new LoadBinaryArg(onDone,onUIThread);

    emscripten_fetch_attr_t arg;
    emscripten_fetch_attr_init(&arg);

    strcpy(arg.requestMethod, "GET");
    arg.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    arg.onsuccess = downloadByteArraySucceeded;
    arg.onerror = downloadFailed;
    arg.userData = parg;
    emscripten_fetch(&arg, path);
}


#else // use std::library

ARTD_END

#include <stdio.h>

ARTD_BEGIN

ByteArray get_file_data(const char* path)
{
    struct stat statbuf;
    if (stat(path, &statbuf) == -1) {
        AD_LOG(debug) << "failed to stat " << path;
        goto error;
    }
    {
      //  const long size = statbuf.st_size;
        FILE *f = std::fopen(path, "rb");
        if (f != nullptr) {
            RcArray<uint8_t> data((int)statbuf.st_size);
            long ret = fread(data.elements(), 1, data.length(), f);
            if (ret != data.length()) {
                goto error;
            }
            return(data);
        }
    }
error:
    return(ByteArray());
}
RcString get_file_string(const char* path)
{
    struct stat statBuf;
    if (stat(path, &statBuf) == -1) {
        AD_LOG(debug) << "failed to stat " << path;
        goto error;
    }
    {
      //  const long size = statBuf.st_size;
      // TODO: this doesn't handle EOL conversions but we don't want
      // to make anything that uses these as EOL type specific.

        FILE *f = std::fopen(path, "rb");
        if (f != nullptr) {
            RcString data((int)statBuf.st_size + 1); // + 1 for terminal null \0
            long ret = fread(data.chars(), 1, data.length(), f);
            if (ret != data.length()) {
                goto error;
            }
            data.chars()[ret] = '\0';
            return(data);
        }
    }
error:
    return(RcString());
}

void
ResourceManagerImpl::_asyncLoadBinaryResource(StringArg path, const std::function<void (ByteArray &)> &onDone, bool onUIThread) {

    RcString resPath = RcString::format("../bin/%s", path.c_str());
    ByteArray data = get_file_data(resPath.c_str());

//    GpuEngineImpl &gpu = *(GpuEngineImpl*)GpuEngineImpl::instance();
//    GraphicsContext &gc = ui.canvas->renderContext_;
    if (onUIThread) { //  && !gc.inMyThread()) {
//        std::function<bool(GraphicsContext &gc)> f = [data, onDone](GraphicsContext &gc) {
//            const ByteArray &rData = data;
//            onDone(const_cast<ByteArray &>(rData));
//            return(false);
//        };
//        ui.runOnUIThread(f);
    }
    else {
        onDone(data);
    }
}
void
ResourceManagerImpl::_asyncLoadStringResource(StringArg path, const std::function<void (RcString&)> &onDone, bool onUIThread) {

    RcString resPath = RcString::format("../bin/%s", path.c_str());
    RcString data = get_file_string(resPath.c_str());

//    GpuEngineImpl &gpu = *(GpuEngineImpl*)GpuEngineImpl::instance();
//    GraphicsContext &gc = ui.canvas->renderContext_;
    if (onUIThread) { //  && !gc.inMyThread()) {
//        std::function<bool(GraphicsContext &gc)> f = [data, onDone](GraphicsContext &gc) {
//            const ByteArray &rData = data;
//            onDone(const_cast<ByteArray &>(rData));
//            return(false);
//        };
//        ui.runOnUIThread(f);
    }
    else {
        onDone(data);
    }
}


#endif // end use std::library



ARTD_END
