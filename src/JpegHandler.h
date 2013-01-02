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

// JpegHandler.h

#ifndef __JPEG_HANDLER_H__
#define __JPEG_HANDLER_H__

extern "C" {
#include "jpeglib.h"
#include "jerror.h"
}
#include <setjmp.h>

class CJpegHandler
{
public:
    CJpegHandler();
    ~CJpegHandler();

    bool decodeHeader(const unsigned char* buffer, int size);

    unsigned char* decodeRGB24(const unsigned char* buffer, int size, int &width, int &height);

private:
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    struct jpeg_source_mgr srcmgr;

    jmp_buf returnpoint;
    char messagebuffer[JMSG_LENGTH_MAX + 1];

    unsigned char* rgbBuffer;
    int rgbBufferSize;

    static void error_exit(j_common_ptr cinfo);
    static void output_message(j_common_ptr cinfo);

    static void init_source(j_decompress_ptr cinfo);
    static boolean fill_input_buffer(j_decompress_ptr cinfo);
    static void skip_input_data(j_decompress_ptr cinof, long num_bytes);
    static void term_source(j_decompress_ptr cinfo);
};

#endif//__JPEG_HANDLER_H__
