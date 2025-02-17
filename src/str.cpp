#include "pocketpy/str.h"

namespace pkpy {

int utf8len(unsigned char c, bool suppress){
    if((c & 0b10000000) == 0) return 1;
    if((c & 0b11100000) == 0b11000000) return 2;
    if((c & 0b11110000) == 0b11100000) return 3;
    if((c & 0b11111000) == 0b11110000) return 4;
    if((c & 0b11111100) == 0b11111000) return 5;
    if((c & 0b11111110) == 0b11111100) return 6;
    if(!suppress) throw std::runtime_error("invalid utf8 char: " + std::to_string(c));
    return 0;
}

    Str::Str(int size, bool is_ascii): size(size), is_ascii(is_ascii) {
        _alloc();
    }

#define STR_INIT()                                  \
        _alloc();                                   \
        for(int i=0; i<size; i++){                  \
            data[i] = s[i];                         \
            if(!isascii(s[i])) is_ascii = false;    \
        }

    Str::Str(const std::string& s): size(s.size()), is_ascii(true) {
        STR_INIT()
    }

    Str::Str(std::string_view s): size(s.size()), is_ascii(true) {
        STR_INIT()
    }

    Str::Str(const char* s): size(strlen(s)), is_ascii(true) {
        STR_INIT()
    }

    Str::Str(const char* s, int len): size(len), is_ascii(true) {
        STR_INIT()
    }

#undef STR_INIT

    Str::Str(const Str& other): size(other.size), is_ascii(other.is_ascii) {
        _alloc();
        memcpy(data, other.data, size);
    }

    Str::Str(Str&& other): size(other.size), is_ascii(other.is_ascii) {
        if(other.is_inlined()){
            data = _inlined;
            for(int i=0; i<size; i++) _inlined[i] = other._inlined[i];
        }else{
            data = other.data;
            other.data = other._inlined;
            other.size = 0;
        }
    }

    Str operator+(const char* p, const Str& str){
        Str other(p);
        return other + str;
    }

    std::ostream& operator<<(std::ostream& os, const Str& str){
        os.write(str.data, str.size);
        return os;
    }

    bool operator<(const std::string_view other, const Str& str){
        return str > other;
    }

    void Str::_alloc(){
        if(size <= 16){
            this->data = _inlined;
        }else{
            this->data = (char*)pool64.alloc(size);
        }
    }

    Str& Str::operator=(const Str& other){
        if(!is_inlined()) pool64.dealloc(data);
        size = other.size;
        is_ascii = other.is_ascii;
        _alloc();
        memcpy(data, other.data, size);
        return *this;
    }

    Str Str::operator+(const Str& other) const {
        Str ret(size + other.size, is_ascii && other.is_ascii);
        memcpy(ret.data, data, size);
        memcpy(ret.data + size, other.data, other.size);
        return ret;
    }

    Str Str::operator+(const char* p) const {
        Str other(p);
        return *this + other;
    }

    bool Str::operator==(const Str& other) const {
        if(size != other.size) return false;
        return memcmp(data, other.data, size) == 0;
    }

    bool Str::operator!=(const Str& other) const {
        if(size != other.size) return true;
        return memcmp(data, other.data, size) != 0;
    }

    bool Str::operator==(const std::string_view other) const {
        if(size != (int)other.size()) return false;
        return memcmp(data, other.data(), size) == 0;
    }

    bool Str::operator!=(const std::string_view other) const {
        if(size != (int)other.size()) return true;
        return memcmp(data, other.data(), size) != 0;
    }

    bool Str::operator==(const char* p) const {
        return *this == std::string_view(p);
    }

    bool Str::operator!=(const char* p) const {
        return *this != std::string_view(p);
    }

    bool Str::operator<(const Str& other) const {
        int ret = strncmp(data, other.data, std::min(size, other.size));
        if(ret != 0) return ret < 0;
        return size < other.size;
    }

    bool Str::operator<(const std::string_view other) const {
        int ret = strncmp(data, other.data(), std::min(size, (int)other.size()));
        if(ret != 0) return ret < 0;
        return size < (int)other.size();
    }

    bool Str::operator>(const Str& other) const {
        int ret = strncmp(data, other.data, std::min(size, other.size));
        if(ret != 0) return ret > 0;
        return size > other.size;
    }

    bool Str::operator<=(const Str& other) const {
        int ret = strncmp(data, other.data, std::min(size, other.size));
        if(ret != 0) return ret < 0;
        return size <= other.size;
    }

    bool Str::operator>=(const Str& other) const {
        int ret = strncmp(data, other.data, std::min(size, other.size));
        if(ret != 0) return ret > 0;
        return size >= other.size;
    }

    Str::~Str(){
        if(!is_inlined()) pool64.dealloc(data);
        if(_cached_c_str != nullptr) free((void*)_cached_c_str);
    }

    Str Str::substr(int start, int len) const {
        Str ret(len, is_ascii);
        memcpy(ret.data, data + start, len);
        return ret;
    }

    Str Str::substr(int start) const {
        return substr(start, size - start);
    }

    char* Str::c_str_dup() const {
        char* p = (char*)malloc(size + 1);
        memcpy(p, data, size);
        p[size] = 0;
        return p;
    }

    const char* Str::c_str() const{
        if(_cached_c_str == nullptr){
            _cached_c_str = c_str_dup();
        }
        return _cached_c_str;
    }

    std::string_view Str::sv() const {
        return std::string_view(data, size);
    }

    std::string Str::str() const {
        return std::string(data, size);
    }

    Str Str::lstrip() const {
        std::string copy(data, size);
        copy.erase(copy.begin(), std::find_if(copy.begin(), copy.end(), [](char c) {
            // std::isspace(c) does not working on windows (Debug)
            return c != ' ' && c != '\t' && c != '\r' && c != '\n';
        }));
        return Str(copy);
    }

    Str Str::strip() const {
        std::string copy(data, size);
        copy.erase(copy.begin(), std::find_if(copy.begin(), copy.end(), [](char c) {
            return c != ' ' && c != '\t' && c != '\r' && c != '\n';
        }));
        copy.erase(std::find_if(copy.rbegin(), copy.rend(), [](char c) {
            return c != ' ' && c != '\t' && c != '\r' && c != '\n';
        }).base(), copy.end());
        return Str(copy);
    }

    Str Str::lower() const{
        std::string copy(data, size);
        std::transform(copy.begin(), copy.end(), copy.begin(), [](unsigned char c){ return std::tolower(c); });
        return Str(copy);
    }

    Str Str::upper() const{
        std::string copy(data, size);
        std::transform(copy.begin(), copy.end(), copy.begin(), [](unsigned char c){ return std::toupper(c); });
        return Str(copy);
    }

    Str Str::escape(bool single_quote) const {
        std::stringstream ss;
        ss << (single_quote ? '\'' : '"');
        for (int i=0; i<length(); i++) {
            char c = this->operator[](i);
            switch (c) {
                case '"':
                    if(!single_quote) ss << '\\';
                    ss << '"';
                    break;
                case '\'':
                    if(single_quote) ss << '\\';
                    ss << '\'';
                    break;
                case '\\': ss << '\\' << '\\'; break;
                case '\n': ss << "\\n"; break;
                case '\r': ss << "\\r"; break;
                case '\t': ss << "\\t"; break;
                default:
                    if ('\x00' <= c && c <= '\x1f') {
                        ss << "\\x" << std::hex << std::setw(2) << std::setfill('0') << (int)c;
                    } else {
                        ss << c;
                    }
            }
        }
        ss << (single_quote ? '\'' : '"');
        return ss.str();
    }

    int Str::index(const Str& sub, int start) const {
        auto p = std::search(data + start, data + size, sub.data, sub.data + sub.size);
        if(p == data + size) return -1;
        return p - data;
    }

    Str Str::replace(const Str& old, const Str& new_, int count) const {
        std::stringstream ss;
        int start = 0;
        while(true){
            int i = index(old, start);
            if(i == -1) break;
            ss << substr(start, i - start);
            ss << new_;
            start = i + old.size;
            if(count != -1 && --count == 0) break;
        }
        ss << substr(start, size - start);
        return ss.str();
    }


    int Str::_unicode_index_to_byte(int i) const{
        if(is_ascii) return i;
        int j = 0;
        while(i > 0){
            j += utf8len(data[j]);
            i--;
        }
        return j;
    }

    int Str::_byte_index_to_unicode(int n) const{
        if(is_ascii) return n;
        int cnt = 0;
        for(int i=0; i<n; i++){
            if((data[i] & 0xC0) != 0x80) cnt++;
        }
        return cnt;
    }

    Str Str::u8_getitem(int i) const{
        i = _unicode_index_to_byte(i);
        return substr(i, utf8len(data[i]));
    }

    Str Str::u8_slice(int start, int stop, int step) const{
        std::stringstream ss;
        if(is_ascii){
            for(int i=start; step>0?i<stop:i>stop; i+=step) ss << data[i];
        }else{
            for(int i=start; step>0?i<stop:i>stop; i+=step) ss << u8_getitem(i);
        }
        return ss.str();
    }

    int Str::u8_length() const {
        return _byte_index_to_unicode(size);
    }

    std::ostream& operator<<(std::ostream& os, const StrName& sn){
        return os << sn.sv();
    }

    StrName StrName::get(std::string_view s){
        auto it = _interned.find(s);
        if(it != _interned.end()) return StrName(it->second);
        uint16_t index = (uint16_t)(_r_interned.size() + 1);
        _interned[s] = index;
        _r_interned.push_back(s);
        return StrName(index);
    }

    Str StrName::escape() const {
        return _r_interned[index-1].escape();
    }

    bool StrName::is_valid(int index) {
        // check _r_interned[index-1] is valid
        return index > 0 && index <= _r_interned.size();
    }

    StrName::StrName(): index(0) {}
    StrName::StrName(uint16_t index): index(index) {}
    StrName::StrName(const char* s): index(get(s).index) {}
    StrName::StrName(const Str& s){
        index = get(s.sv()).index;
    }

    std::string_view StrName::sv() const { return _r_interned[index-1].sv(); }

    FastStrStream& FastStrStream::operator<<(const Str& s){
        parts.push_back(&s);
        return *this;
    }

    Str FastStrStream::str() const{
        int len = 0;
        bool is_ascii = true;
        for(auto& s: parts){
            len += s->length();
            is_ascii &= s->is_ascii;
        }
        Str result(len, is_ascii);
        char* p = result.data;
        for(auto& s: parts){
            memcpy(p, s->data, s->length());
            p += s->length();
        }
        return result;    
    }
} // namespace pkpy