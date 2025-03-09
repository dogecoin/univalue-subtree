// Copyright 2014 BitPay Inc.
// Copyright 2015 Bitcoin Core Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/licenses/mit-license.php.

#ifndef __UNIVALUE_H__
#define __UNIVALUE_H__

#include <charconv>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#include <unordered_map>

class UniValue {
public:
    enum VType { VNULL, VOBJ, VARR, VSTR, VNUM, VBOOL, };

    UniValue() noexcept { typ = VNULL; }
    UniValue(UniValue::VType initialType, const std::string& initialStr = "") {
        typ = initialType;
        val = initialStr;
    }
    UniValue(uint64_t val_) {
        setInt(val_);
    }
    UniValue(int64_t val_) {
        setInt(val_);
    }
    UniValue(bool val_) {
        setBool(val_);
    }
    UniValue(int val_) {
        setInt(val_);
    }
    UniValue(double val_) {
        setFloat(val_);
    }
    UniValue(const std::string& val_) {
        setStr(val_);
    }
    UniValue(const char *val_) {
        std::string s(val_);
        setStr(s);
    }

    // Special member functions
    UniValue(const UniValue& other) :
        typ(other.typ),
        val(other.val),
        keys(other.keys),
        values(other.values)
    {
        if (other.key_lookup) {
            key_lookup = std::make_unique<std::unordered_map<std::string, size_t>>(*other.key_lookup);
        }
    }

    // Move constructor
    UniValue(UniValue&& other) noexcept :
        typ(other.typ),
        val(std::move(other.val)),
        keys(std::move(other.keys)),
        values(std::move(other.values)),
        key_lookup(std::move(other.key_lookup))
    {
        other.typ = VNULL;
    }

    // Assignment operator using copy-and-swap idiom
    UniValue& operator=(UniValue other) noexcept {
        std::swap(typ, other.typ);
        std::swap(val, other.val);
        std::swap(keys, other.keys);
        std::swap(values, other.values);
        std::swap(key_lookup, other.key_lookup);
        return *this;
    }

    ~UniValue() = default;

    void clear();

    bool setNull();
    bool setBool(bool val);
    bool setNumStr(const std::string& val);
    bool setInt(uint64_t val);
    bool setInt(int64_t val);
    bool setInt(int val_) { return setInt((int64_t)val_); }
    bool setFloat(double val);
    bool setStr(const std::string& val);
    bool setArray();
    bool setObject();

    // Query methods that can't throw
    VType getType() const noexcept { return typ; }
    const std::string& getValStr() const noexcept { return val; }
    bool empty() const noexcept { return (values.size() == 0); }
    size_t size() const noexcept { return values.size(); }
    bool isNull() const noexcept { return (typ == VNULL); }
    bool isTrue() const noexcept { return (typ == VBOOL) && (val == "1"); }
    bool isFalse() const noexcept { return (typ == VBOOL) && (val != "1"); }
    bool isBool() const noexcept { return (typ == VBOOL); }
    bool isStr() const noexcept { return (typ == VSTR); }
    bool isNum() const noexcept { return (typ == VNUM); }
    bool isArray() const noexcept { return (typ == VARR); }
    bool isObject() const noexcept { return (typ == VOBJ); }

    bool getBool() const { return isTrue(); }
    void getObjMap(std::map<std::string,UniValue>& kv) const;
    bool checkObject(const std::map<std::string,UniValue::VType>& memberTypes) const;
    const UniValue& operator[](const std::string& key) const;
    const UniValue& operator[](size_t index) const;
    bool exists(const std::string& key) const { size_t i; return findKey(key, i); }

    bool push_back(const UniValue& val);
    bool push_backV(const std::vector<UniValue>& vec);

    void __pushKV(const std::string& key, const UniValue& val);
    bool pushKV(const std::string& key, const UniValue& val);
    bool pushKVs(const UniValue& obj);

    std::string write(unsigned int prettyIndent = 0,
                      unsigned int indentLevel = 0) const;

    bool read(const char *raw, size_t len);
    bool read(const char *raw) { return read(raw, strlen(raw)); }
    bool read(const std::string& rawStr) {
        return read(rawStr.data(), rawStr.size());
    }

private:
    UniValue::VType typ;
    std::string val;
    std::vector<std::string> keys;
    std::vector<UniValue> values;
    static constexpr size_t SMALL_OBJECT_THRESHOLD = 8;
    static constexpr const char* TRUE_STR = "1";
    mutable std::unique_ptr<std::unordered_map<std::string, size_t>> key_lookup;

    bool findKey(const std::string& key, size_t& retIdx) const;
    void writeArray(unsigned int prettyIndent, unsigned int indentLevel, std::string& s) const;
    void writeObject(unsigned int prettyIndent, unsigned int indentLevel, std::string& s) const;

public:
    // Strict type-specific getters, these throw std::runtime_error if the
    // value is of unexpected type
    const std::vector<std::string>& getKeys() const;
    const std::vector<UniValue>& getValues() const;
    template <typename Int>
    auto getInt() const
    {
        static_assert(std::is_integral<Int>::value);
        if (typ != VNUM) {
            throw std::runtime_error("JSON value is not an integer as expected");
        }
        Int result;
        const auto [first_nonmatching, error_condition] = std::from_chars(val.data(), val.data() + val.size(), result);
        if (first_nonmatching != val.data() + val.size() || error_condition != std::errc{}) {
            throw std::runtime_error("JSON integer out of range");
        }
        return result;
    }
    bool get_bool() const;
    const std::string& get_str() const;
    double get_real() const;
    const UniValue& get_obj() const;
    const UniValue& get_array() const;

    enum VType type() const { return getType(); }
    friend const UniValue& find_value( const UniValue& obj, const std::string& name);
};

enum jtokentype {
    JTOK_ERR        = -1,
    JTOK_NONE       = 0,                           // eof
    JTOK_OBJ_OPEN,
    JTOK_OBJ_CLOSE,
    JTOK_ARR_OPEN,
    JTOK_ARR_CLOSE,
    JTOK_COLON,
    JTOK_COMMA,
    JTOK_KW_NULL,
    JTOK_KW_TRUE,
    JTOK_KW_FALSE,
    JTOK_NUMBER,
    JTOK_STRING,
};

extern enum jtokentype getJsonToken(std::string& tokenVal,
                                    unsigned int& consumed, const char *raw, const char *end);
extern const char* uvTypeName(UniValue::VType t) noexcept;

static inline bool jsonTokenIsValue(enum jtokentype jtt) noexcept {
    switch (jtt) {
    case JTOK_KW_NULL:
    case JTOK_KW_TRUE:
    case JTOK_KW_FALSE:
    case JTOK_NUMBER:
    case JTOK_STRING:
        return true;

    default:
        return false;
    }

    // not reached
}

static inline bool json_isspace(int ch) noexcept {
    switch (ch) {
    case 0x20:
    case 0x09:
    case 0x0a:
    case 0x0d:
        return true;

    default:
        return false;
    }

    // not reached
}

extern const UniValue NullUniValue;

const UniValue& find_value( const UniValue& obj, const std::string& name);

#endif // __UNIVALUE_H__