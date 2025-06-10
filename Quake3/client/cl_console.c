/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// console.c

#include "client.h"
#include "con_keyboard.h"

// Virtual keyboard functions
extern qboolean Con_VirtualKeyboardActive(void);
extern void Con_GetVirtualKeyboardInfo(int *cursorX, int *cursorY, const char ***layout);

// Forward declaration
void Con_DrawVirtualKeyboard(void);

int g_console_field_width = 78;


#define	NUM_CON_TIMES 4

#define		CON_TEXTSIZE	32768
typedef struct {
	qboolean	initialized;

	short	text[CON_TEXTSIZE];
	int		current;		// line where next message will be printed
	int		x;				// offset in current line for next print
	int		display;		// bottom of console displays this line

	int 	linewidth;		// characters across screen
	int		totallines;		// total lines in console scrollback

	float	xadjust;		// for wide aspect screens

	float	displayFrac;	// aproaches finalFrac at scr_conspeed
	float	finalFrac;		// 0.0 to 1.0 lines of console to display

	int		vislines;		// in scanlines

	int		times[NUM_CON_TIMES];	// cls.realtime time the line was generated
								// for transparent notify lines
	vec4_t	color;
} console_t;

console_t	con;

cvar_t		*con_conspeed;
cvar_t		*con_autoclear;
cvar_t		*con_notifytime;
cvar_t      *con_scale;
cvar_t      *con_margin;  // Horizontal margin for console (in pixels)

// Function prototypes
int Con_GetDefaultMargin(void);

#define	DEFAULT_CONSOLE_WIDTH	78

/*
================
Con_DrawScaledChar
Draws a character scaled by con_scale
================
*/
void Con_DrawScaledChar(int x, int y, int ch) {
    float scale = con_scale ? con_scale->value : 1.0f;
    if (scale < 0.5f) scale = 0.5f;  // Minimum scale
    if (scale > 3.0f) scale = 3.0f;   // Maximum scale
    
    if (scale == 1.0f) {
        // If scale is 1, use the original function
        SCR_DrawSmallChar(x, y, ch);
        return;
    }
    
    // For scaled text, we need to use the stretched draw functions
    int w = SMALLCHAR_WIDTH * scale;
    int h = SMALLCHAR_HEIGHT * scale;
    
    // Calculate which character to draw from the font sheet
    int row = ch >> 4;
    int col = ch & 15;
    
    float frow = row * 0.0625;  // 1/16
    float fcol = col * 0.0625;
    float size = 0.0625;
    
    re.DrawStretchPic(x, y, w, h, fcol, frow, fcol + size, frow + size, cls.charSetShader);
}

/*
================
Con_GetScaledCharWidth
Returns the scaled width of a character
================
*/
int Con_GetScaledCharWidth(void) {
    float scale = con_scale ? con_scale->value : 1.0f;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 3.0f) scale = 3.0f;
    return SMALLCHAR_WIDTH * scale;
}

/*
================
Con_GetScaledCharHeight
Returns the scaled height of a character
================
*/
int Con_GetScaledCharHeight(void) {
    float scale = con_scale ? con_scale->value : 1.0f;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 3.0f) scale = 3.0f;
    return SMALLCHAR_HEIGHT * scale;
}


/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f (void) {
	// Can't toggle the console when it's the only thing available
	if ( clc.state == CA_DISCONNECTED && Key_GetCatcher( ) == KEYCATCH_CONSOLE ) {
		return;
	}

	if ( con_autoclear->integer ) {
		Field_Clear( &g_consoleField );
	}

	g_consoleField.widthInChars = g_console_field_width;

	Con_ClearNotify ();
	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_CONSOLE );
}

/*
===================
Con_ToggleMenu_f
===================
*/
void Con_ToggleMenu_f( void ) {
	CL_KeyEvent( K_ESCAPE, qtrue, Sys_Milliseconds() );
	CL_KeyEvent( K_ESCAPE, qfalse, Sys_Milliseconds() );
}

/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f (void) {
	chat_playerNum = -1;
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;

	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_MESSAGE );
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f (void) {
	chat_playerNum = -1;
	chat_team = qtrue;
	Field_Clear( &chatField );
	chatField.widthInChars = 25;
	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_MESSAGE );
}

/*
================
Con_MessageMode3_f
================
*/
void Con_MessageMode3_f (void) {
	chat_playerNum = VM_Call( cgvm, CG_CROSSHAIR_PLAYER );
	if ( chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS ) {
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;
	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_MESSAGE );
}

/*
================
Con_MessageMode4_f
================
*/
void Con_MessageMode4_f (void) {
	chat_playerNum = VM_Call( cgvm, CG_LAST_ATTACKER );
	if ( chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS ) {
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;
	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_MESSAGE );
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f (void) {
	int		i;

	for ( i = 0 ; i < CON_TEXTSIZE ; i++ ) {
		con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
	}

	Con_Bottom();		// go to end
}

						
/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f (void)
{
	int		l, x, i;
	short	*line;
	fileHandle_t	f;
	int		bufferlen;
	char	*buffer;
	char	filename[MAX_QPATH];

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("usage: condump <filename>\n");
		return;
	}

	Q_strncpyz( filename, Cmd_Argv( 1 ), sizeof( filename ) );
	COM_DefaultExtension( filename, sizeof( filename ), ".txt" );

	if (!COM_CompareExtension(filename, ".txt"))
	{
		Com_Printf("Con_Dump_f: Only the \".txt\" extension is supported by this command!\n");
		return;
	}

	f = FS_FOpenFileWrite( filename );
	if (!f)
	{
		Com_Printf ("ERROR: couldn't open %s.\n", filename);
		return;
	}

	Com_Printf ("Dumped console text to %s.\n", filename );

	// skip empty lines
	for (l = con.current - con.totallines + 1 ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		for (x=0 ; x<con.linewidth ; x++)
			if ((line[x] & 0xff) != ' ')
				break;
		if (x != con.linewidth)
			break;
	}

#ifdef _WIN32
	bufferlen = con.linewidth + 3 * sizeof ( char );
#else
	bufferlen = con.linewidth + 2 * sizeof ( char );
#endif

	buffer = Hunk_AllocateTempMemory( bufferlen );

	// write the remaining lines
	buffer[bufferlen-1] = 0;
	for ( ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		for(i=0; i<con.linewidth; i++)
			buffer[i] = line[i] & 0xff;
		for (x=con.linewidth-1 ; x>=0 ; x--)
		{
			if (buffer[x] == ' ')
				buffer[x] = 0;
			else
				break;
		}
#ifdef _WIN32
		Q_strcat(buffer, bufferlen, "\r\n");
#else
		Q_strcat(buffer, bufferlen, "\n");
#endif
		FS_Write(buffer, strlen(buffer), f);
	}

	Hunk_FreeTempMemory( buffer );
	FS_FCloseFile( f );
}

						
/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify( void ) {
	int		i;
	
	for ( i = 0 ; i < NUM_CON_TIMES ; i++ ) {
		con.times[i] = 0;
	}
}

						

/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize (void)
{
    int        i, j, width, oldwidth, oldtotallines, numlines, numchars;
    short    tbuf[CON_TEXTSIZE];
    float    scale = con_scale ? con_scale->value : 1.0f;
    int        margin = con_margin ? con_margin->integer : 0;
    
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 3.0f) scale = 3.0f;
    
    // Ensure margin doesn't make console too narrow
    int maxMargin = (SCREEN_WIDTH / 4);  // Don't allow margins bigger than 1/4 screen width each
    if (margin > maxMargin) margin = maxMargin;
    if (margin < 0) margin = 0;

    // Adjust width based on scale and margins
    int availableWidth = SCREEN_WIDTH - (margin * 2);  // Subtract margins from both sides
    width = (availableWidth / (SMALLCHAR_WIDTH * scale)) - 2;

	if (width == con.linewidth)
		return;

	if (width < 1)			// video hasn't been initialized yet
	{
		width = DEFAULT_CONSOLE_WIDTH;
		con.linewidth = width;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		for(i=0; i<CON_TEXTSIZE; i++)
			con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
	}
	else
	{
		oldwidth = con.linewidth;
		con.linewidth = width;
		oldtotallines = con.totallines;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		numlines = oldtotallines;

		if (con.totallines < numlines)
			numlines = con.totallines;

		numchars = oldwidth;
	
		if (con.linewidth < numchars)
			numchars = con.linewidth;

		Com_Memcpy (tbuf, con.text, CON_TEXTSIZE * sizeof(short));
		for(i=0; i<CON_TEXTSIZE; i++)
			con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';

		for (i=0 ; i<numlines ; i++)
		{
			for (j=0 ; j<numchars ; j++)
			{
				con.text[(con.totallines - 1 - i) * con.linewidth + j] =
						tbuf[((con.current - i + oldtotallines) %
							  oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify ();
	}

    con.current = con.totallines - 1;
    con.display = con.current;
    
    // Update console adjustment for margins
    con.xadjust = margin;  // This will offset everything by the margin amount
    
    // Adjust the console field width based on scale and margins
    g_consoleField.widthInChars = width - 2;  // Use the calculated width
    g_console_field_width = g_consoleField.widthInChars;
}

/*
==================
Cmd_CompleteTxtName
==================
*/
void Cmd_CompleteTxtName( char *args, int argNum ) {
	if( argNum == 2 ) {
		Field_CompleteFilename( "", "txt", qfalse, qtrue );
	}
}

/*
================
Con_TestMargin_f
================
*/
void Con_TestMargin_f(void) {
    int detected = Con_GetDefaultMargin();
    Com_Printf("Detected margin for this device: %d\n", detected);
    Com_Printf("Current screen: %d x %d\n", cls.glconfig.vidWidth, cls.glconfig.vidHeight);
    if (cls.glconfig.vidWidth > cls.glconfig.vidHeight) {
        float ratio = (float)cls.glconfig.vidWidth / (float)cls.glconfig.vidHeight;
        Com_Printf("Aspect ratio: %.2f:1\n", ratio);
    }
}

/*
================
Con_ResetMargin_f
================
*/
void Con_ResetMargin_f(void) {
    int detected = Con_GetDefaultMargin();
    Cvar_Set("con_margin", va("%d", detected));
    Com_Printf("Reset con_margin to detected value: %d\n", detected);
}

/*
================
Con_Init
================
*/
void Con_Init (void) {
    int        i;

    con_notifytime = Cvar_Get ("con_notifytime", "3", 0);
    con_conspeed = Cvar_Get ("scr_conspeed", "3", 0);
    con_autoclear = Cvar_Get("con_autoclear", "1", CVAR_ARCHIVE);
    con_scale = Cvar_Get("con_scale", "1", CVAR_ARCHIVE);
    
    // Use intelligent default based on device
    char defaultMargin[32];
    Com_sprintf(defaultMargin, sizeof(defaultMargin), "%d", Con_GetDefaultMargin());
    con_margin = Cvar_Get("con_margin", defaultMargin, CVAR_ARCHIVE);
    Com_Printf("Console margin set to: %s\n", con_margin->string);

    Field_Clear( &g_consoleField );
    g_consoleField.widthInChars = g_console_field_width;
    for ( i = 0 ; i < COMMAND_HISTORY ; i++ ) {
        Field_Clear( &historyEditLines[i] );
        historyEditLines[i].widthInChars = g_console_field_width;
    }
    CL_LoadConsoleHistory( );

    Cmd_AddCommand ("toggleconsole", Con_ToggleConsole_f);
    Cmd_AddCommand ("togglemenu", Con_ToggleMenu_f);
    Cmd_AddCommand ("messagemode", Con_MessageMode_f);
    Cmd_AddCommand ("messagemode2", Con_MessageMode2_f);
    Cmd_AddCommand ("messagemode3", Con_MessageMode3_f);
    Cmd_AddCommand ("messagemode4", Con_MessageMode4_f);
    Cmd_AddCommand ("clear", Con_Clear_f);
    Cmd_AddCommand ("condump", Con_Dump_f);
    Cmd_SetCommandCompletionFunc( "condump", Cmd_CompleteTxtName );
    
    // Margin management
    Cmd_AddCommand ("testmargin", Con_TestMargin_f);
    Cmd_AddCommand ("resetmargin", Con_ResetMargin_f);
}

/*
================
Con_Shutdown
================
*/
void Con_Shutdown(void)
{
	Cmd_RemoveCommand("toggleconsole");
	Cmd_RemoveCommand("togglemenu");
	Cmd_RemoveCommand("messagemode");
	Cmd_RemoveCommand("messagemode2");
	Cmd_RemoveCommand("messagemode3");
	Cmd_RemoveCommand("messagemode4");
	Cmd_RemoveCommand("clear");
	Cmd_RemoveCommand("condump");
}

/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed (qboolean skipnotify)
{
	int		i;

	// mark time for transparent overlay
	if (con.current >= 0)
	{
    if (skipnotify)
		  con.times[con.current % NUM_CON_TIMES] = 0;
    else
		  con.times[con.current % NUM_CON_TIMES] = cls.realtime;
	}

	con.x = 0;
	if (con.display == con.current)
		con.display++;
	con.current++;
	for(i=0; i<con.linewidth; i++)
		con.text[(con.current%con.totallines)*con.linewidth+i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
}

/*
================
CL_ConsolePrint

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
void CL_ConsolePrint( char *txt ) {
	int		y, l;
	unsigned char	c;
	unsigned short	color;
	qboolean skipnotify = qfalse;		// NERVE - SMF
	int prev;							// NERVE - SMF

	// TTimo - prefix for text that shows up in console but not in notify
	// backported from RTCW
	if ( !Q_strncmp( txt, "[skipnotify]", 12 ) ) {
		skipnotify = qtrue;
		txt += 12;
	}
	
	// for some demos we don't want to ever show anything on the console
	if ( cl_noprint && cl_noprint->integer ) {
		return;
	}
	
	if (!con.initialized) {
		con.color[0] = 
		con.color[1] = 
		con.color[2] =
		con.color[3] = 1.0f;
		con.linewidth = -1;
		Con_CheckResize ();
		con.initialized = qtrue;
	}

	color = ColorIndex(COLOR_WHITE);

	while ( (c = *((unsigned char *) txt)) != 0 ) {
		if ( Q_IsColorString( txt ) ) {
			color = ColorIndex( *(txt+1) );
			txt += 2;
			continue;
		}

		// count word length
		for (l=0 ; l< con.linewidth ; l++) {
			if ( txt[l] <= ' ') {
				break;
			}

		}

		// word wrap
		if (l != con.linewidth && (con.x + l >= con.linewidth) ) {
			Con_Linefeed(skipnotify);

		}

		txt++;

		switch (c)
		{
		case '\n':
			Con_Linefeed (skipnotify);
			break;
		case '\r':
			con.x = 0;
			break;
		default:	// display character and advance
			y = con.current % con.totallines;
			con.text[y*con.linewidth+con.x] = (color << 8) | c;
			con.x++;
			if(con.x >= con.linewidth)
				Con_Linefeed(skipnotify);
			break;
		}
	}


	// mark time for transparent overlay
	if (con.current >= 0) {
		// NERVE - SMF
		if ( skipnotify ) {
			prev = con.current % NUM_CON_TIMES - 1;
			if ( prev < 0 )
				prev = NUM_CON_TIMES - 1;
			con.times[prev] = 0;
		}
		else
		// -NERVE - SMF
			con.times[con.current % NUM_CON_TIMES] = cls.realtime;
	}
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

Draw the editline after a ] prompt
================
*/
void Con_DrawInput (void) {
    int        y;
    int        charWidth = Con_GetScaledCharWidth();
    int        charHeight = Con_GetScaledCharHeight();

    if ( clc.state != CA_DISCONNECTED && !(Key_GetCatcher( ) & KEYCATCH_CONSOLE ) ) {
        return;
    }

    y = con.vislines - ( charHeight * 2 );

    re.SetColor( con.color );

    // con.xadjust should already include the margin from Con_DrawSolidConsole
    Con_DrawScaledChar( con.xadjust + 1 * charWidth, y, ']' );

    // Scale the input field text
    float scale = con_scale ? con_scale->value : 1.0f;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 3.0f) scale = 3.0f;
    
    // We need to draw the field character by character with scaling
    int x = con.xadjust + 2 * charWidth;
    int fieldStart = g_consoleField.scroll;
    int fieldEnd = fieldStart + g_consoleField.widthInChars;
    
    // Draw the input text
    for (int i = fieldStart; i < fieldEnd && i < strlen(g_consoleField.buffer); i++) {
        if (g_consoleField.buffer[i]) {
            Con_DrawScaledChar(x, y, g_consoleField.buffer[i]);
            x += charWidth;
        }
    }
    
    // Draw the cursor
    if ((int)(cls.realtime >> 8) & 1) {  // Blinking cursor
        Con_DrawScaledChar(con.xadjust + 2 * charWidth + (g_consoleField.cursor - fieldStart) * charWidth, y, 11);  // 11 is typically the cursor character
    }
}


/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify (void)
{
    int        x, v;
    short    *text;
    int        i;
    int        time;
    int        skip;
    int        currentColor;
    int        charWidth = Con_GetScaledCharWidth();
    int        charHeight = Con_GetScaledCharHeight();
    int        margin = con_margin ? con_margin->integer : 0;

    currentColor = 7;
    re.SetColor( g_color_table[currentColor] );

    v = 0;
    for (i= con.current-NUM_CON_TIMES+1 ; i<=con.current ; i++)
    {
        if (i < 0)
            continue;
        time = con.times[i % NUM_CON_TIMES];
        if (time == 0)
            continue;
        time = cls.realtime - time;
        if (time > con_notifytime->value*1000)
            continue;
        text = con.text + (i % con.totallines)*con.linewidth;

        if (cl.snap.ps.pm_type != PM_INTERMISSION && Key_GetCatcher( ) & (KEYCATCH_UI | KEYCATCH_CGAME) ) {
            continue;
        }

        for (x = 0 ; x < con.linewidth ; x++) {
            if ( ( text[x] & 0xff ) == ' ' ) {
                continue;
            }
            if ( ColorIndexForNumber( text[x]>>8 ) != currentColor ) {
                currentColor = ColorIndexForNumber( text[x]>>8 );
                re.SetColor( g_color_table[currentColor] );
            }
            Con_DrawScaledChar( margin + cl_conXOffset->integer + con.xadjust + (x+1)*charWidth, v, text[x] & 0xff );
        }

        v += charHeight;
    }

    re.SetColor( NULL );

    if (Key_GetCatcher( ) & (KEYCATCH_UI | KEYCATCH_CGAME) ) {
        return;
    }

    // draw the chat line
    if ( Key_GetCatcher( ) & KEYCATCH_MESSAGE )
    {
        // Scale factor for big text (chat input)
        float bigScale = con_scale ? con_scale->value : 1.0f;
        if (bigScale < 0.5f) bigScale = 0.5f;
        if (bigScale > 3.0f) bigScale = 3.0f;
        
        if (chat_team)
        {
            SCR_DrawBigString (8, v, "say_team:", bigScale, qfalse );
            skip = 10;
        }
        else
        {
            SCR_DrawBigString (8, v, "say:", bigScale, qfalse );
            skip = 5;
        }

        Field_BigDraw( &chatField, skip * BIGCHAR_WIDTH * bigScale, v,
            SCREEN_WIDTH - ( skip + 1 ) * BIGCHAR_WIDTH * bigScale, qtrue, qtrue );
    }
}

/*
================
Con_DrawSolidConsole

Draws the console with the solid background
================
*/
void Con_DrawSolidConsole( float frac ) {
    int                i, x, y;
    int                rows;
    short            *text;
    int                row;
    int                lines;
    int                currentColor;
    vec4_t            color;
    int                charWidth = Con_GetScaledCharWidth();
    int                charHeight = Con_GetScaledCharHeight();

    lines = cls.glconfig.vidHeight * frac;
    if (lines <= 0)
        return;

    if (lines > cls.glconfig.vidHeight )
        lines = cls.glconfig.vidHeight;

    // on wide screens, we will center the text
    con.xadjust = 0;
    SCR_AdjustFrom640( &con.xadjust, NULL, NULL, NULL );

    // Apply margin after the adjustment
    int margin = con_margin ? con_margin->integer : 0;
    con.xadjust += margin;

    // draw the background
    y = frac * SCREEN_HEIGHT;
    if ( y < 1 ) {
        y = 0;
    }
    else {
        SCR_DrawPic( 0, 0, SCREEN_WIDTH, y, cls.consoleShader );
    }

    color[0] = 1;
    color[1] = 0;
    color[2] = 0;
    color[3] = 1;
    SCR_FillRect( 0, y, SCREEN_WIDTH, 2, color );

    // draw the version number
    re.SetColor( g_color_table[ColorIndex(COLOR_RED)] );

    i = strlen( Q3_VERSION );

    for (x=0 ; x<i ; x++) {
        Con_DrawScaledChar( cls.glconfig.vidWidth - ( i - x + 1 ) * charWidth,
            lines - charHeight, Q3_VERSION[x] );
    }

    // draw the text
    con.vislines = lines;
    rows = (lines-charHeight)/charHeight;        // rows of text to draw

    y = lines - (charHeight*3);

    // draw from the bottom up
    if (con.display != con.current)
    {
        // draw arrows to show the buffer is backscrolled
        re.SetColor( g_color_table[ColorIndex(COLOR_RED)] );
        for (x=0 ; x<con.linewidth ; x+=4)
            Con_DrawScaledChar( con.xadjust + (x+1)*charWidth, y, '^' );
        y -= charHeight;
        rows--;
    }
    
    row = con.display;

    if ( con.x == 0 ) {
        row--;
    }

    currentColor = 7;
    re.SetColor( g_color_table[currentColor] );

    for (i=0 ; i<rows ; i++, y -= charHeight, row--)
    {
        if (row < 0)
            break;
        if (con.current - row >= con.totallines) {
            // past scrollback wrap point
            continue;
        }

        text = con.text + (row % con.totallines)*con.linewidth;

        for (x=0 ; x<con.linewidth ; x++) {
            if ( ( text[x] & 0xff ) == ' ' ) {
                continue;
            }

            if ( ColorIndexForNumber( text[x]>>8 ) != currentColor ) {
                currentColor = ColorIndexForNumber( text[x]>>8 );
                re.SetColor( g_color_table[currentColor] );
            }
            Con_DrawScaledChar( con.xadjust + (x+1)*charWidth, y, text[x] & 0xff );
        }
    }

    // draw the input prompt, user text, and cursor if desired
    Con_DrawInput ();

    re.SetColor( NULL );
}



/*
==================
Con_DrawConsole
==================
*/
void Con_DrawConsole( void ) {
	// check for console width changes from a vid mode change
	Con_CheckResize ();

	// if disconnected, render console full screen
	if ( clc.state == CA_DISCONNECTED ) {
		if ( !( Key_GetCatcher( ) & (KEYCATCH_UI | KEYCATCH_CGAME)) ) {
			Con_DrawSolidConsole( 1.0 );
			return;
		}
	}

	if ( con.displayFrac ) {
		Con_DrawSolidConsole( con.displayFrac );
	} else {
		// draw notify lines
		if ( clc.state == CA_ACTIVE ) {
			Con_DrawNotify ();
		}
	}
    Con_DrawVirtualKeyboard();
}

//================================================================

/*
==================
Con_RunConsole

Scroll it up or down
==================
*/
void Con_RunConsole (void) {
	// decide on the destination height of the console
	if ( Key_GetCatcher( ) & KEYCATCH_CONSOLE )
		con.finalFrac = 0.5;		// half screen
	else
		con.finalFrac = 0;				// none visible
	
	// scroll towards the destination height
	if (con.finalFrac < con.displayFrac)
	{
		con.displayFrac -= con_conspeed->value*cls.realFrametime*0.001;
		if (con.finalFrac > con.displayFrac)
			con.displayFrac = con.finalFrac;

	}
	else if (con.finalFrac > con.displayFrac)
	{
		con.displayFrac += con_conspeed->value*cls.realFrametime*0.001;
		if (con.finalFrac < con.displayFrac)
			con.displayFrac = con.finalFrac;
	}

}


void Con_PageUp( void ) {
	con.display -= 2;
	if ( con.current - con.display >= con.totallines ) {
		con.display = con.current - con.totallines + 1;
	}
}

void Con_PageDown( void ) {
	con.display += 2;
	if (con.display > con.current) {
		con.display = con.current;
	}
}

void Con_Top( void ) {
	con.display = con.totallines;
	if ( con.current - con.display >= con.totallines ) {
		con.display = con.current - con.totallines + 1;
	}
}

void Con_Bottom( void ) {
	con.display = con.current;
}


void Con_Close( void ) {
	if ( !com_cl_running->integer ) {
		return;
	}
	Field_Clear( &g_consoleField );
	Con_ClearNotify ();
	Key_SetCatcher( Key_GetCatcher( ) & ~KEYCATCH_CONSOLE );
	con.finalFrac = 0;				// none visible
	con.displayFrac = 0;
}

/*
==================
Con_DrawVirtualKeyboard
==================
*/
void Con_DrawVirtualKeyboard(void) {
    if (!Con_VirtualKeyboardActive()) {
        return;
    }
    
    int cursorX, cursorY;
    const char **layout;
    Con_GetVirtualKeyboardInfo(&cursorX, &cursorY, (const char ***)&layout);
    
    // Calculate available space
    float consoleHeight = cls.glconfig.vidHeight * con.displayFrac;
    int availableHeight = cls.glconfig.vidHeight - consoleHeight - 20;
    int availableWidth = cls.glconfig.vidWidth - 40;
    
    // Calculate key dimensions to fill available space
    int keyHeight = (availableHeight - (VK_ROWS - 1) * VK_KEY_SPACING) / VK_ROWS;
    int baseKeyWidth = availableWidth / 14;
    
    // Ensure minimum sizes
    if (keyHeight < 40) keyHeight = 40;
    if (baseKeyWidth < 50) baseKeyWidth = 50;
    
    int startY = consoleHeight + 10;
    
    // Draw background
    vec4_t bgColor = {0.0f, 0.0f, 0.0f, 0.9f};
    re.SetColor(bgColor);
    re.DrawStretchPic(0, consoleHeight, cls.glconfig.vidWidth, cls.glconfig.vidHeight - consoleHeight,
                      0, 0, 1, 1, cls.whiteShader);
    
    // Draw keys
    int x, y;
    for (y = 0; y < VK_ROWS; y++) {
        int rowWidth = 0;
        int keyCount = 0;
        for (x = 0; x < VK_COLS; x++) {
            if (vk_keyWidths[y][x] > 0) {
                rowWidth += (baseKeyWidth * vk_keyWidths[y][x]) + VK_KEY_SPACING;
                keyCount++;
            }
        }
        // Remove the trailing space
        if (keyCount > 0) {
            rowWidth -= VK_KEY_SPACING;
        }
        
        // Calculate max width across all rows (do this only once per frame)
        int maxRowWidth = 0;
        if (y == 0) {  // Only calculate on first row
            int r;
            for (r = 0; r < VK_ROWS; r++) {
                int tempWidth = 0;
                int tempCount = 0;
                int c;
                for (c = 0; c < VK_COLS; c++) {
                    if (vk_keyWidths[r][c] > 0) {
                        tempWidth += (baseKeyWidth * vk_keyWidths[r][c]) + VK_KEY_SPACING;
                        tempCount++;
                    }
                }
                if (tempCount > 0) {
                    tempWidth -= VK_KEY_SPACING;
                }
                if (tempWidth > maxRowWidth) {
                    maxRowWidth = tempWidth;
                }
            }
        }

        // For rows after 0, we need to get the max width somehow
        // One approach is to make it static but recalculate each time
        static int cachedMaxRowWidth = 0;
        if (y == 0) {
            cachedMaxRowWidth = maxRowWidth;
        }
        int startX = (cls.glconfig.vidWidth - (y == 0 ? maxRowWidth : cachedMaxRowWidth)) / 2;
        int currentX = startX;
        
        for (x = 0; x < VK_COLS; x++) {
            const char *key = layout[y * VK_COLS + x];
            int keyWidth = vk_keyWidths[y][x];
            
            if (!key || !key[0] || keyWidth == 0) continue;
            
            int keyX = currentX;
            int keyY = startY + y * (keyHeight + VK_KEY_SPACING);
            int thisKeyWidth = (baseKeyWidth * keyWidth) + (VK_KEY_SPACING * (keyWidth - 1));
            
            // Draw key background
            if (!strcmp(key, "SHIFT") && Con_VirtualKeyboardIsShifted()) {
                vec4_t shiftActiveColor = {0.0f, 0.5f, 1.0f, 1.0f};  // Blue when active
                re.SetColor(shiftActiveColor);
            } else if (x == cursorX && y == cursorY) {
                vec4_t highlightColor = {1.0f, 0.5f, 0.0f, 1.0f};
                re.SetColor(highlightColor);
            } else {
                vec4_t keyColor = {0.3f, 0.3f, 0.3f, 1.0f};
                re.SetColor(keyColor);
            }
            re.DrawStretchPic(keyX, keyY, thisKeyWidth, keyHeight, 0, 0, 1, 1, cls.whiteShader);
            
            // Draw key label
            re.SetColor(colorWhite);
            const char *label = key;

            // Simple labels for arrows
            if (!strcmp(key, "UP")) label = "^";
            else if (!strcmp(key, "DOWN")) label = "v";
            else if (!strcmp(key, "LEFT")) label = "<";
            else if (!strcmp(key, "RIGHT")) label = ">";
            else if (key[0] == ' ') label = "SPACE";

            int len = strlen(label);

            // Save current con_scale
            float oldScale = con_scale ? con_scale->value : 1.0f;

            // For single characters, draw them big
            if (len == 1) {
                // Temporarily set scale for big characters
                if (con_scale) con_scale->value = 3.0f;  // 3x size
                
                int charW = Con_GetScaledCharWidth();
                int charH = Con_GetScaledCharHeight();
                int textX = keyX + (thisKeyWidth - charW) / 2;
                int textY = keyY + (keyHeight - charH) / 2;
                
                Con_DrawScaledChar(textX, textY, label[0]);
            } else {
                // Multi-character labels - use smaller scale
                if (con_scale) con_scale->value = 1.5f;  // 1.5x size
                
                int charW = Con_GetScaledCharWidth();
                int charH = Con_GetScaledCharHeight();
                int totalWidth = len * charW;
                int textX = keyX + (thisKeyWidth - totalWidth) / 2;
                int textY = keyY + (keyHeight - charH) / 2;
                
                int i;
                for (i = 0; i < len; i++) {
                    Con_DrawScaledChar(textX + i * charW, textY, label[i]);
                }
            }

            // Restore original con_scale
            if (con_scale) con_scale->value = oldScale;
            
            currentX += thisKeyWidth + VK_KEY_SPACING;
        }
    }
    
    re.SetColor(NULL);
}

/*
================
Con_GetDefaultMargin
Returns a default margin value based on device
================
*/
int Con_GetDefaultMargin(void) {
    int screenWidth = cls.glconfig.vidWidth;
    int screenHeight = cls.glconfig.vidHeight;
    
    // Only apply margin in landscape
    if (screenWidth > screenHeight) {
        float aspectRatio = (float)screenWidth / (float)screenHeight;
        
        // Modern iPhones with notch/dynamic island have ~2.16:1 or higher ratio
        if (aspectRatio > 2.1f) {
            // iPhone 16 Pro Max and similar need about 130 pixels
            return 130;
        }
    }
    
    return 0;  // No margin by default
}
