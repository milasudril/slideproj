#ifndef SLIDEPROJ_EVENT_TYPES_WINDOWING_EVENTS_HPP
#define SLIDEPROJ_EVENT_TYPES_WINDOWING_EVENTS_HPP

#include "src/utils/utils.hpp"

namespace slideproj::event_types
{
	struct frame_buffer_size_changed_event
	{
		int width;
		int height;
	};

	struct window_is_closing_event
	{};

	class typing_keyboard_scancode
	{
	public:
		constexpr typing_keyboard_scancode(int value): m_value{value}
		{}

		constexpr int value() const
		{ return m_value; }

		constexpr bool operator==(typing_keyboard_scancode const&) const = default;
		constexpr bool operator!=(typing_keyboard_scancode const&) const = default;

	private:
		int m_value;
	};

	enum class button_action
	{
		press,
		release,
		repeat
	};

	enum class typing_keyboard_modifier_mask
	{
		none = 0x0,
		shift = 0x1,
		ctrl = 0x2,
		alt = 0x4,
		super = 0x8,
		caps_lock = 0x10,
		num_lock = 0x20
	};

	consteval void enable_bitmask_operators(typing_keyboard_modifier_mask);

	struct typing_keyboard_event
	{
		typing_keyboard_scancode scancode;
		button_action action;
		typing_keyboard_modifier_mask modifiers;
	};
}

#endif