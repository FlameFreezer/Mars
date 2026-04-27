module;

#include <string>
#include <unordered_map>
#include <vector>

export module json;
import types;
import error;

export namespace JSON {

    using Object = std::unordered_map<std::string, class Value*>;
    using Array = std::vector<Value*>;

    enum class ValueTag : u32 {
        jnull,
        jtrue,
        jfalse,
        jnumber,
        jstring,
        jarray,
        jobject
    };

    union ValueUnion {
        bool boolean;
        double number;
        std::string* string;
        Array* array;
        Object* object;
        ValueUnion() : boolean(false) {}
        ValueUnion(bool b) : boolean(b) {}
        ValueUnion(double f) : number(f) {}
        ValueUnion(std::string&& str) : string(new std::string(std::forward<std::string>(str))) {}
        ValueUnion(Array&& a) : array(new Array(std::forward<Array>(a))) {}
        ValueUnion(Object&& o) : object(new Object(std::forward<Object>(o))) {}
        ~ValueUnion() {}
    };

    class Value {
        ValueTag mTag = ValueTag::jnull;
        ValueUnion mData;
        public:
        Value() noexcept = default;
        explicit Value(bool b) noexcept;        
        explicit Value(int i) noexcept;
        explicit Value(double f) noexcept;
        explicit Value(std::string&& str) noexcept;        
        explicit Value(Array&& a) noexcept;
        explicit Value(Object&& o) noexcept;
        Value(Value&& other) noexcept;        
        Value& operator=(Value&& rhs) noexcept;        
        ~Value() noexcept;        
        ValueTag getTag() const noexcept;        
        ValueUnion& getData() noexcept;        
        const ValueUnion& getData() const noexcept;    
    };

    Error<Value> parse(const std::string& text) noexcept;
    std::string serialize(const Value& v) noexcept;
}
