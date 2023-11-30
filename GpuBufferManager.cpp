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

#define INL ARTD_ALWAYS_INLINE

static const int maxBuffSize = 0x7FFFFF;

BufferChunk::~BufferChunk() {
}

class ARTD_API_GPU_ENGINE GpuBufferManagerImpl
    : public GpuBufferManager
{
    class ManagedGpuBuffer;
    
    class BufferChunkImpl
        : public BufferChunk
    {
        typedef BufferChunk super;
    public:

        BufferChunkImpl()
        {}
//        BufferChunkImpl(Buffer *buf, uint64_t start, uint32_t size)
//            : BufferChunk(buf,start,size)
//        {}
        
        ~BufferChunkImpl() {
            if(parent_) {
                getParent()->removePiece(this);
                parent_ = nullptr;
            }
            size_ = 0;
        }

        INL void setParent(ManagedGpuBuffer *parent) {
            parent_ = static_cast<Buffer*>(parent);
        }

        INL ManagedGpuBuffer *getParent() const {
            return(static_cast<ManagedGpuBuffer*>(parent_));
        }

        INL void setStart(uint32_t start) {
            start_ = start >> 2;
        }
        
        INL void setSize(int32_t size) {
            size_ = size;
        }
    };


    class ManagedGpuBuffer
        : public wgpu::Buffer
    {
    public:
        // TODO: maybe we need to be able to handle buffers larger than 0x07FFFFFFF ?
        int32_t currentEnd_ = 0; // current end of assigned (free or used) space
        int32_t myAllocationSize_ = 0; // size this buffer is currently allocated for
        int32_t freeSize_ = 0;  // total size of free pieces
        int32_t maxUsed_ = 0; // max used in this buffer
        int     usage_ = 0;

        class BufferChunkCompare {
        public:
            INL bool operator()(BufferChunkImpl * const &a, BufferChunkImpl * const &b) const {
                return(a->getStartOffset() < b->getStartOffset());
            }
        };

        typedef std::map < BufferChunkImpl*, BufferChunkImpl*, BufferChunkCompare> MapT;
        MapT pieces_;
        
        ManagedGpuBuffer() : Buffer(nullptr) {
        }
        
        ~ManagedGpuBuffer() {

            AD_LOG(debug) << (void *)this <<" max used " << std::hex << maxUsed_;

            for (auto itx = pieces_.begin(); itx != pieces_.end(); ++itx) {
         //       BufferDataImpl *bd = itx->second;
         //       bd->setParent(nullptr);
            }
            if (*(Buffer *)this != nullptr) {
        //        GLuint uih = glh_;
        //        glh_ = -1;
        //        glDeleteBuffers(1, &uih);
            }
            pieces_.clear();
        }

        bool canBeType(int usage) {
            return((usage_ & usage) == usage);
        }
        INL void setUsage(int usage) {
            usage_ = usage;
        }
        INL BufferUsageFlags getUsage() {
            return(usage_);
        }
        INL Buffer getBuffer() {
            return(*this);
        }
        INL void setBuffer(Buffer b) {
            *(Buffer*)this = b;
        }
        INL void setMyAllocationSize(int32_t allocSize) {
            myAllocationSize_ = allocSize;
        }

        INL void addPiece(BufferChunkImpl *piece) {
            piece->setParent(this);
            pieces_[piece] = piece;
        }
        INL uint32_t alignedSize(uint32_t size) const {
            return((usage_ & BufferUsage::Uniform) ? ARTD_ALIGN_UP(size, 16) : ARTD_ALIGN_UP(size, 4));
        }

        INL int64_t getEndOffset(BufferChunkImpl *piece) const {
            return(piece->getStartOffset() + alignedSize(piece->getSize()));
        }
        

        //       void allocOrReallocPiece(BufferDataImpl *piece, int newSize);

        INL int32_t availableNow() {
            return(myAllocationSize_ - currentEnd_);
        }
        INL int32_t available() {
            return((myAllocationSize_ - currentEnd_) + freeSize_);
        }

    private:
        friend class GpuBufferManagerImpl;

        int32_t firstFreeOffset_ = std::numeric_limits<int>::max();
        bool compactionScheduled_ = false;

        INL bool compactionScheduled() const {
            return(compactionScheduled_);
        }

        void scheduleCompaction() {
            if (!compactionScheduled_) {
                compactionScheduled_ = true;
//                UIEngineImpl::runOnUIThread([&](GraphicsContext &gc) {
//                    if (compactionScheduled_) {
//                        compactPieces();
//                    }
//                    return(false);
//                }, true);
            }
        }

        void allocOrReallocPiece(BufferChunkImpl *piece, int newSize) {
        
            ManagedGpuBuffer *parent = piece->getParent();
            if (parent) {
                parent->removePiece(piece);
            }
        
            // align size for useage
            int allocSize = alignedSize(newSize);
            piece->setStart((uint32_t)currentEnd_);
            currentEnd_ += allocSize;
            if (currentEnd_ > maxUsed_) {
                maxUsed_ = currentEnd_;
            }
            piece->setSize((uint32_t)allocSize);
            addPiece(piece);
        }

        void removePiece(BufferChunkImpl *piece) {
        
            if (getEndOffset(piece) == currentEnd_) {
                // TODO: 64 bit ?
                currentEnd_ = (int32_t)piece->getStartOffset();
                // TODO: remove any free space below end if present.
            }
            else
            {
                freeSize_ += piece->getSize();  // TODO: should this be aligned size ??
                /*
                LOGDEBUG("\n\n\n       %p removing piece at %d freeSize %d oldFree %d", this, piece->getStartOffset(), freeSize_, firstFreeOffset_);
                for (auto itx = pieces_.begin(); itx != pieces_.end(); ++itx) {
                    BufferDataImpl *bd = itx->second;
                    LOGDEBUG("after     %d[%d] end %d", bd->getStartOffset(), bd->getSize(), bd->getEndOffset());
                }
                */
        
                if (((int32_t)piece->getStartOffset()) < firstFreeOffset_) {
                    firstFreeOffset_ = (int32_t)piece->getStartOffset();
                }
                // LOGDEBUG("piece removed new FreeOS %d", firstFreeOffset_);
                scheduleCompaction();
            }
            pieces_.erase(piece);
            /*
            for (auto itx = pieces_.begin(); itx != pieces_.end(); ++itx) {
                BufferDataImpl *bd = itx->second;
                LOGDEBUG("after     %d[%d] end %d", bd->getStartOffset(), bd->getSize(), bd->getEndOffset());
            }
            */
            piece->setParent(nullptr);
            piece->setSize(0);
            // LOGDEBUG("\n\n\n");
        }


        void compactPieces() {

            compactionScheduled_ = false;
        
//            // LOGDEBUG("\n\n");
//        
//            MapT::iterator it;
//        
//            std::vector<BufferDataImpl *> testPieces;
//        
//            if (firstFreeOffset_ == 0) {
//                if (pieces_.empty()) {
//                    currentEnd_ = 0;
//                    freeSize_ = 0;
//                    firstFreeOffset_ = std::numeric_limits<int>::max();
//                    return;
//                }
//                it = pieces_.begin();
//            }
//            else {
//        
//        #ifdef TESTING_COMPACTOR
//                int testSize = 0;
//                BufferDataImpl *pbd;
//        
//                for (int i = 1; i < 9; ++i) {
//                    pbd = new BufferDataImpl();
//                    allocOrReallocPiece(pbd, i * 4);
//                    testPieces.push_back(pbd);
//                }
//        
//                removePiece(testPieces[0]);
//                removePiece(testPieces[2]);
//                removePiece(testPieces[3]);
//                removePiece(testPieces[7]);
//        
//                if (this->freeSize_ == 0) {
//                    return;
//                }
//        #endif
//        
//                BufferChunkImpl bd;
//                bd.setStart(firstFreeOffset_);
//                it = pieces_.lower_bound(&bd);
//                if (it == pieces_.end()) {
//                    if (pieces_.empty() || (freeSize_ == 0)) {
//                        currentEnd_ = 0;
//                        freeSize_ = 0;
//                        firstFreeOffset_ = std::numeric_limits<int>::max();
//                        return;
//                    }
//                    --it;
//                    if (it->second->getEndOffset() <= firstFreeOffset_) {
//                        currentEnd_ = it->second->getEndOffset();
//                        freeSize_ = 0;
//                        firstFreeOffset_ = std::numeric_limits<int>::max();
//                        return;
//                    }
//                }
//            }
//        
//        
//        #ifdef TESTING_COMPACTOR
//            LOGDEBUG("     first free %d freeSize %d end %d", firstFreeOffset_, freeSize_, currentEnd_);
//        
//            for (auto itx = pieces_.begin(); itx != pieces_.end(); ++itx) {
//                BufferDataImpl *bd = itx->second;
//                LOGDEBUG("     %d[%d]", bd->getStartOffset(), bd->getSize());
//            }
//        #endif
//        
//            // create a work buffer for compacting data.
//            GLuint workBuffer;
//    //        glGenBuffers(1, &workBuffer);
//    //        glBindBuffer(type_,workBuffer); // note must be of same type as buffers being copied
//    //        glBufferData(type_, currentEnd_ - (firstFreeOffset_ + freeSize_), NULL, GL_STATIC_DRAW);
//    //        glBindBuffer(GL_COPY_WRITE_BUFFER, workBuffer);
//    //        glBindBuffer(GL_COPY_READ_BUFFER, getGLH());
//        
//            currentEnd_ = firstFreeOffset_;
//            int lastEnd = firstFreeOffset_;
//            int appendStart = 0;
//        
//            int blockSize = 0;
//            int blockStart = 0;
//            bool afterGap = false;
//        
//            MapT::iterator nextIt = it;
//            for (;; it=nextIt) {
//                ++nextIt;
//                BufferChunkImpl *bd = it->second;
//        
//                // see if there is a gap from prior piece ( free space )
//                if (lastEnd < bd->getStartOffset()) {
//        
//                    lastEnd = bd->getEndOffset();
//                    if(!afterGap) {
//                        afterGap = true;
//                    }
//                    else if (blockSize > 0) {   // encountered a second gap after a continguous block
//                        // copy the accumulated block to the compacted buffer
//                        // if(type_ == GL_ELEMENT_ARRAY_BUFFER) LOGDEBUG("copy %d to %d", blockSize, appendStart);
//        
////                        glCopyBufferSubData(GL_COPY_READ_BUFFER,
////                            GL_COPY_WRITE_BUFFER,
////                            blockStart,
////                            appendStart,
////                            blockSize);
//        
//                        appendStart += blockSize;
//                        blockSize = 0;
//                    }
//                    blockStart = bd->getStartOffset();
//                }
//                // set to new starting offset and advance to next
//                bd->setStart(currentEnd_);
//                currentEnd_ = bd->getEndOffset();
//        
//                // append size to block to copy on next gap
//                if(afterGap) {
//                    blockSize += bd->getSize();
//                }
//        
//                if (nextIt == pieces_.end()) {
//                    // if a lingering last block copy it.
//                    if (blockSize > 0) {
//                        // if(type_ == GL_ELEMENT_ARRAY_BUFFER) LOGDEBUG("copy %d to %d", blockSize, appendStart);
//        
////                        glCopyBufferSubData(GL_COPY_READ_BUFFER,
////                            GL_COPY_WRITE_BUFFER,
////                            blockStart,
////                            appendStart,
////                            blockSize);
//        
//                        appendStart += blockSize;
//                    }
//                    break; // done
//                }
//            }
//        
//            // copy any compacted data back to the original buffer
//            if (appendStart > 0) {
////                glBindBuffer(GL_COPY_WRITE_BUFFER, getGLH());
////                glBindBuffer(GL_COPY_READ_BUFFER, workBuffer);
////
////                // if(type_ == GL_ELEMENT_ARRAY_BUFFER) LOGDEBUG("copy %d to %d", blockSize, appendStart);
////
////                glCopyBufferSubData(GL_COPY_READ_BUFFER,
////                    GL_COPY_WRITE_BUFFER,
////                    0,
////                    firstFreeOffset_,
////                    appendStart);
//            }
//        
////            glBindBuffer(GL_COPY_WRITE_BUFFER,0);
////            glBindBuffer(GL_COPY_READ_BUFFER,0);
////            glBindBuffer(type_,0);
//        
//            freeSize_ = 0;
//            firstFreeOffset_ = std::numeric_limits<int>::max();
//        
////            glDeleteBuffers(1, &workBuffer); // done with temp buffer
//        
//        #ifdef TESTING_COMPACTOR
//            for (auto itx = pieces_.begin(); itx != pieces_.end(); ++itx) {
//                BufferChunkImpl *bd = itx->second;
//                AD_LOG(debug) << "after " << bd->getStart() << "[" << bd->getSize() << "] end " <<%d", bd->getEndOffset();
//            }
//            AD_LOG(debug) << "\n\n";
//        
//            currentEnd_ = 0;
//            pieces_.clear();
//        #endif
        }

    };

    
    std::vector<ManagedGpuBuffer*> gpuBuffers_;

    INL Device device() {
        return(owner_.device_);
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
    ~GpuBufferManagerImpl() {
        AD_LOG(info) << "killing buffer manager";

    }
    virtual void shutdown() override {
        AD_LOG(info) << "shutting down!";
    }

    ManagedGpuBuffer *
    createManagedBuffer(int size, BufferUsageFlags usage) {

        BufferDescriptor bufferDesc;

        bufferDesc.size = ARTD_ALIGN_UP(size,16);
        bufferDesc.usage = usage;
        bufferDesc.mappedAtCreation = false;

        Buffer buf = device().createBuffer(bufferDesc);

        auto *bb = new ManagedGpuBuffer();
        gpuBuffers_.push_back(bb);
        bb->setBuffer(buf);
        bb->setUsage(usage);
        bb->setMyAllocationSize(size);
        return(bb);
    }

    void allocOrRealloc(ObjectPtr<BufferChunk> &hBc, int size, BufferUsageFlags usage) {

        ObjectPtr<BufferChunk> retBc = hBc;
        if(retBc == nullptr) {
            retBc = ObjectPtr<BufferChunkImpl>::make();
        }
        
        BufferChunkImpl *bd = static_cast<BufferChunkImpl*>(retBc.get());

        // our siszes are aligned
        int allocSize = ARTD_ALIGN_UP(size, 4);
        ManagedGpuBuffer *bb = bd->getParent();
        int lastSize = bd->getSize();
        if (lastSize < size) {
            for (;;) {

                if (bb) {
                    if (size <= bb->available()) {
                        if ((size - lastSize) > bb->availableNow()) {
                            bb->removePiece(bd);
                            if (bb->compactionScheduled()) {
                                bb->compactPieces();
                            }
                        }
                        bb->allocOrReallocPiece(bd, allocSize);
                        hBc = retBc;
                        return;
                    }
                    bb->removePiece(bd);
                }

                // find a buffer if present with available space
                ManagedGpuBuffer *found = nullptr;
                int foundSize = std::numeric_limits<int>::max();

                for(int i = 0; i < (int)gpuBuffers_.size(); ++i) {
                    ManagedGpuBuffer *glb = gpuBuffers_[i];
                    if (glb == bb || !glb->canBeType(usage)) {
                        continue;
                    }
                    int available = glb->availableNow();
                    if (available >= allocSize) {
                        if (available < foundSize) {
                            found = glb;
                            foundSize = available;
                        }
                    }
                }
                if (found) {
                    bb = found;
                    continue;
                }
                if (allocSize <= maxBuffSize) {
                    bb = createManagedBuffer(maxBuffSize,usage);
                    continue;
                }
                AD_LOG(debug) << "request too large for allocator " << std::hex << allocSize;
            }
            // bb->allocOrReallocPiece(bd, size);
            return;
        }
        else {
            
            // TODO: check size and schedule compaction, or shrink size if drastically smaller.
            if (size == 0) {
                return;
            }
            hBc = retBc;
            if (!bb) {
                bb = gpuBuffers_[0];
                bd->setParent(bb);
                bd->setStart(bb->currentEnd_);
            }
        }
    }

    ObjectPtr<BufferChunk> allocUniformChunk(uint32_t dataSize) override {
        ObjectPtr<BufferChunk> retBc;
        allocOrRealloc(retBc,dataSize,BufferUsage::CopyDst | BufferUsage::Uniform);
        return(retBc);
    }

    ObjectPtr<BufferChunk> allocIndexChunk(int count, const uint16_t *data) override {

        ObjectPtr<BufferChunk> retBc;
        uint32_t dataSize = count * sizeof(*data);
        allocOrRealloc(retBc,dataSize,BufferUsage::CopyDst | BufferUsage::Index);
        if(retBc) {
            owner_.queue.writeBuffer(retBc->getBuffer(), retBc->getStartOffset(), data, retBc->getSize());
        }
        return(retBc);
    }
    ObjectPtr<BufferChunk> allocVertexChunk(int count, const float *data) override {

        ObjectPtr<BufferChunk> retBc;
        uint32_t dataSize = count * sizeof(*data);
        allocOrRealloc(retBc,dataSize,BufferUsage::CopyDst | BufferUsage::Vertex);
        if(retBc) {
            owner_.queue.writeBuffer(retBc->getBuffer(), retBc->getStartOffset(), data, retBc->getSize());
        }
        return(retBc);
    }
    ObjectPtr<BufferChunk> allocStorageChunk(uint32_t dataSize) override {
        ObjectPtr<BufferChunk> retBc;
        allocOrRealloc(retBc,dataSize,BufferUsage::CopyDst | BufferUsage::Storage);
        return(retBc);
    }

    
};


GpuBufferManager::GpuBufferManager(GpuEngineImpl *owner)
    : owner_(*owner)
{
}

GpuBufferManager::~GpuBufferManager() {
    //    shutdown();
}

ObjectPtr<GpuBufferManager>
GpuBufferManager::create(GpuEngineImpl *owner) {
    return( ObjectPtr<GpuBufferManagerImpl>::make(owner));
}


#undef INL

ARTD_END
