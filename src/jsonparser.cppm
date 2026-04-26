module;

#include <cstring>
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
        std::string str;
        Array array;
        Object object;
        ValueUnion() : boolean(false) {}
        ValueUnion(bool b) : boolean(b) {}
        ValueUnion(double f) : number(f) {}
        ValueUnion(std::string&& str) : str(std::forward<std::string>(str)) {}
        ValueUnion(Array&& a) : array(std::forward<Array>(a)) {}
        ValueUnion(Object&& o) : object(std::forward<Object>(o)) {}
        ~ValueUnion() {}
    };

    class Value {
        ValueTag mTag = ValueTag::jnull;
        ValueUnion mData;
        public:
        Value() noexcept = default;
        explicit Value(bool b) noexcept : mData(b) {
            if(b) mTag = ValueTag::jtrue;
            else mTag = ValueTag::jfalse;
        }
        explicit Value(int i) noexcept : mTag(ValueTag::jnumber), mData(static_cast<double>(i)) {}
        explicit Value(double f) noexcept : mTag(ValueTag::jnumber), mData(f) {}
        explicit Value(std::string&& str) noexcept : mTag(ValueTag::jstring), mData(std::forward<std::string>(str)) {}
        explicit Value(Array&& a) noexcept : mTag(ValueTag::jarray), mData(std::forward<Array>(a)) {}
        explicit Value(Object&& o) noexcept : mTag(ValueTag::jobject), mData(std::forward<Object>(o)) {}
        Value(Value&& other) noexcept : mTag(other.mTag) {
            std::memset(reinterpret_cast<void*>(&mData), 0, sizeof(ValueUnion));
            switch(mTag) {
                case ValueTag::jnull: break;
                case ValueTag::jfalse: mData.boolean = false; break;
                case ValueTag::jtrue: mData.boolean = true; break;
                case ValueTag::jnumber: mData.number = other.mData.number; break;
                case ValueTag::jstring: mData.str = std::move(other.mData.str); break;
                case ValueTag::jarray: mData.array = std::move(other.mData.array); break;
                case ValueTag::jobject: mData.object = std::move(other.mData.object); break;
            }
            other.mTag = ValueTag::jnull;
        }
        Value& operator=(Value&& rhs) noexcept {
            std::memset(reinterpret_cast<void*>(&mData), 0, sizeof(ValueUnion));
            mTag = rhs.mTag;
            switch(mTag) {
                case ValueTag::jnull: break;
                case ValueTag::jfalse: mData.boolean = false; break;
                case ValueTag::jtrue: mData.boolean = true; break;
                case ValueTag::jnumber: mData.number = rhs.mData.number; break;
                case ValueTag::jstring: mData.str = std::move(rhs.mData.str); break;
                case ValueTag::jarray: mData.array = std::move(rhs.mData.array); break;
                case ValueTag::jobject: mData.object = std::move(rhs.mData.object); break;
            }
            rhs.mTag = ValueTag::jnull;
            return *this;
        }
        ~Value() noexcept {
            switch(mTag) {
                case ValueTag::jstring: 
                    mData.str.~basic_string();
                    break;
                case ValueTag::jobject: 
                    for(auto [key, v] : mData.object) delete v;
                    mData.object.~Object();
                    break;
                case ValueTag::jarray: 
                    for(Value* v : mData.array) delete v;
                    mData.array.~Array();
                    break;
                default: break;
            }
        }
        ValueTag getTag() const noexcept {
            return mTag;
        }
        ValueUnion& getData() noexcept {
            return mData;
        }

        const ValueUnion& getData() const noexcept {
            return mData;
        }
    };

    Value parse(const std::string& text);
    std::string serialize(const Value& v);
}
