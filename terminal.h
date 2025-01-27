#ifndef TERMINAL_H
#define TERMINAL_H

/* This file is all platform independent, it contains the logic to build
 * the features that users need in a terminal application.
 *
 * The ANSI escape sequences used here are supported by all terminals (Linux,
 * macOS, Windows). All the functionality here must work on all platforms. The
 * Windows terminal is probably the most limiting, and so we restrict to the
 * capabilities that it supports, as documented at:
 *
 * https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
 */


#include "terminal_base.h"

#include <iostream>
#include <string>
#include <vector>

#define CTRL_KEY(k) ((k)&0x1f)
#define ALT_KEY(k) (k + 128)

namespace Term {

enum class style {
    reset = 0,
    bold = 1,
    dim = 2,
    italic = 3,
    underline = 4,
    blink = 5,
    rblink = 6,
    reversed = 7,
    conceal = 8,
    crossed = 9
};

enum class fg {
    black = 30,
    red = 31,
    green = 32,
    yellow = 33,
    blue = 34,
    magenta = 35,
    cyan = 36,
    gray = 37,
    reset = 39
};

enum class bg {
    black = 40,
    red = 41,
    green = 42,
    yellow = 43,
    blue = 44,
    magenta = 45,
    cyan = 46,
    gray = 47,
    reset = 49
};

enum class fgB {
    black = 90,
    red = 91,
    green = 92,
    yellow = 93,
    blue = 94,
    magenta = 95,
    cyan = 96,
    gray = 97
};

enum class bgB {
    black = 100,
    red = 101,
    green = 102,
    yellow = 103,
    blue = 104,
    magenta = 105,
    cyan = 106,
    gray = 107
};

template <typename T>
std::string color(T const value)
{
    return "\033[" + std::to_string(static_cast<int>(value)) + "m";
}

std::string cursor_off()
{
    return "\x1b[?25l";
}

std::string cursor_on()
{
    return "\x1b[?25h";
}

// If an attempt is made to move the cursor out of the window, the result is
// undefined.
std::string move_cursor(int row, int col)
{
    return "\x1b[" + std::to_string(row) + ";" + std::to_string(col) + "H";
}

// If an attempt is made to move the cursor to the right of the right margin,
// the cursor stops at the right margin.
std::string move_cursor_right(int col)
{
    return "\x1b[" + std::to_string(col) + "C";
}

// If an attempt is made to move the cursor below the bottom margin, the cursor
// stops at the bottom margin.
std::string move_cursor_down(int row)
{
    return "\x1b[" + std::to_string(row) + "B";
}

std::string cursor_position_report()
{
    return "\x1b[6n";
}

std::string erase_to_eol()
{
    return "\x1b[K";
}

enum Key {
    BACKSPACE = 1000,
    ENTER,
    ALT_ENTER,
    TAB,
    ARROW_LEFT,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    CTRL_UP,
    CTRL_DOWN,
    CTRL_RIGHT,
    CTRL_LEFT,
    NUMERIC_5,
    DEL,
    HOME,
    INSERT,
    END,
    PAGE_UP,
    PAGE_DOWN,
    ESC,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
};

class Terminal: public BaseTerminal {
    bool restore_screen_;
public:
    Terminal(bool enable_keyboard=false, bool disable_ctrl_c=true)
        : BaseTerminal(enable_keyboard, disable_ctrl_c),
          restore_screen_{false} {}

    virtual ~Terminal() {
        restore_screen();
    }

    void restore_screen()
    {
        if (restore_screen_) {
            write("\033[?1049l"); // restore screen
            write("\0338"); // restore current cursor position
            restore_screen_ = false;
        }
    }

    void save_screen()
    {
        restore_screen_ = true;
        write("\0337"); // save current cursor position
        write("\033[?1049h"); // save screen
    }

    void write(const std::string& s) const
    {
        std::cout << s << std::flush;
    }


    // Waits for a key press, translates escape codes
    int read_key() const
    {
        int key;
        while ((key = read_key0()) == 0) { }
        return key;
    }

    // If there was a key press, returns the translated key from escape codes,
    // otherwise returns 0. If the escape code is not supported, returns a
    // negative number.
    int read_key0() const
    {
        char c;
        if (!read_raw(&c))
            return 0;

        if (c == '\x1b') {
            char seq[4];

            if (!read_raw(&seq[0]))
                return Key::ESC;
            if (!read_raw(&seq[1])) {
                if (seq[0] >= 'a' && seq[0] <= 'z') {
                    // gnome-term, Windows Console
                    return ALT_KEY(seq[0]);
                }
                if (seq[0] == 13) {
                    // gnome-term
                    return Key::ALT_ENTER;
                }
                return -1;
            }

            if (seq[0] == '[') {
                if (seq[1] >= '0' && seq[1] <= '9') {
                    if (!read_raw(&seq[2])) {
                        return -2;
                    }
                    if (seq[2] == '~') {
                        switch (seq[1]) {
                        case '1':
                            return Key::HOME;
                        case '2':
                            return Key::INSERT;
                        case '3':
                            return Key::DEL;
                        case '4':
                            return Key::END;
                        case '5':
                            return Key::PAGE_UP;
                        case '6':
                            return Key::PAGE_DOWN;
                        case '7':
                            return Key::HOME;
                        case '8':
                            return Key::END;
                        }
                    } else if (seq[2] == ';') {
                        if (seq[1] == '1') {
                            if (!read_raw(&seq[2])) {
                                return -10;
                            }
                            if (!read_raw(&seq[3])) {
                                return -11;
                            }
                            if (seq[2] == '5') {
                                switch (seq[3]) {
                                    case 'A': return Key::CTRL_UP;
                                    case 'B': return Key::CTRL_DOWN;
                                    case 'C': return Key::CTRL_RIGHT;
                                    case 'D': return Key::CTRL_LEFT;
                                }
                            }
                            return -12;
                        }
                    } else {
                        if (seq[2] >= '0' && seq[2] <= '9') {
                            if (!read_raw(&seq[3])) {
                                return -3;
                            }
                            if (seq[3] == '~') {
                                if (seq[1] == '1') {
                                    switch (seq[2]) {
                                    case '5':
                                        return Key::F5;
                                    case '7':
                                        return Key::F6;
                                    case '8':
                                        return Key::F7;
                                    case '9':
                                        return Key::F8;
                                    }
                                } else if (seq[1] == '2') {
                                    switch (seq[2]) {
                                    case '0':
                                        return Key::F9;
                                    case '1':
                                        return Key::F10;
                                    case '3':
                                        return Key::F11;
                                    case '4':
                                        return Key::F12;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    switch (seq[1]) {
                    case 'A':
                        return Key::ARROW_UP;
                    case 'B':
                        return Key::ARROW_DOWN;
                    case 'C':
                        return Key::ARROW_RIGHT;
                    case 'D':
                        return Key::ARROW_LEFT;
                    case 'E':
                        return Key::NUMERIC_5;
                    case 'H':
                        return Key::HOME;
                    case 'F':
                        return Key::END;
                    }
                }
            } else if (seq[0] == 'O') {
                switch (seq[1]) {
                case 'F':
                    return Key::END;
                case 'H':
                    return Key::HOME;
                case 'P':
                    return Key::F1;
                case 'Q':
                    return Key::F2;
                case 'R':
                    return Key::F3;
                case 'S':
                    return Key::F4;
                }
            }

            //std::cout << "Unsupported escape sequence:" << std::endl;
            //std::cout << seq[0] << seq[1] << seq[2] << seq[3] << std::endl;
            return -4;
        } else {
            switch (c) {
            case 9:
                return Key::TAB;
            case 13:
                return Key::ENTER;
            case 127:
                return Key::BACKSPACE;
            }
            if (c == -61) {
                if (!read_raw(&c)) {
                    return -8;
                } else {
                    if (c >= -95 && c <= -70) {
                        // xterm
                        return ALT_KEY(c+'a'-(-95));
                    }
                    return -9;
                }
            } else if (c == -62) {
                if (!read_raw(&c)) {
                    return -10;
                } else {
                    if (c == -115) {
                        // xterm
                        return Key::ALT_ENTER;
                    }
                    return -11;
                }
            }
            return c;
        }
    }

    void get_cursor_position(int& rows, int& cols) const
    {
        char buf[32];
        unsigned int i = 0;
        write(cursor_position_report());
        while (i < sizeof(buf) - 1) {
            while (!read_raw(&buf[i])) {
            };
            if (buf[i] == 'R')
                break;
            i++;
        }
        buf[i] = '\0';
        if (i < 5) {
            throw std::runtime_error("get_cursor_position(): too short response");
        }
        if (buf[0] != '\x1b' || buf[1] != '[') {
            throw std::runtime_error("get_cursor_position(): Invalid response");
        }
        if (sscanf(&buf[2], "%d;%d", &rows, &cols) != 2) {
            throw std::runtime_error("get_cursor_position(): Invalid response");
        }
    }

    // This function takes about 23ms, so it should only be used as a fallback
    void get_term_size_slow(int& rows, int& cols) const
    {
        struct CursorOff {
            const Terminal& term;
            CursorOff(const Terminal& term)
                : term{ term }
            {
                term.write(cursor_off());
            }
            ~CursorOff()
            {
                term.write(cursor_on());
            }
        };
        CursorOff cursor_off(*this);
        int old_row, old_col;
        get_cursor_position(old_row, old_col);
        write(move_cursor_right(999) + move_cursor_down(999));
        get_cursor_position(rows, cols);
        write(move_cursor(old_row, old_col));
    }
};



/*----------------------------------------------------------------------------*/

/*-
 * Copyright (c) 2014 Taylor R Campbell
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#define	UTF8_ACCEPT	0
#define	UTF8_REJECT	0xf

static const uint32_t utf8_classtab[0x10] = {
	0x88888888UL,0x88888888UL,0x99999999UL,0x99999999UL,
	0xaaaaaaaaUL,0xaaaaaaaaUL,0xaaaaaaaaUL,0xaaaaaaaaUL,
	0x222222ffUL,0x22222222UL,0x22222222UL,0x22222222UL,
	0x3333333bUL,0x33433333UL,0xfff5666cUL,0xffffffffUL,
};

static const uint32_t utf8_statetab[0x10] = {
	0xfffffff0UL,0xffffffffUL,0xfffffff1UL,0xfffffff3UL,
	0xfffffff4UL,0xfffffff7UL,0xfffffff6UL,0xffffffffUL,
	0x33f11f0fUL,0xf3311f0fUL,0xf33f110fUL,0xfffffff2UL,
	0xfffffff5UL,0xffffffffUL,0xffffffffUL,0xffffffffUL,
};

static inline uint8_t
utf8_decode_step(uint8_t state, uint8_t octet, uint32_t *cpp)
{
	const uint8_t reject = (state >> 3), nonascii = (octet >> 7);
	const uint8_t class_ = (!nonascii? 0 :
	    (0xf & (utf8_classtab[(octet >> 3) & 0xf] >> (4 * (octet & 7)))));

	*cpp = (state == UTF8_ACCEPT
	    ? (octet & (0xffU >> class_))
	    : ((octet & 0x3fU) | (*cpp << 6)));

	return (reject? 0xf :
	    (0xf & (utf8_statetab[class_] >> (4 * (state & 7)))));
}

/*----------------------------------------------------------------------------*/

void codepoint_to_utf8(std::string &s, char32_t c) {
    int nbytes;
    if (c < 0x80) {
        nbytes = 1;
    } else if (c < 0x800) {
        nbytes = 2;
    } else if (c < 0x10000) {
        nbytes = 3;
    } else if (c <= 0x0010FFFF) {
        nbytes = 4;
    } else {
        throw std::runtime_error("Invalid UTF32 codepoint.");
    }
    char u1, u2, u3, u4;
    static const unsigned char mask[4] = {0x00, 0xC0, 0xE0, 0xF0};
    switch (nbytes) {
        case 4: u4 = ((c | 0x80) & 0xBF); c >>= 6; /* fall through */
        case 3: u3 = ((c | 0x80) & 0xBF); c >>= 6; /* fall through */
        case 2: u2 = ((c | 0x80) & 0xBF); c >>= 6; /* fall through */
        case 1: u1 =  (c | mask[nbytes-1]);
    }
    switch (nbytes) {
        case 1: s.push_back(u1); break;
        case 2: s.push_back(u1); s.push_back(u2); break;
        case 3: s.push_back(u1); s.push_back(u2); s.push_back(u3); break;
        case 4: s.push_back(u1); s.push_back(u2); s.push_back(u3); s.push_back(u4); break;
    }
}

/*----------------------------------------------------------------------------*/

// Converts an UTF8 string to UTF32.
std::u32string utf8_to_utf32(const std::string &s) {
    uint32_t codepoint, state=UTF8_ACCEPT;
    std::u32string r;
    for (size_t i=0; i < s.size(); i++) {
        state = utf8_decode_step(state, s[i], &codepoint);
        if (state == UTF8_ACCEPT) {
            r.push_back(codepoint);
        }
        if (state == UTF8_REJECT) {
            throw std::runtime_error("Invalid byte in UTF8 encoded string");
        }
    }
    if (state != UTF8_ACCEPT) {
        throw std::runtime_error("Expected more bytes in UTF8 encoded string");
    }
    return r;
}


// Converts an UTF32 string to UTF8.
std::string utf32_to_utf8(const std::u32string &s)
{
    std::string r;
    for (size_t i=0; i < s.size(); i++) {
        codepoint_to_utf8(r, s[i]);
    }
    return r;
}




/* Represents a rectangular window, as a 2D array of characters and their
 * attributes. The render method can convert this internal representation to a
 * string that when printed will show the Window on the screen.
 *
 * Note: the characters are represented by char32_t, representing their UTF-32
 * code point. The natural way to represent a character in a terminal would be
 * a "unicode grapheme cluster", but due to a lack of a good library for C++
 * that could handle those, we simply use a Unicode code point as a character.
 */
class Window
{
private:
    size_t x0, y0; // top-left corner of the window on the screen
    size_t w, h; // width and height of the window
    std::vector<char32_t> chars; // the characters in row first order
    std::vector<fg> m_fg;
    std::vector<bg> m_bg;
    std::vector<style> m_style;
public:
    Window(size_t x0, size_t y0, size_t w, size_t h)
        : x0{x0}, y0{y0}, w{w}, h{h}, chars(w*h, ' '),
          m_fg(w*h, fg::reset), m_bg(w*h, bg::reset),
          m_style(w*h, style::reset) {}

    char32_t get_char(int x, int y) {
        return chars[(y-1)*w+(x-1)];
    }

    void set_char(int x, int y, char32_t c) {
        chars[(y-1)*w+(x-1)] = c;
    }

    fg get_fg(int x, int y) {
        return m_fg[(y-1)*w+(x-1)];
    }

    void set_fg(int x, int y, fg c) {
        m_fg[(y-1)*w+(x-1)] = c;
    }

    bg get_bg(int x, int y) {
        return m_bg[(y-1)*w+(x-1)];
    }

    void set_bg(int x, int y, bg c) {
        m_bg[(y-1)*w+(x-1)] = c;
    }

    style get_style(int x, int y) {
        return m_style[(y-1)*w+(x-1)];
    }

    void set_style(int x, int y, style c) {
        m_style[(y-1)*w+(x-1)] = c;
    }

    void print_str(int x, int y, const std::string &s) {
        std::u32string s2 = utf8_to_utf32(s);
        for (size_t i=0; i < s2.size(); i++) {
            size_t xpos = x+i;
            if (xpos < w) {
                set_char(xpos, y, s2[i]);
            } else {
                // String is out of the window
                return;
            }
        }
    }

    void fill_fg(int x1, int y1, int x2, int y2, fg color) {
        for (int j=y1; j <= y2; j++) {
            for (int i=x1; i <= x2; i++) {
                set_fg(i, j, color);
            }
        }
    }

    void fill_bg(int x1, int y1, int x2, int y2, bg color) {
        for (int j=y1; j <= y2; j++) {
            for (int i=x1; i <= x2; i++) {
                set_bg(i, j, color);
            }
        }
    }

    void fill_style(int x1, int y1, int x2, int y2, style color) {
        for (int j=y1; j <= y2; j++) {
            for (int i=x1; i <= x2; i++) {
                set_style(i, j, color);
            }
        }
    }

    void print_border(bool unicode=true) {
        print_rect(1, 1, w, h, unicode);
    }

    void print_rect(int x1, int y1, int x2, int y2, bool unicode=true) {
        std::u32string border = utf8_to_utf32("│─┌┐└┘");
        if (unicode) {
            for (int j=y1+1; j <= y2-1; j++) {
                set_char(x1, j, border[0]);
                set_char(x2, j, border[0]);
            }
            for (int i=x1+1; i <= x2-1; i++) {
                set_char(i, y1, border[1]);
                set_char(i, y2, border[1]);
            }
            set_char(x1, y1, border[2]); set_char(x2, y1, border[3]);
            set_char(x1, y2, border[4]); set_char(x2, y2, border[5]);
        } else {
            for (int j=y1+1; j <= y2-1; j++) {
                set_char(x1, j, '|');
                set_char(x2, j, '|');
            }
            for (int i=x1+1; i <= x2-1; i++) {
                set_char(i, y1, '-');
                set_char(i, y2, '-');
            }
            set_char(x1, y1, '+'); set_char(x2, y1, '+');
            set_char(x1, y2, '+'); set_char(x2, y2, '+');
        }
    }

    void clear() {
        for (size_t j=1; j <= h; j++) {
            for (size_t i=1; i <= w; i++) {
                set_char(i, j, ' ');
                set_fg(i, j, fg::reset);
                set_bg(i, j, bg::reset);
                set_style(i, j, style::reset);
            }
        }
    }

    std::string render() {
        std::string out;
        out.append(cursor_off());
        fg current_fg = fg::reset;
        bg current_bg = bg::reset;
        style current_style = style::reset;
        for (size_t j=1; j <= h; j++) {
            out.append(move_cursor(y0+j-1, x0));
            for (size_t i=1; i <= w; i++) {
                bool update_fg = false;
                bool update_bg = false;
                bool update_style = false;
                if (current_fg != get_fg(i,j)) {
                    current_fg = get_fg(i,j);
                    update_fg = true;
                }
                if (current_bg != get_bg(i,j)) {
                    current_bg = get_bg(i,j);
                    update_bg = true;
                }
                if (current_style != get_style(i,j)) {
                    current_style = get_style(i,j);
                    update_style = true;
                    if (current_style == style::reset) {
                        // style::reset resets fg and bg colors too, we have to
                        // set them again if they are non-default, but if fg or
                        // bg colors are reset, we do not update them, as
                        // style::reset already did that.
                        update_fg = (current_fg != fg::reset);
                        update_bg = (current_bg != bg::reset);
                    }
                }
                // Set style first, as style::reset will reset colors too
                if (update_style) out.append(color(get_style(i,j)));
                if (update_fg) out.append(color(get_fg(i,j)));
                if (update_bg) out.append(color(get_bg(i,j)));
                codepoint_to_utf8(out, get_char(i,j));
            }
        }
        if (current_fg != fg::reset) out.append(color(fg::reset));
        if (current_bg != bg::reset) out.append(color(bg::reset));
        if (current_style != style::reset) out.append(color(style::reset));
        out.append(cursor_on());
        return out;
    }
};

} // namespace Term

#endif // TERMINAL_H
