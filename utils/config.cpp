#include "utils/config.h"
#include "utils/logger.h"
#include "commfunctions.h"

namespace utility {

ConfigBase::ConfigBase(std::string sFileName, bool bCanLog/* = true*/)
	 : m_sYamlFileName(sFileName) {
	try {
		m_oRootNode = YAML::LoadFile(m_sYamlFileName);
		if (bCanLog) {
			INFO("load file succ. file name %s", m_sYamlFileName.c_str());
		}
	} catch (const YAML::Exception &err) {
		ERROR("load file failed. file name %s. errmsg %s", m_sYamlFileName.c_str(), err.what());
		exit(0);
	}
}

std::string ConfigBase::GetYamlFileName() {
	return m_sYamlFileName;
}

void ConfigBase::GetNodeValue(const YAML::Node &oNode, bool bDefaultValue, bool &bVariable) {
	try {
		bVariable = oNode.as<bool>();
	} catch (const YAML::Exception& err) {
		ERROR("convert type to bool err. errmsg %s", err.what());
		bVariable = bDefaultValue;
	}
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