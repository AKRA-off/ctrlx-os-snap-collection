/*********************************************************************
* Software License Agreement (BSD License)
 *
 *  Copyright (c) 2025, Kassow Robots
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Kassow Robots nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

#ifndef KR2_KORD_UTILS_TIMEX_H
#define KR2_KORD_UTILS_TIMEX_H

#include <cstdint>
#include <string>
#include <sys/time.h>
#include <time.h>

#include <memory>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

#ifdef __linux
#define NSEC_PER_SEC 1000000000ul
#define NSEC_PER_MSEC 1000000ul // TODO: add include of appropriate header instead
#define USEC_PER_SEC 1000000ul
#endif

#define USED_CLOCK CLOCK_REALTIME
#define CLOCK_NOT_INITIALIZED EFAULT

#define NTM_DIFF(tm1, tm2)                                                                                              \
    (tm1.tv_sec == tm2.tv_sec                                                                                           \
         ? (tm2.tv_nsec - tm1.tv_nsec)                                                                                  \
         : ((tm2.tv_sec - tm1.tv_sec - 1) * NSEC_PER_SEC + NSEC_PER_SEC - tm1.tv_nsec + tm2.tv_nsec))
#define NTM_ADD(tms, tmint)                                                                                             \
    {                                                                                                                   \
        tms.tv_sec += tmint.tv_sec + (tms.tv_nsec + tmint.tv_nsec) / NSEC_PER_SEC;                                      \
        tms.tv_nsec = (tms.tv_nsec + tmint.tv_nsec) % NSEC_PER_SEC;                                                     \
    }

#define TM_EQUAL(__tm1, __tm2) (__tm1.tv_sec == __tm2.tv_sec && __tm1.tv_nsec == __tm2.tv_nsec)

namespace kr2::utils {
class Timex {
public:
    static void initSingletonTimex(int frequency, bool simulation = false);
    static void resetSingletonTimex(int frequency, bool simulation = false);

    static const struct timespec &now();
    static bool now(struct timespec &);
    struct timespec now2();
    static int nanosleepUntil(const struct timespec &wakeup_time);
    int nanosleepUntil2(const struct timespec &wakeup_time);
    static int nanosleep(const struct timespec &period_time);
    static void tick();
    void tick2();

    static double diff(const struct timespec &a_time1, const struct timespec &a_time2);
    static long long nsecDiff(const struct timespec &time1, const struct timespec &time2);
    static bool eq(const struct timespec &time1, const struct timespec &time2);
    static bool soonerEq(const struct timespec &time1, const struct timespec &time2);
    static bool laterEq(const struct timespec &time1, const struct timespec &time2);
    static bool sooner(const struct timespec &time1, const struct timespec &time2);
    static bool later(const struct timespec &time1, const struct timespec &time2);

    static struct timespec thisTick(const struct timespec &ts, int period_offset = 0);
    struct timespec thisTick2(const struct timespec &ts, int period_offset = 0);
    static struct timespec nextTick(const struct timespec &ts, unsigned *a_err_flags = NULL);
    struct timespec nextTick2(const struct timespec &ts, unsigned *a_err_flags = NULL);

    static void addTicks(struct timespec &ts, unsigned short nticks);

    static void addFraction(struct timespec &ts, double a_fraction);

    static std::string timeStampStr();

    static std::string format(long d, long s, long mms);

    enum { ERR_TIME_OP_INCONSISTENCY = 0x0001 };

    static double period();
    double period2() const;

    static bool valid();

    //! Get the timex inital time stamp (the time base).
    static const timespec &tsInit();

    Timex(int frequency, bool simulation = false);
    ~Timex();

    class ClockBase;

private:
    std::unique_ptr<ClockBase> clock_;

    const unsigned long nsecs_per_tick_;
    const unsigned long freq_;
};
} // namespace kr2::utils

#endif // KR2_KORD_UTILS_TIMEX_H
