#include "artd/Texture.h"

ARTD_BEGIN

Texture::Texture() : tex_(nullptr)
{
}
Texture::~Texture() {
    if(tex_) {
        tex_.destroy();
        tex_.release();
        tex_ = nullptr;
    }
}
const char *
Texture::getName() {
    return("null");
}

TextureView::~TextureView() {
    
}

ARTD_END
