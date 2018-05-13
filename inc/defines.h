/*
 * defines.h
 * Copyright (C) 2009-2018 Joachim de Groot <jdegroot@web.de>
 *
 * NLarn is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NLarn is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __DEFINES_H_
#define __DEFINES_H_

typedef enum _speed
{
    SPEED_XSLOW  =  25,
    SPEED_VSLOW  =  50,
    SPEED_SLOW   =  75,
    SPEED_NORMAL = 100,
    SPEED_FAST   = 125,
    SPEED_VFAST  = 150,
    SPEED_XFAST  = 175,
    SPEED_DOUBLE = 200,
    SPEED_MAX
} speed;

typedef enum _esize
{
    ESIZE_TINY         =  25,
    ESIZE_SMALL        =  75,
    ESIZE_MEDIUM       = 100,
    ESIZE_LARGE        = 125,
    ESIZE_HUGE         = 150,
    ESIZE_GARGANTUAN   = 200,
    ESIZE_MAX
} esize;

#endif
