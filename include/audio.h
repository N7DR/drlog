// $Id: audio.h 134 2016-11-15 23:57:13Z  $

// Released under the GNU Public License, version 2

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file audio.h

    Classes and functions anent audio recording. The implementation uses ALSA calls.
    A version intended to support other OSes in addition to Linux could, for example, use sox instead.
*/

#ifndef AUDIO_H
#define AUDIO_H

#include "macros.h"
#include "x_error.h"

#include <array>
#include <string>

#include <alsa/asoundlib.h>

#if 0
#if (!defined(READ_AND_WRITE))

/// Syntactic sugar for read/write access
#define READ_AND_WRITE(y)                                       \
/*! Read access to _##y */                                      \
  inline const decltype(_##y)& y(void) const { return _##y; }   \
/*! Write access to _##y */                                     \
  inline void y(const decltype(_##y)& n) { _##y = n; }

#endif    // !READ_AND_WRITE

#if (!defined(READ))

/// Syntactic sugar for read-only access
#define READ(y)                                                 \
/*! Read-only access to _##y */                                 \
  inline const decltype(_##y)& y(void) const { return _##y; }

#endif    // !READ
# endif // 0

enum AUDIO_FORMAT { AUDIO_FORMAT_DEFAULT = -1,
                    AUDIO_FORMAT_RAW     = 0,
                    AUDIO_FORMAT_VOC     = 1,
                    AUDIO_FORMAT_WAVE    = 2,
                    AUDIO_FORMAT_AU      = 3
                  };

// Errors
const int AUDIO_UNABLE_TO_OPEN                = -1,      ///< unable to open audio device
          AUDIO_UNABLE_TO_OBTAIN_INFO         = -2,      ///< cannot get info about audio device
          AUDIO_NO_CONFIGURATION              = -3,      ///< no configuration abailable for a PCM
          AUDIO_NO_ACCESS_TYPE                = -4,      ///< no access type
          AUDIO_NO_SAMPLE_FORMAT              = -5,      ///< no sample format
          AUDIO_NO_CHANNEL_COUNT              = -6,      ///< no channel count
          AUDIO_RATE_SET_ERROR                = -7,      ///< error setting rate
          AUDIO_INACCURATE_RATE               = -8,      ///< rate is not accurate
          AUDIO_PLUGIN_ERROR                  = -9,      ///< error related to plugin
          AUDIO_INVALID_BUFFER_TIME           = -10,     ///< buffer time is invalid
          AUDIO_INVALID_PERIOD                = -11,     ///< invalid period time or size
          AUDIO_INVALID_BUFFER                = -12,     ///< invalid buffer time or size
          AUDIO_CANNOT_INSTALL_HW_PARAMS      = -13,     ///< unable to install hardware parameters
          AUDIO_EQUAL_PERIOD_AND_BUFFER_SIZE  = -14,     ///< period and buffer size are equal
          AUDIO_UNABLE_TO_GET_PERIOD_SIZE     = -15,     ///< unable to get a single period size
          AUDIO_UNABLE_TO_GET_BUFFER_SIZE     = -16,     ///< unable to get a single buffer size
          AUDIO_UNABLE_TO_GET_SW_PARAMS       = -17,     ///< unable to get software parameters
          AUDIO_UNABLE_TO_SET_AVAIL_MIN       = -18,     ///< unable to set available minimum
          AUDIO_UNABLE_TO_SET_START_THRESHOLD = -19,     ///< cannot set start threshold
          AUDIO_UNABLE_TO_SET_STOP_THRESHOLD  = -20,     ///< cannot set stop threshold
          AUDIO_CANNOT_INSTALL_SW_PARAMS      = -21,     ///< unable to install software parameters
          AUDIO_NO_MEMORY                     = -22,     ///< out of memory
          AUDIO_WAV_WRITE_ERROR               = -23,     ///< error writing file
          AUDIO_DEVICE_READ_ERROR             = -24,     ///< error reading audio device
          AUDIO_WAV_OPEN_ERROR                = -25;     ///< error opening file

// -----------  wav_file  ----------------

/*! \class  wav_file
    \brief  Class to implement functions related to wav files
*/

#include <fstream>

class wav_file
{
protected:

  std::string   _name;          ///< name of file
  uint32_t      _file_size;     ///< total size of file

// The question is whether to use a stream or a C-style FILE*. I choose the latter
// because it is likely to be a bit faster and also because it is easier to write
// large amounts of data without going through contortions to avoid copies
  FILE*         _fp;            ///< file pointer
//  bool          _in_use;

public:

  wav_file(void) //:
//    _in_use(false)
  { }

  READ_AND_WRITE(name);
//  READ_AND_WRITE(in_use);

  void open(void);

  const std::string header(void) const;

  template <typename T>
  void add_chunk(const T& c) const
  { c.write_to_file(_fp);
    fflush(_fp);
  }

  void close(void);

  void append_data(const void* vp, const size_t size);
};

// -----------  audio_recorder  ----------------

/*! \class  audio_recorder
    \brief  Class to implement the needed recording functions
*/

typedef struct { unsigned int channels;
                 snd_pcm_format_t format;
                 unsigned int rate;
               } PARAMS_STRUCTURE ;

class audio_recorder
{
protected:

  u_char*           _audio_buf;                 ///< buffer for audio
  int               _avail_min;                 ///< ?
  size_t            _bits_per_frame;            ///< ?
  snd_pcm_uframes_t _buffer_frames;             ///< ?
  unsigned int      _buffer_time;               ///< ?
  size_t            _period_size_in_bytes;      ///< size of period; http://www.alsa-project.org/main/index.php/FramesPeriods
  snd_pcm_uframes_t _period_size_in_frames;     ///< size of period; http://www.alsa-project.org/main/index.php/FramesPeriods
  int               _file_type;                 ///< format of file
  std::string       _base_filename;             ///< base name of output file
  snd_pcm_t*        _handle;                    ///< handle
  PARAMS_STRUCTURE  _hw_params;                 ///< hardware parameters
  snd_pcm_info_t*   _info;                      ///< pointer to a snd_pcm_info_t
  long long         _max_file_time;             ///< maximum duration in seconds (?)
  bool              _monotonic;                 ///< whether device does monotonic timestamps
  unsigned int      _n_channels;                ///< number of channels to record
  int               _open_mode;                 ///< blocking or non-blocking
  std::string       _pcm_name;                  ///< name of the PCM handle
  snd_pcm_uframes_t _period_frames;             ///< ?
  unsigned int      _period_time;               ///< ?
  int64_t           _record_count;              ///< number of records to capture
  unsigned int      _samples_per_second;        ///< number of samples per second
  snd_pcm_format_t  _sample_format;             ///< format of a single format (U8, SND_PCM_FORMAT_S16_LE, etc.)
  int               _start_delay;               ///< ?
  int               _stop_delay;                ///< ?
  snd_pcm_stream_t  _stream;                    ///< type of stream
  unsigned int      _time_limit;                ///< number of seconds to record

//  std::array<wav_file, 2> _wav_files;
//  wav_file*         _wfp;

// stuff for bext extension
  std::string       _description;           // do not exceed 256 characters /* ASCII : «Description of the sound sequence» */
  std::string       _originator;            // do not exceed 32 characters  /* ASCII : «Name of the originator» */
  std::string       _originator_reference;  // do not exceed 32 characters  /* ASCII : «Name of the originator» */
  std::string       _origination_date;      // 10 characters
  std::string       _origination_time;      // 10 characters
  uint32_t          _time_reference_low;
  uint32_t          _time_reference_high;

  snd_pcm_sframes_t (*_readi_func)(snd_pcm_t *handle, void *buffer, snd_pcm_uframes_t size);
  snd_pcm_sframes_t (*_writei_func)(snd_pcm_t *handle, const void *buffer, snd_pcm_uframes_t size);
  snd_pcm_sframes_t (*_readn_func)(snd_pcm_t *handle, void **bufs, snd_pcm_uframes_t size);
  snd_pcm_sframes_t (*_writen_func)(snd_pcm_t *handle, void **bufs, snd_pcm_uframes_t size);

  pthread_t             _thread_id;         ///< ID for the thread that plays the buffer

/// Declare but do not define copy constructor
  audio_recorder(const audio_recorder&);

  const int64_t _total_bytes_to_read(void) /* const */;
  ssize_t _pcm_read(u_char *data);
  void _set_params(void);               ///< set the parameters for the recording

  void _begin_wave(int fd, size_t count);
  void _end_wave(int fd);

  void* _capture(void*);
  static void* _static_capture(void*);

public:

/// constructor
  audio_recorder(void);

//  READ_AND_WRITE(filename);
  READ_AND_WRITE(n_channels);
  READ_AND_WRITE(pcm_name);
  READ_AND_WRITE(samples_per_second);

  READ_AND_WRITE(max_file_time);

  READ_AND_WRITE(description);
  READ_AND_WRITE(originator);
  READ_AND_WRITE(originator_reference);
  READ_AND_WRITE(origination_date);
  READ_AND_WRITE(origination_time);
  READ_AND_WRITE(time_reference_low);
  READ_AND_WRITE(time_reference_high);


  inline void maximum_duration(const unsigned int secs)
    { max_file_time(secs); }

  READ(base_filename);

  void base_filename(const std::string& name);

  void initialise(void);

  void capture();

/// destructor
  virtual ~audio_recorder(void);

};

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

  uint32_t  _subchunk_2_size;
  u_char*   _data;

public:

  data_chunk(void);
  data_chunk(u_char* d, const uint32_t n_bytes);
  data_chunk(const uint32_t n_bytes);

  READ_AND_WRITE(subchunk_2_size);
  READ_AND_WRITE(data);

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
                                 all channels. I wonder what happens when
                                 this number isn't an integer?
  34        2   BitsPerSample    8 bits = 8, 16 bits = 16, etc.
*/

  uint32_t  _subchunk_1_size;
  uint16_t  _audio_format;
  uint16_t  _num_channels;
  uint32_t  _sample_rate;
  uint16_t  _bits_per_sample;

public:

/// constructor
  fmt_chunk(void);

  READ_AND_WRITE(audio_format);
  READ_AND_WRITE(num_channels);
  READ_AND_WRITE(sample_rate);
  READ_AND_WRITE(bits_per_sample);

  READ(subchunk_1_size);

  inline const uint16_t block_align(void) const
    { return _num_channels * _bits_per_sample / 8; }

  inline const uint32_t byte_rate(void) const
    { return _sample_rate * block_align(); }

  const std::string to_string(void) const;

  void write_to_file(FILE* fp) const;
};

std::ostream& operator<<(std::ostream& ost, const fmt_chunk& chunk);

// -----------  riff_header  ----------------

/*! \class  riff_header
    \brief  Trivial class to implement the RIFF header
*/

class riff_header
{
protected:

  uint32_t  _chunk_size;

public:

/// constructor
  riff_header(void);

  READ_AND_WRITE(chunk_size);

  const std::string to_string(void) const;
};

// -------------------------------------- Errors  -----------------------------------

/*! \class  audio_error
    \brief  Errors related to audio recording
*/

class audio_error : public x_error
{
protected:

public:

/*! \brief      Construct from error code and reason
    \param  n   error code
    \param  s   reason
*/
  inline audio_error(const int n, const std::string& s) :
    x_error(n, s)
  { }
};

#endif /* AUDIO_H */
