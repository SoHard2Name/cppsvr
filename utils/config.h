#pragma once

#include "string"
#include "yaml-cpp/yaml.h"

namespace utility {

class ConfigBase {
public:
	std::string GetFileName();

protected:
	ConfigBase(std::string sFileName);
	virtual ~ConfigBase() {};
	// 用重载的方式封装获取 node 值的操作。
	void GetNodeValue(const YAML::Node &oNode, int iDefaultValue, int &iVariable);
	void GetNodeValue(const YAML::Node &oNode, uint32_t iDefaultValue, uint32_t &iVariable); // 传入的默认值比如 0u
	void GetNodeValue(const YAML::Node &oNode, std::string sDefaultValue, std::string &sVariable);

protected:
	std::string m_sFileName;
	YAML::Node m_oRootNode;
};

}