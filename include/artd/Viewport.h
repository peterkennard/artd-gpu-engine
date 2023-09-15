#pragma once

#include "artd/TransformChild.h"
#include "artd/Matrix4f.h"

#define INL ARTD_ALWAYS_INLINE

ARTD_BEGIN

class ARTD_API_GPU_ENGINE Viewport {

    int x_ = -1;
    int y_ = -1;
    int width_ = -1;
    int height_ = -1;

	int32_t modifiedStamp_ = -1;

    INL void setAltered() {
        if ((modifiedStamp_ & 0x01) == 0) {
            modifiedStamp_ += 3;
        }
    }

public:

    Viewport() {}
	Viewport(int left, int top, int width, int height);
	~Viewport() {}

    INL bool testGetModifiedStamp(int &lastCount) {
        if (lastCount != (int)modifiedStamp_) {
            lastCount = modifiedStamp_ & (~0x01);
            return(true);
        }
        return(false);
    }

    void setRect(int x, int y, int width, int height) {
        if(width_ != width
            || height_ != height
            || x_ != x
            || y_ != y )
        {
            x_ = x;
            y_ = y;
            width_ = width;
            height_ = height;
            setAltered();
        }
    }
    
    INL float getAspectWtoH() {
        return(width_ / height_);
    }
    
	INL int getWidth() const {
	    return(width_);
	}
	INL int getHeight() const {
	    return(height_);
	}
	INL int getX() const {
	    return(x_);
	}
	INL int getY() const {
	    return(y_);
	}

};


#undef INL

ARTD_END
