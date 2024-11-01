#pragma once

#include <stdint.h>
#include <array>
#include <cstddef>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

#include "TapeListener.h"

#define SUPPORTED_VERSION_MAJOR 0x01

#define INITIAL_MAX_BLOCK_COUNT 256
#define BLOCK_COUNT_INCREMENTS 256

enum class error
{
	SUCCESS,							 // no error
	INTERNAL,							 // fatal internal error
	UNSUPPORTED,					 // this operation is unsupported
	OUT_OF_MEMORY,				 // ran out of memory
	INVALID_IMAGE,				 // invalid image
	READ_WRITE_UNSUPPORTED // read/write is not supported by this image format
};

class TzxCas
{
	int block_count = 0;
	uint8_t **blocks = nullptr;

	constexpr uint16_t get_u16le(uint8_t const *buf) noexcept
	{
		return (uint16_t(buf[0]) << 0) | (uint16_t(buf[1]) << 8);
	}
	constexpr uint32_t get_u32le(uint8_t const *buf) noexcept
	{
		return (uint32_t(buf[0]) << 0) | (uint32_t(buf[1]) << 8) | (uint32_t(buf[2]) << 16) | (uint32_t(buf[3]) << 24);
	}
	constexpr int32_t get_s24le(uint8_t const *buf) noexcept
	{
		return sext(get_u24le(buf), 24);
	}
	constexpr uint32_t get_u24le(uint8_t const *buf) noexcept
	{
		return (uint32_t(buf[0]) << 0) | (uint32_t(buf[1]) << 8) | (uint32_t(buf[2]) << 16);
	}
	template <typename T, typename U>
	constexpr std::make_signed_t<T> sext(T value, U width) noexcept
	{
		return std::make_signed_t<T>(value << (8 * sizeof(value) - width)) >> (8 * sizeof(value) - width);
	}
	void toggle_wave_data(TapeListener *tapeListener);
	void tzx_cas_get_blocks(const uint8_t *casdata, int caslen);
	void tzx_cas_handle_block(TapeListener *tapeListener, const uint8_t *bytes, int pause, int data_size, int pilot, int pilot_length, int sync1, int sync2, int bit0, int bit1, int bits_in_last_byte);
	void tzx_handle_direct(TapeListener *tapeListener, const uint8_t *bytes, int pause, int data_size, int tstates, int bits_in_last_byte);
	inline void tzx_handle_symbol(TapeListener *tapeListener, const uint8_t *symtable, uint8_t symbol, int maxp)
	{
		const uint8_t *cursymb = symtable + (2 * maxp + 1) * symbol;

		uint8_t starttype = cursymb[0];

		switch (starttype)
		{
		case 0x00:
			// pulse level has already been toggled so don't change
			break;

		case 0x01:
			// pulse level has already been toggled so revert
			tapeListener->toggleMicLevel();
			break;

		case 0x02:
			// force low
			tapeListener->setMicLow();
			break;

		case 0x03:
			// force high
			tapeListener->setMicHigh();
			break;

		default:
			printf("SYMDEF invalid - bad starting polarity");
		};

		for (int i = 0; i < maxp; i++)
		{
			uint16_t pulse_length = get_u16le(&cursymb[1 + (i * 2)]);

			// shorter lists can be terminated with a pulse_length of 0
			if (pulse_length != 0)
			{
				tapeListener->runForTicks(pulse_length);
				tapeListener->toggleMicLevel();
			}
			else
			{
				break;
			}
		}
	};

	inline int stream_get_bit(const uint8_t *bytes, uint8_t &stream_bit, uint32_t &stream_byte)
	{
		// get bit here
		uint8_t retbit = 0;

		uint8_t byte = bytes[stream_byte];
		byte = byte << stream_bit;

		if (byte & 0x80)
			retbit = 1;

		stream_bit++;

		if (stream_bit == 8)
		{
			stream_bit = 0;
			stream_byte++;
		}

		return retbit;
	};
	void tzx_handle_generalized(TapeListener *tapeListener, const uint8_t *bytes, int pause, uint32_t totp, int npp, int asp, uint32_t totd, int npd, int asd);
	void ascii_block_common_log(const char *block_type_string, uint8_t block_type);
	void tzx_cas_do_work(TapeListener *tapeListener);
public:
	bool load_tzx(TapeListener *tapeListener, uint8_t *casdata, int caslen);
	bool load_tap(TapeListener *tapeListener, uint8_t *casdata, int caslen);
};