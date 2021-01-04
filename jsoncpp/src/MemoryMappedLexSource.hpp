#ifndef HTCW_MEMORY_MAPPED_LEXSOURCE_HPP
#define HTCW_MEMORY_MAPPED_LEXSOURCE_HPP
// shut PlatformIO/VS Code up:
#ifndef ARDUINO
#include "MappedFile.hpp"
#include "LexSource.hpp"
namespace lex {
class MemoryMappedLexSource : public virtual LexSource {
    mem::MappedFile m_mapped;
    const char* m_cur;
    const char* m_start;
    MemoryMappedLexSource(MemoryMappedLexSource& rhs)=delete;
    MemoryMappedLexSource(MemoryMappedLexSource&& rhs)=delete;
    MemoryMappedLexSource& operator=(MemoryMappedLexSource& rhs)=delete;
    
protected:
    int16_t read() final {
        if(nullptr==m_start)   
            return LexSource::Closed;
        if((size_t)(m_cur-m_start)>=m_mapped.size())
            return LexSource::EndOfInput;
        char ch=*(m_cur++);
        return (unsigned char)ch;
    }
    
    bool skipToAny(const char* characters7bit,unsigned long long int& position,int16_t& match,int8_t& error) final {
        if(!m_mapped.open()) {
            error = Closed;
            return false;
        }
        if(position>=m_mapped.size()) {
            error = EndOfInput;
            return false;
        }
        // TODO: i *think* the memory mapped region is padded with zeroes
        // we're always one ahead of where we want to start searching
        const char*sz=strpbrk(m_cur-1, characters7bit);
        if(nullptr==sz) {
            // advance everything to the end since we found jack
            position+=m_mapped.size()-(m_cur-m_start);
            m_cur = m_start+m_mapped.size();
            error = EndOfInput;
            return false;
        }
        
        match = *sz;
        m_cur = sz+1;
        position = m_cur - m_start;
        return true;
    }
    
public:
    MemoryMappedLexSource() :m_cur(nullptr),m_start(nullptr) {
       
    }
    ~MemoryMappedLexSource() override {}
    
    bool open(const char* filename) {
        if(nullptr!=m_start)
            return false;
        if(!m_mapped.open(filename))
            return false;
        reset();
        m_cur = m_start = m_mapped.data();
        return true;
    }   
    
    void close() {
        if(m_mapped.open()) {
            m_mapped.close();
            m_cur = m_start = nullptr;
        }
    } 
};
template<size_t TCapacity> class StaticMemoryMappedLexSource : public StaticLexSource<TCapacity>, public virtual MemoryMappedLexSource {

};

}
#endif
#endif