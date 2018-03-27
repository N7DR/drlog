// $Id: audio.cpp 145 2018-03-19 17:28:50Z  $

// Released under the GNU Public License, version 2

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   audio.cpp

    Classes and functions anent audio recording. The implementation uses ALSA calls.
    A version intended to support OSes in addition to Linux could, for example, use sox instead.
*/

#include "audio.h"
#include "log_message.h"
#include "string_functions.h"

#include <chrono>
#include <limits>
#include <thread>

using namespace std;
using namespace   chrono;        // std::chrono
using namespace   this_thread;   // std::this_thread

extern bool             exiting;                ///< whether drlog is in the process of exiting
extern int              n_running_threads;      ///< the number of running threads
extern message_stream   ost;                    ///< for debugging and logging
extern pt_mutex         thread_check_mutex;     ///< mutex for controllinh threads

extern void         end_of_thread(const string& name);      ///< increase the counter for the number of running threads
extern const string hhmmss(void);                           ///< obtain the current time in HH:MM:SS format
extern const bool   sending_cw(void);                       ///< am I sending Cw?
extern void         start_of_thread(const string& name);    ///< increase the counter for the number of running threads

// -----------  audio_recorder  ----------------

/*! \class  audio_recorder
    \brief  Class to implement the needed recording functions
*/

/*! \brief      Calculate the total number of bytes to be read from the device
    \return     total number of bytes to be read from the sound board

    Returned value is based on the duration and the number of bytes to be read per second
*/
const int64_t audio_recorder::_total_bytes_to_read(void)
{ int64_t total_bytes;

// how many seconds to next time marker?
// current seconds past midnight
  const string now_str = hhmmss();                                      // HH:MM:SS
  const uint64_t hh = from_string<uint64_t>(substring(now_str, 0, 2));
  const uint64_t mm = from_string<uint64_t>(substring(now_str, 3, 2));
  const uint64_t ss = from_string<uint64_t>(substring(now_str, 6, 2));
  const uint64_t now = ss + (mm * 60) + (hh * 3600);

  int remainder = _max_file_time - (now % _max_file_time);

  if (remainder == 0)
    remainder = _max_file_time;

  _time_limit = remainder;

  if (_time_limit == 0)
    total_bytes = _record_count;
  else
  { const decltype(_record_count) bytes_per_second = snd_pcm_format_size(_hw_params.format, _hw_params.rate * _hw_params.channels);

    total_bytes = bytes_per_second * _time_limit;
  }

  return min(total_bytes, _record_count);
}

/*! \brief  Set the parameters for the recording

    Much of this is converted (more or less slavishly) from aplay.c. Probably much of this
    function could be elided, but the lack of explanation of what's going on in aplay.c means
    that it's safer, for now, to leave it in.
*/
void audio_recorder::_set_params(void)
{ snd_pcm_hw_params_t* params = nullptr;
  snd_pcm_sw_params_t* swparams = nullptr;

  snd_pcm_hw_params_alloca(&params);
  snd_pcm_sw_params_alloca(&swparams);

  int err = snd_pcm_hw_params_any(_handle, params);

  if (err < 0)
  { ost << "ERROR: cannot obtain configuration for: " << _pcm_name << endl;
    ost << "Error number is: " << err << " [" << snd_strerror(err) << "]" << endl;
    throw audio_error(AUDIO_NO_CONFIGURATION, "Cannot obtain configuration for: " + _pcm_name);
  }
  else
    ost << "ALSA configuration retrieved for " << _pcm_name << endl;

  err = snd_pcm_hw_params_set_access(_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

  if (err < 0)
  { ost << "ERROR: access type not available for: " << _pcm_name << endl;
    ost << "Error number is: " << err << " [" << snd_strerror(err) << "]" << endl;
    throw audio_error(AUDIO_NO_ACCESS_TYPE, "Access type not available for: " + _pcm_name);
  }

  err = snd_pcm_hw_params_set_format(_handle, params, _hw_params.format);

  if (err < 0)
  { ost << "ERROR: sample format " << _hw_params.format << " not available for: " << _pcm_name << endl;
    ost << "Error number is: " << err << " [" << snd_strerror(err) << "]" << endl;
    throw audio_error(AUDIO_NO_SAMPLE_FORMAT, "Sample format " + to_string(_hw_params.format) + " not available for: " + _pcm_name);
  }

  err = snd_pcm_hw_params_set_channels(_handle, params, _hw_params.channels);

  if (err < 0)
  { ost << "ERROR: channel count " << _hw_params.channels << " not available for: " << _pcm_name << endl;
    ost << "Error number is: " << err << " [" << snd_strerror(err) << "]" << endl;
    throw audio_error(AUDIO_NO_CHANNEL_COUNT, "Channel count " + to_string(_hw_params.channels) + " not available for: " + _pcm_name);
  }

  unsigned int rate = _hw_params.rate;
  err = snd_pcm_hw_params_set_rate_near(_handle, params, &_hw_params.rate, 0);

  if (err < 0)
  { ost << "ERROR: unable to set rate: " << _pcm_name << endl;
    ost << "Error number is: " << err << " [" << snd_strerror(err) << "]" << endl;
    throw audio_error(AUDIO_RATE_SET_ERROR, "Channel count not available for: " + _pcm_name);
  }

  if ((float)rate * 1.05 < _hw_params.rate or (float)rate * 0.95 > _hw_params.rate)
  { ost << "ERROR: inaccurate rate; requested " << rate << ", received " << _hw_params.rate << "for device: " << _pcm_name << endl;
    throw audio_error(AUDIO_INACCURATE_RATE, "inaccurate rate; requested " + to_string(rate) + ", received " + to_string(_hw_params.rate) + "for device: " + _pcm_name);
  }

  rate = _hw_params.rate;

  if (_buffer_time == 0 and _buffer_frames == 0)
  { err = snd_pcm_hw_params_get_buffer_time_max(params, &_buffer_time, 0);

    if (err < 0)
    { ost << "ERROR: buffer time = " << _buffer_time << " for " << _pcm_name << endl;
      ost << "Error number is: " << err << " [" << snd_strerror(err) << "]" << endl;
      throw audio_error(AUDIO_INVALID_BUFFER_TIME, "Invalid buffer time = " + to_string(_buffer_time) + " for " + _pcm_name);
    }

    _buffer_time = min(_buffer_time, static_cast<unsigned int>(500000));
  }

  if (_period_time == 0 and _period_frames == 0)
  { if (_buffer_time > 0)
      _period_time = _buffer_time / 4;
    else
      _period_frames = _buffer_frames / 4;
  }

  if (_period_time > 0)
    err = snd_pcm_hw_params_set_period_time_near(_handle, params, &_period_time, 0);
  else
    err = snd_pcm_hw_params_set_period_size_near(_handle, params, &_period_frames, 0);

  if (err < 0)
  { ost << "ERROR: invalid period for " << _pcm_name << endl;
    ost << "Error number is: " << err << " [" << snd_strerror(err) << "]" << endl;
    throw audio_error(AUDIO_INVALID_PERIOD, "Invalid period for " + _pcm_name);
  }

  if (_buffer_time > 0)
    err = snd_pcm_hw_params_set_buffer_time_near(_handle, params, &_buffer_time, 0);
  else
    err = snd_pcm_hw_params_set_buffer_size_near(_handle, params, &_buffer_frames);

  if (err < 0)
  { ost << "ERROR: invalid buffer for " << _pcm_name << endl;
    ost << "Error number is: " << err << " [" << snd_strerror(err) << "]" << endl;
    throw audio_error(AUDIO_INVALID_BUFFER, "Invalid buffer for " + _pcm_name);
  }

  _monotonic = (snd_pcm_hw_params_is_monotonic(params) == 1);
  err = snd_pcm_hw_params(_handle, params);

  if (err < 0)
  { ost << "ERROR: unable to install params for " << _pcm_name << endl;
    ost << "Error number is: " << err << " [" << snd_strerror(err) << "]" << endl;
    throw audio_error(AUDIO_CANNOT_INSTALL_HW_PARAMS, "Unable to install hardware parameters for " + _pcm_name);
  }

//http://www.alsa-project.org/main/index.php/FramesPeriods
  err = snd_pcm_hw_params_get_period_size(params, &_period_size_in_frames, 0);

  if (err < 0)
  { ost << "ERROR: unable to get period size for " << _pcm_name << endl;
    ost << "Error number is: " << err << " [" << snd_strerror(err) << "]" << endl;
   throw audio_error(AUDIO_UNABLE_TO_GET_PERIOD_SIZE, "Unable to get period size for " + _pcm_name);
  }
  else
    ost << "ALSA period size in frames = " << _period_size_in_frames << endl;

  snd_pcm_uframes_t buffer_size;

  err = snd_pcm_hw_params_get_buffer_size(params, &buffer_size);

  if (err < 0)
  { ost << "ERROR: unable to get buffer size for " << _pcm_name << endl;
    ost << "Error number is: " << err << " [" << snd_strerror(err) << "]" << endl;
    throw audio_error(AUDIO_UNABLE_TO_GET_BUFFER_SIZE, "Unable to get period size for " + _pcm_name);
  }

// buffer must be larger than the number of bytes needed to hold a period
  if (_period_size_in_frames == buffer_size)
  { ost << "ERROR: buffer size = period_size_in_frames = " << _period_size_in_frames << " for " << _pcm_name << endl;
    throw audio_error(AUDIO_EQUAL_PERIOD_AND_BUFFER_SIZE, "Buffer size = period_size_in_frames = " + to_string(_period_size_in_frames) + " for " + _pcm_name);
  }

  err = snd_pcm_sw_params_current(_handle, swparams);

  if (err < 0)
  { ost << "ERROR: unable to get SW parameters for " << _pcm_name << endl;
    ost << "Error number is: " << err << " [" << snd_strerror(err) << "]" << endl;
    throw audio_error(AUDIO_UNABLE_TO_GET_SW_PARAMS, "Unable to get SW parameters for " + _pcm_name);
  }

  size_t n = _period_size_in_frames;
  err = snd_pcm_sw_params_set_avail_min(_handle, swparams, n);

  if (err < 0)
  { ost << "ERROR: unable to set avail_min for " << _pcm_name << endl;
    ost << "Error number is: " << err << " [" << snd_strerror(err) << "]" << endl;
    throw audio_error(AUDIO_UNABLE_TO_SET_AVAIL_MIN, "Unable to set avail_min for " + _pcm_name);
  }

/* round up to closest transfer boundary */
  n = buffer_size;

  snd_pcm_uframes_t start_threshold = ( (double)rate * _start_delay / 1000000 ) + ( (_start_delay <= 0) ? n : 0 );

  start_threshold = LIMIT(start_threshold, 1, n);
  err = snd_pcm_sw_params_set_start_threshold(_handle, swparams, start_threshold);

  if (err < 0)
  { ost << "ERROR: unable to set start threshold for " << _pcm_name << endl;
    ost << "Error number is: " << err << " [" << snd_strerror(err) << "]" << endl;
    throw audio_error(AUDIO_UNABLE_TO_SET_START_THRESHOLD, "Unable to set start threshold for " + _pcm_name);
  }

  const snd_pcm_uframes_t stop_threshold = buffer_size;

  err = snd_pcm_sw_params_set_stop_threshold(_handle, swparams, stop_threshold);

  if (err < 0)
  { ost << "ERROR: unable to set stop threshold for " << _pcm_name << endl;
    ost << "Error number is: " << err << " [" << snd_strerror(err) << "]" << endl;
    throw audio_error(AUDIO_UNABLE_TO_SET_STOP_THRESHOLD, "Unable to set stop threshold for " + _pcm_name);
  }

  err = snd_pcm_sw_params(_handle, swparams);

  if (err < 0)
  { ost << "ERROR: unable to install software params for " << _pcm_name << endl;
    ost << "Error number is: " << err << " [" <<  strerror(err) << "]" << endl;
    throw audio_error(AUDIO_CANNOT_INSTALL_SW_PARAMS, "Unable to install software parameters for " + _pcm_name);
  }

// at least for now, no support for setup_chmap()
  const size_t bits_per_sample = snd_pcm_format_physical_width(_hw_params.format);
  _bits_per_frame = bits_per_sample * _hw_params.channels;
  _period_size_in_bytes = _period_size_in_frames * _bits_per_frame / 8;

  _audio_buf = (u_char *)malloc(_period_size_in_bytes);

  if (_audio_buf == NULL)
  { ost << "ERROR: out of memory for " << _pcm_name << endl;
    throw audio_error(AUDIO_NO_MEMORY, "Out of memory for " + _pcm_name);
  }

  _buffer_frames = buffer_size;    /* for position test */ // ?????
}

/*! \brief          Read from the PCM device
    \param  data    buffer in which to store the read data
    \return         total number of bytes read
*/
const ssize_t audio_recorder::_pcm_read(u_char* data)
{ size_t result = 0;
  size_t count = _period_size_in_frames;

  while (count > 0)
  { const ssize_t r = _readi_func(_handle, data, count);

    if (r == -EAGAIN or (r >= 0 and (size_t)r < count))
    { snd_pcm_wait(_handle, 100);
    }
    else if (r == -EPIPE)
    { ost << "XRUN()!!" << endl;
    }
    else if (r == -ESTRPIPE)
    { ost << "SUSPEND()!!!" << endl;
    }
    else if (r < 0)
    { ost << "ERROR: audio device read error for " << _pcm_name << endl;
      ost << "Error number is: " << r << " [" <<  strerror(r) << "]" << endl;
     throw audio_error(AUDIO_DEVICE_READ_ERROR, "Error reading audio device for " + _pcm_name);
    }

    if (r > 0)
    { result += r;
      count -= r;
      data += r * _bits_per_frame / 8;
    }
  }

  return result;
}

/*! \brief          Wrapper function to capture audio
    \param  arg     "this" pointer
    \return         nullptr
*/
void* audio_recorder::_static_capture(void* arg)
{ audio_recorder* arp = static_cast<audio_recorder*>(arg);

  arp->_capture(nullptr);

  return nullptr;
}

/*! \brief          Close the WAV file associated with a wav_file
    \param  vp      pointer to a wav_file
    \return         nullptr
*/
void* close_it(void* vp)
{ start_of_thread("close_WAV");

  wav_file* wfp = (wav_file*)(vp);

  wfp->close();
  delete wfp;

  end_of_thread("close_WAV");
  return nullptr;
}

/*! \brief          Capture audio
    \return         nullptr
*/
void* audio_recorder::_capture(void*)
{ ost << "starting _capture() thread" << endl;

  const string thread_name = "_capture_" + to_string(_thread_number++);

//  start_of_thread("_capture");
  start_of_thread(thread_name);

  _aborting = false;

create_file:
  wav_file* wfp = new wav_file;

  wfp->name(_base_filename + "-" + date_time_string(INCLUDE_SECONDS));
  wfp->open();

#if 0
// insert bext chunk
  bext_chunk bext;

  bext.description(_description);
  bext.originator("drlog");
  bext.originator_reference(_originator_reference);

  const string dts = date_time_string(INCLUDE_SECONDS);

  bext.origination_date(substring(dts, 0, 10));  //  yyyy:mm:dd; separator can be anything; we use "-"
  bext.origination_time(substring(dts, 11, 8));  //  hh:mm:ss; separator can be anything; we use ":"

// seconds past midnight
  const string now_str = hhmmss();
  const uint64_t hh = from_string<uint64_t>(substring(now_str, 0, 2));
  const uint64_t mm = from_string<uint64_t>(substring(now_str, 2, 2));
  const uint64_t ss = from_string<uint64_t>(substring(now_str, 4, 2));

  const uint64_t now = ss + mm * 60 + hh * 3600;
  const uint64_t time_reference = static_cast<uint64_t>(_samples_per_second) * now;

  uint32_t* ptr = (uint32_t*)(&time_reference);

  _time_reference_low = ptr[0];
  _time_reference_high = ptr[1];  // need to check that these are in the correct order

  bext.time_reference_low(_time_reference_low);
  bext.time_reference_high(_time_reference_high);
#endif

// insert fmt chunk
  fmt_chunk fmt;

  fmt.num_channels(_n_channels);
  fmt.sample_rate(_samples_per_second);
  fmt.bits_per_sample(16);               // to match _sample_format

  wfp->add_chunk(fmt);

  int64_t remaining_bytes_to_read = _total_bytes_to_read();

  if (remaining_bytes_to_read == 0)
    remaining_bytes_to_read = numeric_limits<decltype(remaining_bytes_to_read)>::max();

/* WAVE-file should be even (I'm not sure), but wasting one byte
   isn't a problem (this can only be in 8 bit mono) */
  if (remaining_bytes_to_read < numeric_limits<decltype(remaining_bytes_to_read)>::max())
    remaining_bytes_to_read += remaining_bytes_to_read % 2;
  else
    remaining_bytes_to_read -= remaining_bytes_to_read % 2;

  if (remaining_bytes_to_read > numeric_limits<int32_t>::max())
    remaining_bytes_to_read = numeric_limits<int32_t>::max();

  bool first_time_through_loop = true;

  ost << "about to enter the big loop" << endl;

  do
  { const size_t c = (remaining_bytes_to_read <= (off64_t)_period_size_in_bytes) ? (size_t)remaining_bytes_to_read : _period_size_in_bytes;
    const size_t f = c * 8 / _bits_per_frame;
    const int pr = _pcm_read(_audio_buf);

    if (pr != static_cast<int>(f))
    { ost << "WARNING: pr != f" << endl;
        break;
    }

// read OK from sound card
    if (first_time_through_loop)            // write data chunk if we've just started
    { wfp->add_chunk(data_chunk(_audio_buf, f));
      first_time_through_loop = false;
    }
    else                                    // otherwise append the data we just read
      wfp->append_data(_audio_buf, _period_size_in_bytes);

    remaining_bytes_to_read -= c;
  } while ( (remaining_bytes_to_read > 0) and !exiting and !_aborting );

  { SAFELOCK(thread_check);

    if (exiting or _aborting)
    { //_recording = false;

      //ost << "_recording now FALSE" << endl;

      if (exiting)
        ost << "_capture thread is exiting" << endl;

      if (_aborting)
        ost << "_capture thread is aborting" << endl;

      wfp->close();
//      end_of_thread("_capture");

      ost << "closing status = " << snd_pcm_close(_handle) << endl;
//      snd_pcm_info_free(_info);   // this crashes for some reason

      end_of_thread(thread_name);

      return nullptr;
    }
  }

// we get here only if we are NOT exiting or aborting; close the current file, then start a new file
  static pthread_t closing_file_thread_id;

  try
  { create_thread(&closing_file_thread_id, NULL, &close_it, (void*)wfp, "audio capturefile close");     // close file in separate thread
  }

  catch (const pthread_error& e)
  { ost << "Error creating thread: audio file close" << e.reason() << endl;
    exit(-1);
  }

goto create_file;

  return nullptr;       // should never get here
}

/// constructor
audio_recorder::audio_recorder(void) :
  _aborting(false),                         // we are not aborting a capture
  _audio_buf(nullptr),                      // no buffer by default
  _base_filename("drlog-audio"),            // default output file
  _buffer_frames(0),                        // no frames in buffer?
  _buffer_time(0),                          // no time covered by buffer?
  _file_type(AUDIO_FORMAT_WAVE),            // WAV format
  _handle(nullptr),                         // no PCM handle
  _info(nullptr),                           // explicitly set to uninitialised
  _max_file_time(0),                        // no maximum duration (in seconds)
  _period_size_in_frames(0),
  _monotonic(false),                        // device cannot do monotonic timestamps
  _n_channels(1),                           // monophonic
  _open_mode(0),                            // blocking
  _pcm_name("default"),
  _period_frames(0),
  _period_time(0),
  _readi_func(snd_pcm_readi),               // function to read interleaved frames (the only one that we actually use)
  _readn_func(snd_pcm_readn),               // function to read non-interleaved frames
  _recording(false),                        // initially, not recording
  _record_count(9999999999),                // big number
  _samples_per_second(8000),                // G.711 rate
  _sample_format(SND_PCM_FORMAT_S16_LE),    // my soundcard doesn't support 8-bit formats such as SND_PCM_FORMAT_U8 :-(
  _start_delay(1),
  _stream(SND_PCM_STREAM_CAPTURE),          // we are capturing a stream
  _time_limit(0),                           // no limit
  _thread_number(0),
  _writei_func(snd_pcm_writei),             // function to write interleaved frames
  _writen_func(snd_pcm_writen)              // function to write non-interleaved frames
{ }

/// destructor
audio_recorder::~audio_recorder(void)
{ }

/// initialise the object
void audio_recorder::initialise(void)
{ snd_pcm_info_alloca(&_info);          // create an invalid snd_pcm_info_t

  const PARAMS_STRUCTURE rhwparams { _n_channels, _sample_format, _samples_per_second };
  const int MAX_FAILURES = 1;           // maximum number of permitted failures
  int n_failures = 0;                   // current number of failures
  int status;

  do
  { status = snd_pcm_open(&_handle, _pcm_name.c_str(), _stream, _open_mode);

    if (status < 0)
    { ost << "ERROR: Cannot open audio device: " << _pcm_name << endl;
      ost << "error number " << status << endl;
      ost << snd_strerror(status) << endl;
      n_failures++;
      sleep_for(seconds(5));
    }
  }  while ((status < 0) and (n_failures <= MAX_FAILURES));

  if (n_failures > MAX_FAILURES)
    throw audio_error(AUDIO_UNABLE_TO_OPEN, "Cannot open audio device: " + _pcm_name);

  status = snd_pcm_info(_handle, _info);   // gets info

  if (status < 0)
  { ost << "ERROR: cannot obtain info for audio device: " << _pcm_name << endl;
    ost << "error number " << status << endl;
    ost << snd_strerror(status) << endl;
    throw audio_error(AUDIO_UNABLE_TO_OBTAIN_INFO, "Cannot obtain info for device device: " + _pcm_name);
  }

  _hw_params = rhwparams;

  _set_params();
}

/// public function to capture the audio
void audio_recorder::capture(void)
{ try
  { create_thread(&_thread_id, NULL, &_static_capture, this, "audio capture");
    _recording = true;
//    ost << "started capture thread" << endl;
  }

  catch (const pthread_error& e)
  { ost << "Error creating thread: audio capture" << e.reason() << endl;
    exit(-1);
  }
}

void audio_recorder::abort(void)
{ _aborting = true;
  _recording = false;
}

// -----------  wav_file  ----------------

/*! \class  wav_file
    \brief  Class to implement functions related to wav files
*/

/*! \brief                  Write buffer to disk
    \param  bufp            pointer to buffer
    \param  buffer_size     size of buffer, in bytes
*/
void wav_file::_write_buffer(void* bufp, const size_t buffer_size)
{ if (_is_buffered)
  {  //TODO
  }
  else    // no buffering
  { if ( fwrite(bufp, buffer_size, 1, _fp) != 1 )
      throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing buffer to WAV file; buffer size =  " + to_string(buffer_size));
  }
}

/// default constructor
wav_file::wav_file(void) :
  _is_buffered(false)
{ }

/// return a dummy header string
const string wav_file::header(void) const
{ //riff_header rh;

// https://blogs.msdn.microsoft.com/dawate/2009/06/23/intro-to-audio-programming-part-2-demystifying-the-wav-format/
//  rh.chunk_size(0);             // since we don't know the true file size, we might as well set this number to zero

  //return rh.to_string();
  return riff_header().to_string();
}

/// open the file for writing
void wav_file::open(void)
{ _fp = fopen(_name.c_str(), "wb");

  if (!_fp)
  { ost << "Error opening WAV file: " << _name << endl;
    throw audio_error(AUDIO_WAV_OPEN_ERROR, "Error opening WAV file: " +_name);
  }

  const string header_str = header();   // write a place holder for the real (final) header

  if (fwrite(header_str.c_str(), header_str.size(), 1, _fp) != 1)
  { ost << "Error writing header for WAV file: " << _name << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing header for WAV file: " +_name);
  }
}

/*! \brief  Close the file

    This is complicated because the WAV format requires information related to
    the total length to be placed into chunks at the start of the file.
*/
void wav_file::close(void)
{
// put correct length into RIFF header
// == file size - 8

  fseek(_fp, 0, SEEK_END);

  const uint32_t length = ftell(_fp);
//  uint32_t length_for_riff = length - 8;
  const uint32_t length_for_riff = length - 8;

  int status = fseek(_fp, 4, SEEK_SET);      // go to byte #4

  if (status != 0)
  { ost << "Error seeking in file: " << _name << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error closing WAV file: " +_name);
  }

  int bytes_written = fwrite( (char*)&length_for_riff, 1, 4, _fp);

  if (bytes_written != 4)
  { ost << "Error writing length in RIFF chunk in file: " << _name << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error closing WAV file: " +_name);
  }

// put correct length into data chunk... assumes no bext chunk
  const uint32_t length_for_data_chunk = (length - 44);

  status = fseek(_fp, 40, SEEK_SET);      // go to byte #40

  if (status != 0)
  { ost << "Error seeking (2) in file: " << _name << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error closing WAV file: " +_name);
  }

  bytes_written = fwrite( (char*)&length_for_data_chunk, 1, 4, _fp);

  if (bytes_written != 4)
  { ost << "Error writing length in data chunk in file: " << _name << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error closing WAV file: " +_name);
  }

  fclose(_fp);      // at last we can close the file
}

/*! \brief          Append data to the file
    \param  vp      pointer to buffer holding the data to be appended
    \param  size    number of bytes to be appended
*/
void wav_file::append_data(const void* vp, const size_t size)
{ if (fwrite(vp, size, 1, _fp) != 1)
  { ost << "Error appending data to WAV file: " << _name << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error appending data to WAV file: " +_name);
  }
}

// -----------  bext_chunk  ----------------

/*! \class  bext_chunk
    \brief  Class to implement functions related to wav bext chunks
*/

#if 0
const string convert_for_output(const string& input, const size_t max_size)
{ string rv = create_string(0, max_size);

  for (size_t n = 0; n < input.size(); ++n)
    rv[n] = input[n];

  return rv;
}

const uint32_t bext_chunk::_size_of_data(void) const
{ uint32_t rv = 0;

  rv += 256;                                // Description[256]
  rv += 32;                                 // Originator[32]
  rv += 32;                                 // OriginatorReference[32]
  rv += 10;                                 // OriginationDate[10]
  rv += 8;                                  // OriginationTime[8]
  rv += sizeof(_time_reference_low);        // DWORD TimeReferenceLow
  rv += sizeof(_time_reference_high);       // DWORD TimeReferenceHigh
  rv += sizeof(_version);                   // WORD Version
  rv += 64;                                 // 64 bytes of SMPTE UMID
  rv += sizeof(_loudness_value);            // WORD LoudnessValue
  rv += sizeof(_loudness_range);            // WORD LoudnessRange
  rv += sizeof(_max_true_peak_level);       // WORD MaxTruePeakLevel
  rv += sizeof(_max_momentary_loudness);    // WORD MaxMomentaryLoudness
  rv += sizeof(_max_short_term_loudness);   // WORD MaxShortTermLoudness
  rv += 180;                                // Reserved[180]
  rv += _coding_history.length();           // CodingHistory[]

  return rv;
}

const uint32_t bext_chunk::size_of_chunk(void) const
{ uint32_t rv = _size_of_data();

  rv += (4 + 4);  // DWORD ckID; /* (broadcastextension)ckID=bext. */ + DWORD ckSize; /* size of extension chunk */

  return rv;
}

void bext_chunk::write_to_file(FILE* fp) const
{ const string id("bext");

  size_t items = fwrite(id.data(), id.size(), 1, fp);

  if (items != 1)
  { ost << "Error writing bext chunk ID in WAV file" << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing bext chunk ID in WAV file");
  }

  const uint32_t size = size_of_chunk();

  items = fwrite(&size, sizeof(size), 1, fp);

  if (items != 1)
  { ost << "Error writing bextchunk chunk size in WAV file" << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing bext chunk chunk size in WAV file");
  }

// now write the actual data
//  string str_1 = convert_for_output(_description, 256);

  items = fwrite(convert_for_output(_description, 256).data(), 1, 256, fp);

  if (items != 256)
  { ost << "Error writing bext description in WAV file" << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing bext description in WAV file");
  }

  items = fwrite(convert_for_output(_originator, 32).data(), 1, 32, fp);

  if (items != 32)
  { ost << "Error writing bext originator in WAV file" << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing bext originator in WAV file");
  }

  items = fwrite(convert_for_output(_originator_reference, 32).data(), 1, 32, fp);

  if (items != 32)
  { ost << "Error writing bext originator reference in WAV file" << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing bext originator reference in WAV file");
  }

  items = fwrite(convert_for_output(_origination_date, 10).data(), 1, 10, fp);

  if (items != 32)
  { ost << "Error writing bext origination date in WAV file" << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing bext origination date in WAV file");
  }

  items = fwrite(convert_for_output(_origination_time, 8).data(), 1, 8, fp);

  if (items != 32)
  { ost << "Error writing bext origination time in WAV file" << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing bext origination time in WAV file");
  }

  items = fwrite(&_time_reference_low, sizeof(_time_reference_low), 1, fp);

  if (items != 1)
  { ost << "Error writing bext time reference low in WAV file" << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing bext time reference low in WAV file");
  }

  items = fwrite(&_time_reference_high, sizeof(_time_reference_high), 1, fp);

  if (items != 1)
  { ost << "Error writing bext time reference high in WAV file" << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing bext time reference high in WAV file");
  }

  items = fwrite(&_version, sizeof(_version), 1, fp);

  if (items != 1)
  { ost << "Error writing bext version in WAV file" << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing bext version in WAV file");
  }

  items = fwrite(&_umid[0], 1, 64, fp);

  if (items != 64)
  { ost << "Error writing bext umid in WAV file" << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing bext umid in WAV file");
  }

  items = fwrite(&_loudness_value, sizeof(_loudness_value), 1, fp);

  if (items != 1)
  { ost << "Error writing bext loudness value in WAV file" << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing bext loudness value in WAV file");
  }

  items = fwrite(&_loudness_range, sizeof(_loudness_range), 1, fp);

  if (items != 1)
  { ost << "Error writing bext loudness range in WAV file" << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing bext loudness range in WAV file");
  }

  items = fwrite(&_max_true_peak_level, sizeof(_max_true_peak_level), 1, fp);

  if (items != 1)
  { ost << "Error writing bext max true peak level value in WAV file" << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing bext max true peak level value in WAV file");
  }

  items = fwrite(&_max_momentary_loudness, sizeof(_max_momentary_loudness), 1, fp);

  if (items != 1)
  { ost << "Error writing bext max momentary loudness in WAV file" << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing bext max momentary loudness in WAV file");
  }

  items = fwrite(&_max_short_term_loudness, sizeof(_max_short_term_loudness), 1, fp);

  if (items != 1)
  { ost << "Error writing bext max short term loudness in WAV file" << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing bext max short term loudness in WAV file");
  }

  items = fwrite(&_reserved[0], 1, 180, fp);

  if (items != 180)
  { ost << "Error writing bext reserved in WAV file" << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing bext reserved in WAV file");
  }

  items = fwrite(&_coding_history[0], 1, _coding_history.size(), fp);

  if (items != _coding_history.size())
  { ost << "Error writing bext coding history in WAV file" << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing bext coding history in WAV file");
  }
}
#endif

// -----------  data_chunk  ----------------

/*! \class  data_chunk
    \brief  Class to implement functions related to wav data chunks
*/

/*
36        4   Subchunk2ID      Contains the letters "data"
                               (0x64617461 big-endian form).
40        4   Subchunk2Size    == NumSamples * NumChannels * BitsPerSample/8
                               This is the number of bytes in the data.
                               You can also think of this as the size
                               of the read of the subchunk following this
                               number.
44        *   Data             The actual sound data.
*/

/*! \brief              Construct from a buffer
    \param  d           pointer to buffer
    \param  n_bytes     size of buffer
*/
data_chunk::data_chunk(u_char* d, const uint32_t n_bytes) :
  _subchunk_2_size(n_bytes),
  _data(d)
{ }

/*! \brief      Write to a file
    \param  fp  file pointer
*/
void data_chunk::write_to_file(FILE* fp) const
{ const string id("data");

  if (fwrite(id.data(), id.size(), 1, fp) != 1)
  { ost << "Error writing data chunk ID in WAV file" << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing data chunk ID in WAV file");
  }

// this is all silly, since the size will get overwritten when we close the file
  if (fwrite(&_subchunk_2_size, sizeof(_subchunk_2_size), 1, fp) != 1)
  { ost << "Error writing data chunk chunk size in WAV file" << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing data chunk chunk size in WAV file");
  }

  if (fwrite(_data, _subchunk_2_size, 1, fp)!= 1)
  { ost << "Error writing data chunk data in WAV file" << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing data chunk data in WAV file");
  }
}

// -----------  fmt_chunk  ----------------

/*! \class  fmt_chunk
    \brief  Class to implement functions related to wav fmt chunks
*/

/*
  12        4   Subchunk1ID      Contains the letters "fmt "
                                 (0x666d7420 big-endian form).
  16        4   Subchunk1Size    16 for PCM.  This is the size of the
                                 rest of the Subchunk which follows this number.
  20        2   AudioFormat      PCM = 1 (i.e. Linear quantization)
                                 Values other than 1 indicate some
                                 form of compression.
  22        2   NumChannels      Mono = 1, Stereo = 2, etc.
  24        4   SampleRate       8000, 44100, etc.
  28        4   ByteRate         == SampleRate * NumChannels * BitsPerSample/8
  32        2   BlockAlign       == NumChannels * BitsPerSample/8
                                 The number of bytes for one sample including
                                 all channels.
  34        2   BitsPerSample    8 bits = 8, 16 bits = 16, etc.
*/

/// constructor
fmt_chunk::fmt_chunk(void) :
  _subchunk_1_size(16), // PCM
  _audio_format(1),     // PCM
  _num_channels(1),     // mono
  _sample_rate(8000),   // 64 kbps
  _bits_per_sample(8)   // 8-bit samples
{ }

/*! \brief      Convert to a string that holds the fmt chunk in ready-to-use form
    \return     string containing the fmt chunk
*/
const string fmt_chunk::to_string(void) const
{ string rv = "fmt " + create_string(' ', 20);

  rv = replace_substring(rv, 4, _subchunk_1_size);
  rv = replace_substring(rv, 8, _audio_format);
  rv = replace_substring(rv, 10, _num_channels);
  rv = replace_substring(rv, 12, _sample_rate);
  rv = replace_substring(rv, 16, byte_rate());
  rv = replace_substring(rv, 20, block_align());
  rv = replace_substring(rv, 22, _bits_per_sample);

  return rv;
}

/*! \brief      Write to a file
    \param  fp  file pointer
*/
void fmt_chunk::write_to_file(FILE* fp) const
{ const string str = to_string();

  if (fwrite(str.c_str(), str.size(), 1, fp) != 1)
  { ost << "Error writing fmt chunk in WAV file" << endl;
    throw audio_error(AUDIO_WAV_WRITE_ERROR, "Error writing fmt chunk in WAV file");
  }
}

/*! \brief          Write a <i>fmt_chunk</i> object to an output stream
    \param  ost     output stream
    \param  chunk   object to write
    \return         the output stream
*/
ostream& operator<<(ostream& ost, const fmt_chunk& chunk)
{ ost << "subchunk_1_size = " << chunk.subchunk_1_size() << endl
      << "audio format = " << chunk.audio_format() << endl
      << "num channels = " << chunk.num_channels() << endl
      << "sample rate = " << chunk.sample_rate() << endl
      << "byte rate = " << chunk.byte_rate() << endl
      << "block align = " << chunk.block_align() << endl
      << "bits per sample = " << chunk.bits_per_sample();

  return ost;
}

// -----------  riff_header  ----------------

/*! \class  riff_header
    \brief  Trivial class to implement the RIFF header
*/

/// constructor
//riff_header::riff_header(void) :
//  _chunk_size(0)
//{ }

/*! \brief      Convert to string
    \return     the RIFF header as a string
*/
//const string riff_header::to_string(void) const
//{ return replace_substring(string("RIFF    WAVE"), 4, _chunk_size);
//}

/*! \brief          Write a <i>PARAMS_STUCTURE</i> object to an output stream
    \param  ost     output stream
    \param  params  object to write
    \return         the output stream
*/
ostream& operator<<(ostream& ost, const PARAMS_STRUCTURE& params)
{ ost << "channels = " << params.channels << endl
      << "format = " << params.format << endl
      << "rate = " << params.rate;

  return ost;
}
