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
#include <string.h>
// #include "turtle-parser.tab.h"
#include "turtle-parser.h"

static char buffer[1024];
static int cpt_buffer = 0;

%} 

SYMBOL		[A-Za-z]
FLOAT		[0-9]+[\.,]?[0-9]*

%option		warn
%option		yylineno
%option		stack

%x IN_COMMENT
%s IN_STRING IN_EXITEM

%%

<*>[ ]+		; // putchar(' '); // whitespace compression
<*>[ \t]+$	; // ignore whitespaces at the end of lines
\t+		;
{FLOAT}		{ yylval.dval = strtod (yytext, NULL); return NUM; }

<INITIAL>{	// Pseudo C-Style comments. We recopy only their contents.
"/*"		yy_push_state(IN_COMMENT);
}

<IN_COMMENT>{
"*/"		yy_pop_state();
[^*]+		{ printf("Comment: %s", yytext); } // copy-paste all
}

"Axiom"		{ return AXIOM_WORD; }
"axiom"		{ return AXIOM_WORD; }

"Rules"		{ return RULES_WORD; }
"rules"		{ return RULES_WORD; }

"Angle"		{ return ANGLE_WORD; } // ahem, too much redundance
"angle"		{ return ANGLE_WORD; }

"Tetha"		{ return TETHA_WORD; }
"tetha"		{ return TETHA_WORD; }

"Sigma"		{ return SIGMA_WORD; }
"sigma"		{ return SIGMA_WORD; }

"Depth"		{ return DEPTH_WORD; }
"depth"		{ return DEPTH_WORD; }

"xyinit"	{ return XYINIT_WORD; }
"xyscale"	{ return XYSCALE_WORD; }

"->"		{ return ARROW; }

{SYMBOL}	{ yylval.sym = yytext[0]; return SYMBOL; }
.               { return yytext[0]; } // match all single characters

%%

#include <stdio.h>

int yywrap (void)
{
	return 1;
}

int yyerror (char *error)
{
	printf ("Line %d: %s (while parsing '%s')\n", yylineno, error, yytext);
	return 1;
}

