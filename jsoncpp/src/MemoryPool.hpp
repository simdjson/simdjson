#ifdef _MSC_VER
#pragma once
#endif
#ifndef HTCW_MEMORYPOOL
#define HTCW_MEMORYPOOL
#ifndef ARDUINO
#include <type_traits>
#include <cinttypes>
#include <cstddef>
#endif
namespace mem {
    // represents an interface/contract for a memory pool
    struct MemoryPool {
    public:
        // allocates the specified number of bytes
        // returns nullptr if there's not enough free
        virtual void* alloc(size_t size)=0;
        // unallocates the most recently allocated bytes of the specified sizesize
        // returns nullptr if failed, or the new next()
        virtual void* unalloc(size_t size)=0;
        // invalidates all the pointers in the pool and frees the memory
        virtual void freeAll()=0;
        // retrieves the base pointer for the pool
        virtual void* base()=0;
        // retrieves the next pointer that will be allocated
        // (for optimization opportunities)
        virtual void* next() const=0;
        // indicates the maximum capacity of the pool
        virtual size_t capacity() const =0;
        // indicates how many bytes are currently used
        virtual size_t used() const=0;
        virtual ~MemoryPool() {}
    };

    // represents a memory pool whose maximum capacity is known at compile time
    template<size_t TCapacity> class StaticMemoryPool : public MemoryPool {
#ifndef ARDUINO
        static_assert(0<TCapacity,
                  "StaticMemoryPool requires a postive value for TCapacity");
#endif
        // the actual buffer
        uint8_t m_heap[TCapacity];
        // the next free pointer
        uint8_t *m_next;
        StaticMemoryPool(const StaticMemoryPool& rhs) = delete;
        StaticMemoryPool(const StaticMemoryPool&& rhs) = delete;
        StaticMemoryPool& operator=(const StaticMemoryPool& rhs) = delete;
    public:
        // allocates the specified number of bytes
        // returns nullptr if there's not enough free
        void* alloc(const size_t size) override {
            // if we don't have enough room return null
            if(used()+size>TCapacity)
                return nullptr;
            // get our pointer and reserve the space
            void* result = m_next;
            m_next+=size;
            // return it
            return result;
        }
        // unallocates the most recently allocated bytes of the specified sizesize
        // returns nullptr if failed, or the new next()
        void* unalloc(size_t size) override {
            if(size>used()) return nullptr;
            m_next-=size;
            return m_next;
        }
        // invalidates all the pointers in the pool and frees the memory
        void freeAll() override {
            // just set next to the beginning
            m_next = m_heap;
        }
        // retrieves the base pointer for the pool
        void* base() override {
            return m_heap;
        }
        // retrieves the next pointer that will be allocated
        // (for optimization opportunities)
        void *next() const override {
            return m_next;
        }
        // indicates the maximum capacity of the pool
        size_t capacity() const override { return TCapacity; }
        // indicates how many bytes are currently used
        size_t used() const override {return m_next-m_heap;}
        StaticMemoryPool() : m_next(m_heap) {}
    
    };
    
    // represents a memory pool whose maximum capacity is determined at runtime
    class DynamicMemoryPool : public MemoryPool {
        // the actual buffer
        uint8_t *m_heap;
        // the capacity
        size_t m_capacity;
        // the next free pointer
        uint8_t *m_next;
        DynamicMemoryPool(const DynamicMemoryPool& rhs) = delete;
        DynamicMemoryPool(const DynamicMemoryPool&& rhs) = delete;
        DynamicMemoryPool& operator=(const DynamicMemoryPool& rhs) = delete;
    public:
        // initializes the dynamic pool with the specified capacity
        DynamicMemoryPool(const size_t capacity) {
            // special case for 0 cap pool
            if(0==capacity) {
                m_heap=m_next = nullptr;
                m_capacity=0;
                return;
            }
            // reserve space from the heap
            m_heap = new uint8_t[capacity];
            // initialize the next pointer
            m_next=m_heap;
            m_capacity = capacity;
            if(nullptr==m_heap)
                m_capacity=0;
            
        }
        // allocates the specified number of bytes
        // returns nullptr if there's not enough free
        void* alloc(const size_t size) override {
            if(nullptr==m_heap)
                return nullptr;
            // if we don't have enough room, return null
            if(used()+size>m_capacity) {
                return nullptr;
            }
            // store the result pointer, then increment next
            // reserving space
            void* result = m_next;
            m_next+=size;
            // return it
            return result;
        }
        // unallocates the most recently allocated bytes of the specified size
        // returns nullptr if failed, or the new next()
        void* unalloc(size_t size) override {
            if(size>used()) return nullptr;
            m_next-=size;
            return m_next;
        }
        // invalidates all the pointers in the pool and frees the memory
        void freeAll() override {
            // just set next to the beginning
            m_next = m_heap;
        }
        // retrieves the base pointer for the pool
        void* base() override {
            return m_heap;
        }
        // retrieves the next pointer that will be allocated
        // (for optimization opportunities)
        void* next() const override {
            // just return the next pointer
            return m_next;
        }
        // indicates the maximum capacity of the pool
        size_t capacity() const override { if(nullptr==m_heap) return 0; return m_capacity; }
        // indicates how many bytes are currently used
        size_t used() const override { return m_next-m_heap;}
        ~DynamicMemoryPool() { if(nullptr!=m_heap) delete m_heap;}
    };
}
#endif
