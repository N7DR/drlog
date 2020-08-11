// $Id: procfs.h 163 2020-08-06 19:46:33Z  $

/*! \file procfs.h

    Class for accessing /proc/[pid] values
*/

#ifndef PROCFS_H
#define PROCFS_H

#include "string_functions.h"

#include <chrono>
#include <string>

#include <sys/types.h>
#include <unistd.h>

using namespace std::literals::chrono_literals;
using namespace std::literals::string_literals;

// -----------  procfs ----------------

/*! \class  procfs
    \brief  Information from the /proc/[pid] subsystem

    Currently, each access causes a new read from the procfs.
*/

class procfs
{
protected:

  std::chrono::system_clock::time_point _last_update_time;         ///< time at which /proc/stat/[PID] was last read
  std::chrono::system_clock::duration   _minimum_interval;         ///< minimum interval between unforced reads of /proc/stat/[PID]

  pid_t         _pid;                                              ///< PID of the user's process

  std::vector<std::string>              _last_stat_vec;            ///< the last-retrieved stat_vec

  inline const std::string _stat(void)
    { return read_file("/proc/"s + to_string(_pid) + "/stat"s); }

  const std::vector<std::string> _statvec(void);                    // this does all the work

public:

/// constructor
  inline procfs(const std::chrono::system_clock::duration min_int = 1s) :
    _pid(getpid())
  { }

  READ_AND_WRITE(minimum_interval);         ///< minimum interval between unforced reads from the the /proc filesystem

/*

  See also: https://en.cppreference.com/w/c/io/fprintf

  (1) pid                    %d   The process ID.
  (2) comm                   %s   The filename of the executable, in parentheses.  This is visible whether or not the executable is swapped out.
  (3) state                  %c   One of the following characters, indicating process state:
                                    R  Running
                                    S  Sleeping in an interruptible wait
                                    D  Waiting in uninterruptible disk sleep
                                    Z  Zombie
                                    T  Stopped (on a signal) or (before Linux 2.6.33) trace stopped
                                    t  Tracing stop (Linux 2.6.33 onward)
                                    W  Paging (only before Linux 2.6.0)
                                    X  Dead (from Linux 2.6.0 onward)
                                    x  Dead (Linux 2.6.33 to 3.13 only)
                                    K  Wakekill (Linux 2.6.33 to 3.13 only)
                                    W  Waking (Linux 2.6.33 to 3.13 only)
                                    P  Parked (Linux 3.9 to 3.13 only)
  (4) ppid                   %d   The PID of the parent of this process.
  (5) pgrp                   %d   The process group ID of the process.
  (6) session                %d   The session ID of the process.
  (7) tty_nr                 %d   The controlling terminal of the process.  (The minor device number is contained in the combination of bits 31 to 20 and 7 to 0; the major device number is in bits 15 to 8.)
  (8) tpgid                  %d   The ID of the foreground process group of the controlling terminal of the process.
  (9) flags                  %u   The kernel flags word of the process.  For bit meanings, see the PF_* defines in the Linux kernel source file include/linux/sched.h.  Details depend on the kernel version.
                                   The format for this field was %lu before Linux 2.6.
  (10) minflt                %lu  The number of minor faults the process has made which have not required loading a memory page from disk.
  (11) cminflt               %lu  The number of minor faults that the process's waited-for children have made.
  (12) majflt                %lu  The number of major faults the process has made which have required loading a memory page from disk.
  (13) cmajflt               %lu  The number of major faults that the process's waited-for children have made.
  (14) utime                 %lu  Amount  of  time that this process has been scheduled in user mode, measured in clock ticks (divide by sysconf(_SC_CLK_TCK)).  This includes guest time, guest_time (time spent running a virtual CPU, see below), so
                                    that applications that are not aware of the guest time field do not lose that time from their calculations.
  (15) stime                 %lu  Amount of time that this process has been scheduled in kernel mode, measured in clock ticks (divide by sysconf(_SC_CLK_TCK)).
  (16) cutime                %ld  Amount of time that this process's waited-for children have been scheduled in user mode, measured in clock ticks (divide by sysconf(_SC_CLK_TCK)).  (See also times(2).)  This includes guest time, cguest_time (time
                                    spent running a virtual CPU, see below).
  (17) cstime                %ld  Amount of time that this process's waited-for children have been scheduled in kernel mode, measured in clock ticks (divide by sysconf(_SC_CLK_TCK)).
  (18) priority              %ld  (Explanation  for  Linux 2.6) For processes running a real-time scheduling policy (policy below; see sched_setscheduler(2)), this is the negated scheduling priority, minus one; that is, a number in the range -2 to
                                    -100, corresponding to real-time priorities 1 to 99.  For processes running under a non-real-time scheduling policy, this is the raw nice value (setpriority(2)) as represented in the  kernel.   The  kernel  stores
                                    nice values as numbers in the range 0 (high) to 39 (low), corresponding to the user-visible nice range of -20 to 19.

                                    Before Linux 2.6, this was a scaled value based on the scheduler weighting given to this process.
  (19) nice                  %ld  The nice value (see setpriority(2)), a value in the range 19 (low priority) to -20 (high priority).
  (20) num_threads           %ld  Number of threads in this process (since Linux 2.6).  Before kernel 2.6, this field was hard coded to 0 as a placeholder for an earlier removed field.
  (21) itrealvalue           %ld  The time in jiffies before the next SIGALRM is sent to the process due to an interval timer.  Since kernel 2.6.17, this field is no longer maintained, and is hard coded as 0.
  (22) starttime             %llu The time the process started after system boot.  In kernels before Linux 2.6, this value was expressed in jiffies.  Since Linux 2.6, the value is expressed in clock ticks (divide by sysconf(_SC_CLK_TCK)).
                                    The format for this field was %lu before Linux 2.6.
  (23) vsize                 %lu  Virtual memory size in bytes.
  (24) rss                   %ld  Resident Set Size: number of pages the process has in real memory.  This is just the pages which count toward text, data, or stack space.  This does not include pages which have not been demand-loaded in, or which
                                    are swapped out.
  (25) rsslim                %lu  Current soft limit in bytes on the rss of the process; see the description of RLIMIT_RSS in getrlimit(2).
  (26) startcode             %lu  [PT]
                                    The address above which program text can run.
  (27) endcode               %lu  [PT]
                                    The address below which program text can run.
  (28) startstack            %lu  [PT]
                                    The address of the start (i.e., bottom) of the stack.
  (29) kstkesp               %lu  [PT]
                                    The current value of ESP (stack pointer), as found in the kernel stack page for the process.
  (30) kstkeip               %lu  [PT]
                                    The current EIP (instruction pointer).
  (31) signal                %lu  The bitmap of pending signals, displayed as a decimal number.  Obsolete, because it does not provide information on real-time signals; use /proc/[pid]/status instead.
  (32) blocked               %lu  The bitmap of blocked signals, displayed as a decimal number.  Obsolete, because it does not provide information on real-time signals; use /proc/[pid]/status instead.
  (33) sigignore             %lu  The bitmap of ignored signals, displayed as a decimal number.  Obsolete, because it does not provide information on real-time signals; use /proc/[pid]/status instead.
  (34) sigcatch              %lu  The bitmap of caught signals, displayed as a decimal number.  Obsolete, because it does not provide information on real-time signals; use /proc/[pid]/status instead.
  (35) wchan                 %lu  [PT]
                                    This is the "channel" in which the process is waiting.  It is the address of a location in the kernel where the process is sleeping.  The corresponding symbolic name can be found in /proc/[pid]/wchan.
  (36) nswap                 %lu  Number of pages swapped (not maintained).
  (37) cnswap                %lu  Cumulative nswap for child processes (not maintained).
  (38) exit_signal           %d   (since Linux 2.1.22)
                                    Signal to be sent to parent when we die.
  (39) processor             %d   (since Linux 2.2.8)
                                    CPU number last executed on.
  (40) rt_priority           %u   (since Linux 2.5.19)
                                    Real-time scheduling priority, a number in the range 1 to 99 for processes scheduled under a real-time policy, or 0, for non-real-time processes (see sched_setscheduler(2)).
  (41) policy                %u   (since Linux 2.5.19)
                                    Scheduling policy (see sched_setscheduler(2)).  Decode using the SCHED_* constants in linux/sched.h.
                                      The format for this field was %lu before Linux 2.6.22.
  (42) delayacct_blkio_ticks %llu  (since Linux 2.6.18)
                                     Aggregated block I/O delays, measured in clock ticks (centiseconds).
  (43) guest_time            %lu  (since Linux 2.6.24)
                                     Guest time of the process (time spent running a virtual CPU for a guest operating system), measured in clock ticks (divide by sysconf(_SC_CLK_TCK)).
  (44) cguest_time           %ld  (since Linux 2.6.24)
                                    Guest time of the process's children, measured in clock ticks (divide by sysconf(_SC_CLK_TCK)).
  (45) start_data            %lu  (since Linux 3.3)  [PT]
                                    Address above which program initialized and uninitialized (BSS) data are placed.
  (46) end_data              %lu  (since Linux 3.3)  [PT]
                                    Address below which program initialized and uninitialized (BSS) data are placed.
  (47) start_brk             %lu  (since Linux 3.3)  [PT]
                                    Address above which program heap can be expanded with brk(2).
  (48) arg_start             %lu  (since Linux 3.5)  [PT]
                                    Address above which program command-line arguments (argv) are placed.
  (49) arg_end               %lu  (since Linux 3.5)  [PT]
                                    Address below program command-line arguments (argv) are placed.
  (50) env_start             %lu  (since Linux 3.5)  [PT]
                                    Address above which program environment is placed.
  (51) env_end               %lu  (since Linux 3.5)  [PT]
                                    Address below which program environment is placed.
  (52) exit_code             %d   (since Linux 3.5)  [PT]
                                    The thread's exit status in the form reported by waitpid(2).


    31992 (drlog) S 31957 31992 31957 34819 31992 1077936128 49385 12102 0 23 12306 3380 54 17 20 0 9 0 32185012 270811136 25521 4294967295 4648960 9011865 3219523152 0 0 0 0 0 134758402 0 0 0 17 0 0 0 0 0 0 9019972 9053940 32260096 3219531084 3219531140 3219531140 3219533782 0
*/

// map format code to type
  using C   = char;
  using D   = int;
  using LD  = long int;
  using LLU = long long unsigned int;
  using LU  = long unsigned int;
  using S   = std::string;
  using U   = unsigned int;

  inline const D stat_pid(void)
    { return from_string<D>(_statvec()[0]); }

  inline const S stat_comm(void)
    { return ( delimited_substring(_statvec()[1], '(', ')') ); }

  inline const C stat_state(void)
    { return (_statvec()[2][0]); }

  inline const D stat_ppid(void)
    { return from_string<D>(_statvec()[3]); }

  inline const D stat_pprp(void)
    { return from_string<D>(_statvec()[4]); }

  inline const D stat_session(void)
    { return from_string<D>(_statvec()[5]); }

  inline const D stat_tty_nr(void)
    { return from_string<D>(_statvec()[6]); }

  inline const D stat_tpgid(void)
    { return from_string<D>(_statvec()[7]); }

  inline const U stat_flags(void)
    { return from_string<U>(_statvec()[8]); }

  inline const LU stat_minflt(void)
    { return from_string<LU>(_statvec()[9]); }

  inline const LU stat_cminflt(void)
    { return from_string<LU>(_statvec()[10]); }

  inline const LU stat_majflt(void)
    { return from_string<LU>(_statvec()[11]); }

  inline const LU stat_cmajflt(void)
    { return from_string<LU>(_statvec()[12]); }

  inline const LU stat_utime(void)
    { return from_string<LU>(_statvec()[13]); }

  inline const LU stat_stime(void)
    { return from_string<LU>(_statvec()[14]); }

  inline const int64_t stat_cutime(void)
    { return from_string<int64_t>(_statvec()[15]); }

  inline const LD stat_cstime(void)
    { return from_string<LD>(_statvec()[16]); }

  inline const LD stat_priority(void)
    { return from_string<LD>(_statvec()[17]); }

  inline const LD stat_nice(void)
    { return from_string<LD>(_statvec()[18]); }

  inline const LD stat_num_threads(void)
    { return from_string<LD>(_statvec()[19]); }

  inline const LD stat_itrealvalue(void)
    { return from_string<LD>(_statvec()[20]); }

  inline const LLU stat_starttime(void)
    { return from_string<LLU>(_statvec()[21]); }

  inline const LU stat_vsize(void)
    { return from_string<LU>(_statvec()[22]); }

  inline const LD stat_rss(void)
    { return from_string<LD>(_statvec()[23]); }

  inline const LU stat_rsslim(void)
    { return from_string<LU>(_statvec()[24]); }

  inline const LU stat_startcode(void)
    { return from_string<LU>(_statvec()[25]); }

  inline const LU stat_endcode(void)
    { return from_string<LU>(_statvec()[26]); }

  inline const LU stat_startstack(void)
    { return from_string<LU>(_statvec()[27]); }

  inline const LU stat_kstkesp(void)
    { return from_string<LU>(_statvec()[28]); }

  inline const LU stat_kstkeip(void)
    { return from_string<LU>(_statvec()[29]); }

  inline const LU stat_wchan(void)
    { return from_string<LU>(_statvec()[34]); }

  inline const LU stat_nswap(void)
    { return from_string<LU>(_statvec()[35]); }

  inline const LU stat_cnswap(void)
    { return from_string<LU>(_statvec()[36]); }

  inline const D stat_exit_signal(void)
    { return from_string<D>(_statvec()[37]); }

  inline const U stat_rt_priority(void)
    { return from_string<U>(_statvec()[39]); }

  inline const U stat_policy(void)
    { return from_string<U>(_statvec()[40]); }

  inline const LLU stat_delayacct_blkio_ticks(void)
    { return from_string<LLU>(_statvec()[41]); }

  inline const LU stat_guest_time(void)
    { return from_string<LU>(_statvec()[42]); }

  inline const LD stat_cguest_time(void)
    { return from_string<LD>(_statvec()[43]); }

  inline const LU stat_start_data(void)
    { return from_string<LU>(_statvec()[44]); }

  inline const LU stat_end_data(void)
    { return from_string<LU>(_statvec()[45]); }

  inline const LU stat_start_brk(void)
    { return from_string<LU>(_statvec()[46]); }

  inline const LU stat_arg_start(void)
    { return from_string<LU>(_statvec()[47]); }

  inline const LU stat_arg_end(void)
    { return from_string<LU>(_statvec()[48]); }

  inline const LU stat_env_start(void)
    { return from_string<LU>(_statvec()[49]); }

  inline const LU stat_env_end(void)
    { return from_string<LU>(_statvec()[50]); }

  inline const D stat_exit_code(void)
    { return from_string<D>(_statvec()[51]); }
};

#endif /* PROCFS_H */
