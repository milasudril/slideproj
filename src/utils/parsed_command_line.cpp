//@	{"target":{"name":"./parsed_command_line.o"}}

#include "./parsed_command_line.hpp"

#include <stdexcept>
#include <format>
#include <cstring>

slideproj::utils::parsed_arg slideproj::utils::parse_arg(
	char const* val,
	string_lookup_table<std::string> const& valid_options
)
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
					if(!valid_options.contains(ret.name))
					{ throw std::runtime_error{std::format("Unsuppoted option {}", ret.name)}; }

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

	if(!valid_options.contains(ret.name))
	{ throw std::runtime_error{std::format("Unsuppoted option {}", ret.name)}; }

	return ret;
}

slideproj::utils::parsed_command_line::parsed_command_line(
	char const* appname,
	std::span<char const* const> argv,
	string_lookup_table<action_info> const& valid_actions
)
{
	size_t current_arg = 1;
	if(current_arg >= std::size(argv))
	{ throw std::runtime_error{std::format("Bad command line, try {} help", appname), }; }

	if(strcmp(argv[current_arg], "help") == 0)
	{
		m_action = action_info{
			.main = [](string_lookup_table<std::vector<std::string>> const&){
				printf("Show some help\n");
				return 0;
			},
			.valid_options = {}
		};
	}
	else
	{
		auto const i = valid_actions.find(argv[current_arg]);
		if(i == valid_actions.end())
		{
			throw std::runtime_error{
				std::format("Unsupported action {}, try {} help", argv[current_arg], appname)
			};
		}
		m_action = i->second;
	}
	++current_arg;

	for(; current_arg < std::size(argv); ++current_arg)
	{
		auto parsed_arg = parse_arg(argv[current_arg], m_action.valid_options);
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