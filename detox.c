/* detox - Tandy Model 100 BASIC de-tokenize
 * gcc -o detox detox.c
 * ./detox out/bach.ba ver/bach.do
 */
// for f in out/*.ba; do ./detox "$f" "ver/${f##*/}"; done
// for f in out/*.ba; do g="${f%.*}" ; ./detox "$f" "ver/${g##*/}.do"; done

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#define fl_message(...) fprintf (stderr, __VA_ARGS__)

int debug = 0;
#define db_message(...) if (debug) fprintf (stderr, __VA_ARGS__)

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

int offset = 0;
int ilen = 0;
int wlen = 0;

#define MAX_BUF 0x7fff
char inp[MAX_BUF+1];
char out[MAX_BUF+1];

int err_addr = 0;

unsigned char get_memory8(unsigned short address)
{
    unsigned char c = 0xff;
    
    address -= offset;
    if (address >= 0 && address < ilen) 
        c = inp[address];
    else err_addr = address;
    db_message("get_memory8(0x%04x): 0x%02x\n", address, c);
    return c;
}

unsigned short get_memory16(unsigned short address)
{
    return get_memory8(address) + (get_memory8(address + 1) << 8);
}

int detox(unsigned char* in, FILE *fd, unsigned short offset)
{
    unsigned short	addr1, addr2;
    unsigned char	ch;
    unsigned short len = 0;
    unsigned short	line;

    addr1 = offset;
    while (!err_addr && (addr2 = get_memory16(addr1)) != 0)
    {
        addr1 += 2;
        // Print line number
        line = get_memory16(addr1);
        addr1 += 2;
        len += fprintf(fd, "%u ", line);

        // Read remaining data & detokenize
        while (addr1 < addr2)
        {
            // Get next byte
            ch = get_memory8(addr1++);
            
            // Check if byte is ':'
            if (ch == ':')
            {
                // Get next character
                ch = get_memory8(addr1);

                // Check for REM tick
                if (ch == 0x8E)
                {
                    if (get_memory8(addr1+1) == 0xFF)
                    {
                        ch = '\'';
                        len += fwrite(&ch, 1, 1, fd);
                        addr1 += 2;
                        continue;
                    }
                }

                // Check for ELSE
                if (ch == 0x91)
                    continue;

                ch = ':';
                len += fwrite(&ch, 1, 1, fd);
            }
            else if (ch > 0x7F)
            {
                len += fprintf(fd, "%s", gKeywordTable[ch & 0x7F]);
            }
            else if (ch == 0)
            {
                len += fprintf(fd, "\n");
            }
            else
            {
                len += fwrite(&ch, 1, 1, fd);
            }
        }
    }
    return len;
}

char *rname = "stdin";
char *wname = "stdout";

int main(int argc, char **argv)
{
    FILE *rf = NULL, *wf = NULL;

    if (argc > 1) {
        if (!strcmp(argv[1], "-d")) {
            debug = 1;
            ++argv, --argc; // consume argument
        } else if (argv[1][0]=='-') {
            fl_message("usage: detox [-d] [in.ba [out.do]]\n");
            fl_message("  in.ba, out.do  BASIC/TEXT files or stdin/stdout\n");
            fl_message("             -d  debug\n");
            exit(1);
        }
    }
    
    ++argv, --argc;         /* skip over program name */
    if (argc > 0) { 
        rf = fopen(argv[0], "rb"); 
        rname = argv[0]; 
    } else { rf = stdin; }
    if (!rf) { perror(argv[0]); exit(1);  }

    ++argv, --argc;         /* shift args */
    if (argc > 0) { 
        wf = fopen(argv[0], "w+");
        wname = argv[0]; 
    } else { wf = stdout; }
    if (!wf) { perror(argv[0]); exit(1);  }

    ilen = fread(inp, 1, MAX_BUF, rf);
    inp[ilen++] = 0; 
    inp[ilen++] = 0; // append final 0x0000 EOF
    fl_message("input %d bytes from %s\n", ilen, rname);

    offset = get_memory16(0);
    db_message("offset 0x%04x\n", offset);
    offset -= strnlen(inp + 4, ilen) + 5; // 2 offs + 2 lin.num + 1 (0 term)
    db_message("strnlen 0x%04x\n", strnlen(inp + 4, ilen));
    db_message("offset 0x%04x\n", offset);

    wlen = detox(inp, wf, offset);  // 0x8034
    fl_message("written %d bytes to %s\n", wlen, wname);
    fclose(wf);

    if (err_addr) {
        fl_message("ERROR at address 0x%04x\n", err_addr);
    }

    fl_message("\n");
    return 0;
}
