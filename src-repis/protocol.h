#pragma once
#include <string>
#include <vector>

inline std::string build_command(const std::list<std::string>& redis_cmd) {
	{
		std::string cmd = "*" + std::to_string(redis_cmd.size()) + "\r\n";

		for (const auto& cmd_part : redis_cmd)
			cmd += "$" + std::to_string(cmd_part.length()) + "\r\n" + cmd_part + "\r\n";

		return cmd;
	}
}