// $Id: audio.h 154 2020-03-05 15:36:24Z  $

// Released under the GNU Public License, version 2

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   audio.h

    Classes and functions anent audio recording. The implementation uses ALSA calls.
    A version intended to support other OSes in addition to Linux could, for example, use sox instead.
*/

#ifndef AUDIO_H
#define AUDIO_H

#include "macros.h"
#include "string_functions.h"
#include "x_error.h"

#include <array>
#include <fstream>
#include <string>

#include <alsa/asoundlib.h>

#undef BEXT_EXTENSION

/// audio formats -- these are not actually used
enum class AUDIO_FORMAT { DEFAULT = -1,
                          RAW     = 0,
                          VOC     = 1,
                          WAVE    = 2,
                          AU      = 3
                        };

//enum AUDIO_FORMAT { AUDIO_FORMAT_DEFAULT = -1,
//                    AUDIO_FORMAT_RAW     = 0,
//                    AUDIO_FORMAT_VOC     = 1,
//                    AUDIO_FORMAT_WAVE    = 2,
//                    AUDIO_FORMAT_AU      = 3
//                  };

// Errors
constexpr int AUDIO_UNABLE_TO_OPEN                { -1 },      ///< unable to open audio device
              AUDIO_UNABLE_TO_OBTAIN_INFO         { -2 },      ///< cannot get info about audio device
              AUDIO_NO_CONFIGURATION              { -3 },      ///< no configuration abailable for a PCM
              AUDIO_NO_ACCESS_TYPE                { -4 },      ///< no access type
              AUDIO_NO_SAMPLE_FORMAT              { -5 },      ///< no sample format
              AUDIO_NO_CHANNEL_COUNT              { -6 },      ///< no channel count
              AUDIO_RATE_SET_ERROR                { -7 },      ///< error setting rate
              AUDIO_INACCURATE_RATE               { -8 },      ///< rate is not accurate
              AUDIO_PLUGIN_ERROR                  { -9 },      ///< error related to plugin
              AUDIO_INVALID_BUFFER_TIME           { -10 },     ///< buffer time is invalid
              AUDIO_INVALID_PERIOD                { -11 },     ///< invalid period time or size
              AUDIO_INVALID_BUFFER                { -12 },     ///< invalid buffer time or size
              AUDIO_CANNOT_INSTALL_HW_PARAMS      { -13 },     ///< unable to install hardware parameters
              AUDIO_EQUAL_PERIOD_AND_BUFFER_SIZE  { -14 },     ///< period and buffer size are equal
              AUDIO_UNABLE_TO_GET_PERIOD_SIZE     { -15 },     ///< unable to get a single period size
              AUDIO_UNABLE_TO_GET_BUFFER_SIZE     { -16 },     ///< unable to get a single buffer size
              AUDIO_UNABLE_TO_GET_SW_PARAMS       { -17 },     ///< unable to get software parameters
              AUDIO_UNABLE_TO_SET_AVAIL_MIN       { -18 },     ///< unable to set available minimum
              AUDIO_UNABLE_TO_SET_START_THRESHOLD { -19 },     ///< cannot set start threshold
              AUDIO_UNABLE_TO_SET_STOP_THRESHOLD  { -20 },     ///< cannot set stop threshold
              AUDIO_CANNOT_INSTALL_SW_PARAMS      { -21 },     ///< unable to install software parameters
              AUDIO_NO_MEMORY                     { -22 },     ///< out of memory
              AUDIO_WAV_WRITE_ERROR               { -23 },     ///< error writing file
              AUDIO_DEVICE_READ_ERROR             { -24 },     ///< error reading audio device
              AUDIO_WAV_OPEN_ERROR                { -25 };     ///< error opening file

// -----------  riff_header  ----------------

/*! \class  riff_header
    \brief  Trivial class to implement the RIFF header

    See: http://soundfile.sapp.org/doc/WaveFormat/
*/

class riff_header
{
protected:

  uint32_t  _chunk_size { 0 };                ///< file size - 8 (bytes)

public:

/// constructor
  inline riff_header(void) = default;

  READ_AND_WRITE(chunk_size);          ///< file size - 8 (bytes)

/*! \brief      Convert to string
    \return     the RIFF header as a string
*/
  inline const std::string to_string(void) const
    { return replace_substring(std::string("RIFF    WAVE"), 4, _chunk_size); }
};

// -----------  wav_file  ----------------

/*! \class  wav_file
    \brief  Class to implement functions related to wav files
*/

class wav_file
{
protected:

  bool _is_buffered;            ///< whether to use buffering to avoid writing when sending CW (not yet supported)
  std::string   _name;          ///< name of file

// The question is whether to use a stream or a C-style FILE*. I choose the latter
// because:
//   it is likely to be a bit faster
//   I am more familiar with it
//   it seems easier to write large amounts of data without going through contortions to avoid copies
  FILE*         _fp;            ///< file pointer

/*! \brief                  Write buffer to disk
    \param  bufp            pointer to buffer
    \param  buffer_size     size of buffer, in bytes
*/
  void _write_buffer(void* bufp, const size_t buffer_size);

public:

/// default constructor
  inline wav_file(void) :
    _is_buffered(false)
  { }

  READ_AND_WRITE(is_buffered)    ///< whether to use buffering to avoid writing when sending CW (not yet supported)
  READ_AND_WRITE(name);          ///< name of file

/// open the file for writing
  void open(void);

/*! \brief  Close the file

    This is complicated because the WAV format requires information related to
    the total length to be placed into chunks at the start of the file.
*/
  void close(void);

/// return a dummy header string
  inline const std::string header(void) const
    { return riff_header().to_string(); }

/*! \brief      Append a chunk
    \param  c   chunk to append
*/
  template <typename T>
  void add_chunk(const T& c) const
  { c.write_to_file(_fp);
    fflush(_fp);
  }

/*! \brief          Append data to the file
    \param  vp      pointer to buffer holding the data to be appended
    \param  size    number of bytes to be appended
*/
  void append_data(const void* vp, const size_t size);
};

// -----------  audio_recorder  ----------------

/*! \class  audio_recorder
    \brief  Class to implement the needed recording functions
*/

/// structure to encapsulate parameters
typedef struct { unsigned int     channels;     ///< number of channels
                 snd_pcm_format_t format;       ///< format number; defined in alsa/pcm.h
                 unsigned int     rate;         ///< rate (bytes per second)
               } PARAMS_STRUCTURE ;

class audio_recorder
{
protected:

  bool              _aborting;                  ///< whether aborting a capture
  u_char*           _audio_buf;                 ///< buffer for audio
  std::string       _base_filename;             ///< base name of output file
  size_t            _bits_per_frame;            ///< bits per sample * number of channels
  snd_pcm_uframes_t _buffer_frames;             ///< number of frames in buffer?
  unsigned int      _buffer_time;               ///< amount of time in buffer?
//  int               _file_type;                 ///< format of file
  snd_pcm_t*        _handle;                    ///< PCM handle
  PARAMS_STRUCTURE  _hw_params;                 ///< hardware parameters
  snd_pcm_info_t*   _info;                      ///< pointer to information structure that corresponds to <i>_handle</i>
  bool              _initialised { false };     ///< has the hardware been initialised, ready for reading?
  int64_t           _max_file_time;             ///< maximum duration in seconds
  size_t            _period_size_in_bytes;      ///< size of period; http://www.alsa-project.org/main/index.php/FramesPeriods
  snd_pcm_uframes_t _period_size_in_frames;     ///< size of period; http://www.alsa-project.org/main/index.php/FramesPeriods
  bool              _monotonic;                 ///< whether device does monotonic timestamps
  unsigned int      _n_channels;                ///< number of channels to record
  int               _open_mode;                 ///< blocking or non-blocking
  std::string       _pcm_name;                  ///< name of the PCM handle
  snd_pcm_uframes_t _period_frames;             ///< ?
  unsigned int      _period_time;               ///< ?
  bool              _recording;                 ///< whether the recorder is currently recording
  int64_t           _record_count;              ///< number of records to capture
  unsigned int      _samples_per_second;        ///< number of samples per second
  snd_pcm_format_t  _sample_format;             ///< format of a single format (U8, SND_PCM_FORMAT_S16_LE, etc.)
  int               _start_delay;               ///< ?
  snd_pcm_stream_t  _stream;                    ///< type of stream
  pthread_t         _thread_id;                 ///< ID for the thread that plays the buffer
  unsigned int      _thread_number;             ///< number of the thread currently being used
  unsigned int      _time_limit;                ///< number of seconds to record

// stuff for bext extension
#if 0
  std::string       _description;           // do not exceed 256 characters /* ASCII : «Description of the sound sequence» */
  std::string       _originator;            // do not exceed 32 characters  /* ASCII : «Name of the originator» */
  std::string       _originator_reference;  // do not exceed 32 characters  /* ASCII : «Name of the originator» */
  std::string       _origination_date;      // 10 characters
  std::string       _origination_time;      // 10 characters
  uint32_t          _time_reference_low;    // lowest 32 bits of the time reference
  uint32_t          _time_reference_high;   // highest 32 bits of the time reference
#endif

  snd_pcm_sframes_t (*_readi_func)(snd_pcm_t *handle, void *buffer, snd_pcm_uframes_t size);            ///< function to read interleaved frames
  snd_pcm_sframes_t (*_writei_func)(snd_pcm_t *handle, const void *buffer, snd_pcm_uframes_t size);     ///< function to write interleaved frames
  snd_pcm_sframes_t (*_readn_func)(snd_pcm_t *handle, void **bufs, snd_pcm_uframes_t size);             ///< function to read non-interleaved frames
  snd_pcm_sframes_t (*_writen_func)(snd_pcm_t *handle, void **bufs, snd_pcm_uframes_t size);            ///< function to write non-interleaved frames

/// Declare, but do not define, copy constructor
  audio_recorder(const audio_recorder&);

/*! \brief          Read from the PCM device
    \param  data    buffer in which to store the read data
    \return         total number of bytes read
*/
  const ssize_t _pcm_read(u_char* data);

/*! \brief      Calculate the total number of bytes to be read from the device
    \return     total number of bytes to be read from the sound board

    Returned value is based on the duration and the number of bytes to be read per second
*/
  const int64_t _total_bytes_to_read(void);

/*! \brief  Set the parameters for the recording

    Much of this is converted from aplay.c
*/
  void _set_params(void);

/*! \brief      Capture audio
    \return     nullptr
*/
  void* _capture(void*);

/*! \brief          Wrapper function to capture audio
    \param  arg     "this" pointer
    \return         nullptr
*/
  static void* _static_capture(void*);

public:

/// constructor
  inline audio_recorder(void) :
    _aborting(false),                         // we are not aborting a capture
    _audio_buf(nullptr),                      // no buffer by default
    _base_filename("drlog-audio"s),            // default output file
    _buffer_frames(0),                        // no frames in buffer?
    _buffer_time(0),                          // no time covered by buffer?
//    _file_type(AUDIO_FORMAT::WAVE),            // WAV format
    _handle(nullptr),                         // no PCM handle
    _info(nullptr),                           // explicitly set to uninitialised
    _max_file_time(0),                        // no maximum duration (in seconds)
    _period_size_in_frames(0),
    _monotonic(false),                        // device cannot do monotonic timestamps
    _n_channels(1),                           // monophonic
    _open_mode(0),                            // blocking
    _pcm_name("default"s),
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
  inline virtual ~audio_recorder(void) = default;

  READ_AND_WRITE(base_filename);            ///< base name of output file
  READ(initialised);                        ///< has the hardware been initialised, ready for reading?
  READ_AND_WRITE(max_file_time);            ///< maximum duration in seconds
  READ_AND_WRITE(n_channels);               ///< number of channels to record
  READ_AND_WRITE(pcm_name);                 ///< name of the PCM handle
  READ(recording);                          ///< whether the recorder is currently recording
  READ_AND_WRITE(samples_per_second);       ///< number of samples per second

// stuff for bext extension
#if 0
  READ_AND_WRITE(description);
  READ_AND_WRITE(originator);
  READ_AND_WRITE(originator_reference);
  READ_AND_WRITE(origination_date);
  READ_AND_WRITE(origination_time);
  READ_AND_WRITE(time_reference_low);
  READ_AND_WRITE(time_reference_high);
#endif

/// abort recording
  void abort(void);

/*! \brief          Set maximum duration
    \param  secs    maximum duration, in seconds
*/
  inline void maximum_duration(const unsigned int secs)
    { max_file_time(secs); }

/// initialise the object
  void initialise(void);

/// public function to capture the audio
  void capture(void);
};

/*! \brief          Write a <i>PARAMS_STUCTURE</i> object to an output stream
    \param  ost     output stream
    \param  params  object to write
    \return         the output stream
*/
std::ostream& operator<<(std::ostream& ost, const PARAMS_STRUCTURE& params);

// -----------  bext_chunk  ----------------

/*! \class  bext_chunk
    \brief  Class to implement functions related to wav bext chunks
*/

#if 0
typedef struct broadcast_audio_extension {
CHAR
Description[256];
/* ASCII : «Description of the sound sequence» */
CHAR
Originator[32];
/* ASCII : «Name of the originator» */
CHAR
OriginatorReference[32];
/* ASCII : «Reference of the originator» */
CHAR
OriginationDate[10];
/* ASCII : «yyyy:mm:dd» */
CHAR
OriginationTime[8];
/* ASCII : «hh:mm:ss» */
DWORD
TimeReferenceLow;
/* First sample count since midnight, low word */
DWORD
TimeReferenceHigh;
/* First sample count since midnight, high word */
WORD
Version;
/* Version of the BWF; unsigned binary number */
BYTE
UMID_0
/* Binary byte 0 of SMPTE UMID */
....
BYTE
UMID_63
/* Binary byte 63 of SMPTE UMID */
WORD
LoudnessValue;
/* WORD : «Integrated Loudness Value of the file in LUFS (multiplied by 100) » */
WORD
LoudnessRange;
/* WORD : «Loudness Range of the file in LU (multiplied by 100) » */
WORD
MaxTruePeakLevel;
/* WORD : «Maximum True Peak Level of the file expressed as dBTP (multiplied by 100) » */
WORD
MaxMomentaryLoudness;
/* WORD : «Highest value of the Momentary Loudness Level of the file in LUFS (multiplied by 100) » */
WORD
MaxShortTermLoudness;
/* WORD : «Highest value of the Short-Term Loudness Level of the file in LUFS (multiplied by 100) » */
BYTE
Reserved[180];
/* 180 bytes, reserved for future use, set to “NULL” */
CHAR
CodingHistory[];
/* ASCII : « History coding » */
} BROADCAST_EXT;
#endif

#if 0
// See https://tech.ebu.ch/docs/tech/tech3285.pdf

class bext_chunk
{
protected:
  std::string   _description;               // do not exceed 256 characters /* ASCII : «Description of the sound sequence» */
  std::string   _originator;                // do not exceed 32 characters  /* ASCII : «Name of the originator» */
  std::string   _originator_reference;      // do not exceed 32 characters  /* ASCII : «Reference of the originator» */
  std::string   _origination_date;          // no not exceed 10 characters  /* ASCII : «yyyy:mm:dd» */
  std::string   _origination_time;          // no not exceed 8 characters   /* ASCII : «hh:mm:ss» */
  uint32_t      _time_reference_low;                                        /* First sample count since midnight, low word */
  uint32_t      _time_reference_high;                                        /* First sample count since midnight, high word */
  uint16_t      _version;                                                   /* Version of the BWF; unsigned binary number */
  u_char        _umid[64];                                                  /* SMPTE UMID */
  uint32_t      _loudness_value;                                            /* WORD : «Integrated Loudness Value of the file in LUFS (multiplied by 100) » */
  uint32_t      _loudness_range;                                            /* WORD : «Loudness Range of the file in LU (multiplied by 100) » */
  uint32_t      _max_true_peak_level;                                       /* WORD : «Maximum True Peak Level of the file expressed as dBTP (multiplied by 100) » */
  uint32_t      _max_momentary_loudness;                                    /* WORD : «Highest value of the Momentary Loudness Level of the file in LUFS (multiplied by 100) » */
  uint32_t      _max_short_term_loudness;                                   /* WORD : «Highest value of the Short-Term Loudness Level of the file in LUFS (multiplied by 100) » */
  u_char        _reserved[180];                                             /* 180 bytes, reserved for future use, set to “NULL” */
  std::string   _coding_history;                                            /* ASCII : « History coding » */

  const uint32_t _size_of_data(void) const;

public:

/// constructor
  bext_chunk(void) :
    _version(2)
  { }

  READ_AND_WRITE(description);
  READ_AND_WRITE(originator);
  READ_AND_WRITE(originator_reference);
  READ_AND_WRITE(origination_date);
  READ_AND_WRITE(origination_time);
  READ_AND_WRITE(time_reference_low);
  READ_AND_WRITE(time_reference_high);
  READ_AND_WRITE(version);
  READ_AND_WRITE(loudness_value);
  READ_AND_WRITE(loudness_range);
  READ_AND_WRITE(max_true_peak_level);
  READ_AND_WRITE(max_momentary_loudness);
  READ_AND_WRITE(max_short_term_loudness);
  READ_AND_WRITE(coding_history);

  const uint32_t size_of_chunk(void) const;

  void write_to_file(FILE* fp) const;
};
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

class data_chunk
{
protected:

  uint32_t  _subchunk_2_size;   ///< size in bytes of the remainder of the chunk = bits-per-sample / 8 * number-of-channels * number-of-samples
  u_char*   _data;              ///< pointer to the actual sound data

public:

/*! \brief              Construct from a buffer
    \param  d           pointer to buffer
    \param  n_bytes     size of buffer
*/
  data_chunk(u_char* d, const uint32_t n_bytes);

  READ_AND_WRITE(subchunk_2_size);   ///< size in bytes = bits-per-sample / 8 * number-of-channels * number-of-samples
  READ_AND_WRITE(data);              ///< pointer to the actual sound data

/*! \brief      Write to a file
    \param  fp  file pointer
*/
  void write_to_file(FILE* fp) const;
};

// -----------  fmt_chunk  ----------------

/*! \class  fmt_chunk
    \brief  Class to implement functions related to wav fmt chunks
*/

class fmt_chunk
{
protected:
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

  uint32_t  _subchunk_1_size;   ///< 16, for PCM (size of the remainder of the subchunk)
  uint16_t  _audio_format;      ///< 1, for PCM
  uint16_t  _num_channels;      ///< number of channels
  uint32_t  _sample_rate;       ///< bits per second
  uint16_t  _bits_per_sample;   ///< number of bits in a single sample

public:

/// constructor
  fmt_chunk(void);

  READ_AND_WRITE(audio_format);         ///< indicates the format, 1 for PCM
  READ_AND_WRITE(num_channels);         ///< number of channels
  READ_AND_WRITE(sample_rate);          ///< bits per second
  READ_AND_WRITE(bits_per_sample);      ///< number of bits in a single sample

  READ(subchunk_1_size);                ///< size of the remainder of the subchunk; 16 for PCM

/// the number of bytes occupied by the blocks of a single sample
  inline const uint16_t block_align(void) const
    { return _num_channels * _bits_per_sample / 8; }

/// the number of bytes occupied per second
  inline const uint32_t byte_rate(void) const
    { return _sample_rate * block_align(); }

/*! \brief      Convert to a string that holds the fmt chunk in ready-to-use form
    \return     string containing the fmt chunk
*/
  const std::string to_string(void) const;

/*! \brief      Write to a file
    \param  fp  file pointer
*/
  void write_to_file(FILE* fp) const;
};

/*! \brief          Write a <i>fmt_chunk</i> object to an output stream
    \param  ost     output stream
    \param  chunk   object to write
    \return         the output stream
*/
std::ostream& operator<<(std::ostream& ost, const fmt_chunk& chunk);

// -------------------------------------- Errors  -----------------------------------

ERROR_CLASS(audio_error);   ///< errors related to audio processing

#endif /* AUDIO_H */
