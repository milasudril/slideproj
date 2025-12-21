#ifndef SLIDEPROJ_PIXEL_STORE_PIXEL_TYPES_HPP
#define SLIDEPROJ_PIXEL_STORE_PIXEL_TYPES_HPP

#include "src/utils/numconv.hpp"

#include <cmath>
#include <array>

namespace slideproj::pixel_store
{
	struct linear_intensity_mapping
	{
		template<class T>
		static constexpr float to_linear_float(T value)
		{ return utils::to_normalized_float(value); }
	};

	template<class T>
	consteval auto make_srgb_to_lin_lut()
	{
		std::array<float, 256> ret{};
		for(size_t k = 0; k != std::size(ret); ++k)
		{
			auto const value = static_cast<T>(k);
			auto const val = utils::to_normalized_float(value);
			ret[k] = (val <= 0.04045f)? val/12.92f : std::pow((val + 0.055f)/1.055f, 2.4f);
		}

		return ret;
	}

	struct srgb_intensity_mapping
	{
		template<class T>
		static constexpr float to_linear_float(T value)
		{
			if constexpr(std::is_same_v<T, uint8_t>)
			{
				static constexpr auto srgb_lut = make_srgb_to_lin_lut<T>();
				return srgb_lut[value];
			}
			else
			{
				auto const val = utils::to_normalized_float(value);
				return (val <= 0.04045f)? val/12.92f : std::pow((val + 0.055f)/1.055f, 2.4f);
			}
		}
	};

	struct g22_intensity_mapping
	{
		template<class T>
		static constexpr float to_linear_float(T value)
		{
			auto const val = utils::to_normalized_float(value);
			return std::pow(val, 2.2f);
		}
	};

	template<class SampleType, size_t ChannelCount>
	struct pixel_type
	{};

	template<class SampleType>
	struct pixel_type<SampleType, 1>
	{
		using sample_type = SampleType;

		SampleType gray;
		static constexpr auto channel_count = 1;

		constexpr auto to_linear_float() const
		{
			return pixel_type<float, 1>{
				.gray = gray.to_linear_float()
			};
		}

		constexpr auto& operator+=(pixel_type<SampleType, 1> const& other)
		{
			gray += other.gray;
			return *this;
		}

		constexpr auto& operator/=(float factor)
		{
			gray /= factor;
			return *this;
		}

		constexpr auto to_rgba() const
		{
			return pixel_type<SampleType, 4>{
				.red = gray,
				.green = gray,
				.blue = gray,
				// TODO: Deduce max value from SampleType
				.alpha = 1.0f
			};
		}
	};

	template<class SampleType>
	struct pixel_type<SampleType, 2>
	{
		using sample_type = SampleType;

		SampleType gray;
		SampleType alpha;
		static constexpr auto channel_count = 2;

		constexpr auto to_linear_float() const
		{
			return pixel_type<float, 2>{
				.gray = gray.to_linear_float(),
				.alpha = alpha.to_normalized_float()
			};
		}

		constexpr auto& operator+=(pixel_type<SampleType, 2> const& other)
		{
			gray += other.gray;
			alpha += other.alpha;
			return *this;
		}

		constexpr auto& operator/=(float factor)
		{
			gray /= factor;
			alpha /= factor;
			return *this;
		}

		constexpr auto to_rgba() const
		{
			return pixel_type<SampleType, 4>{
				.red = gray,
				.green = gray,
				.blue = gray,
				.alpha = alpha
			};
		}
	};

	template<class SampleType>
	struct pixel_type<SampleType, 3>
	{
		using sample_type = SampleType;

		SampleType red;
		SampleType green;
		SampleType blue;

		static constexpr auto channel_count = 3;

		constexpr auto to_linear_float() const
		{
			return pixel_type<float, 3>{
				.red = red.to_linear_float(),
				.green = green.to_linear_float(),
				.blue = blue.to_linear_float()
			};
		}

		constexpr auto& operator+=(pixel_type<SampleType, 3> const& other)
		{
			red += other.red;
			green += other.green;
			blue += other.blue;
			return *this;
		}

		constexpr auto& operator/=(float factor)
		{
			red /= factor;
			green /= factor;
			blue /= factor;
			return *this;
		}

		constexpr auto to_rgba() const
		{
			return pixel_type<SampleType, 4>{
				.red = red,
				.green = green,
				.blue = blue,
				// TODO: Deduce max value from SampleType
				.alpha = 1.0f
			};
		}
	};

	template<class SampleType>
	struct pixel_type<SampleType, 4>
	{
		using sample_type = SampleType;

		SampleType red;
		SampleType green;
		SampleType blue;
		SampleType alpha;

		static constexpr auto channel_count = 4;

		constexpr auto to_linear_float() const
		{
			return pixel_type<float, 4>{
				.red = red.to_linear_float(),
				.green = green.to_linear_float(),
				.blue = blue.to_linear_float(),
				.alpha = alpha.to_normalized_float()
			};
		}

		constexpr auto& operator+=(pixel_type<SampleType, 4> const& other)
		{
			red += other.red;
			green += other.green;
			blue += other.blue;
			alpha += other.alpha;
			return *this;
		}

		constexpr auto& operator/=(float factor)
		{
			red /= factor;
			green /= factor;
			blue /= factor;
			alpha /= factor;
			return *this;
		}

		constexpr auto to_rgba() const
		{
			return pixel_type<SampleType, 4>{
				.red = red,
				.green = green,
				.blue = blue,
				.alpha = alpha
			};
		}
	};
}
#endif