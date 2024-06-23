#include "utils/config.h"
#include "utils/logger.h"

namespace utility {

ConfigBase::ConfigBase(std::string sFileName) : m_sFileName(sFileName) {
	try {
		m_oRootNode = YAML::LoadFile(m_sFileName);
		INFO("load file succ. file name %s", m_sFileName.c_str());
	} catch (const YAML::Exception &err) {
		ERROR("load file failed. file name %s. errmsg %s", m_sFileName.c_str(), err.what());
		exit(0);
	}
}

std::string ConfigBase::GetFileName() {
	return m_sFileName;
}

void ConfigBase::GetNodeValue(const YAML::Node &oNode, int iDefaultValue, int &iVariable){
	try {
		iVariable = oNode.as<int>();
	} catch (const YAML::Exception& err) {
		ERROR("convert type to int err. errmsg %s", err.what());
		iVariable = iDefaultValue;
	}
}

void ConfigBase::GetNodeValue(const YAML::Node &oNode, uint32_t iDefaultValue, uint32_t &iVariable){
	try {
		iVariable = oNode.as<uint32_t>();
	} catch (const YAML::Exception& err) {
		ERROR("convert type to uint32 err. errmsg %s", err.what());
		iVariable = iDefaultValue;
	}
}

void ConfigBase::GetNodeValue(const YAML::Node &oNode, std::string sDefaultValue, std::string &sVariable){
	try {
		sVariable = oNode.as<std::string>();
	} catch (const YAML::Exception& err) {
		ERROR("convert type to string err. errmsg %s", err.what());
		sVariable = sDefaultValue;
	}
}


}