#ifndef ENVTABLELAYERDATA_H
#define ENVTABLELAYERDATA_H

#include <string>

// One row of EnvTableLayer: a title line (the key) above a smaller detail line (the value).
struct EnvTableLayerData {
    std::string key;
    std::string value;

    EnvTableLayerData(const std::string& key, const std::string& value) : key(key), value(value) {}
};

#endif
