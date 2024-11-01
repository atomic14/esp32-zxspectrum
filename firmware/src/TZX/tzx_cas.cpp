#include "tzx_cas.h"
#include <string.h>
#include <cmath>

int compute_log2(int val)
{
	int count = 0;

	while(val > 1)
	{
		if (val & 1)
			return -1;
		val /= 2;
		count++;
	}
	return (val == 0) ? -1 : count;
}

static const uint8_t TZX_HEADER[8] = { 'Z','X','T','a','p','e','!',0x1a };

void TzxCas::TzxCas::toggle_wave_data(TapeListener *tapeListener)
{
	tapeListener->toggleMicLevel();
}

void TzxCas::TzxCas::tzx_cas_get_blocks(const uint8_t *casdata, int caslen)
{
	int pos = sizeof(TZX_HEADER) + 2;
	int max_block_count = INITIAL_MAX_BLOCK_COUNT;
	int loopcount = 0, loopoffset = 0;
	blocks = (uint8_t**)malloc(max_block_count * sizeof(uint8_t*));
	memset(blocks,0,max_block_count);
	block_count = 0;

	while (pos < caslen)
	{
		uint32_t datasize;
		uint8_t blocktype = casdata[pos];

		if (block_count == max_block_count)
		{
			void *old_blocks = blocks;
			int old_max_block_count = max_block_count;
			max_block_count = max_block_count + BLOCK_COUNT_INCREMENTS;
			blocks = (uint8_t**)malloc(max_block_count * sizeof(uint8_t*));
			memset(blocks, 0, max_block_count);
			memcpy(blocks, old_blocks, old_max_block_count * sizeof(uint8_t*));
			free(old_blocks);
		}

		blocks[block_count] = (uint8_t*)&casdata[pos];

		pos += 1;

		switch (blocktype)
		{
		case 0x10:
			pos += 2;
			datasize = get_u16le(&casdata[pos]);
			pos += 2 + datasize;
			break;
		case 0x11:
			pos += 0x0f;
			datasize = get_u24le(&casdata[pos]);
			pos += 3 + datasize;
			break;
		case 0x12:
			pos += 4;
			break;
		case 0x13:
			datasize = casdata[pos];
			pos += 1 + 2 * datasize;
			break;
		case 0x14:
			pos += 7;
			datasize = get_u24le(&casdata[pos]);
			pos += 3 + datasize;
			break;
		case 0x15:
			pos += 5;
			datasize = get_u24le(&casdata[pos]);
			pos += 3 + datasize;
			break;
		case 0x20: case 0x23:
			pos += 2;
			break;

		case 0x24:
			loopcount = get_u16le(&casdata[pos]);
			pos +=2;
			loopoffset = pos;
			break;

		case 0x21: case 0x30:
			datasize = casdata[pos];
			pos += 1 + datasize;
			break;
		case 0x22: case 0x27:
			break;

		case 0x25:
			if (loopcount>0)
			{
				pos = loopoffset;
				loopcount--;
			}
			break;

		case 0x26:
			datasize = get_u16le(&casdata[pos]);
			pos += 2 + 2 * datasize;
			break;
		case 0x28: case 0x32:
			datasize = get_u16le(&casdata[pos]);
			pos += 2 + datasize;
			break;
		case 0x31:
			pos += 1;
			datasize = casdata[pos];
			pos += 1 + datasize;
			break;
		case 0x33:
			datasize = casdata[pos];
			pos += 1 + 3 * datasize;
			break;
		case 0x34:
			pos += 8;
			break;
		case 0x35:
			pos += 0x10;
			datasize = get_u32le(&casdata[pos]);
			pos += 4 + datasize;
			break;
		case 0x40:
			pos += 1;
			datasize = get_u24le(&casdata[pos]);
			pos += 3 + datasize;
			break;
		case 0x5A:
			pos += 9;
			break;
		default:
			datasize = get_u32le(&casdata[pos]);
			pos += 4 + datasize;
			break;
		}

		block_count++;
	}
}

void TzxCas::tzx_cas_handle_block(TapeListener *tapeListener, const uint8_t *bytes, int pause, int data_size, int pilot, int pilot_length, int sync1, int sync2, int bit0, int bit1, int bits_in_last_byte)
{
	/* PILOT */
	for ( ; pilot_length > 0; pilot_length--)
	{
		tapeListener->runForTicks(pilot);
		toggle_wave_data(tapeListener);
	}
	/* SYNC1 */
	if (sync1 > 0)
	{
		tapeListener->runForTicks(sync1);
		toggle_wave_data(tapeListener);
	}
	/* SYNC2 */
	if (sync2 > 0)
	{
		tapeListener->runForTicks(sync2);
		toggle_wave_data(tapeListener);
	}
	/* data */
	for (int data_index = 0; data_index < data_size; data_index++)
	{
		uint8_t byte = bytes[data_index];
		int bits_to_go = (data_index == (data_size - 1)) ? bits_in_last_byte : 8;

		for ( ; bits_to_go > 0; byte <<= 1, bits_to_go--)
		{
			int bit_length = (byte & 0x80) ? bit1 : bit0;
			tapeListener->runForTicks(bit_length);
			toggle_wave_data(tapeListener);
			tapeListener->runForTicks(bit_length);
			toggle_wave_data(tapeListener);
		}
	}
	/* pause */
	if (pause > 0)
	{
		tapeListener->pause1Millis();
		tapeListener->setMicLow();
		for(int i = 0; i < pause - 1; i++) {
			tapeListener->pause1Millis();
		}
	}
}

void TzxCas::tzx_handle_direct(TapeListener *tapeListener, const uint8_t *bytes, int pause, int data_size, int tstates, int bits_in_last_byte)
{
	/* data */
	for (int data_index = 0; data_index < data_size; data_index++)
	{
		uint8_t byte = bytes[data_index];
		int bits_to_go = (data_index == (data_size - 1)) ? bits_in_last_byte : 8;

		for ( ; bits_to_go > 0; byte <<= 1, bits_to_go--)
		{
			if (byte & 0x80) {
				tapeListener->setMicHigh();
			}
			else {
				tapeListener->setMicLow();
			}
			tapeListener->runForTicks(tstates);
		}
	}

	/* pause */
	if (pause > 0)
	{
		tapeListener->pause1Millis();
		tapeListener->setMicLow();
		for(int i = 0; i < pause - 1; i++) {
			tapeListener->pause1Millis();
		}
	}
}

void TzxCas::tzx_handle_generalized(TapeListener *tapeListener, const uint8_t *bytes, int pause, uint32_t totp, int npp, int asp, uint32_t totd, int npd, int asd)
{
	if (totp > 0)
	{
	//  printf("pilot block table %04x\n", totp);

		const uint8_t *symtable = bytes;
		const uint8_t *table2 = symtable + (2 * npp + 1)*asp;

		// the Pilot and sync data stream has an RLE encoding
		for (uint32_t i = 0; i < totp*3; i+=3)
		{
			uint8_t symbol = table2[i + 0];
			uint16_t repetitions = get_u16le(&table2[i + 1]);
			//printf("symbol %02x repetitions %04x\n", symbol, repetitions); // does 1 mean repeat once, or that it only occurs once?

			for (int j = 0; j < repetitions; j++)
			{
				tzx_handle_symbol(tapeListener, symtable, symbol, npp);
			}
		}

		// advance to after this data
		bytes += ((2 * npp + 1)*asp) + totp * 3;
	}

	if (totd > 0)
	{
	//  printf("data block table %04x (has %0d symbols, max symbol length is %d)\n", totd, asd, npd);

		const uint8_t *symtable = bytes;
		const uint8_t *table2 = bytes + (2 * npd + 1)*asd;

		int NB = std::ceil(compute_log2(asd)); // number of bits needed to represent each symbol

		uint8_t stream_bit = 0;
		uint32_t stream_byte = 0;

		for (uint32_t i = 0; i < totd; i++)
		{
			uint8_t symbol = 0;

			for (int j = 0; j < NB; j++)
			{
				symbol |= stream_get_bit(table2, stream_bit, stream_byte) << j;
			}
			tzx_handle_symbol(tapeListener, symtable, symbol, npd);
		}
	}

	/* pause */
	if (pause > 0)
	{
		tapeListener->pause1Millis();
		tapeListener->setMicLow();
		for(int i = 0; i < pause - 1; i++) {
			tapeListener->pause1Millis();
		}
	}
}



void TzxCas::ascii_block_common_log( const char *block_type_string, uint8_t block_type)
{
	printf("%s (type %02x) encountered:\n", block_type_string, block_type);
}

const char *const archive_ident[] =
{
	"Full title",
	"Software house/publisher",
	"Author(s)",
	"Year of publication",
	"Language",
	"Game/utility type",
	"Price",
	"Protection scheme/loader",
	"Origin",
};

const char *const hw_info[] =
{
	"Tape runs on this machine / this hardware",
	"Tape needs this machine / this hardware",
	"Tape runs on this machine / this hardware, but does not require its special features",
	"Tape does not run on this machine / this hardware",
};

void TzxCas::tzx_cas_do_work(TapeListener *tapeListener)
{
	int current_block = 0;

	tapeListener->setMicLow();

	int loopcount = 0, loopoffset = 0;

	while (current_block < block_count)
	{
		int pause_time;
		uint32_t data_size;
		uint32_t text_size, total_size, i;
		int pilot, pilot_length, sync1, sync2;
		int bit0, bit1, bits_in_last_byte;
		uint8_t *cur_block = blocks[current_block];
		uint8_t block_type = cur_block[0];
		uint16_t tstates = 0;

	/* Uncomment this to include into error.log a list of the types each block */
	printf("tzx_cas_fill_wave: block %d, block_type %02x\n", current_block, block_type);

		switch (block_type)
		{
		case 0x10:  /* Standard Speed Data Block (.TAP block) */
			pause_time = get_u16le(&cur_block[1]);
			data_size = get_u16le(&cur_block[3]);
			pilot_length = (cur_block[5] < 128) ?  8063 : 3223;
			tzx_cas_handle_block(tapeListener, &cur_block[5], pause_time, data_size, 2168, pilot_length, 667, 735, 855, 1710, 8);
			current_block++;
			break;
		case 0x11:  /* Turbo Loading Data Block */
			pilot = get_u16le(&cur_block[1]);
			sync1 = get_u16le(&cur_block[3]);
			sync2 = get_u16le(&cur_block[5]);
			bit0 = get_u16le(&cur_block[7]);
			bit1 = get_u16le(&cur_block[9]);
			pilot_length = get_u16le(&cur_block[11]);
			bits_in_last_byte = cur_block[13];
			pause_time = get_u16le(&cur_block[14]);
			data_size = get_u24le(&cur_block[16]);
			tzx_cas_handle_block(tapeListener, &cur_block[19], pause_time, data_size, pilot, pilot_length, sync1, sync2, bit0, bit1, bits_in_last_byte);
			current_block++;
			break;
		case 0x12:  /* Pure Tone */
			pilot = get_u16le(&cur_block[1]);
			pilot_length = get_u16le(&cur_block[3]);
			tzx_cas_handle_block(tapeListener, cur_block, 0, 0, pilot, pilot_length, 0, 0, 0, 0, 0);
			current_block++;
			break;
		case 0x13:  /* Sequence of Pulses of Different Lengths */
			for (data_size = 0; data_size < cur_block[1]; data_size++)
			{
				pilot = cur_block[2 + 2 * data_size] + (cur_block[3 + 2 * data_size] << 8);
				tzx_cas_handle_block(tapeListener, cur_block, 0, 0, pilot, 1, 0, 0, 0, 0, 0);
			}
			current_block++;
			break;
		case 0x14:  /* Pure Data Block */
			bit0 = get_u16le(&cur_block[1]);
			bit1 = get_u16le(&cur_block[3]);
			bits_in_last_byte = cur_block[5];
			pause_time = get_u16le(&cur_block[6]);
			data_size = get_u24le(&cur_block[8]);
			tzx_cas_handle_block(tapeListener, &cur_block[11], pause_time, data_size, 0, 0, 0, 0, bit0, bit1, bits_in_last_byte);
			current_block++;
			break;
		case 0x20:  /* Pause (Silence) or 'Stop the Tape' Command */
			pause_time = get_u16le(&cur_block[1]);
			if (pause_time == 0)
			{
				/* pause = 0 is used to let an emulator automagically stop the tape
				   in MAME we do not do that, so we insert a 5 second pause. */
				pause_time = 5000;
			}
			tzx_cas_handle_block(tapeListener, cur_block, pause_time, 0, 0, 0, 0, 0, 0, 0, 0);
			current_block++;
			break;
		case 0x16:  /* C64 ROM Type Data Block */       // Deprecated in TZX 1.20
		case 0x17:  /* C64 Turbo Tape Data Block */     // Deprecated in TZX 1.20
		case 0x34:  /* Emulation Info */                // Deprecated in TZX 1.20
		case 0x40:  /* Snapshot Block */                // Deprecated in TZX 1.20
			printf("Deprecated block type (%02x) encountered.\n", block_type);
			printf("Please look for an updated .tzx file.\n");
			current_block++;
			break;
		case 0x30:  /* Text Description */
			ascii_block_common_log("Text Description Block", block_type);
			for (data_size = 0; data_size < cur_block[1]; data_size++)
				printf("%c", cur_block[2 + data_size]);
			printf("\n");
			current_block++;
			break;
		case 0x31:  /* Message Block */
			ascii_block_common_log("Message Block", block_type);
			printf("Expected duration of the message display: %02x\n", cur_block[1]);
			printf("Message: \n");
			for (data_size = 0; data_size < cur_block[2]; data_size++)
			{
				printf("%c", cur_block[3 + data_size]);
				if (cur_block[3 + data_size] == 0x0d)
					printf("\n");
			}
			printf("\n");
			current_block++;
			break;
		case 0x32:  /* Archive Info */
			ascii_block_common_log("Archive Info Block", block_type);
			total_size = get_u16le(&cur_block[1]);
			text_size = 0;
			for (data_size = 0; data_size < cur_block[3]; data_size++)  // data_size = number of text blocks, in this case
			{
				if (cur_block[4 + text_size] < 0x09) {
					printf("%s: \n", archive_ident[cur_block[4 + text_size]]);
				}
				else {
					printf("Comment(s): \n");
				}

				for (i = 0; i < cur_block[4 + text_size + 1]; i++)
				{
					printf("%c", cur_block[4 + text_size + 2 + i]);
				}
				text_size += 2 + i;
			}
			printf("\n");
			if (text_size != total_size)
				printf("Malformed Archive Info Block (Text length different from the declared one).\n Please verify your tape image.\n");
			current_block++;
			break;
		case 0x33:  /* Hardware Type */
			ascii_block_common_log("Hardware Type Block", block_type);
			for (data_size = 0; data_size < cur_block[1]; data_size++)  // data_size = number of hardware blocks, in this case
			{
				printf("Hardware Type %02x - Hardware ID %02x - ", cur_block[2 + data_size * 3], cur_block[2 + data_size * 3 + 1]);
				printf("%s \n ", hw_info[cur_block[2 + data_size * 3 + 2]]);
			}
			current_block++;
			break;
		case 0x35:  /* Custom Info Block */
			ascii_block_common_log("Custom Info Block", block_type);
			for (data_size = 0; data_size < 10; data_size++)
			{
				printf("%c", cur_block[1 + data_size]);
			}
			printf(":\n");
			text_size = get_u32le(&cur_block[11]);
			for (data_size = 0; data_size < text_size; data_size++)
				printf("%c", cur_block[15 + data_size]);
			printf("\n");
			current_block++;
			break;
		case 0x5A:  /* "Glue" Block */
			printf("Glue Block (type %02x) encountered.\n", block_type);
			printf("Please use a .tzx handling utility to split the merged tape files.\n");
			current_block++;
			break;
		case 0x24:  /* Loop Start */
			loopcount = get_u16le(&cur_block[1]);
			current_block++;
			loopoffset = current_block;

			printf("loop start %d %d\n",  loopcount, current_block);
			break;
		case 0x25:  /* Loop End */
			if (loopcount>0)
			{
				current_block = loopoffset;
				loopcount--;
				printf("do loop\n");
			}
			else
			{
				current_block++;
			}
			break;

		case 0x21:  /* Group Start */
		case 0x22:  /* Group End */
		case 0x23:  /* Jump To Block */
		case 0x26:  /* Call Sequence */
		case 0x27:  /* Return From Sequence */
		case 0x28:  /* Select Block */
		case 0x2A:  /* Stop Tape if in 48K Mode */
		case 0x2B:  /* Set signal level */
		default:
			printf("Unsupported block type (%02x) encountered.\n", block_type);
			current_block++;
			break;

		case 0x15:  /* Direct Recording */ // used on 'bombscar' in the cpc_cass list
			// having this missing is fatal
			tstates = get_u16le(&cur_block[1]);
			pause_time = get_u16le(&cur_block[3]);
			bits_in_last_byte = cur_block[5];
			data_size = get_u24le(&cur_block[6]);
			tzx_handle_direct(tapeListener, &cur_block[9], pause_time, data_size, tstates, bits_in_last_byte);
			current_block++;
			break;

		case 0x18:  /* CSW Recording */
			// having this missing is fatal
			printf("Unsupported block type (0x15 - CSW Recording) encountered.\n");
			current_block++;
			break;

		case 0x19:  /* Generalized Data Block */
			{
				// having this missing is fatal
				// used crudely by batmanc in spectrum_cass list (which is just a redundant encoding of batmane ?)
				data_size = get_u32le(&cur_block[1]);
				pause_time = get_u16le(&cur_block[5]);

				uint32_t totp = get_u32le(&cur_block[7]);
				int npp = cur_block[11];
				int asp = cur_block[12];
				if (asp == 0 && totp > 0) asp = 256;

				uint32_t totd = get_u32le(&cur_block[13]);
				int npd = cur_block[17];
				int asd = cur_block[18];
				if (asd == 0 && totd > 0) asd = 256;

				tzx_handle_generalized(tapeListener, &cur_block[19], pause_time, totp, npp, asp, totd, npd, asd);

				current_block++;
			}
			break;

		}
	}
	// Adding 1 ms. pause to ensure that the last edge is properly finished at the end of tape
	tapeListener->pause1Millis();
}

bool TzxCas::load_tzx(TapeListener *listener, uint8_t *casdata, int caslen)
{
	/* Header size plus major and minor version number */
	if (caslen < 10)
	{
		printf("tzx_cas_to_wav_size: cassette image too small\n");
		return false;
	}

	/* Check for correct header */
	if (memcmp(casdata, TZX_HEADER, sizeof(TZX_HEADER)))
	{
		printf("tzx_cas_to_wav_size: cassette image has incompatible header\n");
		return false;
	}

	/* Check major version number in header */
	if (casdata[0x08] > SUPPORTED_VERSION_MAJOR)
	{
		printf("tzx_cas_to_wav_size: unsupported version\n");
		return false;
	}
	tzx_cas_get_blocks(casdata, caslen);
	printf("tzx_cas_to_wav_size: %d blocks found\n", block_count);
	if (block_count == 0)
	{
		printf("tzx_cas_to_wav_size: no blocks found!\n");
		return false;
	}
	tzx_cas_do_work(listener);
	return true;
}

bool TzxCas::load_tap(TapeListener *tapeListener, uint8_t *casdata, int caslen)
{
	const uint8_t *p = casdata;

	while (p < casdata + caslen)
	{
		int data_size = get_u16le(&p[0]);
		int pilot_length = (p[2] == 0x00) ? 8063 : 3223;
		printf("tap_cas_fill_wave: Handling TAP block containing 0x%X bytes\n", data_size);
		p += 2;
		tzx_cas_handle_block(tapeListener, p, 1000, data_size, 2168, pilot_length, 667, 735, 855, 1710, 8);
		p += data_size;
	}
	return true;
}

