// $Id: screen.cpp 54 2014-03-16 21:45:12Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file screen.cpp

        Classes and functions related to screen management
*/

#include "screen.h"
#include "string_functions.h"

#include <exception>
#include <fstream>
#include <iostream>
#include <string>

#include <cstdlib>

#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

using namespace std;

pt_mutex screen_mutex;

cpair colours;

// -----------  screen  ----------------

/*! \class screen
  \brief A single terminal (e.g., an xterm)
        
  The screen will have the size given by the COLUMNS and LINES environment variables,
  or, in their absence, 80 columns and 25 lines.
*/

/// default constructor
screen::screen(void)
{
  initscr();
  start_color();

  if (!has_colors())
  { cerr << "NO COLOURS" << endl;
    exit(-1);
  }

  refresh();               // clear the screen ready for use

// ncurses does not seem to supply any real control over the look of the cursor.
// I would like a blinking underline, but there seems to be no way to get that.
// TODO: look into this in more depth
  curs_set(1);             // a medium cursor

#if 0
  _sp = newterm(NULL, stdout, stdin);

  if (_sp == NULL)
    throw exception();
   
// get the dimensions
  getmaxyx(stdscr, _height, _width);
  
// turn on colour
  start_color();

// set colour pairs; these are currently stolen from tlf  
  init_pair(COLOR_BLACK, COLOR_BLACK, COLOR_WHITE);
  init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_YELLOW);
  init_pair(COLOR_RED, COLOR_WHITE, COLOR_RED);
  init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_WHITE);
  init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
  init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_WHITE);
  init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_YELLOW);
  init_pair(COLOR_YELLOW, COLOR_WHITE, COLOR_BLACK);  

// turn off echoing
  echo(false);
#endif    // 0

}

/// destructor
screen::~screen(void)
{ endwin();
}

void screen::clear(void)
{ ::clear();
  refresh();
}

/// default constructor
window::window(const unsigned int flags) :
  _x(0),
  _y(0),
  _width(0),
  _height(0),
  _vertical(false),
  _column_width(0),
  _wp(nullptr),
  _scrolling(false),
//  _hidden_cursor(false)
  _hidden_cursor(flags & WINDOW_NO_CURSOR),
  _insert(flags & WINDOW_INSERT),
  _pp(nullptr),
  _process_input(nullptr),
  _fg(COLOUR_WHITE),
  _bg(COLOUR_BLACK)
{  const int pair_nr = colours.add(_fg, _bg);
   default_colours(COLOUR_PAIR(pair_nr));
}

/// create from an x, y, w, h array
window::window(const window_information& wi, const unsigned int flags) :
  _width(wi.w()),
  _height(wi.h()),
  _x(wi.x()),
  _y(wi.y()),
  _vertical(false),
  _column_width(0),
  _wp(nullptr),
  _scrolling(false),
//  _hidden_cursor(false)
  _hidden_cursor(flags & WINDOW_NO_CURSOR),
  _insert(flags & WINDOW_INSERT),
  _pp(nullptr),
  _process_input(nullptr),
  _fg(string_to_colour(wi.fg_colour())),
  _bg(string_to_colour(wi.bg_colour()))
{ if (_width and _height)
  { SAFELOCK(screen);

    _wp = newwin(_height, _width, LINES - _y /* - 1 */ - _height, /*COLS - */_x);
    keypad(_wp, true);

    _pp = new_panel(_wp);

    const int pair_nr = colours.add(_fg, _bg);
    default_colours(COLOUR_PAIR(pair_nr));
  }
}

/// destructor
window::~window(void)
{ if (_pp)
    del_panel(_pp);

  if (_wp)
    delwin(_wp);
}

/// initialize an empty window from an x, y, w, h array
void window::init(const window_information& wi, const unsigned int flags)
{ _x = wi.x();
  _y = wi.y();
  _width = wi.w();
  _height = wi.h();
  _vertical = false;
  _column_width = 0;
  _wp = nullptr;
  _scrolling = false;
//  _hidden_cursor = false;
  _hidden_cursor = (flags & WINDOW_NO_CURSOR);
  _insert = (flags & WINDOW_INSERT);
  _pp = nullptr;
  
  if (_width and _height)
  { SAFELOCK(screen);

    _wp = newwin(_height, _width, LINES - _y /* - 1 */ - _height, /*COLS - */_x);
    keypad(_wp, true);

    _pp = new_panel(_wp);
  }

  _fg = string_to_colour(wi.fg_colour());
  _bg = string_to_colour(wi.bg_colour());

//  ost << "fg colour string = " << wi.fg_colour() << "; _fg = " << _fg << endl;
//  ost << "bg colour string = " << wi.bg_colour() << "; _bg = " << _bg << endl;

  int pair_nr = colours.add(_fg, _bg);

//  ost << "pair_nr = " << pair_nr << endl;
//  ost << "CP(pair_nr) = " << COLOUR_PAIR(pair_nr) << endl;

  default_colours(COLOUR_PAIR(pair_nr));

// clear the window (this also correctly sets the background on the screen)
  (*this) <= WINDOW_CLEAR;
}

// sets to fg/bg *IF* wi.colours_set() is false
void window::init(const window_information& wi, int fg, int bg, const unsigned int flags)
{ _x = wi.x();
  _y = wi.y();
  _width = wi.w();
  _height = wi.h();
  _vertical = false;
  _column_width = 0;
  _wp = nullptr;
  _scrolling = false;
//  _hidden_cursor = false;
  _hidden_cursor = (flags & WINDOW_NO_CURSOR);
  _insert = (flags & WINDOW_INSERT);
  _pp = nullptr;

  if (_width and _height)
  { SAFELOCK(screen);

    _wp = newwin(_height, _width, LINES - _y /* - 1 */ - _height, /*COLS - */_x);
    keypad(_wp, true);

    _pp = new_panel(_wp);
  }

  if (wi.colours_set())
  { _fg = string_to_colour(wi.fg_colour());
    _bg = string_to_colour(wi.bg_colour());
  }
  else
  { _fg = fg;
    _bg = bg;
  }

//  ost << "fg colour string = " << wi.fg_colour() << "; _fg = " << _fg << endl;
//  ost << "bg colour string = " << wi.bg_colour() << "; _bg = " << _bg << endl;

  int pair_nr = colours.add(_fg, _bg);

//  ost << "pair_nr = " << pair_nr << endl;
//  ost << "CP(pair_nr) = " << COLOUR_PAIR(pair_nr) << endl;

  default_colours(COLOUR_PAIR(pair_nr));

// clear the window (this also correctly sets the background on the screen)
  (*this) <= WINDOW_CLEAR;
}

/// move cursor
window& window::move_cursor(const int new_x, const int new_y)
{ if (!_wp)
    return *this;
    
  { SAFELOCK(screen);

    wmove(_wp, _limit(_height - new_y - 1, 0, _height - 1), _limit(new_x, 0, _width - 1));
  }
  
  return *this;
}

window& window::move_cursor(const cursor& c)
{ return move_cursor(c.x(), c.y()); 
}

/*! \brief  write a string to the window
    \param  s   string to write
    \return the window

    wprintw has fairly obnoxious behaviour regarding newlines: if a string
    reaches the end of a window line, then an LF is automatically added. For
    now we live with this, but we might want at some time to write a more complex
    function that performs the writes without ever (silently) adding something
    to the string
*/
window& window::operator<(const string& s)
{ if (!_wp)
    return *this;
    
  SAFELOCK(screen);

  if (!_insert)
  { //SAFELOCK(screen);
 
    wprintw(_wp, s.c_str());
  }
  else    // insert mode
  { //SAFELOCK(screen);

    const cursor c = cursor_position();
    const string remainder = read(c.x(), c.y());

    wprintw(_wp, (s + remainder).c_str());
    
    *this < cursor(c.x() + s.length(), c.y());
  }
  
  return *this;
}

/*
/// write a vector of strings to a window
window& window::operator<(const std::vector<std::string>& vec)
{ 
// ****************************** HERE ***************************
// start with the simplest case
  for (unsigned int n = 0; n < vec.size(); ++n)
  { *this < vec[n];
  
    if (n != vec.size() - 1)
      *this < " ";
  }
  
  return *this;
}
*/

/// write a set of strings to a window
window& window::operator<(const set<string>& vec)
{ if (!_wp)
    return *this;

  vector<string> v;
    for (set<string>::const_iterator cit = vec.begin(); cit != vec.end(); ++cit)
      v.push_back(*cit);

  sort(v.begin(), v.end(), compare_calls);

  unsigned int idx = 0;
  
  for (vector<string>::const_iterator cit = v.begin(); cit != v.end(); ++cit)
  { 
// see if there's enough room on this line    
    cursor_position();
    const int remaining_space = width() - _cursor_x;

 // stop writing if there's insufficient room for the next string
    if (remaining_space < static_cast<int>(cit->length()))
      if (!scrolling() and (_cursor_y == 0))
        break;
    
    if (remaining_space < static_cast<int>(cit->length()))
      *this < "\n";

    *this < *cit;
  
// add space unless we're at the end of a line or this is the last string
    const bool end_of_line = (remaining_space == static_cast<int>(cit->length()));
    
    if ((idx != v.size() - 1) and !end_of_line)
      *this < " ";
    
    idx++;
  }
  
//  this->cpair(4);
//  *this < colours(COLOUR_GREEN, COLOUR_BLACK);
//  this->cpair(FGBG(COLOUR_GREEN, COLOUR_BLACK));

//  const int n = COLOUR_PAIR(colours.add(COLOUR_GREEN, COLOUR_BLACK));

//  ost << "colour pair = " << n << endl;

//  const int pair_nr = colours.add(_fg, _bg);
//  default_colours(COLOUR_PAIR(pair_nr));


//  this->cpair(colours.add(COLOUR_GREEN, COLOUR_BLACK));

  *this < COLOURS(COLOUR_GREEN, COLOUR_BLACK);
//  *this < " burble1 " < " burble2 ";
//  this->cpair(colours.add(_fg, _bg));
  *this < COLOURS(_fg, _bg);

//  *this < colours(_fg, _bg);
  return *this;
}

/// write a vector of strings to a window
window& window::operator<(const vector<string>& v)
{ if (!_wp)
    return *this;

  unsigned int idx = 0;

  for (vector<string>::const_iterator cit = v.begin(); cit != v.end(); ++cit)
  {
// see if there's enough room on this line
    cursor_position();
    const int remaining_space = width() - _cursor_x;

 // stop writing if there's insufficient room for the next string
    if (remaining_space < static_cast<int>(cit->length()))
      if (!scrolling() and (_cursor_y == 0))
        break;

    if (remaining_space < static_cast<int>(cit->length()))
      *this < "\n";

    *this < *cit;

// add space unless we're at the end of a line or this is the last string
    const bool end_of_line = (remaining_space == static_cast<int>(cit->length()));

    if ((idx != v.size() - 1) and !end_of_line)
      *this < " ";

    idx++;
  }

//  this->cpair(4);
//  *this < colours(COLOUR_GREEN, COLOUR_BLACK);
//  this->cpair(FGBG(COLOUR_GREEN, COLOUR_BLACK));

//  const int n = COLOUR_PAIR(colours.add(COLOUR_GREEN, COLOUR_BLACK));

//  ost << "colour pair = " << n << endl;

//  const int pair_nr = colours.add(_fg, _bg);
//  default_colours(COLOUR_PAIR(pair_nr));


//  this->cpair(colours.add(COLOUR_GREEN, COLOUR_BLACK));

  *this < COLOURS(COLOUR_GREEN, COLOUR_BLACK);
//  *this < " burble1 " < " burble2 ";
//  this->cpair(colours.add(_fg, _bg));
  *this < COLOURS(_fg, _bg);

//  *this < colours(_fg, _bg);
  return *this;
}

/// write a vector of strings with possible different colours
window& window::operator<(const std::vector<std::pair<std::string /* callsign */, int /* colour pair number */ > >& vec)
{ if (!_wp)
    return *this;

  unsigned int idx = 0;

  for (vector<pair<string, int> >::const_iterator cit = vec.begin(); cit != vec.end(); ++cit)
  { const string& callsign = cit->first;
    const int& cp = cit->second;

// see if there's enough room on this line
    cursor_position();
    const int remaining_space = width() - _cursor_x;

 // stop writing if there's insufficient room for the next string
    if (remaining_space < static_cast<int>(callsign.length()))
      if (!scrolling() and (_cursor_y == 0))
        break;

    if (remaining_space < static_cast<int>(callsign.length()))
      *this < "\n";

    this->cpair(cp);
    *this < callsign;
    *this < COLOURS(_fg, _bg);    // back to default colours

// add space unless we're at the end of a line or this is the last string
    const bool end_of_line = (remaining_space == static_cast<int>(callsign.length()));

    if ((idx != vec.size() - 1) and !end_of_line)
      *this < " ";

    idx++;
  }

//  this->cpair(4);
//  *this < colours(COLOUR_GREEN, COLOUR_BLACK);
//  this->cpair(FGBG(COLOUR_GREEN, COLOUR_BLACK));

//  const int n = COLOUR_PAIR(colours.add(COLOUR_GREEN, COLOUR_BLACK));

//  ost << "colour pair = " << n << endl;

//  const int pair_nr = colours.add(_fg, _bg);
//  default_colours(COLOUR_PAIR(pair_nr));


//  this->cpair(colours.add(COLOUR_GREEN, COLOUR_BLACK));

//  *this < COLOURS(COLOUR_GREEN, COLOUR_BLACK);
//  *this < " burble1 " < " burble2 ";
//  this->cpair(colours.add(_fg, _bg));
//  *this < COLOURS(_fg, _bg);

//  *this < colours(_fg, _bg);
  return *this;
}

/// get cursor position
const cursor window::cursor_position(void)
{ if (!_wp)
    return cursor(0, 0);
    
  { SAFELOCK(screen);  

    getyx(_wp, _cursor_y, _cursor_x);
  }

  _cursor_y = height() - _cursor_y - 1;
  
  return cursor(_cursor_x, _cursor_y);
}

/// move cursor relative
window& window::move_cursor_relative(const int delta_x, const int delta_y)
{ if (!_wp)
    return *this;
    
  cursor_position();

  move_cursor(_cursor_x + delta_x, _cursor_y + delta_y);
  return *this;
}

/// set the colour pair
window& window::cpair(const int pair_nr)
{ if (!_wp)
    return *this;
    
  { SAFELOCK(screen);  

    wcolor_set(_wp, pair_nr, NULL);
  }
  
  return *this;
}

/// set the background (which also sets the foreground!!
/// bg is actually a number that is both foreground and background
window& window::default_colours(chtype bg)
{ if (!_wp)
    return *this;
    
  { SAFELOCK(screen);  

    wbkgd(_wp, bg);
  }
  
  return *this;
}

/// set the default colours
window& window::default_colours(const int foreground_colour, const int background_colour)
{ if (!_wp)
    return *this;

  _fg = foreground_colour;
  _bg = background_colour;

  return default_colours(FGBG(foreground_colour, background_colour));
}


/// control an attribute or perform a simple operation
window& window::operator<(const enum WINDOW_ATTRIBUTES wa)
{ if (!_wp)
    return *this;
    
  switch (wa)
  { case WINDOW_NORMAL :
      { SAFELOCK(screen);
        wstandend(_wp);
//        wattr_on(_wp, WA_NORMAL, NULL);
      }
      break;
    case WINDOW_BOLD :
      { SAFELOCK(screen);
        wattr_on(_wp, WA_BOLD, NULL);
      }
      break;
    case WINDOW_HIGHLIGHT :
      { SAFELOCK(screen);
        wattr_on(_wp, WA_STANDOUT, NULL);
      }
      break;
    case WINDOW_DIM :
      { SAFELOCK(screen);
        wattr_on(_wp, WA_DIM, NULL);
      }
      break;
    case WINDOW_REVERSE :
      { SAFELOCK(screen);
        wattr_on(_wp, WA_REVERSE, NULL);
      }
      break;
    case WINDOW_REFRESH :
    case WINDOW_UPDATE :
      { SAFELOCK(screen);
//        wrefresh(_wp);
        refresh();
      }
      break;
    case CURSOR_TOP_LEFT :
    case WINDOW_TOP_LEFT :    
      move_cursor(0, height() - 1);
      break;
    case CURSOR_TOP_RIGHT :
    case WINDOW_TOP_RIGHT :
      move_cursor(width() - 1, height() - 1);
      break;
    case CURSOR_BOTTOM_LEFT :
    case WINDOW_BOTTOM_LEFT :
      move_cursor(0, 0);
      break;
    case CURSOR_BOTTOM_RIGHT :
    case WINDOW_BOTTOM_RIGHT :
      move_cursor(width() - 1, 0);
      break;
    case WINDOW_CLEAR :
      clear();
      break;
    case WINDOW_CLEAR_TO_EOL :
      { SAFELOCK(screen);
        wclrtoeol(_wp);
      }
      break;
    case WINDOW_CLEAR_TO_END :
      { SAFELOCK(screen);
        wclrtobot(_wp);
      }
      break;  
    case CURSOR_START_OF_LINE :
      move_cursor(0, cursor_position().y());
      break;  
    case CURSOR_UP :
      move_cursor_relative(0, 1);
      break;
    case CURSOR_DOWN :
      move_cursor_relative(0, -1);
      break;  
    case WINDOW_SCROLL_UP :
      scrollit(1);
      break; 
    case WINDOW_SCROLL_DOWN :
      scrollit(-1);
      break;
    case CURSOR_HIDE :
      _hidden_cursor = true;
      break;
    case CURSOR_END_OF_LINE :
    	{ const size_t posn = read().find_last_not_of(" ");
    	  move_cursor(posn + 1, cursor_position().y());
        break;
    	}

  }
  return *this;
}

/// clear a window
window& window::clear(void)
{ if (!_wp)
    return *this;
    
  { SAFELOCK(screen);

    werase(_wp);
  }

  return *this;
}

/// refresh
window& window::refresh(void)
{ if (!_wp)
    return *this;
    
  { SAFELOCK(screen);

    if (_hidden_cursor)
      getsyx(_sy, _sx);   

    wrefresh(_wp);

    if (_hidden_cursor)
    { setsyx(_sy, _sx);
      doupdate();
    }
  }

  return *this;
}

/// control scrolling
window& window::scrolling(const bool enable_or_disable)
{ if (!_wp)
    return *this;
    
  _scrolling = enable_or_disable;
  
  SAFELOCK(screen);
//  SAVE_CURSOR(this);

  scrollok(_wp, enable_or_disable);
  
  return *this;
}

/// scroll
window& window::scrollit(const int n_lines)
{ if (!_wp)
    return *this;
    
  SAFELOCK(screen);
//  SAVE_CURSOR(this);

  wscrl(_wp, n_lines);
  
  return *this;
}

/// control leaveok
window& window::leave_cursor(const bool enable_or_disable)
{ if (!_wp)
    return *this;
    
  _leaveok = enable_or_disable;
  
  SAFELOCK(screen);
//  SAVE_CURSOR(this);

  leaveok(_wp, enable_or_disable);
  
  return *this;
}

/// change to a defined colour pair
//window& window::operator<(const enum COLOUR_PAIR_USES cp)
//{ return cpair(static_cast<int>(cp));
//}

window& operator<(window& win, const cursor& c)
{ return win.move_cursor(c.x(), c.y());
}

window& operator<(window& win, const cursor_relative& cr)
{ return win.move_cursor_relative(cr.x(), cr.y());
}

window& operator<(window& win, const centre& c)
{ win.move_cursor((win.width() - c.s().length()) / 2, c.y());
  return win < c.s();
}

/// read window
const string window::read(int x, int y)
{ if (!_wp)
    return string();
  
  char tmp[500];
  tmp[0] = '\0';
  
  SAFELOCK(screen);
  
// we need to save the current logical cursor position in the window, because mvwinstr() silently moves it
  const cursor c = cursor_position();
  const int n_chars = mvwinstr(_wp,  _limit(_height - y - 1, 0, _height - 1),  _limit(x, 0, _width - 1), tmp);
  move_cursor(c.x(), c.y());          // restore the logical cursor position
  
//  ost << "n_chars = " << n_chars << endl;
  if (n_chars != ERR)
    tmp[n_chars] = '\0';   // in theory, not necessary

  return string(tmp);
}

/// snapshot of all the contents
const vector<string> window::snapshot(void)
{ vector<string> rv;

//  ost << "height(): = " << static_cast<int>(height());

  for (size_t n = height() - 1; n < height(); --n)
    rv.push_back(getline(n));

  return rv;
}

/// clear a line
window& window::clear_line(const int line_nr)
{ if (!_wp)
    return *this;

  SAFELOCK(screen);

  const cursor c = cursor_position();

  move_cursor(0, line_nr);

  (*this) <= WINDOW_CLEAR_TO_EOL;

  move_cursor(c.x(), c.y());          // restore the logical cursor position

  return *this;
}

/// delete a character
/*
window& window::delete_character(const int n)
{ if (!_wp)
    return *this; 
    
  SAFELOCK(screen);
    
  const string current_contents = read();
  if (n < 0 or n >= static_cast<int>(current_contents.length()))
    return *this;
  
  const cursor c = cursor_position(); 
  const string new_contents = substring(current_contents, 0, n) + substring(current_contents, n + 1);
  *this < CURSOR_START_OF_LINE;
  
// ensure we are not in insert mode
  const bool insert_enabled = _insert;
  _insert = false;
  *this < new_contents;
  _insert = insert_enabled;         // restore old insert mode
  move_cursor(c);
  
  if (c.x() > n)
    move_cursor_relative(-1, 0);
  
  return *this;
}
*/

/// delete a character within the current line
window& window::delete_character(const int n)
{ if (!_wp)
    return *this;

  SAFELOCK(screen);    // ensure that nothing changes

//  const int line_nr = cursor_position().y();

  return delete_character( n, cursor_position().y() );
}

/// delete a character within a line
window& window::delete_character(const int n, const int line_nr)
//#if 0
//window& window::delete_char( const int n, const int line_nr )
{ if (!_wp)
    return *this;

  if ( (line_nr < 0) or (line_nr >= height()) )
    return *this;

// make sure nothing changes from this point onwards
  SAFELOCK(screen);

  const string old_line = getline(line_nr);

  if (old_line.empty())
    return *this;
  
  if (n >= old_line.length())
    return *this;

  const string new_line = old_line.substr(0, n) + old_line.substr(n + 1);
  const cursor c = cursor_position();

  move_cursor(0, line_nr);

// ensure we are not in insert mode
  { SAFELOCK(screen);

    const bool insert_enabled = _insert;

    _insert = false;
    (*this) < WINDOW_CLEAR_TO_EOL < new_line;
    _insert = insert_enabled;         // restore old insert mode
  }

  move_cursor(c);                   // restore the logical cursor position
  move_cursor_relative(-1, 0);      // move cursor to the left

  return *this;
}
//#endif

/// hide the window
void window::hide(void)
{ if (_pp)
  { SAFELOCK(screen);

    hide_panel(_pp);
  }
}

/// show the window
void window::show(void)
{ if (_pp)
  { SAFELOCK(screen);

    show_panel(_pp);
  }
}

/// is the panel hidden?
const bool window::hidden(void) const
{ if (_pp)
    return static_cast<bool>(panel_hidden(_pp));

  return false;                // default
}

// returns whether the event has been processed
const bool window::common_processing(const keyboard_event& e)    // processing that is the same in multiple windows
{ window& win = (*this);

//  ost << "Inside common_processing" << endl;
//  ost << "AND mask = " << (e.xkey_state() & (ShiftMask | ControlMask | Mod1Mask)) << endl;

// a..z A..Z
  if (e.is_letter())
  { const string s = to_upper(e.str());

    win <= s;
    return true;
  }

// 0..9
  if (e.is_digit())
  { win <= e.str();
    return true;
  }

// BACKSPACE
  if (e.is_unmodified() and e.symbol() == XK_BackSpace)
  { win.delete_character(win.cursor_position().x() - 1);
    win.refresh();
    return true;
  }

// DELETE
  if (e.is_unmodified() and e.symbol() == XK_Delete)
  { win.delete_character(win.cursor_position().x());
    win.refresh();
    return true;
  }

// END
  if (e.is_unmodified() and e.symbol() == XK_End)
  { const string contents = win.read();
    const size_t posn = contents.find_last_not_of(" ");

    win <= cursor(posn + 1, 0);
    return true;
  }

// HOME
  if (e.is_unmodified() and e.symbol() == XK_Home)
  { win <= CURSOR_START_OF_LINE;
    return true;
  }

 // CURSOR LEFT
  if (e.is_unmodified() and e.symbol() == XK_Left)
  { win <= cursor_relative(-1, 0);
    return true;
  }

// CURSOR RIGHT
  if (e.is_unmodified() and e.symbol() == XK_Right)
  { win <= cursor_relative(1, 0);     // ncurses fills the window with spaces; we can't differentiate between those and any we might have added ourself
    return true;
  }

// INSERT -- toggle insert mode
  if (e.is_unmodified() and e.symbol() == XK_Insert)
  { win.insert(!win.insert());
    return true;
  }

  return false;
}

/// utterly trivial class for changing colour to a colour pair
window& operator<(window& win, const colour_pair& cpair)
{ return win.cpair(cpair.pair_nr());
}


SAVE_CURSOR::SAVE_CURSOR(window& w) :
  _wp(&w)
{ if (_wp->hidden_cursor())
  { //getsyx(_y, _x); 
    
//    ost << "Saved cursor at : " << _y << "; " << _x << endl;
  }
}

SAVE_CURSOR::SAVE_CURSOR(window* wp) :
  _wp(wp)
{ if (_wp->hidden_cursor())
  { //getsyx(_y, _x); 
    
//    ost << "Saved cursor at : " << _y << "; " << _x << endl;
  }
}

SAVE_CURSOR::~SAVE_CURSOR(void)
{ if (_wp->hidden_cursor())
  { //setsyx(_y, _x);

//    ost << "Restored cursor at : " << _y << "; " << _x << endl;
    //doupdate();
  }
}

// -----------  colour_pair  ----------------
// return the pair number
// note the pair number 0 cannot be changed, so we ignore it here and start counting from one
const int cpair::add(const int fg, const int bg)
{ const pair<int, int> fgbg = make_pair(fg, bg);

  SAFELOCK(_colours);

  if (_colours.empty())
  { _colours.push_back(make_pair(fg, bg));
    init_pair(_colours.size(), fg, bg);
    return _colours.size();
  }

//  const vector< pair<int, int> >::iterator it = find(_colours.begin(), _colours.end(), fgbg);
  const auto cit = find(_colours.cbegin(), _colours.cend(), fgbg);

 // ost << "trying to add pair: " << fg << ", " << bg << endl;

  if (cit == _colours.cend())
  { //ost << "colour pair not found; adding it" << endl;

    _colours.push_back(make_pair(fg, bg));
    init_pair(_colours.size(), fg, bg);

    //ost << "added as pair number " << _colours.size() << endl;

    return _colours.size();
  }
  else
  { //ost << "colour pair found as pair number " << (it - _colours.begin()) << endl;
    //ost << "colour pair found" <<  endl;
    return (cit - _colours.cbegin() + 1);    // the first entry (at _colours.begin()) is ncurses colour pair number 1

  }
}

const int cpair::fg(const int pair_nr)
{ short f;
  short b;

  pair_content(pair_nr, &f, &b);

  return f;
}

const int cpair::bg(const int pair_nr)
{ short f;
  short b;

  pair_content(pair_nr, &f, &b);

  return b;
}

// -----------  log_window  ----------------

/*!     \class log_window
        \brief A special class for the log window
*/

#include "qso.h"

void log_window::operator<(QSO& qso)
{ (*this) < CURSOR_BOTTOM_LEFT < WINDOW_SCROLL_UP <= qso.log_line();
}

// convert the name of a colour to a colour
const int string_to_colour(const string& str)
{ static const map<string, int> colour_map { make_pair("BLACK",   COLOUR_BLACK),
                                             make_pair("BLUE",    COLOUR_BLUE),
                                             make_pair("CYAN",    COLOUR_CYAN),
                                             make_pair("GREEN",   COLOUR_GREEN),
                                             make_pair("MAGENTA", COLOUR_MAGENTA),
                                             make_pair("RED",     COLOUR_RED),
                                             make_pair("WHITE",   COLOUR_WHITE),
                                             make_pair("YELLOW",  COLOUR_YELLOW)
                                           };

  const string s = to_upper(remove_peripheral_spaces(str));
  auto cit = colour_map.find(s);

  if (cit != colour_map.cend())
    return cit->second;

// should change this so it works with a colour name and not just a number

  if (begins_with(s, "COLOUR_"))
    return (from_string<int>(substring(s, 7)));

  if (begins_with(s, "COLOR_"))
    return (from_string<int>(substring(s, 6)));

  bool found_non_digit = false;
  unsigned int index = 0;

  while (!found_non_digit and index < s.length())
    found_non_digit = !isdigit(s[index++]);

  if (!found_non_digit)
    return from_string<int>(s);

  return COLOUR_BLACK;
}
