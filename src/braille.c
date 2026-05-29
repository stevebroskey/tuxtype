/*
   braille.c:

   Description: Functions for loding braille map and key's
				order correcter 
   
   Copyright 2013.
   Author: Nalin.x.Linux < Nalin.x.Linux@gmail.com > 
   Project email: <tux4kids-tuxtype-dev@lists.alioth.debian.org>
   Project website: http://tux4kids.alioth.debian.org

   braille.c is part of Tux Typing, a.k.a "tuxtype".

Tux Typing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Tux Typing is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "globals.h"

/* Arrange the given disordered keycombination 
 * to assigned combination (fdsjkl) */
/* Reorder the given chord into canonical fdsjkl (dots 1-2-3-4-5-6) order
 * so the lookup against braille_key_value_map[].key works regardless of
 * which physical keys the user pressed first.
 * Source: archie2x/tuxtype a2x/sdl3_port commit a491d0c0 [B-006] */
void braille_reorder(wchar_t* disorder)
{
    static const wchar_t dots[] = L"fdsjkl";
    wchar_t              in[8]  = {0};
    int                  out_i  = 0;
    int                  in_len;

    wcsncpy(in, disorder, 7);
    in_len = (int)wcslen(in);
    disorder[0] = L'\0';

    for (int d = 0; d < 6; d++)
        for (int j = 0; j < in_len; j++)
            if (dots[d] == in[j])
            {
                disorder[out_i++] = dots[d];
                break;
            }
    disorder[out_i] = L'\0';
}



/* Braille map loading function 
 * 
 * The format of input file is as folows
 * keycombination<space>beginning_value<space>middle_value<space>end_value
 * 
 * For some specific language's which have same braille code for
 * alphabets and signs at begining, middle and end position.
 * 
 * The keyvombination should be writen in the order fdsjkl.
 * means first f second d so on. */
int braille_language_loader(char* language)
{
	int iter = 0;
	FILE *fp;
	char file[100];
	
	snprintf(file, sizeof(file), "%s/braille/%s", settings.default_data_path, language);
	fp = fopen(file,"r");
	
	if(fp == NULL)
	{
		DEBUGCODE{fprintf(stderr,"Couldn't open map for reading"); }
		return 0;
	}
			
	while(!feof(fp))
	{
		fscanf(fp,"%S %S %S %S\n",braille_key_value_map[iter].key,
			braille_key_value_map[iter].value_begin,
			braille_key_value_map[iter].value_middle,
			braille_key_value_map[iter].value_end);
		iter++;
	}
	return 1;
}
