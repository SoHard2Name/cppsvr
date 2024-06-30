#include "cppsvr/configbase.h"
#include "cppsvr/commfunctions.h"
#include "cppsvr/logger.h"

namespace cppsvr {

ConfigBase::ConfigBase(std::string sFileName, bool bCanLog /* = true*/)
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
		if (oNode) {
			bVariable = oNode.as<bool>();
		} else {
			bVariable = bDefaultValue;
		}
	} catch (const YAML::Exception &err) {
		ERROR("convert type to bool err. errmsg %s", err.what());
		bVariable = bDefaultValue;
	}
}

void ConfigBase::GetNodeValue(const YAML::Node &oNode, int iDefaultValue, int &iVariable) {
	try {
		if (oNode) {
			iVariable = oNode.as<int>();
		} else {
			iVariable = iDefaultValue;
		}
	} catch (const YAML::Exception &err) {
		ERROR("convert type to int err. errmsg %s", err.what());
		iVariable = iDefaultValue;
	}
}

void ConfigBase::GetNodeValue(const YAML::Node &oNode, uint32_t iDefaultValue, uint32_t &iVariable) {
	try {
		if (oNode) {
			iVariable = oNode.as<uint32_t>();
		} else {
			iVariable = iDefaultValue;
		}
	} catch (const YAML::Exception &err) {
		ERROR("convert type to uint32 err. errmsg %s", err.what());
		iVariable = iDefaultValue;
	}
}

void ConfigBase::GetNodeValue(const YAML::Node &oNode, std::string sDefaultValue, std::string &sVariable) {
	try {
		if (oNode) {
			sVariable = oNode.as<std::string>();
		} else {
			sVariable = sDefaultValue;
		}
	} catch (const YAML::Exception &err) {
		ERROR("convert type to string err. errmsg %s", err.what());
		sVariable = sDefaultValue;
	}
}

}