#include "utils/utility.h"
#include <iostream>
#include <vector>
#include <yaml-cpp/yaml.h>

// 这个 test2 主要是测试一下我的 Config 类的操作。

class Phone {
public:
	std::string m_sNumber;
	std::string m_sModel;
};

class TestConfig : utility::ConfigBase {
public:
	static TestConfig* GetSingleton(std::string sFileName) {
		static TestConfig oTestConfig(sFileName);
		return &oTestConfig;
	}
	const std::string &GetName() const {
		return m_sName;
	}
	const std::string &GetSex() const {
		return m_sSex;
	}
	uint32_t GetAge() const {
		return m_iAge;
	}
	const std::vector<Phone> &GetVecPhones() const {
		return m_vecPhones;
	}
private:
	TestConfig(std::string sFileName) : utility::ConfigBase(sFileName) { // 东西放这里才线程安全
		GetNodeValue(m_oRootNode["name"], "NJK", m_sName);
		GetNodeValue(m_oRootNode["sex"], "male", m_sSex);
		GetNodeValue(m_oRootNode["age"], 0u, m_iAge);
		const auto &oPhones = m_oRootNode["phone"];
		for (const auto &oPhone : oPhones) {
			Phone oOutPhone;
			GetNodeValue(oPhone["number"], "", oOutPhone.m_sNumber);
			GetNodeValue(oPhone["model"], "", oOutPhone.m_sModel);
			m_vecPhones.push_back(std::move(oOutPhone));
		}
	}
	~TestConfig() override {}
	std::string m_sName;
	std::string m_sSex;
	uint32_t m_iAge;
	std::vector<Phone> m_vecPhones;
};

int main() {
	auto &oTestConfig = *TestConfig::GetSingleton("./test/test.yaml");
	INFO("name: %s", oTestConfig.GetName().c_str());
	INFO("sex: %s", oTestConfig.GetSex().c_str());
	INFO("age: %u", oTestConfig.GetAge());
	const auto &vecPhones = oTestConfig.GetVecPhones();
	for (int i = 0; i < vecPhones.size(); i++) {
		const auto &oPhone = vecPhones[i];
		INFO("phone %d: number %s, model %s", i, oPhone.m_sNumber.c_str(), oPhone.m_sModel.c_str());
	}
	
	return 0;
}
/*
老铁没毛病
控制台：
MSG: get logger conf succ. file name ./logs/cppsvr.log, max size 8192, console 0, level DEBUG
日志文件：
2024-06-23 03:10:22 [INFO] /home/abcpony/QQMail/cppsvr/utils/config.cpp:12 load file succ. file name ./test/test.yaml
2024-06-23 03:10:22 [INFO] /home/abcpony/QQMail/cppsvr/test/testyaml2.cpp:54 name: NJK
2024-06-23 03:10:22 [INFO] /home/abcpony/QQMail/cppsvr/test/testyaml2.cpp:55 sex: male
2024-06-23 03:10:22 [INFO] /home/abcpony/QQMail/cppsvr/test/testyaml2.cpp:56 age: 21
2024-06-23 03:10:22 [INFO] /home/abcpony/QQMail/cppsvr/test/testyaml2.cpp:60 phone 0: number 13352, model 红米
2024-06-23 03:10:22 [INFO] /home/abcpony/QQMail/cppsvr/test/testyaml2.cpp:60 phone 1: number 19174, model 荣耀
*/