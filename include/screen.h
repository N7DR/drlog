// $Id: screen.h 166 2020-08-22 20:59:30Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef SCREEN_H
#define SCREEN_H

/*! \file   screen.h

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

extern message_stream ost;                  ///< for debugging, info

class cpair;                                ///< forward declaration for a pair of colours
extern cpair colours;                       ///< singleton used to hold a pair of colours

/// attributes and pre-defined cursor movement
enum class WINDOW_ATTRIBUTES { WINDOW_NORMAL,
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
constexpr unsigned int WINDOW_NO_CURSOR { 1 };        ///< do not display the cursor
constexpr unsigned int WINDOW_INSERT    { 2 };        ///< INSERT mode

// the tests at https://invisible-island.net/ncurses/ncurses-examples.html, ftp://ftp.invisible-island.net/ncurses-examples/ncurses-examples.tar.gz
// use NCURSES_COLOR_T as a synonym for short, which is 16 bits. Ditto for NCURSES_PAIR_T.
using COLOUR_TYPE        = short;
using PAIR_NUMBER_TYPE   = short;

/// allow English spelling for colour names; silly documentation is present so that doxygen doesn't complain
constexpr COLOUR_TYPE COLOUR_BLACK   { COLOR_BLACK },         ///< black
                      COLOUR_RED     { COLOR_RED },           ///< red
                      COLOUR_GREEN   { COLOR_GREEN },         ///< green
                      COLOUR_YELLOW  { COLOR_YELLOW },        ///< yellow
                      COLOUR_BLUE    { COLOR_BLUE },          ///< blue
                      COLOUR_MAGENTA { COLOR_MAGENTA },       ///< magenta
                      COLOUR_CYAN    { COLOR_CYAN },          ///< cyan
                      COLOUR_WHITE   { COLOR_WHITE };         ///< white
                       
extern pt_mutex screen_mutex;                   ///< mutex for the screen

/*! \brief      Return a pair of colours
    \param  n   pair number
    \return     the pair of colours that constitute pair number <i>n</i>
    
    This is a workaround for the macro COLOR_PAIR().
    
    I note that ncurses is inconsistent about the types used to hold colour pairs; the actual definition of COLOR_PAIR()
    defines the return value as an int, so that's what we use here
*/
int COLOUR_PAIR(const PAIR_NUMBER_TYPE n);

// -----------  cursor  ----------------

/// class used for moving the cursor; encapsulates x,y coordinates
WRAPPER_2(cursor, int, x, int, y);

// -----------  cpair  ----------------

/*! \class  cpair
    \brief  A class to hold information about used colour pairs. There should be only one instantiation of this
*/

class cpair
{
protected:

  std::vector< std::pair< COLOUR_TYPE, COLOUR_TYPE> > _colours;    ///< the (globally) used colour pairs

  pt_mutex _colours_mutex { "CPAIR COLOURS"s };                         ///< allow thread-safe access

/*! \brief      Private function to add a new pair of colours
    \param  p   foreground colour, background colour
    \return     the number of the colour pair
*/
  PAIR_NUMBER_TYPE _add_to_vector(const std::pair<COLOUR_TYPE, COLOUR_TYPE>& p);

public:

  cpair(void) = default;
  cpair(const cpair&) = delete;

/*! \brief          Add a pair of colours
    \param  fg      foreground colour
    \param  bg      background colour
    \return         the number of the colour pair

    If the pair is already known, returns the number of the known pair.
    Note that the pair number 0 cannot be changed, so we ignore it here and start counting from one
*/
  PAIR_NUMBER_TYPE add(const COLOUR_TYPE fg, const COLOUR_TYPE bg);

/*! \brief              Get the foreground colour of a pair
    \param  pair_nr     number of the pair
    \return             the foreground colour of the pair number <i>pair_nr</i>
*/
  COLOUR_TYPE fg(const PAIR_NUMBER_TYPE pair_nr) const;

/*! \brief              Get the background colour of a pair
    \param  pair_nr     number of the pair
    \return             the background colour of the pair number <i>pair_nr</i>
*/
  COLOUR_TYPE bg(const PAIR_NUMBER_TYPE pair_nr) const;
};

// -----------  screen  ----------------

/*! \class  screen
    \brief  A dummy class to allow for easy initialisation and termination
*/

class screen
{
protected:
    
public:

/// default constructor
  screen(void);

/// destructor
  inline /* virtual */ ~screen(void)
    { endwin(); }
};

// -----------  window_information ----------------

/*! \class  window_information
    \brief  encapsulate position and colour information of a window
*/

class window_information
{
protected:

  int   _x { 0 };                     ///< x location on the screen
  int   _y { 0 };                     ///< y location on the screen
  int   _w { 0 };                     ///< width
  int   _h { 0 };                     ///< height

  std::string _fg_colour { "white"s };       ///< name of foreground colour
  std::string _bg_colour { "black"s };       ///< name of background colour

  bool _colours_set { false };           ///< have the colours been set explicitly?

public:

/// default constructor
  window_information(void) = default;

  READ_AND_WRITE(x);                ///< x location on the screen
  READ_AND_WRITE(y);                ///< y location on the screen
  READ_AND_WRITE(w);                ///< width
  READ_AND_WRITE(h);                ///< height

  READ_AND_WRITE(fg_colour);        ///< name of foreground colour
  READ_AND_WRITE(bg_colour);        ///< name of background colour

  READ_AND_WRITE(colours_set);      ///< have the colours been set explicitly?
};

// -----------  window  ----------------

/*! \class  window
    \brief  A single ncurses window
*/

// forward declaration
class window;

/// define the type used for functions that process events
using WINDOW_PROCESS_INPUT_TYPE = void (*) (window*, const keyboard_event&);

class window
{
protected:
  
  std::string   _name               { EMPTY_STR };    ///< (optional) name of window
  
  unsigned int  _column_width;      ///< width of columns
  int           _cursor_x;          ///< used to hold x cursor
  int           _cursor_y;          ///< used to hold y cursor
  bool          _echoing;           ///< whether echoing characters
  int           _height;            ///< height
  bool          _hidden_cursor;     ///< whether to hide the cursor
  bool          _insert;            ///< whether in insert mode (default = false)
  bool          _leaveok;           ///< whether leaveok is set
  bool          _scrolling;         ///< whether scrolling is enabled
  bool          _vertical;          ///< whether containers of strings are to be displayed vertically
  int           _width;             ///< width
  int           _x;                 ///< x of origin (in proper coordinates)
  int           _y;                 ///< y of origin (in proper coordinates)
  
  WINDOW* _wp;                  ///< ncurses handle
  PANEL*  _pp;                  ///< panel associated with this window

  int    _sx;                   ///< system cursor x value
  int    _sy;                   ///< system cursor y value
  
  COLOUR_TYPE    _fg;                   ///< foreground colour
  COLOUR_TYPE    _bg;                   ///< background colour

  WINDOW_PROCESS_INPUT_TYPE _process_input;    ///< function to handle input to this window

/*! \brief          Set the default colours
    \param  fgbg    colour pair
    \return         the window

    Does not change _fg/_bg because I can't find a guaranteed way to go from a big integer that
    combines the two colours to the individual colours
*/
    window& _default_colours(const chtype fgbg);

/*! \brief          Perform basic initialisation
    \param  wi      window information
    \param  flags   possible flags are WINDOW_INSERT, WINDOW_NO_CURSOR
*/
  void _init(const window_information& wi, const unsigned int flags);

public:

/*! \brief          Default constructor
    \param  flags   see screen.h; possible flags are WINDOW_INSERT, WINDOW_NO_CURSOR

    The window is not ready for use after this constructor: it still needs to be initialised.
*/
  inline explicit window(const std::string& win_name = ""s, const unsigned int flags = 0) :
    _name(win_name),
    _x(0),
    _y(0),
    _width(0),
    _height(0),
    _vertical(false),
    _column_width(0),
    _wp(nullptr),
    _scrolling(false),
    _hidden_cursor(flags bitand WINDOW_NO_CURSOR),
    _insert(flags bitand WINDOW_INSERT),
    _pp(nullptr),
    _process_input(nullptr),
    _fg(COLOUR_WHITE),
    _bg(COLOUR_BLACK)
  { _default_colours(COLOUR_PAIR(colours.add(_fg, _bg))); }

/*! \brief          Create using position and size information from the configuration file
    \param  wi      window position and size
    \param  flags   see screen.h; possible flags are WINDOW_INSERT, WINDOW_NO_CURSOR

    The window is ready for use after this constructor.
*/
  window(const window_information& wi, const unsigned int flags = 0);

/// forbid copying
  window(const window&) = delete;

/// destructor
  ~window(void);

/*! \brief          Initialise using position and size information from the configuration file
    \param  wi      window position and size
    \param  flags   see screen.h; possible flags are WINDOW_INSERT, WINDOW_NO_CURSOR

    The window is ready for use after this function has been called.
*/
  void init(const window_information& wi, const unsigned int flags = 0);

/*! \brief          Initialise using position and size information from the configuration file, and possibly set colours explicitly
    \param  wi      window position, size and (possibly) colour
    \param  fg      foreground colour
    \param  bg      background colour
    \param  flags   see screen.h; possible flags are WINDOW_INSERT, WINDOW_NO_CURSOR

    The window is ready for use after this function has been called. <i>fg</i> and <i>bg</i>
    override <i>wi.fg_colour()</i> and <i>wi.bg_colour()</i> iff wi.colours_set() is false.
*/
  void init(const window_information& wi, const COLOUR_TYPE fg, const COLOUR_TYPE bg, const unsigned int flags = 0);

// RO access
  READ(height);                     ///< height
  READ(hidden_cursor);              ///< whether to hide the cursor
  READ(width);                      ///< width

// RW access
  READ_AND_WRITE(bg);               ///< background colour
  READ_AND_WRITE(column_width);     ///< width of columns
  READ_AND_WRITE(fg);               ///< foreground colour
  READ_AND_WRITE(insert);           ///< whether in insert mode
  READ_AND_WRITE(vertical);         ///< whether containers of strings are to be displayed vertically
  
/*! \brief      Get a pointer to the underlying WINDOW
    \return     a pointer to the WINDOW
*/
  inline WINDOW* wp(void) const
    { return _wp; }
  
/*! \brief      Is the window usable?
    \return     whether the window is usable
*/
  inline bool defined(void) const
    { return (_wp != nullptr); }

/*! \brief      Is the window usable?
    \return     whether the window is usable

    Synonym for defined()
*/
  inline bool valid(void) const
    { return defined(); }

/*! \brief          Move the logical cursor
    \param  new_x   x position
    \param  new_y   y position
    \return         the window
*/
  window& move_cursor(const int new_x, const int new_y);
 
/*! \brief      Move the logical cursor
    \param  c   new cursor (used to derive the new location)
    \return     the window
*/
  inline window& move_cursor(const cursor& c)
    { return move_cursor(c.x(), c.y()); }
  
/*! \brief              Move the logical cursor (relative to current location)
    \param  delta_x     change in x position
    \param  delta_y     change in y position
    \return             the window
*/
  window& move_cursor_relative(const int delta_x, const int delta_y);  
  
/*! \brief      Get cursor position
    \return     the current position of the cursor
*/
  cursor cursor_position(void);

/*! \brief                      Control scrolling
    \param  enable_or_disable   whether to enable scrolling
    \return                     the window
*/
  window& scrolling(const bool enable_or_disable);

/*! \brief      Enable scrolling
    \return     the window
*/
  inline window& enable_scrolling(void)
    { return scrolling(true); }
  
/*! \brief      Disable scrolling
    \return     the window
*/
  inline window& disable_scrolling(void)
    { return scrolling(false); }

/*! \brief      Is scrolling enabled?
    \return     whether scrolling is enabled
*/
  inline bool scrolling(void) const
    { return _scrolling; }

/*! \brief              scroll a window
    \param  n_lines     number of lines to by which to scroll
    \return             the window

    Can't call it 'scroll' because there's a silly ncurses *macro* with the same name
*/
  window& scrollit(const int n_lines);

/*! \brief                      Control leaveok
    \param  enable_or_disable   whether to enable scrolling
    \return                     the window
*/
  window& leave_cursor(const bool enable_or_disable);  
  
/*! \brief      Refresh the window
    \return     the window
*/
  window& refresh(void);
  
/// hide the window
  void hide(void);

/// show the window
  void show(void);

/// is the panel hidden?
  inline bool hidden(void) const
    { return (_pp ? static_cast<bool>(panel_hidden(_pp)) : false); }

/*! \brief      Is the window hidden?
    \return     whether the window is hidden
*/
  inline bool is_hidden(void) const
    { return hidden(); }

/*! \brief      Character processing that is the same in multiple windows
    \param  e   keyboard event to be processed
    \return     whether the event was processed
*/
  bool common_processing(const keyboard_event& e);

/*! \brief      define the <= operator to be the same as <, except that it causes a refresh at the end of the operation
    \param  t   the object to be output
    \return     the window

    The universal reference is needed in order to handle the case where t is an rvalue
*/
  template <class T>
  window& operator<=(T&& t)
  { if (!_wp)
      return *this;
    
    window& rv { (*this < t) };
    
    { SAFELOCK(screen);

      refresh();
    }
    
    return rv;
  }

/*! \brief      Write a string to the window
    \param  s   the string to write
    \return     the window

    wprintw has fairly obnoxious behaviour regarding newlines: if a string
    reaches the end of a window line, then an LF is automatically added. For
    now we live with this, but we might want at some time to write a more complex
    function that performs the writes without ever (silently) adding something
    to the string

    Also see the function reformat_for_wprintw().
*/
  window& operator<(const std::string& s);

/*! \brief      Write a vector of strings to a window
    \param  v   the vector of strings to be written
    \return     the window

    Wraps words to new lines. Stops writing if there's insufficient room for the next string.
*/
  window& operator<(const std::vector<std::string>& v);

/*! \brief          Write a set or unordered set of strings to a window
    \param  sus     set/unordered set to write
    \return         the window

    The set/unordered set is written in callsign order.
    Wraps words to new lines.
    Stops writing if there's insufficient room for the next string.
*/
template <class T>
window& operator<(const T& sus)
  requires (is_set_v<T> == true || is_unordered_set_v<T> == true) && (std::is_same_v<typename T::value_type, std::string> == true)
{ if (!_wp)
    return *this;

  std::vector<std::string> v { sus.cbegin(), sus.cend() };

//  sort(v.begin(), v.end(), compare_calls);
  SORT(v, compare_calls);

  return (*this < v);
}

//  window& operator<(const std::set<std::string>& ss);

/*! \brief          Write a set of strings to a window
    \param  ss      set to write
    \return         the window

    The set is written in callsign order.
    Wraps words to new lines.
    Stops writing if there's insufficient room for the next string.
*/
//  window& operator<(const std::unordered_set<std::string>& ss);

/*! \brief          Write a vector of strings with possible different colours to a window
    \param  vec     vector of pairs <string, int [colour number]> to write
    \return         the window

    Wraps words to new lines. Stops writing if there's insufficient room for the next string.
*/
  window& operator<(const std::vector<std::pair<std::string /* callsign */, PAIR_NUMBER_TYPE /* colour pair number */ > >& vec);

/*! \brief      Write an integral type to a window
    \param  n   value to write
    \return     the window
*/
template <class T>
window& operator<(const T n)
  requires (std::is_integral_v<T> == true)
    { return (*this < to_string(n)); }

/*! \brief      Write an integer to a window
    \param  n   integer to write
    \return     the window
*/
//  inline window& operator<(const int n)
//    { return (*this < to_string(n)); }

/*! \brief      Write a uint64_t to a window
    \param  n   uint64_t to write
    \return     the window
*/
//  inline window& operator<(const uint64_t n)
//    { return (*this < to_string(n)); }
  
/*! \brief              Set the colour pair
    \param  pair_nr     number of the new colour pair
    \return             the window
*/
  window& set_colour_pair(const PAIR_NUMBER_TYPE pair_nr);

/*! \brief                      Set the default colours
    \param  foreground_colour   foreground colour
    \param  background_colour   background colour
    \return                     the window
*/
  window& default_colours(const COLOUR_TYPE foreground_colour, const COLOUR_TYPE background_colour);
  
/*! \brief      Control an attribute or perform a simple operation
    \param  wa  the attribute or operation
    \return     the window
*/
  window& operator<(const enum WINDOW_ATTRIBUTES wa);
  
/*! \brief      Clear the window
    \return     the window
*/
  window& clear(void);
  
/*! \brief      Read to end of window
    \param  x   x value from which to read (0 is leftmost column)
    \param  y   y value from which to read (0 is bottommost row)
    \return     contents of the window, starting at the position (<i>x</i>, <i>y</i>)

    By default reads the entirety of the bottom line.
    Limits both <i>x</i> and <i>y</i> to valid values for the window before reading the line.
*/
  std::string read(int x = 0, int y = 0);

/*! \brief              Read a line
    \param  line_nr     number of line to read (0 is bottommost row)
    \return             contents of line number <i>line_nr</i>

    Limits <i>line_nr</i> to a valid value for the window before reading the line.
*/
  inline std::string getline(const int line_nr = 0)
    { return read(0, line_nr); }

/*! \brief      Obtain a line-by-line snapshot of all the contents; lines go from top to bottom
    \return     a line-by-line snapshot of all the contents; element [0] is the top line
*/
  std::vector<std::string> snapshot(void);

/*! \brief              Is a line empty?
    \param  line_nr     number of line to test (0 is bottommost row)
    \return             whether line number <i>line_nr</i> is empty

    Removes any blank spaces before testing.
    Limits <i>line_nr</i> to a valid value for the window before testing the line.
*/
  inline bool line_empty(const int line_nr = 0)
    { return remove_peripheral_spaces(getline(line_nr)).empty(); }

/*! \brief              Clear a line
    \param  line_nr     number of line to clear (0 is bottommost row)
    \return             the window

    Limits <i>line_nr</i> to a valid value for the window before clearing the line.
*/
  window& clear_line(const int line_nr = 0);

/*! \brief      Delete a character in the current line
    \param  n   number of character to delete (wrt 0)
    \return     the window

    Does nothing if character number <i>n</i> does not exist
*/
  window& delete_character(const int n);

/*! \brief          Delete a character within a particular line
    \param  n       number of character to delete (wrt 0)
    \param line_nr  number of line (wrt 0)
    \return         the window

    Line number zero is the bottom line
*/
  window& delete_character( const int n, const int line_nr );

/// set function used to process input
  inline void process_input_function(WINDOW_PROCESS_INPUT_TYPE pf)
    { _process_input = pf; }

/// process a keyboard event
  inline void process_input(const keyboard_event& e)
    { if (_process_input != nullptr)
        _process_input(this, e);
    }

/// is the window empty?
  inline bool empty(void)
    { return remove_peripheral_spaces(read()).empty(); }

/// toggle the hide/show status of the cursor
  inline window& toggle_hidden(void)
    { return (_hidden_cursor = !_hidden_cursor, *this); }

/// hide the cursor
  inline window& hide_cursor(void)
    { return (_hidden_cursor = true, *this); }

/// show the cursor
  inline window& show_cursor(void)
    { return (_hidden_cursor = false, *this); }

/// toggle the insert mode
  inline window& toggle_insert(void)
    { return (_insert = !_insert, *this); }

/*! \brief          Obtain a readable description of the window properties
    \return         the window properties as a human-readable string
    
    Cannot be const, as it uses snapshot, which internally moves the cursor and restores it
*/
std::string properties(const std::string& name = std::string());

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

/*! \brief          Move the cursor in a window
    \param  win     the window to be affected
    \param  c       cursor coordinates to which to move
    \return         the window
*/
inline window& operator<(window& win, const cursor& c)
  { return win.move_cursor(c.x(), c.y()); }

/// trivial class for moving the cursor (relative)
WRAPPER_2(cursor_relative, int, x, int, y);

/*! \brief          Move the cursor in a window, using relative movement
    \param  win     the window to be affected
    \param  cr      (relative) cursor coordinates to which to move
    \return         the window
*/
inline window& operator<(window& win, const cursor_relative& cr)
  { return win.move_cursor_relative(cr.x(), cr.y()); }

/// trivial class for centring a string on line y of a window
WRAPPER_2(centre, std::string, s, int, y);

/*! \brief          Write a centred string in a window
    \param  win     target window
    \param  c       object containing the string to be written, and the vertical line number on which it is to be written
    \return         the window

    Correctly accounts for UTF-8 encoding
*/
window& operator<(window& win, const centre& c);

/// utterly trivial class for changing colour to a colour pair
WRAPPER_1(colour_pair, PAIR_NUMBER_TYPE, pair_nr);

/*! \brief          Change the colours of a window
    \param  win     target window
    \param  cpair   the new colours
    \return         the window
*/
inline window& operator<(window& win, const colour_pair& cpair)
  { return win.set_colour_pair(cpair.pair_nr()); }

// the next two lines allow us to write:
// win < COLOURS(fgcolour, bgcolour)
// in order to change the colours

/// encapsulate foreground and background colours
WRAPPER_2(COLOURS, COLOUR_TYPE, fg, COLOUR_TYPE, bg);

/// window < COLOURS
inline window& operator<(window& win, const COLOURS& CP)
  { return win.set_colour_pair(colours.add(CP.fg(), CP.bg())); }

/// obtain colour pair corresponding to foreground and background colours
inline int FGBG(const COLOUR_TYPE fg, const COLOUR_TYPE bg)
  { return COLOUR_PAIR(colours.add(fg, bg)); }

/*! \brief          Convert the name of a colour to a colour
    \param  str     name of a colour
    \return         the colour corresponding to <i>str</i>
*/
COLOUR_TYPE string_to_colour(const std::string& str);

#endif    // SCREEN_H
