#include "./GpuEngineImpl.h"

ARTD_BEGIN


Material::Material(GpuEngineImpl */*owner*/) {
    std::memset(&data_,0,sizeof(data_));
}

Material::~Material() {

}

ARTD_END

