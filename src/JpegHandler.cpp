/*
 * Copyright (C) 2009 Ionut Dediu <deionut@yahoo.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// JpegHandler.cpp

#include <cstdio>
#include <cstdlib>
#include <cmath>

#include "JpegHandler.h"

void CJpegHandler::init_source(j_decompress_ptr cinfo)
{
}

boolean CJpegHandler::fill_input_buffer(j_decompress_ptr cinfo)
{
    ERREXIT(cinfo, JERR_FILE_READ);
    return TRUE;
}

void CJpegHandler::skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
    cinfo->src->bytes_in_buffer -= num_bytes;
    cinfo->src->next_input_byte += num_bytes;
}

void CJpegHandler::term_source(j_decompress_ptr cinfo)
{
}

CJpegHandler::CJpegHandler()
{
    srcmgr.init_source = init_source;
    srcmgr.fill_input_buffer = fill_input_buffer;
    srcmgr.skip_input_data = skip_input_data;
    srcmgr.resync_to_restart = jpeg_resync_to_restart;
    srcmgr.term_source = term_source;

    cinfo.client_data = (void*) this;

    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = error_exit;
    jerr.output_message = output_message;

    jpeg_create_decompress(&cinfo);
    cinfo.src = &srcmgr;

    rgbBuffer = NULL;
    rgbBufferSize = 0;
}

CJpegHandler::~CJpegHandler()
{
    jpeg_destroy_decompress(&cinfo);
    free(rgbBuffer);
}

bool CJpegHandler::decodeHeader(const unsigned char* buffer, int size)
{
    if (setjmp(returnpoint)) {
        printf("Error: %s\n", messagebuffer);
        return false;
    }
    srcmgr.bytes_in_buffer = size;
    srcmgr.next_input_byte = buffer;

    jpeg_read_header(&cinfo, FALSE);
    return true;
}

unsigned char* CJpegHandler::decodeRGB24(const unsigned char* buffer, int size, int &width, int &height)
{
    if (setjmp(returnpoint)) {
        printf("Error: %s\n", messagebuffer);
        return NULL;
    }
    srcmgr.bytes_in_buffer = size;
    srcmgr.next_input_byte = buffer;

    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    width = cinfo.output_width;
    height = cinfo.output_height;

    if (rgbBuffer == NULL || rgbBufferSize < 3 * width * height) {
        rgbBufferSize = 3 * width * height;
        free(rgbBuffer);
        rgbBuffer = (unsigned char*) malloc(rgbBufferSize);
    }

    while (cinfo.output_scanline < cinfo.output_height)
    {
        unsigned char* crtRGBRow = rgbBuffer + 3 * cinfo.image_width * cinfo.output_scanline;
        jpeg_read_scanlines(&cinfo, (JSAMPARRAY) &crtRGBRow, 1);
    }
    jpeg_finish_decompress(&cinfo);
    return rgbBuffer;
}

void CJpegHandler::error_exit(j_common_ptr cinfo)
{
    CJpegHandler* data = (CJpegHandler*) cinfo->client_data;
    cinfo->err->format_message(cinfo, data->messagebuffer);
    longjmp(data->returnpoint, 1);
}

void CJpegHandler::output_message(j_common_ptr cinfo)
{
    printf("Outputting message:\n");
    char buf[JMSG_LENGTH_MAX + 1];
    cinfo->err->format_message(cinfo, buf);
    printf("%s\n", buf);
}
