// $Id: screen.cpp 281 2025-12-07 20:02:13Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   screen.cpp

    Classes and functions related to screen management
*/

#include "screen.h"
#include "string_functions.h"

#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include <cstdlib>

#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

using namespace std;
using namespace   this_thread;   // std::this_thread

extern cpair colours;
extern message_stream   ost;                        ///< for debugging, info

// globals
pt_mutex screen_mutex { "SCREEN"s };          ///< mutex for access to screen

/*! \brief      Return a pair of colours
    \param  n   pair number
    \return     the pair of colours that constitute pair number <i>n</i>
    
    This is a workaround for the macro COLOR_PAIR().
    
    I note that ncurses is inconsistent about the types used to hold colour pairs; the actual definition of COLOR_PAIR()
    defines the return value as an int, so that's what we use here
*/
int COLOUR_PAIR(const PAIR_NUMBER_TYPE n)
{ const int result { COLOR_PAIR(n) }; 
  
  if (result == ERR)
  { ost << "Error result from calling COLOR_PAIR with parameter: " << n << endl;
    throw exception();
  }
  
  return result;
}

// -----------  screen  ----------------

/*! \class  screen
    \brief  A single terminal (e.g., an xterm)
        
    The screen will have the size given by the COLUMNS and LINES environment variables,
    or, in their absence, 80 columns and 25 lines.
*/

/// default constructor
screen::screen(void)
{ constexpr auto SLEEP_TIME { 2s };   // time to wait before exiting

  if (setlocale(LC_ALL, "") == nullptr)
  { cerr << "Unable to set locale" << endl;
    sleep_for(SLEEP_TIME);
    exit(-1);
  }

  if (initscr() == nullptr)
  { cerr << "Unable to initialise curses" << endl;
    sleep_for(SLEEP_TIME);
    exit(-1);
  }

  if (start_color() == ERR)
  { cerr << "Unable to start colours on screen" << endl;
    sleep_for(SLEEP_TIME);
    endwin();
    exit(-1);
  }

  if (!has_colors())
  { cerr << "NO COLOURS" << endl;
    sleep_for(SLEEP_TIME);
    endwin();
    exit(-1);
  }

  if (refresh() == ERR)
  { cerr << "Error calling refresh()" << endl;
//    sleep(2);
    sleep_for(SLEEP_TIME);
    endwin();
    exit(-1);
  }

// ncurses does not seem to supply any real control over the look of the cursor.
// I would like a blinking underline, but there seems to be no way to get that.
// TODO: look into this in more depth
  curs_set(1);             // a medium cursor
}

// -----------  window_information ----------------

/*! \class  window_information
    \brief  encapsulate position and colour information of a window
*/

/// convert to readable string
string window_information::to_string(void) const
{ string rv { };

  rv = "  x: "s + ::to_string(_x) + EOL
     + "  y: "s + ::to_string(_y) + EOL
     + "  w: "s + ::to_string(_w) + EOL
     + "  h: "s + ::to_string(_h) + EOL;

  rv += (" fg: "s + _fg_colour + EOL);
  rv += (" bg: "s + _bg_colour + EOL);

  rv += "Colours_set: "s + (_colours_set ? string {"true"} : string {"false"} );   // C++ idiotically defaults to char[], so these are of different types

  return rv;
}

// -----------  window  ----------------

/*! \class  window
    \brief  A single ncurses window
*/

/*! \brief          Set the default colours
    \param  fgbg    colour pair
    \return         the window

    Does NOT change _fg/_bg because I can't find a guaranteed way to go from a big integer that
    combines the two colours to the individual colours.
*/
window& window::_default_colours(const chtype fgbg)
{ if (_wp)
  { SAFELOCK(screen);

    wbkgd(_wp, fgbg);
  }

  return *this;
}

/*! \brief          Perform basic initialisation
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
  _hidden_cursor = (flags bitand WINDOW_NO_CURSOR);
  _insert = (flags bitand WINDOW_INSERT);
  _pp = nullptr;

  if (_width and _height)
  { SAFELOCK(screen);

    _wp = newwin(_height, _width, LINES - _y /* - 1 */ - _height, /*COLS - */_x);

    if (!_wp)
    { ost << "Error creating window; aborting" << endl
          << "  requested x, y, w, h = " << _x << ", " << _y << ", " << _width << ", " << _height << endl;
      sleep_for(2s);
      exit(-1);
    }

    keypad(_wp, true);

    _pp = new_panel(_wp);
  }
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
  _hidden_cursor(flags bitand WINDOW_NO_CURSOR),
  _insert(flags bitand WINDOW_INSERT),
  _pp(nullptr),
  _process_input(nullptr),
  _fg(string_to_colour(wi.fg_colour())),
  _bg(string_to_colour(wi.bg_colour()))
{ if (_width and _height)
  { SAFELOCK(screen);

    _wp = newwin(_height, _width, LINES - _y - _height, _x);

    if (!_wp)
    { ost << "Error creating window; aborting" << endl;
      sleep_for(2s);
      exit(-1);
    }

    keypad(_wp, true);
    _pp = new_panel(_wp);

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

  _default_colours(COLOUR_PAIR(colours.add(_fg, _bg)));

  (*this) <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;                  // clear the window (this also correctly sets the background on the screen)
}

/*! \brief          Initialise using position and size information from the configuration file, and possibly set colours explicitly
    \param  wi      window position, size and (possibly) colour
    \param  fg      foreground colour
    \param  bg      background colour
    \param  flags   see screen.h; possible flags are WINDOW_INSERT, WINDOW_NO_CURSOR

    The window is ready for use after this function has been called. <i>fg</i> and <i>bg</i>
    override <i>wi.fg_colour()</i> and <i>wi.bg_colour()</i> iff wi.colours_set() is false.
*/
void window::init(const window_information& wi, const COLOUR_TYPE fg, const COLOUR_TYPE bg, const unsigned int flags)
{ _init(wi, flags);

  _fg = (wi.colours_set() ? string_to_colour(wi.fg_colour()) : fg);
  _bg = (wi.colours_set() ? string_to_colour(wi.bg_colour()) : bg);

  _default_colours(COLOUR_PAIR(colours.add(_fg, _bg)));

  (*this) <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;                  // clear the window (this also correctly sets the background on the screen)
}

/*! \brief          Move the logical cursor
    \param  new_x   x position
    \param  new_y   y position
    \return         the window
*/
window& window::move_cursor(const WIN_INT_TYPE new_x, const WIN_INT_TYPE new_y)
{ if (_wp)
  { SAFELOCK(screen);

    wmove(_wp, LIMIT(_height - new_y - 1, 0, _height - 1), LIMIT(new_x, 0, _width - 1));
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

    Also see the function reformat_for_wprintw().

    There is no point in supplying operator<(string_view s) because we have
    to create a std::string anayway, because string_view does not supply c_str().
*/
window& window::operator<(const string& s)
{ if (!_wp)
    return *this;
    
  SAFELOCK(screen);

  if (!_insert)
    waddstr(_wp, s.c_str());
  else                                           // insert mode
  { const cursor c         { cursor_position() };
    const string remainder { read(c.x(), c.y()) };

    waddstr(_wp, (s + remainder).c_str());
    
    *this < cursor(c.x() + s.length(), c.y());
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

  unsigned int idx { 0 };

  for (const auto& str : v)
  {
// see if there's enough room on this line
    cursor_position();
    
    const int remaining_space { width() - _cursor_x };
    const int int_len         { static_cast<int>(str.length()) };

 // stop writing if there's insufficient room for the next string
    if (remaining_space < int_len)
      if (!scrolling() and (_cursor_y == 0))
        break;

    if (remaining_space < int_len)
      *this < "\n";

    *this < str;

// add space unless we're at the end of a line or this is the last string
    const bool end_of_line { (remaining_space == int_len) };

    if ( (idx != (v.size() - 1)) and !end_of_line )
      *this < SPACE_STR;

    idx++;
  }

  return *this;
}

/*! \brief          Write a vector of strings with possible different colours to a window
    \param  vec     vector of pairs <string, PAIR_TYPE [colour pair number]> to write
    \return         the window

    Wraps words to new lines. Stops writing if there's insufficient room for the next string.
*/
window& window::operator<(const vector<std::pair<string, PAIR_NUMBER_TYPE /* colour pair number */ > >& vec)     // bizarrely, doxygen complains if I remove the std:: qualifier
{ if (!_wp)
    return *this;

  unsigned int idx { 0 };

  for (const auto& [ str, cp ] : vec)
  {
// see if there's enough room on this line
    cursor_position();

    const int remaining_space { width() - _cursor_x };

// stop writing if there's insufficient room for the next string
    if (remaining_space < static_cast<int>(str.length()))
      if (!scrolling() and (_cursor_y == 0))
        break;

    if (remaining_space < static_cast<int>(str.length()))
      *this < "\n";

    this->set_colour_pair(cp);

    *this < str < COLOURS(_fg, _bg);    // back to default colours

// add space unless we're at the end of a line or this is the last string
    const bool end_of_line { (remaining_space == static_cast<int>(str.length())) };

    if ( (idx != (vec.size() - 1)) and !end_of_line )
      *this < SPACE_STR;

    idx++;
  }

  return *this;
}

/*! \brief      Get cursor position
    \return     the current position of the cursor
*/
cursor window::cursor_position(void)
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
window& window::move_cursor_relative(const int16_t delta_x, const int16_t delta_y)
{ if (_wp)
  { cursor_position();

    WIN_INT_TYPE new_x { static_cast<WIN_INT_TYPE>(_cursor_x + delta_x) };
    WIN_INT_TYPE new_y { static_cast<WIN_INT_TYPE>(_cursor_y + delta_y) };

    if (delta_x < 0)
    { if ((-delta_x) >= _cursor_x)
        new_x = 0;
    }

    if (delta_y < 0)
    { if ((-delta_y) >= _cursor_y)
        new_y = 0;
    }

    if (new_x >= width())
      new_x = width() - 1;

    if (new_y >= height())
      new_y = height() - 1;

    move_cursor(new_x, new_y);
  }

  return *this;
}

/*! \brief              Set the colour pair
    \param  pair_nr     number of the new colour pair
    \return             the window
*/
window& window::set_colour_pair(const PAIR_NUMBER_TYPE pair_nr)
{ if (_wp)
  { SAFELOCK(screen);  

    wcolor_set(_wp, pair_nr, NULL);
  }
  
  return *this;
}

/*! \brief                      Set the default colours
    \param  foreground_colour   foreground colour
    \param  background_colour   background colour
    \return                     the window
*/
window& window::default_colours(const COLOUR_TYPE foreground_colour, const COLOUR_TYPE background_colour)
{ if (!_wp)
    return *this;

  SAFELOCK(screen);

  _fg = foreground_colour;
  _bg = background_colour;

  return _default_colours(FGBG(foreground_colour, background_colour));
}

/*! \brief      Control an attribute or perform a simple operation
    \param  wa  the attribute or operation
    \return     the window
*/
window& window::operator<(const enum WINDOW_ATTRIBUTES wa)
{ using enum WINDOW_ATTRIBUTES;

  if (!_wp or (wa == WINDOW_NOP) )
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
    { const size_t posn { read().find_last_not_of(' ') };

      move_cursor(posn + 1, cursor_position().y());
      break;
    }

    case WINDOW_NOP :       // logically unnecessary, but needed to keep the compiler happy
      break;
  }

  return *this;
}

/// clear the window
window& window::clear(void)
{ if (_wp)
  { SAFELOCK(screen);

    werase(_wp);
  }

  return *this;
}

/// refresh
window& window::refresh(void)
{ if (_wp)
  { SAFELOCK(screen);

    if (_hidden_cursor)
      getsyx(_sy, _sx);   

    wrefresh(_wp);

    if (_hidden_cursor)
    { setsyx(_sy, _sx);
      doupdate();           // is this necessary?
    }
  }

  return *this;
}

/*! \brief                      Control scrolling
    \param  enable_or_disable   whether to enable scrolling
    \return                     the window
*/
window& window::scrolling(const bool enable_or_disable)
{ if (_wp)
  { SAFELOCK(screen);

    _scrolling = enable_or_disable;
    scrollok(_wp, enable_or_disable);
  }
  
  return *this;
}

/*! \brief              scroll a window
    \param  n_lines     number of lines to by which to scroll
    \return             the window

    Can't call it 'scroll' because there's a silly ncurses *macro* with the same name
*/
window& window::scrollit(const int n_lines)
{ if (_wp)
  { SAFELOCK(screen);

    wscrl(_wp, n_lines);
  }
  
  return *this;
}

/*! \brief                      Control leaveok
    \param  enable_or_disable   whether to enable scrolling
    \return                     the window
*/
window& window::leave_cursor(const bool enable_or_disable)
{ if (_wp)
  { _leaveok = enable_or_disable;
  
    SAFELOCK(screen);

    leaveok(_wp, enable_or_disable);
  }
  
  return *this;
}

/*! \brief          Write a centred string in a window
    \param  win     target window
    \param  c       object containing the string to be written, and the vertical line number on which it is to be written
    \return         the window

    Correctly accounts for UTF-8 encoding
*/
window& operator<(window& win, const centre& c)
{ win.move_cursor((win.width() - n_chars(c.s())) / 2, c.y());    // correctly accounts for UTF-8 encoding

  return win < c.s();
}

/*! \brief      Read to end of window
    \param  x   x value from which to read (0 is leftmost column)
    \param  y   y value from which to read (0 is bottommost row)
    \return     contents of the window, starting at the position (<i>x</i>, <i>y</i>)

    By default reads the entirety of the bottom line.
    Limits both <i>x</i> and <i>y</i> to valid values for the window before reading the line.
    Cannot be const, because because mvwinnstr() silently moves the cursor and we then move it back
*/
string window::read(const WIN_INT_TYPE x, const WIN_INT_TYPE y)
{ if (!_wp)
    return string { };
  
  constexpr unsigned int BUF_SIZE { 1024 };

  char tmp[BUF_SIZE];

  tmp[0] = '\0';    // ncurses requires a NULL end-of-string marker
  
  SAFELOCK(screen);
  
// we need to save the current logical cursor position in the window, because mvwinstr() silently moves it
  const cursor c       { cursor_position() };
  const int    n_chars { mvwinnstr(_wp,  LIMIT(_height - y - 1, 0, _height - 1),  LIMIT(x, 0, _width - 1), tmp, BUF_SIZE - 1) };

  move_cursor(c.x(), c.y());          // restore the logical cursor position
  
  if (n_chars != ERR)
    tmp[n_chars] = '\0';   // in theory not necessary

  return string { tmp };
}

/// line by line snapshot of all the contents; lines go from top to bottom
vector<string> window::snapshot(void)
{ vector<string> rv;

  if (_wp)
  { for (size_t n { static_cast<size_t>(height() - 1) }; n < static_cast<size_t>(height()); --n)    // stops when n wraps to big number when decremented from zero
      rv += getline(n);                                                                             // no need to lock, as the height won't change and getline will lock
  }

  return rv;
}

/*! \brief              Clear a line
    \param  line_nr     number of line to clear (0 is bottommost row)
    \return             the window

    Limits <i>line_nr</i> to a valid value for the window before clearing the line.
*/
window& window::clear_line(const WIN_INT_TYPE line_nr)
{ if (_wp)
  { SAFELOCK(screen);

    const cursor c { cursor_position() };

    move_cursor(0, line_nr);
    (*this) <= WINDOW_ATTRIBUTES::WINDOW_CLEAR_TO_EOL;
    move_cursor(c.x(), c.y());          // restore the logical cursor position
  }

  return *this;
}

/*! \brief      Delete a character in the current line
    \param  n   number of character to delete (wrt 0)
    \return     the window

    Does nothing if character number <i>n</i> does not exist
*/
window& window::delete_character(const WIN_INT_TYPE n)
{ if (!_wp)
    return *this;

  SAFELOCK(screen);    // ensure that nothing changes

  return delete_character( n, cursor_position().y() );
}

/*! \brief          Delete a character within a particular line
    \param n        number of character to delete (wrt 0)
    \param line_nr  number of line (wrt 0)
    \return         the window

    Line number zero is the bottom line
*/
window& window::delete_character(const WIN_INT_TYPE n, const WIN_INT_TYPE line_nr)
{ if (!_wp)
    return *this;

// make sure nothing changes from this point onwards
  SAFELOCK(screen);

  if ( (line_nr < 0) or (line_nr >= height()) )
    return *this;

  const string old_line { getline(line_nr) };

  if (old_line.empty())
    return *this;
  
  if (n >= static_cast<int>(old_line.length()))
    return *this;

  const string new_line { old_line.substr(0, n) + old_line.substr(n + 1) };
  const cursor c        { cursor_position() };

  move_cursor(0, line_nr);

// ensure we are not in insert mode
  const bool insert_enabled { _insert };

  _insert = false;
  (*this) < WINDOW_ATTRIBUTES::WINDOW_CLEAR_TO_EOL < new_line;
  _insert = insert_enabled;             // restore old insert mode

  move_cursor(c);                       // restore the logical cursor position
  move_cursor_relative(-1, 0);          // move cursor to the left

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

/*! \brief          Obtain a readable description of the window properties
    \param    name  name of the window, if any
    \return         the window properties as a human-readable string

    Cannot be const, as it uses snapshot, which internally moves the cursor and restores it
*/
string window::properties(const string_view name)
{ string rv { "Window "s + (name.empty() ? EMPTY_STR : name + SPACE_STR) + "is "s + (defined() ? EMPTY_STR : "NOT "s) + "defined"s + EOL };
  
  if (defined())
  { rv += "  x origin = " + to_string(_x) + EOL;
    rv += "  y origin = " + to_string(_y) + EOL;
    rv += "  height = " + to_string(height()) + EOL;
    rv += "  width = " + to_string(width()) + EOL;
    rv += "  foreground colour = " + to_string(fg()) + EOL;
    rv += "  background colour = " + to_string(bg()) + EOL;
    rv += "  cursor is "s + (hidden_cursor() ? ""s : "NOT "s) + "hidden"s + EOL;
    rv += "  x cursor = "s + to_string(_cursor_x) + EOL;
    rv += "  y cursor = "s + to_string(_cursor_y) + EOL;
    rv += "  window is "s + (insert() ? ""s : "NOT "s) + "in INSERT mode"s + EOL;
    rv += "  column width = " + to_string(column_width()) + EOL;
    rv += "  string containers are " + (vertical() ? ""s : "NOT "s) + "displayed vertically"s + EOL;
    rv += "  characters are " + (_echoing ? ""s : "NOT "s) + "echoed"s + EOL;
    rv += "  leaveok is " + (_leaveok ? ""s : "NOT "s) + "set"s + EOL;
    rv += "  scrolling is " + (_scrolling ? ""s : "NOT "s) + "enabled"s + EOL;
    rv += "  function to process input " + ( (_process_input != nullptr) ? "EXISTS"s : "DOES NOT EXIST"s ) + EOL;
    rv += "  window is " + (hidden() ? ""s : "NOT "s) + "hidden"s + EOL;

    const vector<string> lines = snapshot();
    
    rv += "  number of lines in snapshot = "s + to_string(lines.size()) + EOL;
    
    for (size_t n { 0 }; n < lines.size(); ++n)
      rv += "    ["s + to_string(n) + "]: "s + lines[n] + EOL;
  }
  
  return rv;
}

/*! \brief      character processing that is the same in multiple windows
    \param  e   keyboard event to be processed
    \return     whether the event was processed
*/
bool window::common_processing(const keyboard_event& e)
{ window& win { (*this) };

// a..z A..Z
  if (e.is_letter())
    return (win <= to_upper(e.str()), true);

// 0..9
  if (e.is_digit())
    return (win <= e.str(), true);

// /
  if (e.is_unmodified() and e.is_char('/'))
    return (win <= e.str(), true);

// DELETE
  if (e.is_unmodified() and e.symbol() == XK_Delete)
  { win.delete_character(win.cursor_position().x());
    win.refresh();
    return true;
  }

// END
  if (e.is_unmodified() and e.symbol() == XK_End)
  { const string contents { win.read() };
    const size_t posn     { contents.find_last_not_of(' ') };

    return (win <= cursor(posn + 1, 0), true);
  }

// HOME
  if (e.is_unmodified() and e.symbol() == XK_Home)
    return (win <= WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE, true);

// CURSOR LEFT
  if (e.is_unmodified() and e.symbol() == XK_Left)
    return (win <= cursor_relative(-1, 0), true);

// CURSOR RIGHT
  if (e.is_unmodified() and e.symbol() == XK_Right)
    return (win <= cursor_relative(1, 0), true);     // ncurses fills the window with spaces; we can't differentiate between those and any we might have added ourself

// INSERT -- toggle insert mode
  if (e.is_unmodified() and e.symbol() == XK_Insert)
    return (win.insert(!win.insert()), true);

  return false;
}

/*  \brief            Obtain all the overlapping pairs of windows from a container (map)
    \param  windows   map containing all the windows
    \return           all the pairs of names of overlapping windows
*/
vector<pair<string, string>> window_overlaps(const STRING_MAP<window_information>& windows)   // key = window name
{ vector<pair<string, string>> rv;

  for (auto it { windows.cbegin() }; it != prev(windows.cend()); ++it)
  { const auto& [ name1, wi1 ] { *it };

    for (auto it2 { next(it) }; it2 != windows.cend(); ++it2)
    { const auto& [ name2, wi2 ] { *it2 };

      if (overlap(wi1.x(), wi1.y(), wi1.w(),  wi1.h(), wi2.x(), wi2.y(), wi2.w(),  wi2.h()))
        rv += { name1, name2 };
    }
  }

  return rv;
}

// -----------  colour_pair  ----------------

/*! \class  colour_pair
    \brief  A class to hold information about used colour pairs
*/

/*! \brief      Private function to add a new pair of colours
    \param  p   foreground colour, background colour
    \return     the number of the colour pair
*/
PAIR_NUMBER_TYPE cpair::_add_to_vector(const pair<COLOUR_TYPE, COLOUR_TYPE>& fgbg)
{ _colours += fgbg;

  const auto [fg, bg] { fgbg };

  if (fg >= COLORS)
  { ost << "Attempt to set foreground to colour " << fg << " with only " << COLORS << " colours available" << endl;
    throw exception();
  }

  if (bg >= COLORS)
  { ost << "Attempt to set background to colour " << bg << " with only " << COLORS << " colours available" << endl;
    throw exception();
  }

  if (const auto status { init_pair(static_cast<PAIR_NUMBER_TYPE>(_colours.size()), fg, bg) }; status == ERR)
  { ost << "Error returned from init_pair with parameters: " << _colours.size() << ", " << fg << ", " << bg << endl;
    throw exception();
  }

  return static_cast<PAIR_NUMBER_TYPE>(_colours.size());
}

/*! \brief      Add a pair of colours
    \param  fg  foreground colour
    \param  bg  background colour
    \return     the number of the colour pair

    If the pair is already known, returns the number of the known pair.
    Note the pair number 0 cannot be changed, so we ignore it here and start counting from one
*/
PAIR_NUMBER_TYPE cpair::add(const COLOUR_TYPE fg, const COLOUR_TYPE bg)
{ const pair<COLOUR_TYPE, COLOUR_TYPE> fgbg { fg, bg };

  SAFELOCK(_colours);

  if (_colours.empty())
    return _add_to_vector(fgbg);

  const auto it { ranges::find(_colours, fgbg) };

  return ( (it == _colours.end()) ? _add_to_vector(fgbg) : static_cast<PAIR_NUMBER_TYPE>( distance(_colours.begin(), it) + 1));
}

/*! \brief              Get the foreground and background colours of a pair
    \param  pair_nr     number of the pair
    \return             the foreground and background colours of the pair number <i>pair_nr</i>
*/
pair<COLOUR_TYPE, COLOUR_TYPE> cpair::fgbg(const PAIR_NUMBER_TYPE pair_nr) const
{ short f;      // defined to be short in the man page for pair_content
  short b;      // defined to be short in the man page for pair_content

  if (const auto status { pair_content(pair_nr, &f, &b) }; status == ERR)
    ost << "Error extracting colours from colour pair number " << pair_nr << endl;

  return { static_cast<COLOUR_TYPE>(f), static_cast<COLOUR_TYPE>(b) };
}

/*! \brief          Convert the name of a colour to a colour
    \param  str     name of a colour
    \return         the colour corresponding to <i>str</i>
*/
COLOUR_TYPE string_to_colour(const string_view str)
{ static const UNORDERED_STRING_MAP<COLOUR_TYPE> colour_map { { "BLACK"s,   COLOUR_BLACK },
                                                              { "BLUE"s,    COLOUR_BLUE },
                                                              { "CYAN"s,    COLOUR_CYAN },
                                                              { "GREEN"s,   COLOUR_GREEN },
                                                              { "MAGENTA"s, COLOUR_MAGENTA },
                                                              { "RED"s,     COLOUR_RED },
                                                              { "WHITE"s,   COLOUR_WHITE },
                                                              { "YELLOW"s,  COLOUR_YELLOW }
                                                            };

  const string s { to_upper(remove_peripheral_spaces <std::string_view> (str)) };

  if (const optional<COLOUR_TYPE> opt_clr { OPT_MUM_VALUE(colour_map, s) }; opt_clr)
    return opt_clr.value();

// should change this so it works with a colour name and not just a number
  if (const string_view str { "COLOUR_"sv }; s.starts_with(str))
    return from_string<COLOUR_TYPE>(remove_from_start <std::string_view> (s, str));

  if (const string_view str { "COLOR_"sv }; s.starts_with(str))
    return from_string<COLOUR_TYPE>(remove_from_start <std::string_view> (s, str));

  if (s.find_first_not_of(DIGITS) == string::npos)  // if all digits
    return from_string<COLOUR_TYPE>(s);

  return COLOUR_BLACK;
}

/*! \brief        Do a pair of rectangles overlap?
    \param  x1    minimum x coordinate of rectangle 1
    \param  y1    minimum y coordinate of rectangle 1
    \param  w1    width of rectangle 1
    \param  h1    height of rectangle 1
    \param  x2    minimum x coordinate of rectangle 2
    \param  y2    minimum y coordinate of rectangle 2
    \param  w2    width of rectangle 2
    \param  h2    height of rectangle 2
    \return       whether rectangle 1 and rectangle 2 overlap
*/
bool overlap(const WIN_INT_TYPE x1, const WIN_INT_TYPE y1, const WIN_INT_TYPE w1, const WIN_INT_TYPE h1, const WIN_INT_TYPE x2, const WIN_INT_TYPE y2, const WIN_INT_TYPE w2, const WIN_INT_TYPE h2)
{ auto in_range = [] (const WIN_INT_TYPE v, const WIN_INT_TYPE vmin, const WIN_INT_TYPE vmax) { return (v >= vmin) and (v <= vmax); };   // is v in the range [vmin, vmax] ?

  const WIN_INT_TYPE l1 { x1 };
  const WIN_INT_TYPE r1 { static_cast<WIN_INT_TYPE>(x1 + w1 - 1) };
  const WIN_INT_TYPE l2 { x2 };
  const WIN_INT_TYPE r2 { static_cast<WIN_INT_TYPE>(x2 + w2 - 1) };

  const WIN_INT_TYPE b1 { y1 };
  const WIN_INT_TYPE t1 { static_cast<WIN_INT_TYPE>(y1 + h1 - 1) };
  const WIN_INT_TYPE b2 { y2 };
  const WIN_INT_TYPE t2 { static_cast<WIN_INT_TYPE>(y2 + h2 - 1) };

  auto print_corners = [b1, b2, l1, l2, r1, r2, t1, t2] (void)
    { ost << "RECTANGLES: (" << l1 << ", " << b1 << ") , (" << r1 << ", " << t1 << ") and (" << l2 << ", " << b2 << ") , (" << r2 << ", " << t2 << ")" << endl; };

// are any of the corners of w2 inside w1?
  if ( in_range(l2, l1, r1) and in_range(b2, b1, t1) )
  { print_corners();
    ost << "OVERLAP 1" << endl;
    return true;
  }

  if ( in_range(l2, l1, r1) and in_range(t2, b1, t1) )
  { print_corners();
    ost << "OVERLAP 2" << endl;
    return true;
  }

  if ( in_range(r2, l1, r1) and in_range(b2, b1, t1 ))
  { print_corners();
    ost << "OVERLAP 3" << endl;
    return true;
  }

  if ( in_range(r2, l1, r1) and in_range(t2, b1, t1) )
  { print_corners();
    ost << "OVERLAP 4" << endl;
    return true;
  }

// are any of the corners of w1 inside w2?
  if ( in_range(l1, l2, r2) and in_range(b1, b2, t2) )
  { print_corners();
    ost << "OVERLAP 5" << endl;
    return true;
  }

  if ( in_range(l1, l2, r2) and in_range(t1, b2, t2) )
  { print_corners();
    ost << "OVERLAP 6" << endl;
    return true;
  }

  if ( in_range(r1, l2, r2) and in_range(b1, b2, t2) )
  { print_corners();
    ost << "OVERLAP 7" << endl;
    return true;
  }

  if ( in_range(r1, l2, r2) and in_range(t1, b2, t2) )
  { print_corners();
    ost << "OVERLAP 8" << endl;
    return true;
  }

  return false;
}
