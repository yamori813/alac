/*
 *  alac_wrapper.c - ALAC decoder wrapper
 *
 *  (c) Philippe, philippe_44@outlook.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdlib.h>

#include "ALACDecoder.h"
#include "ALACBitUtilities.h"
#include "alac_wrapper.h"

typedef struct alac_codec_s {
	ALACDecoder *Decoder;
} alac_codec_t;

/*----------------------------------------------------------------------------*/
extern "C" struct alac_codec_s *alac_create(int magic_cookie_size, unsigned char *magic_cookie) {
	struct alac_codec_s *codec = (struct alac_codec_s*) malloc(sizeof(struct alac_codec_s));

	codec->Decoder = new ALACDecoder;
	codec->Decoder->Init(magic_cookie, magic_cookie_size);

	return codec;
}

/*----------------------------------------------------------------------------*/
extern "C" void alac_delete(struct alac_codec_s *codec) {
	delete (ALACDecoder*) codec->Decoder;
	free(codec);
}

/*----------------------------------------------------------------------------*/
extern "C" int alac_decode(struct alac_codec_s *codec, unsigned char* input,
    int input_size, unsigned char *output, int frames_per_packet,char channels, unsigned *out_frames) {
	BitBuffer input_buffer;

	BitBufferInit(&input_buffer, input, input_size);
	return codec->Decoder->Decode(&input_buffer, output, frames_per_packet, channels, out_frames) == ALAC_noErr;
}

