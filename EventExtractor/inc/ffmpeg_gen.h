#ifndef FFMPEG_GEN_H
#define FFMPEG_GEN_H

#include <cstdint>
#include <cstddef>

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

// 4. Wrap the actual C headers
extern "C" {
// IWYU pragma: begin_exports
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/mem.h>
#include <libavutil/samplefmt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/avassert.h>
#include <libavutil/avstring.h>

#include <libavcodec/avcodec.h>
#include <libavcodec/defs.h>

#include <libavformat/avformat.h>
#include <libavformat/avio.h>

#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

#include <libswresample/swresample.h>
// IWYU pragma: end_exports

// Add any other FFmpeg headers you use here

#define FFMPEG_NEW_CHANNEL_LAYOUT (LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 28, 100))

#ifndef FF_PROFILE_H264_MAIN
#ifdef AV_PROFILE_H264_MAIN
#define FF_PROFILE_H264_MAIN AV_PROFILE_H264_MAIN
#else
#define FF_PROFILE_H264_MAIN 77
#endif
#endif

}

#endif // FFMPEG_GEN_H
