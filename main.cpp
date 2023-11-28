#include "artd/GpuEngine.h"
#include "artd/Scene.h"
#include "artd/CameraNode.h"
#include "artd/LightNode.h"
#include "artd/MeshNode.h"

int runGpuTest(int argc, char **argv)
{
using namespace artd;

    GpuEngine &test = GpuEngine::getInstance();

    int ret = test.init(false, 1920, 1080);
    if(ret != 0) {
        return(ret);
    }
    return(test.run());
}

int main (int argc, char**argv) {
    return(runGpuTest(argc,argv));
}

