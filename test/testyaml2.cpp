#include "utils/utility.h"
#include <iostream>
#include <yaml-cpp/yaml.h>

// 这个 test2 主要是测试一下我的 Config 类的操作。

int main(int argc, char **argv) {
	YAML::Node node = YAML::LoadFile("./test/test.yaml");
	INFO("name: %s", node["name"].as<std::string>().c_str());
	INFO("sex: %s", node["sex"].as<std::string>().c_str());
	INFO("age: %d", node["age"].as<int>());
	// 遍历 phone 列表
	const YAML::Node &phones = node["phone"];
	for (std::size_t i = 0; i < phones.size(); ++i) {
		const YAML::Node &phone = phones[i];
		INFO("phone number: %s", phone["number"].as<std::string>().c_str());
		INFO("phone model: %s", phone["model"].as<std::string>().c_str());
	}
	return 0;
}
