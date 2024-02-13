#include "globalConfig.h"

GlobalConfig::GlobalConfig(std::string filepath) {
    std::ifstream configFile(filepath);
    std::string line;
    while (std::getline(configFile, line)) {
        if (line.empty() || line[0] == '#')
            continue;

        size_t delimiterPos = line.find('=');
        if (delimiterPos == std::string::npos) {
            std::cerr << "Error: Invalid line format in the configuration file: " << line << std::endl;
            continue;
        }

        std::string key = line.substr(0, delimiterPos);
        std::string value = line.substr(delimiterPos + 1);
        std::cout << key  << " = " << value << std::endl;
        properties[key] = value;
    }

    hashNum = std::stoi(properties["hashNum"]);
    alpha = std::stod(properties["alpha"]);
    beta = std::stod(properties["beta"]);
    tao = std::stod(properties["tao"]);
    batchSize = std::stoi(properties["batchSize"]);
    threads = std::stoi(properties["threads"]);
    partitionNum = std::stoi(properties["partitionNum"]);
    vCount = std::stoi(properties["vCount"]);
    eCount = std::stod(properties["eCount"]);
    k = std::stoi(properties["k"]);
    inputGraphPath = properties["inputGraphPath"];
}

double GlobalConfig::getMaxClusterVolume() {
    return  eCount / partitionNum * 2.0;
}

double GlobalConfig::getAverageDegree() {
    return 2.0 * eCount / vCount;
}