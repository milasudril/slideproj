//@	{
//@	 "target":{"name":"user_dir_provider.o"}
//@	}

#include "./user_dir_provider.hpp"

#include <filesystem>
#include <stdexcept>
#include <memory>
#include <sys/types.h>
#include <pwd.h>

std::filesystem::path slideproj::config::get_home_dir()
{
	auto const home_env = getenv("HOME");
	if(home_env == nullptr)
	{
		auto const pw = getpwuid(getuid());
		return pw->pw_dir;
	}
	return home_env;
}

std::filesystem::path slideproj::config::get_state_dir()
{
	auto const state_dir_env = getenv("XDG_STATE_HOME");
	if(state_dir_env == nullptr)
	{ return ".local/state"; }
	return state_dir_env;
}

slideproj::config::user_dirs
slideproj::config::get_user_dirs()
{
	auto const home = get_home_dir();
	auto const pictures = home/dgettext("xdg-user-dirs", "Pictures");
	auto const state = home/get_state_dir();

	std::filesystem::create_directories(pictures);
	std::filesystem::create_directories(state);

	return user_dirs{
		.pictures = std::move(pictures),
		.savestates = std::move(state)
	};
}