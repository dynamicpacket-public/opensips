/* 
 * $Id$ 
 *
 * Flatstore module interface
 *
 * Copyright (C) 2004 FhG Fokus
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
 *
 * History:
 * --------
 *  2003-03-11  updated to the new module exports interface (andrei)
 *  2003-03-16  flags export parameter added (janakj)
 */

#include <string.h>
#include <ctype.h>
#include "../../mem/mem.h"
#include "../../dprint.h"
#include "flat_pool.h"
#include "flat_con.h"
#include "flatstore_mod.h"
#include "flatstore.h"


static int parse_flat_url(const str* url, str* path)
{
	if (!url || !url->s || !path) {
		LM_ERR("invalid parameter value\n");
		return -1;
	}
	path->s = strchr(url->s, ':') + 1;
	path->len = strlen(path->s);
	return 0;
}



/*
 * Initialize database module
 * No function should be called before this
 */
db_con_t* flat_db_init(const str* url)
{
	db_con_t* res;
	str* path;

	if (!url || !url->s) {
		LM_ERR("invalid parameter value\n");
		return 0;
	}

	/* We do not know the name of the table (and the name of the corresponding
	 * file) at this point, we will simply store the path taken from the url 
	 * parameter in the table variable, flat_use_table will then pick that 
	 * value and open the file
	 */
	/* as the table (path) is a substring of the received str, we need to 
	 * allocate a separate str struct for it -bogdan
	 */
	res = pkg_malloc(sizeof(db_con_t)+sizeof(struct flat_con*)+sizeof(str));
	if (!res) {
		LM_ERR("no pkg memory left\n");
		return 0;
	}
	memset(res, 0, sizeof(db_con_t) + sizeof(struct flat_con*) + sizeof(str));
	path = (str*)(((char*)res) + sizeof(db_con_t) + sizeof(struct flat_con*));

	if (parse_flat_url(url, path) < 0) {
		pkg_free(res);
		return 0;
	}
	res->table = path;

	return res;
}


/*
 * Store name of table that will be used by
 * subsequent database functions
 */
int flat_use_table(db_con_t* h, const str* t)
{
	struct flat_con* con;

	if (!h || !t || !t->s) {
		LM_ERR("invalid parameter value\n");
		return -1;
	}

	if (CON_TABLE(h)->s != t->s) {
		if (CON_TAIL(h)) {
			/* Decrement the reference count
			 * of the connection but do not remove
			 * it from the connection pool
			 */
			con = (struct flat_con*)CON_TAIL(h);
			con->ref--;
		}

		CON_TAIL(h) = (unsigned long)
			flat_get_connection((char*)CON_TABLE(h)->s, (char*)t->s);
		if (!CON_TAIL(h)) {
			return -1;
		}
	}
	
	return 0;
}


void flat_db_close(db_con_t* h)
{
	struct flat_con* con;

	if (!h) {
		LM_ERR("invalid parameter value\n");
		return;
	}

	con = (struct flat_con*)CON_TAIL(h);

	if (con) {
		flat_release_connection(con);
	}
	pkg_free(h);
}


/*
 * Insert a row into specified table
 * h: structure representing database connection
 * k: key names
 * v: values of the keys
 * n: number of key=value pairs
 */
int flat_db_insert(const db_con_t* h, const db_key_t* k, const db_val_t* v,
		const int n)
{
	FILE* f;
	int i;
	int l;
	char *s, *p;

	if (local_timestamp < *flat_rotate) {
		flat_rotate_logs();
		local_timestamp = *flat_rotate;
	}

	if ( !h || !CON_TAIL(h) || (f=CON_FILE(h))==NULL ) {
		LM_ERR("uninitialized connection\n");
		return -1;
	}

	for(i = 0; i < n; i++) {
		switch(VAL_TYPE(v + i)) {
		case DB_INT:
			fprintf(f, "%d", VAL_INT(v + i));
			break;

		case DB_DOUBLE:
			fprintf(f, "%f", VAL_DOUBLE(v + i));
			break;

		case DB_STRING:
			fprintf(f, "%s", VAL_STRING(v + i));
			break;

		case DB_STR:
			fprintf(f, "%.*s", VAL_STR(v + i).len, VAL_STR(v + i).s);
			break;

		case DB_DATETIME:
			fprintf(f, "%u", (unsigned int)VAL_TIME(v + i));
			break;

		case DB_BLOB:
			l = VAL_BLOB(v+i).len;
			s = p = VAL_BLOB(v+i).s;
			while (l--) {
				if ( !(isprint((int)*s) && *s != '\\' && *s != '|')) {
					fprintf(f,"%.*s\\x%02X",(int)(s-p),p,(*s & 0xff));
					p = s+1;
				}
				++s;
			}
			if (p!=s)
				fprintf(f,"%.*s",(int)(s-p),p);
			break;

		case DB_BITMAP:
			fprintf(f, "%u", VAL_BITMAP(v + i));
			break;
		}

		if (i < (n - 1)) {
			fprintf(f, "%c", *flat_delimiter);
		}
	}

	fprintf(f, "\n");

	if (flat_flush) {
		fflush(f);
	}

	return 0;
}
