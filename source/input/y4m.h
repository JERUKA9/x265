/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

#ifndef X265_Y4M_H
#define X265_Y4M_H

#include "input.h"
#include <fstream>

namespace x265 {
// x265 private namespace

class Y4MInput : public Input
{
protected:

    int rateNum;

    int rateDenom;

    int width;

    int height;

    char* buf;

    std::ifstream ifs;

    void parseHeader();

public:

    Y4MInput(const char *filename);

    virtual ~Y4MInput();

    void setDimensions(int, int)                  { /* ignore, warn */ }

    void setBitDepth(int)                         { /* ignore, warn */ }

    float getRate() const                         { return ((float)rateNum) / rateDenom; }

    int getWidth() const                          { return width; }

    int getHeight() const                         { return height; }

    bool isEof() const                            { return ifs.eof(); }

    bool isFail()                                 { return !ifs.is_open(); }

    void release()                                { delete this; }

    int  guessFrameCount();

    void skipFrames(int numFrames);

    bool readPicture(x265_picture_t&);

    const char *getName() const                   { return "y4m"; }
};
}

#endif // ifndef X265_Y4M_H
