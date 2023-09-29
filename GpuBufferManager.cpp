#include "GpuEngineImpl.h"
#include "artd/GpuBufferManager.h"
#include "artd/pointer_math.h"
#include <vector>
#include <map>

ARTD_BEGIN

// This is needed as a similar item to
// The Vulkan Memory Manager.  This is simple and done quickly bu good enough for now.
// The GPUs all have limited memory manageent limited handles and limited alignment need
// and many other things that need to be dealt with.  This is here to isolate "user" code from this.
//
// something like it is:  https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator


#define INL ARTD_ALWAYS_INLINE
// #define MIX_BUFFER_TYPES

using namespace wgpu;

class ARTD_API_GPU_ENGINE GpuBufferManagerImpl
    : public GpuBufferManager
{

    int64_t nextIndexStart_ = -1;
    int64_t nextVertexStart_ = -1;

    INL Device device() {
        return(owner_.device);
    }

    void disposeBuffer(Buffer &b) {
        if(b) {
            b.destroy();
            b.release();
            b = nullptr;
        }
    }

public:
    GpuBufferManagerImpl(GpuEngineImpl *owner)
        : GpuBufferManager(owner)
    {
    }
    ~GpuBufferManagerImpl() override {
        AD_LOG(info) << "killing buffer manager";

    }
    virtual void shutdown() override {
        AD_LOG(info) << "shutting down!";
        disposeBuffer(vertices_);
        disposeBuffer(indices_);
        disposeBuffer(storage_);
    }

    Buffer initStorageBuffer() override {
        if(!storage_) {
            BufferDescriptor bufferDesc;
            bufferDesc.size = ARTD_ALIGN_UP(0x0FFFFFF,4);
            bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Storage;
            bufferDesc.mappedAtCreation = false;
            storage_ = device().createBuffer(bufferDesc);
        }
        return(storage_);
    }

    BufferChunk allocIndexChunk(int count, const uint16_t *data) override {
        BufferChunk ret;
        if(nextIndexStart_ <= 0) {
                nextIndexStart_ = 1024;
                BufferDescriptor bufferDesc;
                bufferDesc.size = ARTD_ALIGN_UP(0x0FFFF,4);
                bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
                bufferDesc.mappedAtCreation = false;
                indices_ = device().createBuffer(bufferDesc);
            }
        ret.start = nextIndexStart_;
        size_t dataSize = count * sizeof(uint16_t);
        ret.size = dataSize;
        auto alignedSize = ARTD_ALIGN_UP(dataSize, 4);

        owner_.queue.writeBuffer(indices_, nextIndexStart_, data, dataSize);
        nextIndexStart_ += alignedSize;
        return(ret);
    }
    BufferChunk allocVertexChunk(int count, const float *data) override {
        BufferChunk ret;
        if(nextVertexStart_ <= 0) {
            nextVertexStart_ = 0;
            BufferDescriptor bufferDesc;
            bufferDesc.size = 0x0FFFFFF * sizeof(float);
            bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
            bufferDesc.mappedAtCreation = false;
            vertices_ = device().createBuffer(bufferDesc);
        }

        ret.start = nextVertexStart_;
        size_t dataSize = count * sizeof(*data);
        ret.size = dataSize;
        auto alignedSize = ARTD_ALIGN_UP(dataSize, 4);

        owner_.queue.writeBuffer(vertices_, nextVertexStart_, data, dataSize);
        nextVertexStart_ += alignedSize;
        return(ret);
    }

    std::vector<ManagedGpuBuffer*> gpuBuffers_;

    static BufferDataImpl *getHandleData(BufferHandle &h);

    ManagedGpuBuffer *createManagedBuffer(int size,int type);

};


GpuBufferManager::GpuBufferManager(GpuEngineImpl *owner)
    : owner_(*owner)
{
}

ObjectPtr<GpuBufferManager>
GpuBufferManager::create(GpuEngineImpl *owner) {
    return( ObjectBase::make<GpuBufferManagerImpl>(owner));
}

GpuBufferManager::~GpuBufferManager() {
}





//
//class ManagedGpuBuffer
//    : public ParentBuffer
//{
//public:
//    int32_t currentEnd_ = 0; // current end of assigned (free or used) space
//    int32_t myAllocationSize_ = 0; // size this buffer is currently allocated for
//    int32_t freeSize_ = 0;  // total size of free pieces
//    int32_t maxUsed_ = 0; // max used in this buffer
//    int type_ = 0;
//
//    ~ManagedGpuuffer();
//
//    bool canBeType(int type) {
//        return(type_ == 0 || type == type_);
//    }
//    INL void setType(int type) {
//        type_ = type;
//    }
//    INL int getType() {
//        return(type_);
//    }
//    INL int getGLH() {
//        return(glh_);
//    }
//    INL void setGLH(int glh) {
//        glh_ = glh;
//    }
//    INL void setMyAllocationSize(int32_t allocSize) {
//        myAllocationSize_ = allocSize;
//    }
//    class BufferDataCompare {
//    public:
//        INL bool operator()(const BufferDataImpl *a, const BufferDataImpl *b) const {
//            return(((const BufferData *)a)->getStartOffset() < ((const BufferData *)b)->getStartOffset());
//        }
//    };
//
//    typedef std::map < BufferDataImpl*, BufferDataImpl*, BufferDataCompare> MapT;
//    MapT pieces_;
//
//    void releaseBuffer(BufferDataImpl *piece);
//    void addPiece(BufferDataImpl *piece);
//    void allocOrReallocPiece(BufferDataImpl *piece, int newSize);
//
//    INL int32_t availableNow() {
//        return(myAllocationSize_ - currentEnd_);
//    }
//    INL int32_t available() {
//        return((myAllocationSize_ - currentEnd_) + freeSize_);
//    }
//
//private:
//    friend class GpuBufferManager;
//
//    int32_t firstFreeOffset_ = std::numeric_limits<int>::max();
//    bool compactionScheduled_ = false;
//    INL bool compactionScheduled() const {
//        return(compactionScheduled_);
//    }
//    void scheduleCompaction();
//    void compactPieces(); // GraphicsContext &gc);
//    void removePiece(BufferDataImpl *piece);
//};
//
//
//class BufferDataImpl
//    : public BufferData
//{
//    typedef BufferData super;
//public:
//    uint32_t size_;     // allocated size
//    uint16_t refs_ = 0; // maybe not 0 here
//
//    INL ManagedGLBuffer *getParent() {
//        return(static_cast<ManagedGLBuffer *>(parent_));
//    }
//    INL GLsizeiptr getSize() const { return((GLsizeiptr)size_); }
//    INL int32_t getEndOffset() const { return(start_ + size_); }
//
//    INL void setParent(ManagedGLBuffer *parent) { parent_ = parent; }
//    INL void setStart(uint32_t start) { start_ = start; }
//    INL void setSize(uint32_t size) { size_ = (uint16_t)size; }
//    INL void setFree() {
//        if (parent_) {
//            getParent()->releaseBuffer(this);
//        } else {
//            delete(this);
//        }
//    }
//
//    INL BufferDataImpl() : size_(0)  {
//        parent_ = NULL;
//        setStart(0);
//    }
//};
//
//void
//BufferHandle::addRef() const {
//    BufferDataImpl *bd = const_cast<BufferDataImpl *>(static_cast<const BufferDataImpl *>(p));
//    ++bd->refs_;
//};
//void
//BufferHandle::release() const {
//    BufferDataImpl *bd = const_cast<BufferDataImpl *>(static_cast<const BufferDataImpl *>(p));
//    if (--bd->refs_ == 0) {
//        bd->setFree();        //        LOGDEBUG("deleting buffer @%0x%X %d", (int)bd->getStart(), bd->getSize());
//        const_cast<BufferHandle *>(this)->p = nullptr;
//    }
//};
//
//
//ManagedGpuBuffer::~ManagedGpuBuffer() {
//
//    LOGDEBUG("%p max used 0x%07x", this, maxUsed_);
//
//    for (auto itx = pieces_.begin(); itx != pieces_.end(); ++itx) {
//        BufferDataImpl *bd = itx->second;
//        bd->setParent(NULL);
//    }
//    if (glh_ >= 0) {
//        GLuint uih = glh_;
//        glh_ = -1;
//        glDeleteBuffers(1, &uih);
//    }
//    pieces_.clear();
//}
//
//void
//ManagedGLBuffer::compactPieces(/* GraphicsContext &gc*/ ) {
//    compactionScheduled_ = false;
//
//    // LOGDEBUG("\n\n");
//
//    MapT::iterator it;
//
//    std::vector<BufferDataImpl *> testPieces;
//
//    if (firstFreeOffset_ == 0) {
//        if (pieces_.empty()) {
//            currentEnd_ = 0;
//            freeSize_ = 0;
//            firstFreeOffset_ = std::numeric_limits<int>::max();
//            return;
//        }
//        it = pieces_.begin();
//    }
//    else {
//
//#ifdef TESTING_COMPACTOR
//        int testSize = 0;
//        BufferDataImpl *pbd;
//
//        for (int i = 1; i < 9; ++i) {
//            pbd = new BufferDataImpl();
//            allocOrReallocPiece(pbd, i * 4);
//            testPieces.push_back(pbd);
//        }
//
//        removePiece(testPieces[0]);
//        removePiece(testPieces[2]);
//        removePiece(testPieces[3]);
//        removePiece(testPieces[7]);
//
//        if (this->freeSize_ == 0) {
//            return;
//        }
//#endif
//
//        BufferDataImpl bd;
//        bd.setStart(firstFreeOffset_);
//        it = pieces_.lower_bound(&bd);
//        if (it == pieces_.end()) {
//            if (pieces_.empty() || (freeSize_ == 0)) {
//                currentEnd_ = 0;
//                freeSize_ = 0;
//                firstFreeOffset_ = std::numeric_limits<int>::max();
//                return;
//            }
//            --it;
//            if (it->second->getEndOffset() <= firstFreeOffset_) {
//                currentEnd_ = it->second->getEndOffset();
//                freeSize_ = 0;
//                firstFreeOffset_ = std::numeric_limits<int>::max();
//                return;
//            }
//        }
//    }
//
//
//#ifdef TESTING_COMPACTOR
//    LOGDEBUG("     first free %d freeSize %d end %d", firstFreeOffset_, freeSize_, currentEnd_);
//
//    for (auto itx = pieces_.begin(); itx != pieces_.end(); ++itx) {
//        BufferDataImpl *bd = itx->second;
//        LOGDEBUG("     %d[%d]", bd->getStartOffset(), bd->getSize());
//    }
//#endif
//
//    // create a work buffer for compacting data.
//    GLuint workBuffer;
//    glGenBuffers(1, &workBuffer);
//    glBindBuffer(type_,workBuffer); // note must be of same type as buffers being copied
//    glBufferData(type_, currentEnd_ - (firstFreeOffset_ + freeSize_), NULL, GL_STATIC_DRAW);
//    glBindBuffer(GL_COPY_WRITE_BUFFER, workBuffer);
//    glBindBuffer(GL_COPY_READ_BUFFER, getGLH());
//
//    currentEnd_ = firstFreeOffset_;
//    int lastEnd = firstFreeOffset_;
//    int appendStart = 0;
//
//    int blockSize = 0;
//    int blockStart = 0;
//    bool afterGap = false;
//
//    MapT::iterator nextIt = it;
//    for (;; it=nextIt) {
//        ++nextIt;
//        BufferDataImpl *bd = it->second;
//
//        // see if there is a gap from prior piece ( free space )
//        if (lastEnd < bd->getStartOffset()) {
//
//            lastEnd = bd->getEndOffset();
//            if(!afterGap) {
//                afterGap = true;
//            }
//            else if (blockSize > 0) {   // encountered a second gap after a continguous block
//                // copy the accumulated block to the compacted buffer
//                // if(type_ == GL_ELEMENT_ARRAY_BUFFER) LOGDEBUG("copy %d to %d", blockSize, appendStart);
//
//                glCopyBufferSubData(GL_COPY_READ_BUFFER,
//                    GL_COPY_WRITE_BUFFER,
//                    blockStart,
//                    appendStart,
//                    blockSize);
//
//                appendStart += blockSize;
//                blockSize = 0;
//            }
//            blockStart = bd->getStartOffset();
//        }
//        // set to new starting offset and advance to next
//        bd->setStart(currentEnd_);
//        currentEnd_ = bd->getEndOffset();
//
//        // append size to block to copy on next gap
//        if(afterGap) {
//            blockSize += bd->getSize();
//        }
//
//        if (nextIt == pieces_.end()) {
//            // if a lingering last block copy it.
//            if (blockSize > 0) {
//                // if(type_ == GL_ELEMENT_ARRAY_BUFFER) LOGDEBUG("copy %d to %d", blockSize, appendStart);
//
//                glCopyBufferSubData(GL_COPY_READ_BUFFER,
//                    GL_COPY_WRITE_BUFFER,
//                    blockStart,
//                    appendStart,
//                    blockSize);
//
//                appendStart += blockSize;
//            }
//            break; // done
//        }
//    }
//
//    // copy any compacted data back to the original buffer
//    if (appendStart > 0) {
//        glBindBuffer(GL_COPY_WRITE_BUFFER, getGLH());
//        glBindBuffer(GL_COPY_READ_BUFFER, workBuffer);
//
//        // if(type_ == GL_ELEMENT_ARRAY_BUFFER) LOGDEBUG("copy %d to %d", blockSize, appendStart);
//
//        glCopyBufferSubData(GL_COPY_READ_BUFFER,
//            GL_COPY_WRITE_BUFFER,
//            0,
//            firstFreeOffset_,
//            appendStart);
//    }
//
//    glBindBuffer(GL_COPY_WRITE_BUFFER,0);
//    glBindBuffer(GL_COPY_READ_BUFFER,0);
//    glBindBuffer(type_,0);
//
//    freeSize_ = 0;
//    firstFreeOffset_ = std::numeric_limits<int>::max();
//
//    glDeleteBuffers(1, &workBuffer); // done with temp buffer
//
//#ifdef TESTING_COMPACTOR
//    for (auto itx = pieces_.begin(); itx != pieces_.end(); ++itx) {
//        BufferDataImpl *bd = itx->second;
//        LOGDEBUG("after     %d[%d] end %d", bd->getStartOffset(), bd->getSize(), bd->getEndOffset());
//    }
//    LOGDEBUG("\n\n");
//
//    currentEnd_ = 0;
//    pieces_.clear();
//#endif
//}
//void
//ManagedGLBuffer::scheduleCompaction() {
//    if (!compactionScheduled_) {
//        compactionScheduled_ = true;
//        UIEngineImpl::runOnUIThread([&](GraphicsContext &gc) {
//            if (compactionScheduled_) {
//                compactPieces();
//            }
//            return(false);
//        }, true);
//    }
//}
//void
//ManagedGLBuffer::releaseBuffer(BufferDataImpl *piece) {
//    removePiece(piece);
//    delete(piece);
//}
//void
//ManagedGLBuffer::addPiece(BufferDataImpl *piece) {
//    piece->setParent(this);
//    pieces_[piece] = piece;
//}
//void
//ManagedGLBuffer::removePiece(BufferDataImpl *piece) {
//
//    if (piece->getEndOffset() == currentEnd_) {
//        currentEnd_ = piece->getStartOffset();
//        // TODO: remove any free space below end if present.
//    }
//    else
//    {
//        freeSize_ += piece->size_;
//        /*
//        LOGDEBUG("\n\n\n       %p removing piece at %d freeSize %d oldFree %d", this, piece->getStartOffset(), freeSize_, firstFreeOffset_);
//        for (auto itx = pieces_.begin(); itx != pieces_.end(); ++itx) {
//            BufferDataImpl *bd = itx->second;
//            LOGDEBUG("after     %d[%d] end %d", bd->getStartOffset(), bd->getSize(), bd->getEndOffset());
//        }
//        */
//
//        if (piece->getStartOffset() < firstFreeOffset_) {
//            firstFreeOffset_ = piece->getStartOffset();
//        }
//        // LOGDEBUG("piece removed new FreeOS %d", firstFreeOffset_);
//        scheduleCompaction();
//    }
//    pieces_.erase(piece);
//    /*
//    for (auto itx = pieces_.begin(); itx != pieces_.end(); ++itx) {
//        BufferDataImpl *bd = itx->second;
//        LOGDEBUG("after     %d[%d] end %d", bd->getStartOffset(), bd->getSize(), bd->getEndOffset());
//    }
//    */
//    piece->setParent(NULL);
//    piece->size_ = 0;
//    // LOGDEBUG("\n\n\n");
//}
//void
//ManagedGLBuffer::allocOrReallocPiece(BufferDataImpl *piece, int newSize) {
//
//    ManagedGLBuffer *parent = piece->getParent();
//    if (parent) {
//        parent->removePiece(piece);
//    }
//
//    // align to float 4 bytes
//    int allocSize = ARTD_ALIGN_UP(newSize, 4);
//    piece->setStart((uint32_t)currentEnd_);
//    currentEnd_ += allocSize;
//    if (currentEnd_ > maxUsed_) {
//        maxUsed_ = currentEnd_;
//    }
//    piece->setSize((uint32_t)allocSize);
//    addPiece(piece);
//}
//
//
//static const int maxBuffSize = 0x7FFFFF;
//
//
//INL BufferDataImpl *
//GLBufferManager::getHandleData(BufferHandle &h) {
//    return(root_cast<BufferDataImpl, BufferData>(const_cast<BufferData *>(h.operator->())));
//}
//
//ManagedGLBuffer *
//GLBufferManager::createManagedBuffer(int size, int type) {
//    GLuint bHandle;
//    glGenBuffers(1, &bHandle);
//    auto *bb = new ManagedGLBuffer();
//    glBuffers_.push_back(bb);
//    bb->setType(type);
//    bb->setGLH(bHandle);
//    glBindBuffer(type, bb->getGLH());
//    bb->setMyAllocationSize(size);
//    glBufferData(type, size, NULL, GL_STATIC_DRAW);
//    return(bb);
//}
//
//
//void
//GLBufferManager::onContextCreated() {
//
//    const int numBuffs = 2;
//
//    for(int i = 0; i < numBuffs; ++i) {
//        createManagedBuffer(maxBuffSize, GL_ARRAY_BUFFER);
//    }
//}
//
//GLBufferManager::~GLBufferManager() {
//    shutdown();
//}
//
//void
//GLBufferManager::shutdown() {
//    if (glBuffers_.size() > 0) {
//        LOGDEBUG("shutting down buffer manager");
//        for (int i = glBuffers_.size(); i > 0;) {
//            deleteZ(glBuffers_[--i]);
//        }
//        glBuffers_.clear();
//    }
//}
//
//INL BufferHandle
//GLBufferManager::createBuffer() {
//    auto *bd = new BufferDataImpl();
//    return(BufferHandle(bd));
//}
//void
//GLBufferManager::allocOrRealloc(BufferHandle &buf, int size, int type) {
//
//    BufferDataImpl *bd = getHandleData(buf);
//
//    // our siszes are aligned
//    int allocSize = ARTD_ALIGN_UP(size, 4);
//    ManagedGLBuffer *bb = bd->getParent();
//    int lastSize = bd->getSize();
//    if (lastSize < size) {
//
//        for (;;) {
//
//            if (bb) {
//                if (size <= bb->available()) {
//                    if ((size - lastSize) > bb->availableNow()) {
//                        bb->removePiece(bd);
//                        if (bb->compactionScheduled()) {
//                            bb->compactPieces();
//                        }
//                    }
//                    bb->allocOrReallocPiece(bd, allocSize);
//                    break;
//                }
//                bb->removePiece(bd);
//            }
//
//            // find a buffer if present with available space
//            ManagedGLBuffer *found = nullptr;
//            int foundSize = std::numeric_limits<int>::max();
//
//            for(int i = 0; i < glBuffers_.size(); ++i) {
//                ManagedGLBuffer *glb = glBuffers_[i];
//                if (glb == bb || !glb->canBeType(type)) {
//                    continue;
//                }
//                int available = glb->availableNow();
//                if (available >= allocSize) {
//                    if (available < foundSize) {
//                        found = glb;
//                        foundSize = available;
//                    }
//                }
//            }
//            if (found) {
//                bb = found;
//                continue;
//            }
//            if (allocSize <= maxBuffSize) {
//                bb = createManagedBuffer(maxBuffSize,type);
//                continue;
//            }
//
//            LOGDEBUG("request too large for allocator 0x%08x", allocSize);
//        }
//        // bb->allocOrReallocPiece(bd, size);
//        return;
//    }
//
//
//    else {
//        // TODO: check size and schedule compaction, or shrink size if drastically smaller.
//        if (size == 0) {
//            return;
//        }
//        if (!bb) {
//            bb = glBuffers_[0];
//            bd->setParent(bb);
//            bd->setStart(bb->currentEnd_);
//        }
//    }
//}
//
//void
//GLBufferManager::loadBuffer(BufferHandle &buf, const void *data, int size, int type) {
//
//#ifdef MIX_BUFFER_TYPES
//    type = GL_ARRAY_BUFFER;
//#endif
//
//    allocOrRealloc(buf, size, type);
//    if (size > 0) {
//        // load data into our section of big buffer
//        glBindBuffer(type, buf->getGLH());
//        glBufferSubData(type, buf->getStartOffset(), size, data);
//        // glBindBuffer(type, 0);
//    }
//}
//
//BufferHandle
//GraphicsContext::newGLBO() {
//    return(((CanvasImpl&)getCurrentCanvas()).bufferManager_.createBuffer());
//}
//void
//GraphicsContext::loadVertices(BufferHandle &buf, const void *data, int size) {
//    ((CanvasImpl&)getCurrentCanvas()).bufferManager_.loadBuffer(buf, data, size, GL_ARRAY_BUFFER);
//}
//void
//GraphicsContext::loadIndices(BufferHandle &buf, const void *data, int size) {
//    ((CanvasImpl&)getCurrentCanvas()).bufferManager_.loadBuffer(buf, data, size, GL_ELEMENT_ARRAY_BUFFER);
//}

#undef INL

ARTD_END
