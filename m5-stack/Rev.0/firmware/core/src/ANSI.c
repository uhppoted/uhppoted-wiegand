// ANSI terminal command strings
//
// Ref. https://www.gnu.org/software/screen/manual/html_node/Control-Sequences.html

const char *TERMINAL_CLEAR = "\033c\033[2J";
const char *TERMINAL_QUERY_CODE = "\033[c";
const char *TERMINAL_QUERY_STATUS = "\033[5n";
const char *TERMINAL_QUERY_SIZE = "\033[s\033[999;999H\033[6n\033[u";
const char *TERMINAL_SET_SCROLL_AREA = "\033[s\033[0;%dr\033[u";
const char *TERMINAL_ECHO = "\033[s\033[%d;0H\033[0K>> %s\033[u";
const char *TERMINAL_CLEARLINE = "\033[s\033[%d;0H\033[0K>> \033[u";
const char *TERMINAL_DISPLAY = "\033[s\033[%d;0H\033[0K>> %s\033[u";
const char *TERMINAL_AT = "\033[s\033[%d;%dH\033[0K%s\033[u";
const char *TERMINAL_UP_ARROW = "\x1b\x5b\x41";
