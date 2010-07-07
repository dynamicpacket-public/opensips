/* 
 * $Id$ 
 *
 * Diversion Header Field Parsing Macros
 *
 * Copyright (C) 2001-2003 FhG Fokus
 *
 * This file is part of opensips, a free SIP server.
 *
 * opensips is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * opensips is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef CASE_DIVE_H
#define CASE_DIVE_H


#define RSIO_CASE                                  \
        switch(LOWER_DWORD(val)) {                 \
        case _rsio_:                               \
	        p += 4;                            \
	        if (LOWER_BYTE(*p) == 'n') {       \
		        hdr->type = HDR_DIVERSION_T; \
                        p++;                       \
                        goto dc_end;               \
                }                                  \
                goto other;                        \
        }


#define dive_CASE         \
        p += 4;           \
        val = READ(p);    \
        RSIO_CASE;        \
        goto other;


#endif /* CASE_DIVE_H */
