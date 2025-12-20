//@	{"target":{"name":"./parsed_command_line.o"}}

#include "./parsed_command_line.hpp"

#include <stdexcept>
#include <format>
#include <cstring>

slideproj::utils::parsed_arg slideproj::utils::parse_arg(
	char const* val,
	string_lookup_table<option_info> const& valid_options
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

	auto const optinfo = valid_options.find(ret.name);
	if(optinfo == std::end(valid_options))
	{ throw std::runtime_error{std::format("Unsupported option {}", ret.name)}; }
	if(ret.value.size() > optinfo->second.cardinality)
	{
		throw std::runtime_error{
			std::format(
				"The option option {} only takes {} value(s)",
				ret.name,
				optinfo->second.cardinality
			)
		};
	}

	if(!optinfo->second.valid_values.empty())
	{
		for(auto const& item : ret.value)
		{
			if(!optinfo->second.valid_values.contains(item))
			{
				throw std::runtime_error{
					std::format("The option {} does not accept the value {}", ret.name, item)
				};
			}
		}
	}

	return ret;
}

slideproj::utils::parsed_command_line::parsed_command_line(
	char const* appname,
	std::span<char const* const> argv,
	string_lookup_table<action_info>&& valid_actions
)
{
	size_t current_arg = 1;
	if(current_arg >= std::size(argv))
	{ throw std::runtime_error{std::format("Bad command line, try {} help", appname), }; }

	if(strcmp(argv[current_arg], "help") == 0)
	{
		m_action = action_info{
			.main = [
				appname = std::string{appname},
				valid_actions = std::move(valid_actions)
			](string_lookup_table<std::vector<std::string>> const&){
				printf("\n%s action [options specific to action]\n", appname.c_str());
				printf("\nSummary of valid actions\n\n");
				for(auto const& action :valid_actions)
				{
					printf("%s -- %s\n", action.first.c_str(), action.second.description.c_str());
				}
				printf("\n");
				for(auto const& action :valid_actions)
				{
					printf("\nDetails about `%s`\n\n", action.first.c_str());
					printf("Options:\n\n");
					for(auto const& option : action.second.valid_options)
					{
						printf("--%s, %s", option.first.c_str(), option.second.description.c_str());
						if(!option.second.valid_values.empty())
						{
							printf(". Valid values are ");
							auto i = option.second.valid_values.begin();
							printf("%s", i->c_str());
							++i;
							while(i != std::end(option.second.valid_values))
							{
								printf(", %s", i->c_str());
								++i;
							}
							printf(".");
						}
						printf("\n");
					}
					printf("\nDefault command:\n\n");
					printf("%s %s ", appname.c_str(), action.first.c_str());
					for(auto const& option : action.second.valid_options)
					{
						printf("--%s=", option.first.c_str());
						if(!option.second.default_value.empty())
						{
							size_t i = 0;
							auto const& vals = option.second.default_value;
							printf("%s", vals[0].c_str());
							++i;
							for(; i != std::size(vals); ++i)
							{ printf(",%s", vals[i].c_str()); }
						}
						printf(" ");
					}
					printf("\n\n");
				}
				return 0;
			},
			.description = "Shows command line help",
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
		m_action = std::move(i->second);
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