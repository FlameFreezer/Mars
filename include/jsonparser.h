#pragma once

#include <cmath>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "mars_types.h"
#include "error.h"

namespace JSON {
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
        using Array = std::vector<Value>;
        using Object = std::unordered_map<std::string, Value>;
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
        explicit Value(const std::string& str) noexcept;
        explicit Value(std::string&& str) noexcept;        
        explicit Value(const Array& a) noexcept;
        explicit Value(Array&& a) noexcept;
        explicit Value(const Object& o) noexcept;
        explicit Value(Object&& o) noexcept;
        Value(const Value& other) noexcept;
        Value(Value&& other) noexcept;        
        Value& operator=(const Value& rhs) noexcept;
        Value& operator=(Value&& rhs) noexcept;        
        ~Value() noexcept;        
        ValueTag getTag() const noexcept;        
        bool getBool() const noexcept;
        template<typename T> requires std::is_arithmetic<T>::value
        T getNumberAs() const noexcept {
            return mNumber.to<T>();
        }
        const Number& getNumber() const noexcept;
        Number& getNumber() noexcept;
        const std::string& getString() const noexcept;
        std::string& getString() noexcept;
        std::string&& moveString() noexcept;
        const Array& getArray() const noexcept;
        Array& getArray() noexcept;
        Array&& moveArray() noexcept;
        const Object& getObject() const noexcept;
        Object& getObject() noexcept;
        Object&& moveObject() noexcept;
    };

    using Object = std::unordered_map<std::string, Value>;
    using Array = std::vector<Value>;

    Error<Value> parse(const std::string& text) noexcept;
    std::string serialize(const Value& v) noexcept;
}
