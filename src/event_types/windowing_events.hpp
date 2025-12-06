#ifndef SLIDEPROJ_EVENT_TYPES_WINDOWING_EVENTS_HPP
#define SLIDEPROJ_EVENT_TYPES_WINDOWING_EVENTS_HPP

#include "src/utils/bitmask_enum.hpp"

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
		constexpr explicit typing_keyboard_scancode(int value): m_value{value}
		{}

		constexpr int value() const
		{ return m_value; }

		constexpr bool operator==(typing_keyboard_scancode const&) const = default;
		constexpr bool operator!=(typing_keyboard_scancode const&) const = default;

		static const typing_keyboard_scancode arrow_up;
		static const typing_keyboard_scancode arrow_down;
		static const typing_keyboard_scancode arrow_left;
		static const typing_keyboard_scancode arrow_right;
		static const typing_keyboard_scancode f_11;

	private:
		int m_value;
	};

	inline constexpr typing_keyboard_scancode typing_keyboard_scancode::arrow_up{103};
	inline constexpr typing_keyboard_scancode typing_keyboard_scancode::arrow_down{108};
	inline constexpr typing_keyboard_scancode typing_keyboard_scancode::arrow_left{105};
	inline constexpr typing_keyboard_scancode typing_keyboard_scancode::arrow_right{106};
	inline constexpr typing_keyboard_scancode typing_keyboard_scancode::f_11{87};

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

	class mouse_button_index
	{
	public:
		constexpr explicit mouse_button_index(int value): m_value{value}
		{}

		constexpr int value() const
		{ return m_value; }

		constexpr bool operator==(mouse_button_index const&) const = default;
		constexpr bool operator!=(mouse_button_index const&) const = default;

		static const mouse_button_index left;
		static const mouse_button_index right;

	private:
		int m_value;
	};

	inline constexpr mouse_button_index mouse_button_index::left{0};
	inline constexpr mouse_button_index mouse_button_index::right{1};

	struct mouse_button_event
	{
		mouse_button_index button;
		button_action action;
		typing_keyboard_modifier_mask modifiers;
	};
}

#endif