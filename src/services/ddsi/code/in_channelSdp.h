#ifndef IN_CHANNEL_SDP_H
#define IN_CHANNEL_SDP_H

/* OS abstraction includes. */
#include "os_defs.h"
#include "os_classbase.h"
#include "os_stdlib.h"

#include "in_channelTypes.h"
#include "in_streamPair.h"
#include "u_participant.h"
/**
 * Allow usage of this C code from C++ code.
 */
#if defined (__cplusplus)
extern "C" {
#endif

/**
 * Macro that allows the implementation of type checking when casting an
 * object. The signature of the 'casting macro' must look like this:
 */
#define in_channelSdp(_this) ((in_channelSdp)_this)

/**
 * Macro that calls the in_objects validity check function with the
 * appropiate type
 */
#define in_channelSdpIsValid(_this) in_objectIsValid(in_object(_this))

/**
 * Macro to make the in_objectKeep operation type specific
 */
#define in_channelSdpKeep(_this) in_channelSdp(in_objectKeep(in_object(_this)))

/**
 * Macro to make the in_objectFree operation type specific
 */
#define in_channelSdpFree(_this) in_objectFree(in_object(_this))

in_channelSdp
in_channelSdpNew(
    in_stream stream,
    in_plugKernel plug);

/* Close the brace that allows the usage of this code in C++. */
#if defined (__cplusplus)
}
#endif

#endif /* IN_CHANNEL_SDP_H */
