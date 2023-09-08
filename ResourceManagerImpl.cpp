#include <sys/stat.h>
#include "ResourceManagerImpl.h"
#include "GpuEngineImpl.h"

ARTD_BEGIN

ResourceManager *
ResourceManager::instance_ = 0;

ResourceManagerImpl::ResourceManagerImpl() {
    instance_ = this;
}
ResourceManagerImpl::~ResourceManagerImpl() {
    instance_ = nullptr;
}
void
ResourceManagerImpl::shutdown() {
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


#else // _MSC_VER Windows native

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
        FILE *f = fopen(path, "rb");
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


#endif // end windows



ARTD_END
