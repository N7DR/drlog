// $Id: screen.cpp 101 2015-04-04 01:49:14Z  $

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

pt_mutex screen_mutex;          ///< mutex for access to screen

cpair colours;                  ///< global repository for information about colour pairs

// -----------  screen  ----------------

/*! \class screen
    \brief A single terminal (e.g., an xterm)
        
    The screen will have the size given by the COLUMNS and LINES environment variables,
    or, in their absence, 80 columns and 25 lines.
*/

/// default constructor
screen::screen(void)
{ setlocale(LC_ALL, "");  // for wide characters; see http://www.roguebasin.com/index.php?title=Ncursesw
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
}

/// destructor
screen::~screen(void)
{ endwin();
}

// -----------  window  ----------------

/*!     \class window
        \brief A single ncurses window
*/

/*! \brief          Set the default colours
    \param  fgbg    colour pair

    Does NOT change _fg/_bg because I can't find a guaranteed way to go from a big integer that
    combines the two colours to the individual colours.
*/
window& window::_default_colours(const chtype fgbg)
{ if (!_wp)
    return *this;

  { SAFELOCK(screen);

    wbkgd(_wp, fgbg);
  }

  return *this;
}

/*! \brief          Perform common portion of initialisation
    \param  wi      window information
    \param  flags   see screen.h; possible flags are WINDOW_INSERT, WINDOW_NO_CURSOR
*/
void window::_init(const window_information& wi, const unsigned int flags)
{ _x = wi.x();
  _y = wi.y();
  _width = wi.w();
  _height = wi.h();
  _vertical = false;
  _column_width = 0;
  _wp = nullptr;
  _scrolling = false;
  _hidden_cursor = (flags & WINDOW_NO_CURSOR);
  _insert = (flags & WINDOW_INSERT);
  _pp = nullptr;

  if (_width and _height)
  { SAFELOCK(screen);

    _wp = newwin(_height, _width, LINES - _y /* - 1 */ - _height, /*COLS - */_x);
    keypad(_wp, true);

    _pp = new_panel(_wp);
  }
}

/*! \brief          Default constructor
    \param  flags   see screen.h; possible flags are WINDOW_INSERT, WINDOW_NO_CURSOR

    The window is not ready for use after this constructor. It still needs to be initialised.
*/
window::window(const unsigned int flags) :
  _x(0),
  _y(0),
  _width(0),
  _height(0),
  _vertical(false),
  _column_width(0),
  _wp(nullptr),
  _scrolling(false),
  _hidden_cursor(flags & WINDOW_NO_CURSOR),
  _insert(flags & WINDOW_INSERT),
  _pp(nullptr),
  _process_input(nullptr),
  _fg(COLOUR_WHITE),
  _bg(COLOUR_BLACK)
{  //const int pair_nr = colours.add(_fg, _bg);
   //default_colours(COLOUR_PAIR(pair_nr));
  _default_colours(COLOUR_PAIR(colours.add(_fg, _bg)));
}

/*! \brief          Create using position and size information from the configuration file
    \param  wi      window position and size
    \param  flags   see screen.h; possible flags are WINDOW_INSERT, WINDOW_NO_CURSOR

    The window is ready for use after this constructor.
*/
window::window(const window_information& wi, const unsigned int flags) :
  _width(wi.w()),
  _height(wi.h()),
  _x(wi.x()),
  _y(wi.y()),
  _vertical(false),
  _column_width(0),
  _wp(nullptr),
  _scrolling(false),
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

//    const int pair_nr = colours.add(_fg, _bg);

    _default_colours(COLOUR_PAIR(colours.add(_fg, _bg)));
  }
}

/// destructor
window::~window(void)
{ if (_pp)
    del_panel(_pp);

  if (_wp)
    delwin(_wp);
}

/*! \brief          Initialise using position and size information from the configuration file
    \param  wi      window position and size
    \param  flags   see screen.h; possible flags are WINDOW_INSERT, WINDOW_NO_CURSOR

    The window is ready for use after this function has been called.
*/
void window::init(const window_information& wi, const unsigned int flags)
{ _init(wi, flags);

  _fg = string_to_colour(wi.fg_colour());
  _bg = string_to_colour(wi.bg_colour());

//  const int pair_nr = colours.add(_fg, _bg);

  _default_colours(COLOUR_PAIR(colours.add(_fg, _bg)));
  (*this) <= WINDOW_CLEAR;                  // clear the window (this also correctly sets the background on the screen)
}

/*! \brief          Initialise using position and size information from the configuration file, and possibly set colours explicitly
    \param  wi      window position and size
    \param  fg      foreground colour
    \param  bg      background colour
    \param  flags   see screen.h; possible flags are WINDOW_INSERT, WINDOW_NO_CURSOR

    The window is ready for use after this function has been called. <i>fg</i> and <i.bg</i>
    override <i>wi.fg_colour()</i> and <i>wi.bg_colour()</i> iff wi.colours_set() is false.
*/
void window::init(const window_information& wi, int fg, int bg, const unsigned int flags)
{ _init(wi, flags);

  if (wi.colours_set())
  { _fg = string_to_colour(wi.fg_colour());
    _bg = string_to_colour(wi.bg_colour());
  }
  else
  { _fg = fg;
    _bg = bg;
  }

//  const int pair_nr = colours.add(_fg, _bg);

  _default_colours(COLOUR_PAIR(colours.add(_fg, _bg)));
  (*this) <= WINDOW_CLEAR;                  // clear the window (this also correctly sets the background on the screen)
}

/*! \brief          Move the logical cursor
    \param  new_x   x position
    \param  new_y   y position
    \return         the window
*/
window& window::move_cursor(const int new_x, const int new_y)
{ if (!_wp)
    return *this;
    
  { SAFELOCK(screen);

    wmove(_wp, _limit(_height - new_y - 1, 0, _height - 1), _limit(new_x, 0, _width - 1));
  }
  
  return *this;
}

/*! \brief      Write a string to the window
    \param  s   string to write
    \return     the window

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
    wprintw(_wp, s.c_str());
  else    // insert mode
  { const cursor c = cursor_position();
    const string remainder = read(c.x(), c.y());

    wprintw(_wp, (s + remainder).c_str());
    
    *this < cursor(c.x() + s.length(), c.y());
  }
  
  return *this;
}

/*! \brief          Write a set of strings to a window
    \param  ss      set to write
    \return         the window

    Wraps words to new lines. Stops writing if there's insufficient room for the next string.
*/
window& window::operator<(const set<string>& ss)
{ if (!_wp)
    return *this;

  vector<string> v(ss.cbegin(), ss.cend());

  sort(v.begin(), v.end(), compare_calls);

  unsigned int idx = 0;
  
  for (const auto& str : v)
  { 
// see if there's enough room on this line    
    cursor_position();
    const int remaining_space = width() - _cursor_x;

// stop writing if there's insufficient room for the next string
    if (remaining_space < static_cast<int>(str.length()))
      if (!scrolling() and (_cursor_y == 0))
        break;
    
    if (remaining_space < static_cast<int>(str.length()))
      *this < "\n";

    *this < str;
  
// add space unless we're at the end of a line or this is the last string
    const bool end_of_line = (remaining_space == static_cast<int>(str.length()));
    
    if ((idx != v.size() - 1) and !end_of_line)
      *this < " ";
    
    idx++;
  }

  return *this;
}

/*! \brief      Write a vector of strings to a window
    \param  v   vector to write
    \return     the window

    Wraps words to new lines. Stops writing if there's insufficient room for the next string.
*/
window& window::operator<(const vector<string>& v)
{ if (!_wp)
    return *this;

  unsigned int idx = 0;

  for (const auto& str : v)
  {
// see if there's enough room on this line
    cursor_position();
    const int remaining_space = width() - _cursor_x;

 // stop writing if there's insufficient room for the next string
    if (remaining_space < static_cast<int>(str.length()))
      if (!scrolling() and (_cursor_y == 0))
        break;

    if (remaining_space < static_cast<int>(str.length()))
      *this < "\n";

    *this < str;

// add space unless we're at the end of a line or this is the last string
    const bool end_of_line = (remaining_space == static_cast<int>(str.length()));

    if ((idx != v.size() - 1) and !end_of_line)
      *this < " ";

    idx++;
  }

  return *this;
}

/*! \brief          Write a vector of strings with possible different colours to a window
    \param  vec     vector of pairs <string, int [colour number]> to write
    \return         the window

    Wraps words to new lines. Stops writing if there's insufficient room for the next string.
*/
window& window::operator<(const vector<pair<string, int /* colour pair number */ > >& vec)
{ if (!_wp)
    return *this;

  unsigned int idx = 0;

  for (const auto& psi : vec)
  { const string& callsign = psi.first;
    const int& cp = psi.second;

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

  return *this;
}

/// get cursor position
const cursor window::cursor_position(void)
{ if (!_wp)
    return cursor(0, 0);
    
  SAFELOCK(screen);

  getyx(_wp, _cursor_y, _cursor_x);
  _cursor_y = height() - _cursor_y - 1;
  
  return cursor(_cursor_x, _cursor_y);
}

/*! \brief              Move the logical cursor (relative to current location)
    \param  delta_x     change in x position
    \param  delta_y     change in y position
    \return             the window
*/
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

/// set the default colours
window& window::default_colours(const int foreground_colour, const int background_colour)
{ if (!_wp)
    return *this;

  _fg = foreground_colour;
  _bg = background_colour;

  return _default_colours(FGBG(foreground_colour, background_colour));
}

/// control an attribute or perform a simple operation
window& window::operator<(const enum WINDOW_ATTRIBUTES wa)
{ if (!_wp or (wa == WINDOW_NOP) )
    return *this;
    
  switch (wa)
  { case WINDOW_NORMAL :
      { SAFELOCK(screen);
        wstandend(_wp);
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

/// clear the window
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

  scrollok(_wp, enable_or_disable);
  
  return *this;
}

/// scroll
window& window::scrollit(const int n_lines)
{ if (!_wp)
    return *this;
    
  SAFELOCK(screen);

  wscrl(_wp, n_lines);
  
  return *this;
}

/// control leaveok
window& window::leave_cursor(const bool enable_or_disable)
{ if (!_wp)
    return *this;
    
  _leaveok = enable_or_disable;
  
  SAFELOCK(screen);

  leaveok(_wp, enable_or_disable);
  
  return *this;
}

/// window << cursor
//window& operator<(window& win, const cursor& c)
//{ return win.move_cursor(c.x(), c.y());
//}

/// window << cursor_relative
//window& operator<(window& win, const cursor_relative& cr)
//{ return win.move_cursor_relative(cr.x(), cr.y());
//}

/// window << centre
window& operator<(window& win, const centre& c)
{ win.move_cursor((win.width() - n_chars(c.s())) / 2, c.y());    // correctly accounts for UTF-8 encoding

  return win < c.s();
}

/*! \brief      read to end of window
    \param  x   x value from which to read (0 is leftmost column)
    \param  y   y value from which to read (0 is bottommost row)
    \return     contents of the window, starting at the position (<i>x</i>, <i>y</i>)

    By default reads the entirety of the bottom line
*/
const string window::read(int x, int y)
{ if (!_wp)
    return string();
  
  const unsigned int BUF_SIZE = 1024;
  char tmp[BUF_SIZE];

  tmp[0] = '\0';
  
  SAFELOCK(screen);
  
// we need to save the current logical cursor position in the window, because mvwinstr() silently moves it
  const cursor c = cursor_position();
  const int n_chars = mvwinnstr(_wp,  _limit(_height - y - 1, 0, _height - 1),  _limit(x, 0, _width - 1), tmp, BUF_SIZE - 1);
  move_cursor(c.x(), c.y());          // restore the logical cursor position
  
  if (n_chars != ERR)
    tmp[n_chars] = '\0';   // in theory, not necessary

  return string(tmp);
}

/// line by line snapshot of all the contents; lines go from top to bottom
const vector<string> window::snapshot(void)
{ vector<string> rv;

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

/// delete a character within the current line
window& window::delete_character(const int n)
{ if (!_wp)
    return *this;

  SAFELOCK(screen);    // ensure that nothing changes

  return delete_character( n, cursor_position().y() );
}

/// delete a character within a particular line
window& window::delete_character(const int n, const int line_nr)

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

/*! \brief  character processing that is the same in multiple windows
    \param  e   keyboard event to be processed
    \return whether the event was processed
*/
const bool window::common_processing(const keyboard_event& e)
{ window& win = (*this);

// a..z A..Z
  if (e.is_letter())
  { win <= to_upper(e.str());
    return true;
  }

// 0..9
  if (e.is_digit())
  { win <= e.str();
    return true;
  }

// /
  if (e.is_unmodified() and e.is_char('/'))
  { win <= e.str();
    return true;
  }

// BACKSPACE -- remove because LOG window is different
//  if (e.is_unmodified() and e.symbol() == XK_BackSpace)
//  { win.delete_character(win.cursor_position().x() - 1);
//    win.refresh();
//    return true;
//  }

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

// -----------  colour_pair  ----------------

/*! \class colour_pair
    \brief A class to hold information about used colour pairs
*/

/*! \brief          Private function to add a new pair of colours
    \param  p       foreground colour, background colour
    \return         the number of the colour pair
*/
const unsigned int cpair::_add_to_vector(const pair< int, int>& fgbg)
{ _colours.push_back(fgbg);
  init_pair(_colours.size(), fgbg.first, fgbg.second);

  return _colours.size();
}

/*! \brief          Add a pair of colours
    \param  fg      foreground colour
    \param  bg      background colour
    \return         the number of the colour pair

    If the pair is already known, returns the number of the known pair.
    Note the pair number 0 cannot be changed, so we ignore it here and start counting from one
*/
const unsigned int cpair::add(const int fg, const int bg)
{ const pair<int, int> fgbg { fg, bg };

  SAFELOCK(_colours);

  if (_colours.empty())
    return _add_to_vector(fgbg);

  const auto cit = find(_colours.cbegin(), _colours.cend(), fgbg);

  if (cit == _colours.cend())
    return _add_to_vector(fgbg);
  else
    return (cit - _colours.cbegin() + 1);    // the first entry (at _colours.begin()) is ncurses colour pair number 1
}

/// return the foreground colour of a pair
const int cpair::fg(const int pair_nr) const
{ short f;
  short b;

  pair_content(pair_nr, &f, &b);

  return static_cast<int>(f);
}

/// return the background colour of a pair
const int cpair::bg(const int pair_nr) const
{ short f;
  short b;

  pair_content(pair_nr, &f, &b);

  return static_cast<int>(b);
}

// convert the name of a colour to a colour
const int string_to_colour(const string& str)
{ static const map<string, int> colour_map { { "BLACK",   COLOUR_BLACK },
                                             { "BLUE",    COLOUR_BLUE },
                                             { "CYAN",    COLOUR_CYAN },
                                             { "GREEN",   COLOUR_GREEN },
                                             { "MAGENTA", COLOUR_MAGENTA },
                                             { "RED",     COLOUR_RED },
                                             { "WHITE",   COLOUR_WHITE },
                                             { "YELLOW",  COLOUR_YELLOW }
                                           };

  const string s = to_upper(remove_peripheral_spaces(str));
  const auto cit = colour_map.find(s);

  if (cit != colour_map.cend())
    return cit->second;

// should change this so it works with a colour name and not just a number

  if (begins_with(s, "COLOUR_"))
    return (from_string<int>(substring(s, 7)));

  if (begins_with(s, "COLOR_"))
    return (from_string<int>(substring(s, 6)));

  if (s.find_first_not_of("0123456789") == string::npos)  // if all digits
    return from_string<int>(s);

  return COLOUR_BLACK;    // default
}
