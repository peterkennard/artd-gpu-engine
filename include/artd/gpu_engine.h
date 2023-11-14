#pragma once

#include "artd/jlib_base.h"
#include "artd/ObjectBase.h"

// TODO: do we need to handle this for building either static or dll libraries !!

#ifdef BUILDING_artd_gpu_engine
	#define ARTD_API_GPU_ENGINE ARTD_SHARED_LIBRARY_EXPORT
#else
	#define ARTD_API_GPU_ENGINE ARTD_SHARED_LIBRARY_IMPORT
#endif

