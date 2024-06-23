#include "utils/utility.h"
#include <iostream>
#include <yaml-cpp/yaml.h>

// 这个 test1 主要是测试一下我的 yaml-cpp 安装和 cmake 配置是否有问题

int main() {
	std::string sFileName = "./test/test.yaml";
	YAML::Node node;
	try {
		node = YAML::LoadFile(sFileName);
		INFO("load file succ. file name %s", sFileName.c_str());
	} catch (const YAML::Exception &err) {
		ERROR("load file failed. file name %s, errmsg %s", sFileName.c_str(), err.what());
		exit(0);
	}
	INFO("name: %s", node["name"].as<std::string>().c_str());
	INFO("sex: %s", node["sex"].as<std::string>().c_str());
	INFO("age: %d", node["age"].as<int>());
	// 遍历 phone 列表
	const YAML::Node &phones = node["phone"];
	for (std::size_t i = 0; i < phones.size(); ++i) {
		const YAML::Node &phone = phones[i];
		INFO("phone number: %s", phone["number"].as<std::string>().c_str());
		try {
			INFO("phone model: %d", phone["model"].as<int>());
		} catch (const YAML::Exception& err) {
			ERROR("convert type to int err. errmsg %s", err.what());
		}
	}
	return 0;
}
