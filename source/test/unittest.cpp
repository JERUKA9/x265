/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Gopu Govindaswamy <gopu@govindaswamy.org>
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

#include "unittest.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>

using namespace x265;
using namespace std;

//Compare the Two given Buffers with Binary Mode
int UnitTest::CompareBuffer(unsigned char *Buff_one, unsigned char *Buff_two)
{
    int Buff_one_length;
    int Buff_two_length;

    Buff_one_length = strlen((char*)Buff_one);
    Buff_two_length = strlen((char*)Buff_two);

    //Check for both the buffer size is same or not
    if (Buff_one_length != Buff_two_length)
    {
        cout << "Both the Buffers are Not Same size" << endl;
        return WRONG_SIZE;
    }

    //Compare the Buffer
    while (Buff_one_length)
    {
        if (*Buff_one != *Buff_two)
            return NOT_MATCHED;

        Buff_one++;
        Buff_two++;
        Buff_one_length--;
    }

    return MATCHED;
}

//Compare the Two given Files with Binary Mode
int UnitTest::CompareFiles(char *file1, char *file2)
{
    fstream oldf;
    fstream newf;

    unsigned char *oldb, *newb;
    fstream::pos_type olength, nlength;

    oldf.open(file1, ios::in);
    if (oldf.is_open())
    {
        oldf.seekg(0, ios::end);
        olength = oldf.tellg();
        oldf.seekg(0, ios::beg);
        oldb = new unsigned char[olength];
        oldf.read((char*)oldb, olength);
        if (oldf.bad())
            return FILE_READ_ERROR;

        oldf.close();
    }
    else
    {
        oldf.close();
        return FILE_OPEN_ERROR;
    }

    newf.open(file2, ios::in);
    if (newf.is_open())
    {
        newf.seekp(0, ios::end);
        nlength = newf.tellg();
        newf.seekg(0, ios::beg);
        newb = new unsigned char[nlength];
        newf.read((char*)newb, nlength);
        if (newf.bad())
            return FILE_READ_ERROR;

        newf.close();
    }
    else
    {
        newf.close();
        return FILE_OPEN_ERROR;
    }

    //Compare the Two files output
    return CompareBuffer(oldb, newb);
}

//Write the Buffer To File with Append Mode
int UnitTest::DumpBuffer(unsigned char *Buffer, char *Filename)
{
    fstream   fhandle;

    fhandle.open(Filename, ios::binary | ios::app | ios::out);

    if (fhandle.is_open())
    {
        if (strlen((char*)Buffer) == 0)
            return WRONG_BUFFER;

        fhandle.write((char*)Buffer, strlen((char*)Buffer));
        if (fhandle.bad())
        {
            fhandle.close();
            return FILEWRITE_ERROR;
        }

        fhandle.close();
    }
    else
        return FILE_OPEN_ERROR;

    return 0;
}

//Compare two YUV Output Files
int UnitTest::CompareYUVOutputFile(char *file1, char *file2)
{
    fstream oldf;
    fstream newf;

    unsigned char *oldb, *newb;
    fstream::pos_type olength, nlength;

    oldf.open(file1, ios::in);
    if (oldf.is_open())
    {
        oldf.seekg(0, ios::end);
        olength = oldf.tellg();
        oldf.seekg(0, ios::beg);
        oldb = new unsigned char[olength];
        oldf.read((char*)oldb, olength);
        if (oldf.bad())
            return FILE_READ_ERROR;

        oldf.close();
    }
    else
    {
        oldf.close();
        return FILE_OPEN_ERROR;
    }

    newf.open(file2, ios::in);
    if (newf.is_open())
    {
        newf.seekp(0, ios::end);
        nlength = newf.tellg();
        newf.seekg(0, ios::beg);
        newb = new unsigned char[nlength];
        newf.read((char*)newb, nlength);
        if (newf.bad())
            return FILE_READ_ERROR;

        newf.close();
    }
    else
    {
        newf.close();
        return FILE_OPEN_ERROR;
    }

    return CompareYUVBuffer(oldb, newb);
}

int UnitTest::CompareYUVBuffer(unsigned char *Buff_old, unsigned char *Buff_new)
{
    //unsigned int offset =0;
    unsigned int a_size, b_size;
    unsigned int y_plane_area, u_plane_area, v_plane_area;

    unsigned int width = 416;
    unsigned int height = 240;
    char plane;

    unsigned int pix, offs;
    double pixel_x, pixel_y;
    int frame, macroblock;

    a_size = strlen((char*)Buff_old);
    b_size = strlen((char*)Buff_new);

    if (a_size != b_size)
        return NOT_MATCHED;

    unsigned int i = 0;
    while (i <= a_size)
    {
        if (Buff_old[i] != Buff_new[i])
        {
            offs =  i;
            y_plane_area = width * height;
            u_plane_area = y_plane_area + y_plane_area * 0.25;
            v_plane_area = u_plane_area + y_plane_area * 0.25;

            pix = offs % v_plane_area;
            frame = offs / v_plane_area;

            if (pix < y_plane_area)
            {
                plane = 'Y';
                pixel_x = pix % width;
                pixel_y = pix / width;
                macroblock = ceil(pixel_x / 16.0) + ceil(pixel_y / 16.0) * 16;
            }
            else if (pix < u_plane_area)
            {
                plane = 'U';
                pix -= y_plane_area;
                pixel_x = pix % width;
                pixel_y = pix / width;
                macroblock = ceil(pixel_x / 8.0) + ceil(pixel_y / 8.0) * 8;
            }
            else
            {
                plane = 'V';
                pix -= u_plane_area;
                pixel_x = pix % width;
                pixel_y = pix / width;
                macroblock = ceil(pixel_x / 8.0) + ceil(pixel_y / 8.0) * 8;
            }

            printf("New File Differs from Referance file at frame - %d,"
                   " macroblock - %d on the %c plane (offset %d)",
                   frame, macroblock, plane, offs);
            return NOT_MATCHED;
        }
        else
            i++;
    }

    return 0;
}
