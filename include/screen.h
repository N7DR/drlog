// $Id: screen.h 85 2014-12-01 23:26:41Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef SCREEN_H
#define SCREEN_H

/*!     \file screen.h

        Classes and functions related to screen management
*/

#include "keyboard.h"
#include "log_message.h"
#include "macros.h"
#include "pthread_support.h"
#include "string_functions.h"

#include <array>
#include <iostream>
#include <list>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include <ncursesw/ncurses.h>
#include <panel.h>

extern message_stream ost;

class cpair;
extern cpair colours;

enum WINDOW_ATTRIBUTES { WINDOW_NORMAL,
                         WINDOW_BOLD,
                         WINDOW_HIGHLIGHT,
                         WINDOW_DIM,
                         WINDOW_REVERSE,
                         WINDOW_REFRESH,
                         WINDOW_UPDATE,
                         WINDOW_TOP_LEFT,
                         CURSOR_TOP_LEFT,
                         WINDOW_TOP_RIGHT,
                         CURSOR_TOP_RIGHT,
                         WINDOW_BOTTOM_LEFT,
                         CURSOR_BOTTOM_LEFT,
                         WINDOW_BOTTOM_RIGHT,
                         CURSOR_BOTTOM_RIGHT,
                         WINDOW_CLEAR,
                         WINDOW_CLEAR_TO_EOL,
                         WINDOW_CLEAR_TO_END,
                         CURSOR_START_OF_LINE,
                         CURSOR_UP,
                         CURSOR_DOWN,
                         WINDOW_SCROLL_UP,
                         WINDOW_SCROLL_DOWN,
                         CURSOR_HIDE,
                         CURSOR_END_OF_LINE,
                         WINDOW_NOP
                       };

// window creation flags
const unsigned int WINDOW_NO_CURSOR = 1,            ///< do not display the cursor
                   WINDOW_INSERT    = 2;            ///< INSERT mode
                    
// allow English spelling for colour names
const int COLOUR_BLACK   = COLOR_BLACK,
          COLOUR_RED     = COLOR_RED,
          COLOUR_GREEN   = COLOR_GREEN,
          COLOUR_YELLOW  = COLOR_YELLOW,
          COLOUR_BLUE    = COLOR_BLUE,
          COLOUR_MAGENTA = COLOR_MAGENTA,
          COLOUR_CYAN    = COLOR_CYAN,
          COLOUR_WHITE   = COLOR_WHITE;

#define COLOUR_PAIR(n)  COLOR_PAIR(n)
                       
extern pt_mutex screen_mutex;                       ///< mutex for the screen

// -----------  cursor  ----------------

/*! \class cursor
    \brief Trivial class for moving the cursor
*/

WRAPPER_2(cursor, int, x, int, y);

// -----------  colour_pair  ----------------

/*! \class colour_pair
    \brief A class to hold information about used colour pairs
*/

class cpair
{
protected:

  std::vector< std::pair< int, int> > _colours;    ///< the (globally) used colour pairs

  pt_mutex _colours_mutex;                         ///< allow thread-safe access

/*!     \brief          Private function to add a new pair of colours
        \param  p       foreground colour, background colour
        \return         the number of the colour pair
*/
  const unsigned int _add_to_vector(const std::pair< int, int>& p);

public:

/*!     \brief          Add a pair of colours
        \param  fg      foreground colour
        \param  bg      background colour
        \return         the number of the colour pair

        If the pair is already known, returns the number of the known pair.
        Note the pair number 0 cannot be changed, so we ignore it here and start counting from one
*/
  const unsigned int add(const int fg, const int bg);

/// return the foreground colour of a pair
  const int fg(const int pair_nr) const;

/// return the background colour of a pair
  const int bg(const int pair_nr) const;
};

// -----------  screen  ----------------

/*! \class screen
    \brief A dummy class to allow for easy initialisation and closing
*/

class screen
{
protected:
    
public:

/// default constructor
  screen(void);

/// destructor
  virtual ~screen(void);
};

// -----------  window_information ----------------


class window_information
{
protected:
  int    _x;                    ///< x location on the screen
  int    _y;                    ///< y location on the screen
  int    _w;                    ///< width
  int    _h;                    ///< height

  std::string _fg_colour;       ///< name of foreground colour
  std::string _bg_colour;       ///< name of background colour

  bool   _colours_set;          ///< have the colours been set explicitly?

public:

/// default constructor
  window_information(void) :
    _x(0),
    _y(0),
    _w(0),
    _h(0),
    _fg_colour("white"),
    _bg_colour("black"),
    _colours_set(false)
  { }

  READ_AND_WRITE(x);
  READ_AND_WRITE(y);
  READ_AND_WRITE(w);
  READ_AND_WRITE(h);

  READ_AND_WRITE(fg_colour);
  READ_AND_WRITE(bg_colour);

  READ_AND_WRITE(colours_set);
};


// -----------  window  ----------------

/*!     \class window
        \brief A single ncurses window
*/

// forward declaration
class window;

typedef void (* WINDOW_PROCESS_INPUT_TYPE) (window*, const keyboard_event&);

class window
{
protected:
  
  bool    _scrolling;          ///< whether scrolling is enabled
  bool    _leaveok;            ///< whether leaveok is set

  bool    _echoing;            ///< whether echoing characters
  bool    _vertical;           ///< containers of strings are to be displayed vertically
  unsigned int _column_width;  ///< width of columns

  int     _y;                  ///< y of origin (in proper coordinates)
  int     _width;              ///< width
  int     _height;             ///< height
  int     _x;                  ///< x of origin (in proper coordinates)

  int    _cursor_x;            ///< used to hold x cursor
  int    _cursor_y;            ///< used to hold y cursor
  
  WINDOW* _wp;                 ///< ncurses handle 
  PANEL*  _pp;                 ///< panel associated with this window
  
  bool   _hidden_cursor;       ///< whether to hide the cursor
  bool   _insert;              ///< whether in insert mode (default = false)
  
  int    _sx;                  ///< system cursor x value
  int    _sy;                  ///< system cursor y value
  
  int    _fg;                  ///< foreground colour
  int    _bg;                  ///< background colour

//  std::string _input_buffer;   ///< a place to put characters that have been input to the window

  WINDOW_PROCESS_INPUT_TYPE _process_input;    ///< function to handle input to this window

/// forbid copying
  window(const window&);  
  
/*!     \brief  bound a value within limits
        \param  val      value to bound
        \param  low_val  lower bound
        \param  high_val upper bound
*/
  template <class T>
  inline const T _limit(const T val, const T low_val, const T high_val)
    { return (val < low_val ? low_val : (val > high_val ? high_val : val)); }
    
public:

/*! \brief  default constructor
    \param  flags   see screen.h; possible flags are WINDOW_INSERT, WINDOW_NO_CURSOR

    The window is not ready for use after this constructor. It still needs to be initialised.
*/
  window(const unsigned int flags = 0);
  
/// create from an x, y, w, h array
//  window(const std::array<int, 4>& xywh, const unsigned int flags = 0);

/*! \brief          create using position and size information from the configuration file
    \param  wi      window position and size
    \param  flags   see screen.h; possible flags are WINDOW_INSERT, WINDOW_NO_CURSOR

    The window is ready for use after this constructor.
*/
  window(const window_information& wi, const unsigned int flags = 0);

/// destructor
  virtual ~window(void);

/*! \brief          Initialise using position and size information from the configuration file
    \param  wi      window position and size
    \param  flags   see screen.h; possible flags are WINDOW_INSERT, WINDOW_NO_CURSOR

    The window is ready for use after this function has been called.
*/
  void init(const window_information& wi, const unsigned int flags = 0);

/*! \brief          Initialise using position and size information from the configuration file, and possibly set colours explicitly
    \param  wi      window position and size
    \param  fg      foreground colour
    \param  bg      background colour
    \param  flags   see screen.h; possible flags are WINDOW_INSERT, WINDOW_NO_CURSOR

    The window is ready for use after this function has been called. <i>fg</i> and <i.bg</i>
    override <i>wi.fg_colour()</i> and <i>wi.bg_colour()</i> iff wi.colours_set() is false.
*/
  void init(const window_information& wi, int fg, int bg, const unsigned int flags = 0);

// RO access
  READ(height);                     ///< height
  READ(hidden_cursor);              ///< whether to hide the cursor
  READ(width);                      ///< width

// RW access
  READ_AND_WRITE(bg);               ///< background colour
  READ_AND_WRITE(column_width);     ///< width of columns
  READ_AND_WRITE(fg);
  READ_AND_WRITE(insert);
  READ_AND_WRITE(vertical);
  
  inline WINDOW* wp(void)
    { return _wp; }
  
/*! \brief          Move the logical cursor
    \param  new_x   x position
    \param  new_y   y position
*/
  window& move_cursor(const int new_x, const int new_y);
  
/// move cursor
  inline window& move_cursor(const cursor& c)
    { return move_cursor(c.x(), c.y()); }
  
/// move cursor relative
  window& move_cursor_relative(const int delta_x, const int delta_y);  
  
/// get cursor position
  const cursor cursor_position(void);
  
/// control scrolling
  window& scrolling(const bool enable_or_disable);

/// enable scrolling
  inline window& enable_scrolling(void)
    { return scrolling(true); }
  
/// disable scrolling
  inline window& disable_scrolling(void)
    { return scrolling(false); }

/// is scolling enabled?
  inline const bool scrolling(void) const
    { return _scrolling; }

/*!     \brief          scroll a window
        \param  n       number of lines to by which to scroll

        Can't call it 'scroll' because there's a silly ncurses *macro* with the same name
*/
  window& scrollit(const int n_lines);

/// control leaveok
  window& leave_cursor(const bool enable_or_disable);  
  
/// refresh
  window& refresh(void);
  
/// hide the window
  void hide(void);

/// show the window
  void show(void);

/// is the panel hidden?
  const bool hidden(void) const;

/// is the panel hidden?
  inline const bool is_hidden(void) const
    { return hidden(); }

/*! \brief  character processing that is the same in multiple windows
    \param  e   keyboard event to be processed
    \return whether the event was processed
*/
  const bool common_processing(const keyboard_event& e);    // processing that is the same in multiple windows

/// define the <= operator to be the same as <, except that it causes a refresh at the end of the operation
  template <class T>
  window& operator<=(const T& t)
  { if (!_wp)
      return *this;
    
    window& rv = (*this < t);
    
    { SAFELOCK(screen);

      refresh();
    }
    
    return rv;
  }

/// define the <= operator to be the same as <, except that it causes a refresh at the end of the operation
  template <class T>
  window& operator<=(T& t)
  { if (!_wp)
      return *this;
    
    window& rv = (*this < t);
    
    { SAFELOCK(screen);

      refresh();
    }
    
    return rv;
  }

/// write a string to a window
  window& operator<(const std::string& s);
  
/// write a vector of strings to a window
  window& operator<(const std::vector<std::string>& vec);

/*! \brief          Write a set of strings to a window
    \param  ss      set to write

    Wraps words to new lines. Stops writing if there's insufficient room for the next string.
*/
  window& operator<(const std::set<std::string>& ss);

/// write a vector of strings with possible different colours
  window& operator<(const std::vector<std::pair<std::string /* callsign */, int /* colour pair number */ > >& vec);

/// write an integer to a window
  inline window& operator<(const unsigned int n)
    { return (*this < to_string(n)); }

/// write an integer to a window
  inline window& operator<(const int n)
    { return (*this < to_string(n)); }
  
/// set the colour pair
  window& cpair(const int pair_nr);
  
/// set the default colours; does NOT change _fg/_bg because I can't find a guaranteed
/// way to go from a big integer that combines the two colours to the individual colours
  window& default_colours(chtype bg);

/// set the default colours; DOES change _fg/_bg
  window& default_colours(const int foreground_colour, const int background_colour);
  
/// control an attribute
  window& operator<(const enum WINDOW_ATTRIBUTES);
  
/// clear a window
  window& clear(void);
  
/*! \brief      read to end of window
    \param  x   x value from which to read (0 is leftmost column)
    \param  y   y value from which to read (0 is bottommost row)
    \return     contents of the window, starting at the position (<i>x</i>, <i>y</i>)

    By default reads the entirety of the bottom line
*/
  const std::string read(int x = 0, int y = 0);

/// read a line
  inline const std::string getline(const int line_nr = 0)
    { return read(0, line_nr); }

/// snapshot of all the contents
  const std::vector<std::string> snapshot(void);

/// is a line empty?
  inline const bool line_empty(const int line_nr = 0)
    { return remove_peripheral_spaces(getline(line_nr)).empty(); }

/// clear a line
  window& clear_line(const int line_nr = 0);

/// delete a character in the current line
  window& delete_character(const int n);

/// delete a character within a line
  window& delete_character( const int n, const int line_nr );

/// set function used to process input
  inline void process_input_function(WINDOW_PROCESS_INPUT_TYPE pf)
    { _process_input = pf; }

/// process input
  inline void process_input(const keyboard_event& e)
    { if (_process_input)
        _process_input(this, e);
    }

/// is the window empty?
  inline const bool empty(void)
    { return remove_peripheral_spaces(read()).empty(); }

/// toggle the hide/show status of the cursor
    inline window& toggle_hidden(void)
    { _hidden_cursor = !_hidden_cursor;
      return *this;
    }

/// hide the cursor
  inline window& hide_cursor(void)
    { _hidden_cursor = true;
      return *this;
    }

/// show the cursor
  inline window& show_cursor(void)
    { _hidden_cursor = false;
      return *this;
    }

/// toggle the insert mode
  inline window& toggle_insert(void)
    { _insert = !_insert;
      return *this;
    }

// http://stackoverflow.com/questions/1154212/how-could-i-print-the-contents-of-any-container-in-a-generic-way 
/* I do not understand why this template is not used in:   win_remaining_mults < (context.remaining_country_mults_list());
template
    < template<typename ELEM, typename ALLOC=std::allocator<ELEM> > class Container
    >
  window& operator<(const Container<std::string>& vec)
  { 
// ****************************** HERE ***************************
// start with the simplest case
//    for (unsigned int n = 0; n < vec.size(); ++n)
    int idx = 0;
    
    for (typename Container<std::string>::const_iterator cit = vec.begin(); cit != vec.end(); ++cit)
    { *this < (*cit);
  
      if (idx != vec.size() - 1)
        *this < " ";
      idx++;
    }
  
    return *this;
  }
*/

};

/// window < cursor
window& operator<(window& win, const cursor& c);

/// trivial class for moving the cursor (relative)
WRAPPER_2(cursor_relative, int, x, int, y);

/// window < cursor_relative
window& operator<(window& win, const cursor_relative& cr);

/// trivial class for centring a string on line y of a window
WRAPPER_2(centre, std::string, s, int, y);

/// window < centre
window& operator<(window& win, const centre& c);

/// utterly trivial class for changing colour to a colour pair
WRAPPER_1(colour_pair, int, pair_nr);
window& operator<(window& win, const colour_pair& cpair);

// the next two lines allow us to write:
// win < COLOURS(fgcolour, bgcolour)
// in order to change the colours

WRAPPER_2(COLOURS, int, fg, int, bg);

/// window < COLOURS
inline window& operator<(window& win, const COLOURS& CP)
  { return win.cpair(colours.add(CP.fg(), CP.bg())); }

inline const int FGBG(const int fg, const int bg)
{ return COLOUR_PAIR(colours.add(fg, bg));
}

// convert the name of a colour to a colour
const int string_to_colour(const std::string& str);

#endif    // SCREEN_H
