/*
   This file is part of gnome-turtle
   Written by: Mathieu Lutfy <mathieu@bidon.ca>
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
   
   For further information, please contact at mathieu@bidon.ca
*/

%{
/*
#include <config.h>
*/
#include "turtle-helpers.h"
%}

%union {
char sym;
double dval;
}

%token <dval> NUM
%token <sym>  SYMBOL

%token ARROW
%token AXIOM_WORD
%token RULES_WORD
%token ANGLE_WORD
%token TETHA_WORD
%token SIGMA_WORD
%token DEPTH_WORD
%token XYINIT_WORD
%token XYSCALE_WORD

%type <dval> repeat_num

%%

input: definition ;

definition: axiom rules parameters ;

/* expected examples:  "Axiom: F", "Axiom: F[F+]G", etc. */
axiom: AXIOM_WORD ':' pattern_non_empty { turtle_axiom_ready(); }
	;

pattern_non_empty: SYMBOL repeat_num
				{ 
					char newname;
					if ($2 > 0) {
						newname = turtle_create_special_rule($1, $2);
						turtle_add_to_buffer(newname);
					} else {
						turtle_add_to_buffer($1);
					}
				}
			pattern
	| '|' { turtle_add_to_buffer('|'); } pattern
	| '$' { turtle_add_to_buffer('$'); } pattern
	| '[' { turtle_add_to_buffer('['); } pattern_non_empty ']' { turtle_add_to_buffer(']'); } pattern
	| '+' { turtle_add_to_buffer('+'); } pattern
	| '-' { turtle_add_to_buffer('-'); } pattern
	| '%' { turtle_add_to_buffer('%'); } /* empty rule */
	;

pattern: /* empty */
	| pattern_non_empty 
	;

repeat_num: /* empty */ { $$ = 0; }
	| NUM { $$ = $1; }
	;

rules: RULES_WORD ':' rules_decl_non_empty
	;

/* there may be many rules, unlike the axiom */
rules_decl_non_empty: SYMBOL ARROW pattern_non_empty { turtle_rule_ready($1); } rules_decl 
	| ';' rules_decl_non_empty 
	;

rules_decl: /* empty */
	| rules_decl_non_empty
	;

parameters: /* empty */
	| angle parameters
	| sigma parameters
	| tetha parameters
	| depth parameters
	| xyinit parameters
	| xyscale parameters
	;

angle: ANGLE_WORD ':' NUM { turtle_set_angle($3); }

sigma: SIGMA_WORD ':' NUM { turtle_set_sigma($3); }

tetha: TETHA_WORD ':' NUM { turtle_set_tetha($3); }

depth: DEPTH_WORD ':' NUM { turtle_set_depth((int) $3); }

xyinit: XYINIT_WORD ':' NUM NUM { turtle_set_xyinit($3, $4); }

xyscale: XYSCALE_WORD ':' NUM NUM { turtle_set_xyscale($3, $4); }

%%

#include <stdio.h>

