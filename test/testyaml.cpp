#include "utils/logger.h"
#include <iostream>
#include <yaml-cpp/yaml.h>

int main(int argc, char **argv) {
	YAML::Node node = YAML::LoadFile("./test/test.yaml");
	INFO("name: %s", node["name"].as<std::string>().c_str());
	INFO("sex: %s", node["sex"].as<std::string>().c_str());
	INFO("age: %s", node["age"].as<std::string>().c_str());
	// 遍历 phone 列表
	const YAML::Node &phones = node["phone"];
	for (std::size_t i = 0; i < phones.size(); ++i) {
		const YAML::Node &phone = phones[i];
		INFO("phone number: %s", phone["number"].as<std::string>().c_str());
		INFO("phone model: %s", phone["model"].as<std::string>().c_str());
	}
	return 0;
}
