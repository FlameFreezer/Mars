module;

#include <cmath>
#include <string>
#include <type_traits>
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

    struct Number {
        u64 whole = 0;
        u64 part = 0;
        u64 partPlace = 1;
        u64 exponent = 0;
        i32 sign = 1;
        i32 exponentSign = 1;
        template<typename T> requires std::is_arithmetic<T>::value
        T to() const noexcept {
            return (sign * (whole + static_cast<double>(part) / partPlace)) * std::pow(10, static_cast<i64>(exponent * exponentSign));
        }
    };

    union ValueUnion {
        bool boolean;
        Number number;
        std::string* string;
        Array* array;
        Object* object;
        ValueUnion() : boolean(false) {}
        ValueUnion(bool b) : boolean(b) {}
        ValueUnion(Number n) : number(n) {}
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
        explicit Value(Number n) noexcept;
        explicit Value(std::string&& str) noexcept;        
        explicit Value(Array&& a) noexcept;
        explicit Value(Object&& o) noexcept;
        Value(Value&& other) noexcept;        
        Value& operator=(Value&& rhs) noexcept;        
        ~Value() noexcept;        
        ValueTag getTag() const noexcept;        
        ValueUnion& getData() noexcept;        
        const ValueUnion& getData() const noexcept;    
        bool getBool() const noexcept;
        template<typename T> requires std::is_arithmetic<T>::value
        T getNumberAs() const noexcept {
            return mData.number.to<T>();
        }
        const Number& getNumber() const noexcept;
        std::string* getString() noexcept;
        const std::string* getString() const noexcept;
        Array* getArray() noexcept;
        const Array* getArray() const noexcept;
        Object* getObject() noexcept;
        const Object* getObject() const noexcept;
    };

    Error<Value> parse(const std::string& text) noexcept;
    std::string serialize(const Value& v) noexcept;
}
