#ifndef VLC_BITTORRENT_VLC_H
#define VLC_BITTORRENT_VLC_H

#ifndef VLC_ATOMIC_H
#define VLC_ATOMIC_H

// Workaround because VLC's vlc_atomic.h is borked for VLC 2.2.x.

#include <atomic>

#if 0
typedef std::atomic_uint_least32_t vlc_atomic_float;

static inline void vlc_atomic_init_float(vlc_atomic_float *var, float f)
{
    union { float f; uint32_t i; } u;
    u.f = f;
    atomic_init(var, u.i);
}

static inline float vlc_atomic_load_float(vlc_atomic_float *atom)
{
    union { float f; uint32_t i; } u;
    u.i = atomic_load(atom);
    return u.f;
}

static inline void vlc_atomic_store_float(vlc_atomic_float *atom, float f)
{
    union { float f; uint32_t i; } u;
    u.f = f;
    atomic_store(atom, u.i);
}
#endif

#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_input.h>
#include <vlc_demux.h>
#include <vlc_access.h>
#include <vlc_stream.h>
#include <vlc_url.h>
#include <vlc_variables.h>
#pragma GCC diagnostic pop

#endif
