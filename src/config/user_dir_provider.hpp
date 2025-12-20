//@	{"dependencies_extra":[{"ref":"./user_dir_provider.o", "rel":"implementation"}]}

#ifndef SLIDEPROJ_CONFIG_USER_DIR_PROVIDER_HPP
#define SLIDEPROJ_CONFIG_USER_DIR_PROVIDER_HPP

#include <filesystem>

namespace slideproj::config
{
	struct user_dirs
	{
		std::filesystem::path pictures;
		std::filesystem::path savestates;
	};

	std::filesystem::path get_home_dir();

	std::filesystem::path get_state_dir();

	user_dirs get_user_dirs();
}

#endif