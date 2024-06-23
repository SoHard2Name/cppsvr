#pragma once

#include "string"
#include "yaml-cpp/yaml.h"

namespace utility {

class ConfigBase {
public:
	// 还在读 logger config 的时候肯定不能输出日志。不然就 core dump 了。
	ConfigBase(std::string sFileName, bool bCanLog = true);
	virtual ~ConfigBase() {};
	std::string GetYamlFileName();

protected:
	// 用重载的方式封装获取 node 值的操作。
	void GetNodeValue(const YAML::Node &oNode, bool bDefaultValue, bool &bVariable);
	void GetNodeValue(const YAML::Node &oNode, int iDefaultValue, int &iVariable);
	void GetNodeValue(const YAML::Node &oNode, uint32_t iDefaultValue, uint32_t &iVariable); // 传入的默认值比如 0u
	void GetNodeValue(const YAML::Node &oNode, std::string sDefaultValue, std::string &sVariable);

protected: 
	bool m_bCanLog;
	std::string m_sYamlFileName;
	YAML::Node m_oRootNode;
};

}