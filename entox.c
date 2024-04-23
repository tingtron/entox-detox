/* entox - Tandy Model 100 BASIC tokenize
 * gcc -o entox entox.c
 * ./entox out/bach.do ver/bach.ba
 */
// for f in inp/*.ba; do ./entox "$f" "out/${f##*/}"; done
// for f in inp/*.do; do g="${f%.*}" ; ./entox "$f" "out/${g##*/}.ba"; done

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#define fl_message(...) fprintf (stderr, __VA_ARGS__)

int debug = 0;
#define db_message(...) if (debug) fprintf (stderr, __VA_ARGS__)

const char          *gIllformedBasic = "Ill formed BASIC file";

const char *gKeywordTable[] = {
    "END",    "FOR",   "NEXT",  "DATA",    "INPUT", "DIM",    "READ",   "LET",
    "GOTO",   "RUN",   "IF",    "RESTORE", "GOSUB", "RETURN", "REM",    "STOP",
    "WIDTH",  "ELSE",  "LINE",  "EDIT",    "ERROR", "RESUME", "OUT",    "ON",
    "DSKO$",  "OPEN",  "CLOSE", "LOAD",    "MERGE", "FILES",  "SAVE",   "LFILES",
    "LPRINT", "DEF",   "POKE",  "PRINT",   "CONT",  "LIST",   "LLIST",  "CLEAR",
    "CLOAD",  "CSAVE", "TIME$", "DATE$",   "DAY$",  "COM",    "MDM",    "KEY",
    "CLS",    "BEEP",  "SOUND", "LCOPY",   "PSET",  "PRESET", "MOTOR",  "MAX",
    "POWER",  "CALL",  "MENU",  "IPL",     "NAME",  "KILL",   "SCREEN", "NEW",

// ==========================================
// Function keyword table TAB to <
// ==========================================
    "TAB(",   "TO",    "USING", "VARPTR",  "ERL",   "ERR",    "STRING$","INSTR",
    "DSKI$",  "INKEY$","CSRLIN","OFF",     "HIMEM", "THEN",   "NOT",    "STEP",
    "+",      "-",     "*",     "/",       "^",     "AND",    "OR",     "XOR",
    "EQV",    "IMP",   "MOD",   "\\",      ">",     "=",      "<",

// ==========================================
// Function keyword table SGN to MID$
// ==========================================
    "SGN",    "INT",   "ABS",   "FRE",     "INP",   "LPOS",   "POS",    "SQR",
    "RND",    "LOG",   "EXP",   "COS",     "SIN",   "TAN",    "ATN",    "PEEK",
    "EOF",    "LOC",   "LOF",   "CINT",    "CSNG",  "CDBL",   "FIX",    "LEN",
    "STR$",   "VAL",   "ASC",   "CHR$",    "SPACE$","LEFT$",  "RIGHT$", "MID$",

// End of table marker
    ""
};

/*
=======================================================
This is the tokenizer that converts ASCII mode BASIC files
to actual .BA files in the Model 100/102/200 format.
=======================================================
*/
#define STATE_LINENUM   1
#define STATE_WHITE     2
#define STATE_TOKENIZE  3
#define STATE_CR        4

int tokenize(unsigned char* in, unsigned char* out, unsigned short addr)
{
    int             state;
    int             c, x;
    unsigned short  line_num;
    int             line_num_len;
    unsigned char   tok_line[256];
    int             line_len;
    unsigned char   token;
    int             tok_len;
    int             out_len;
    int             last_line_num;
    int             next_line_offset;

    c = 0;
    line_num = 0;
    line_num_len = 0;
    line_len = 0;
    out_len = 0;
    last_line_num = -1;
    next_line_offset = 0;
    state = STATE_LINENUM;

    // Initialize out array with a blank line
    out[0] = 0;
    out[1] = 0;

    // Parse until end of string
    while ((in[c] != 0) && (in[c] != 0x1A))
    {
        switch (state)
        {
        // Read linenumber until non numeric
        case STATE_LINENUM:
            if ((in[c] < '0') || (in[c] > '9'))
            {
                // Check for blank lines
                if ((in[c] == 0x0d) || (in[c] == 0x0a))
                {
                    // Skip this character
                    c++;
                }
                // Check for characters with no line number
                else if (line_num_len == 0)
                {
                    fl_message("%s", gIllformedBasic);
                    return 0;
                }

                // First space not added to output - skip it
                if (in[c] == ' ')
                    c++;

                // Line number found, go to tokenize state
                state = STATE_TOKENIZE;
            }
            else
            {
                // Add byte to line number
                line_num = line_num * 10 + in[c] - '0';
                line_num_len++;
                c++;
            }
            break;

        case STATE_TOKENIZE:
            // Check for '?' and assign to PRINT
            if (in[c] == '?')
            {
                tok_line[line_len++] = 0xA3;
                c++;
            }
            // Check for REM tick
            else if (in[c] == '\'')
            {
                tok_line[line_len++] = ':';
                tok_line[line_len++] = 14 | 0x80;
                c++;
                tok_line[line_len++] = 0xFF;

                // Copy bytes to token line until 0x0D or 0x0A
                while ((in[c] != 0x0d) && (in[c] != 0x0a)
                    && (in[c] != 0))
                {
                    tok_line[line_len++] = in[c++];
                }
            }
            // Check for characters greater than 127 and skip
            else if ((in[c] > 127) || (in[c] == 0x0d))
            {
                c++;
            }
            // Check for quote
            else if (in[c] == '"')
            {
                tok_line[line_len++] = in[c++];
                // Copy bytes to token line until quote ended
                while ((in[c] != '"') && (in[c] != 0x0d) && (in[c] != 0x0a)
                    && (in[c] != 0))
                {
                    tok_line[line_len++] = in[c++];
                }

                // Add trailing quote to tok_line
                if (in[c] == '"')
                    tok_line[line_len++] = in[c++];
            }
            // Check for end of line
            else if (in[c] == 0x0a)
            {
                // Skip this byte - don't add it to the line
                c++;

                if (line_len != 0)
                {
                    // Add line to output
                    if (line_num > last_line_num)
                    {
                        // Add line to end of output
                        out[next_line_offset]   = (addr + next_line_offset + 5 + line_len) & 0xFF;
                        out[next_line_offset+1] = (addr + next_line_offset + 5 + line_len) >> 8;
                        next_line_offset += 2;

                        // Add line number
                        out[next_line_offset++] = line_num & 0xFF;
                        out[next_line_offset++] = line_num >> 8;

                        // Add tokenized line
                        for (x = 0; x < line_len; x++)
                            out[next_line_offset++] = tok_line[x];

                        // Add terminating zero
                        out[next_line_offset++] = 0;

                        // Add 0x0000 as next line number offset
                        out[next_line_offset]   = 0;
                        out[next_line_offset+1] = 0;

                        // Update out_len to reflect current length
                        out_len += line_len + 5;

                        // Update last_line_num
                        last_line_num = line_num;

                    }
                    else
                    {
                        // Add line somewhere in the middle

                        // NEED TO ADD CODE HERE!!!!!
                    }
                }
                state = STATE_LINENUM;
                line_len = 0;
                line_num = 0;
            }
            // Add '0' through ';' to tokenized line "as-is"
            else if ((in[c] >= '0') && (in[c] <= ';'))
            {
                tok_line[line_len++] = in[c++];
            }
            else
            {
                // Search token table for match
                for (token = 0; strlen(gKeywordTable[token]) != 0; token++)
                {
                    tok_len = strlen(gKeywordTable[token]);
                    // Compare next Keyword with input
                    if (strncmp(gKeywordTable[token], (char *) &in[c], 
                        tok_len) == 0)
                    {
                        // Add ':' prior to ELSE if not already there
                        if ((token == 17) && (tok_line[line_len-1] != ':'))
                        {
                            tok_line[line_len++] = ':';
                        }
                        tok_line[line_len++] = token + 0x80;
                        c += tok_len;

                        // If REM token, add bytes to end of line 
                        if (token == 14)
                        {
                            // Copy bytes to token line until 0x0D or 0x0A
                            while ((in[c] != 0x0d) && (in[c] != 0x0a)
                                && (in[c] != 0))
                            {
                                tok_line[line_len++] = in[c++];
                            }
                        }

                        // If DATA token, add bytes to EOL or ':'
                        if (token == 3)
                        {
                            // Copy bytes to token line until 0x0D or 0x0A
                            while ((in[c] != ':') && (in[c] != 0x0d) && (in[c] != 0x0a)
                                && (in[c] != 0))
                            {
                                tok_line[line_len++] = in[c++];
                            }
                        }
                        break;
                    }
                }

                // Check if token not found - add byte directly
                if (token == 0x7F)
                {
                    tok_line[line_len++] = in[c++];
                }
            }
            break;
        }
    }

    return out_len + 2;
}

char *rname = "stdin";
char *wname = "stdout";

#define MAX_BUF 0x7fff
char inp[MAX_BUF+1];
char out[MAX_BUF+1];

int main(int argc, char **argv)
{
    FILE *rf = NULL, *wf = NULL;
    size_t len = 0;
    size_t ilen, wlen;

    if (argc > 1) {
        if (!strcmp(argv[1], "-d")) {
            debug = 1;
            ++argv, --argc; // consume argument
        } else if (argv[1][0]=='-') {
            fl_message("usage: entox [-d] [in.do [out.ba]]\n");
            fl_message("  in.do, out.ba  TEXT/BASIC files or stdin/stdout\n");
            fl_message("             -d  debug\n");
            exit(1);
        }
    }

    ++argv, --argc;         /* skip over program name */
    if (argc > 0) { 
        rf = fopen(argv[0], "r"); 
        rname = argv[0]; 
    } else { rf = stdin; }
    if (!rf) { perror(argv[0]); exit(1);  }

    ++argv, --argc;         /* shift args */
    if (argc > 0) { 
        wf = fopen(argv[0], "wb+");
        wname = argv[0]; 
    } else { wf = stdout; }
    if (!wf) { perror(argv[0]); exit(1);  }

    ilen = fread(inp, 1, MAX_BUF, rf);
    inp[ilen] = 0;
    fl_message("input %d bytes from %s\n", ilen, rname);

    len = tokenize(inp, out, 0);  // 0x8034
    len = len - 2;
    db_message("tokenized %d bytes\n", len);

    wlen = fwrite(out, 1, len, wf);
    if (wf == stdout) fl_message("\n");
    fl_message("written %d bytes to %s\n", wlen, wname);
    fclose(wf);

    fl_message("\n");
    return 0;
}
