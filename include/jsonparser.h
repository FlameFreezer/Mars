#pragma once

#include <cmath>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "mars_types.h"
#include "error.h"

namespace JSON {

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

    class Value {
        ValueTag mTag = ValueTag::jnull;
        union {
            bool mBoolean {false};
            std::string mString;
            Number mNumber;
            Array mArray;
            Object mObject;
        };
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
        bool getBool() const noexcept;
        template<typename T> requires std::is_arithmetic<T>::value
        T getNumberAs() const noexcept {
            return mNumber.to<T>();
        }
        const Number& getNumber() const noexcept;
        std::string&& moveString() noexcept;
        const std::string& getString() const noexcept;
        const Array& getArray() const noexcept;
        Array&& moveArray() noexcept;
        const Object& getObject() const noexcept;
        Object&& moveObject() noexcept;
    };

    Error<Value> parse(const std::string& text) noexcept;
    std::string serialize(const Value& v) noexcept;
}
