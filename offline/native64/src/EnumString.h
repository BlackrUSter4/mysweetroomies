// EnumString<T>: a two-way table between an enum and the strings the story JSON uses for it.
//
// The original declared one `static` instance per enum in a header, so every translation unit had its own copy built
// by its static initialiser; here each table exists once (see ExEnums.h / ExEnums.cpp for the instances and their
// contents). Both maps are public because the original code reads them directly (e.g. FilterData::getName() does
// `enumToString[type]`, ExScenarioData::parseJson does `stringToEnum[json]`).
#ifndef ENUMSTRING_H
#define ENUMSTRING_H

#include <map>
#include <string>
#include <vector>

template <typename T>
class EnumString {
public:
    // values[i] <-> names[i]. If a name or value repeats, the first pairing wins for the reverse lookup of that key
    // and the last name wins for the value (std::map insertion semantics of the original). orig 001c5504 & co.
    EnumString(std::vector<T> values, std::vector<std::string> names)
    {
        for (size_t i = 0; i < values.size(); ++i) {
            stringToEnum[names.at(i)] = values[i];
            enumToString[values[i]] = names.at(i);
        }
    }
    ~EnumString() {}

    // Value for a JSON string, or `fallback` when the string is unknown. The original used map::operator[], which
    // yields 0 for an unknown string (and inserted it into the table); callers that relied on that pass T(0).
    T toEnum(const std::string &name, T fallback = T(0)) const
    {
        typename std::map<std::string, T>::const_iterator it = stringToEnum.find(name);
        return it == stringToEnum.end() ? fallback : it->second;
    }

    // JSON string of a value, or "" when the value has no name.
    std::string toString(T value) const
    {
        typename std::map<T, std::string>::const_iterator it = enumToString.find(value);
        return it == enumToString.end() ? std::string() : it->second;
    }

    bool hasName(const std::string &name) const { return stringToEnum.count(name) != 0; }

    std::map<std::string, T> stringToEnum;  // orig offset 0x00
    std::map<T, std::string> enumToString;  // orig offset 0x18
};

#endif
