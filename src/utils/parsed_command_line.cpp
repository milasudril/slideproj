//@	{"target":{"name":"./parsed_command_line.o"}}

#include "./parsed_command_line.hpp"

#include <stdexcept>
#include <format>

slideproj::utils::parsed_arg slideproj::utils::parse_arg(char const* val)
{
	parsed_arg ret{};
	enum class state{init_1, init_2, key_1, key_2, value, value_escape};
	auto current_state = state::init_1;
	while(*val != '\0')
	{
		auto const ch_in = *val;
		switch(current_state)
		{
			case state::init_1:
				if(ch_in != '-')
				{ throw std::runtime_error{std::format("Invalid option {}", val)}; }
				current_state = state::init_2;
				break;

			case state::init_2:
				if(ch_in != '-')
				{ throw std::runtime_error{std::format("Invalid option {}", val)}; }
				current_state = state::key_1;
				break;

			case state::key_1:
				switch(ch_in)
				{
					case '-':
						throw std::runtime_error{std::format("Invalid option {}", val)};
					case '=':
						throw std::runtime_error{std::format("Invalid option {}", val)};
					default:
						ret.name += ch_in;
						current_state = state::key_2;
				}
				break;

			case state::key_2:
				if(ch_in == '=')
				{
					current_state = state::value;
					ret.value.push_back(std::string{});
				}
				else
				{ ret.name += ch_in; }
				break;

			case state::value:
				switch(ch_in)
				{
					case '\\':
						current_state = state::value_escape;
						break;
					case ',':
						ret.value.push_back(std::string{});
						break;
					default:
						ret.value.back() += ch_in;
				}
				break;

			case state::value_escape:
				ret.value.back() += ch_in;
				current_state = state::value;
				break;
		}
		++val;
	}

	return ret;
}

slideproj::utils::parsed_command_line::parsed_command_line(int argc, char const* const* argv)
{
	auto current_arg = 1;
	if(current_arg >= argc)
	{ throw std::runtime_error{"Bad command line, try slideproj help"}; }

	m_action = argv[current_arg];
	++current_arg;

	for(; current_arg < argc; ++current_arg)
	{
		auto parsed_arg = parse_arg(argv[current_arg]);
		if(parsed_arg.name.empty())
		{ continue; }

		// Intentionally create or update
		auto& item = m_args[std::move(parsed_arg.name)];
		item.insert(
			std::end(item),
			std::make_move_iterator(std::begin(parsed_arg.value)),
			std::make_move_iterator(std::end(parsed_arg.value))
		);
	}
}