// VT100 terminal command strings
//
// Ref. https://www.gnu.org/software/screen/manual/html_node/Control-Sequences.html

const char *TERMINAL_CLEAR = "\033c\033[2J";
const char *TERMINAL_QUERY_CODE = "\033[c";
const char *TERMINAL_QUERY_STATUS = "\033[5n";
const char *TERMINAL_QUERY_SIZE = "\0337\033[999;999H\033[6n\0338";
const char *TERMINAL_SET_SCROLL_AREA = "\0337\033[0;%dr\0338";
const char *TERMINAL_ECHO = "\0337\033[%d;0H\033[0K>> %s\0338";
const char *TERMINAL_CLEARLINE = "\0337\033[%d;0H\033[0K>> \0338";
const char *TERMINAL_DISPLAY = "\0337\033[%d;0H\033[0K>> %s\0338";
const char *TERMINAL_AT = "\0337\033[%d;%dH\033[0K%s\0338";
