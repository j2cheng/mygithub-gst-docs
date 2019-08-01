# Events

Events are objects passed around in parallel to the buffer dataflow to
notify elements of various events.

Events are received on pads using the event function. Some events should
be interleaved with the data stream so they require taking the
`STREAM_LOCK`, others don’t.

Different types of events exist to implement various functionalities.

* `GST_EVENT_FLUSH_START`: data is to be discarded
* `GST_EVENT_FLUSH_STOP`: data is allowed again
* `GST_EVENT_CAPS`: Format information about the following buffers
* `GST_EVENT_SEGMENT`: Timing information for the following buffers
* `GST_EVENT_TAG`: Stream metadata.
* `GST_EVENT_BUFFERSIZE`: Buffer size requirements
* `GST_EVENT_SINK_MESSAGE`: An event turned into a message by sinks
* `GST_EVENT_EOS`: no more data is to be expected on a pad.
* `GST_EVENT_QOS`: A notification of the quality of service of the stream
* `GST_EVENT_SEEK`: A seek should be performed to a new position in the stream
* `GST_EVENT_NAVIGATION`: A navigation event.
* `GST_EVENT_LATENCY`: Configure the latency in a pipeline
* `GST_EVENT_STEP`: Stepping event
* `GST_EVENT_RECONFIGURE`: stream reconfigure event

- `GST_EVENT_DRAIN`: Play all data downstream before returning.
  > not yet implemented, under investigation, might be needed to do
    still frames in DVD.

## src pads

A `gst_pad_push_event()` on a srcpad will first store the sticky event
in the sticky array before sending the event to the peer pad. If there
is no peer pad and the event was not stored in the sticky array, FALSE
is returned.

Flushing pads will refuse the events and will not store the sticky
events.

## sink pads

A `gst_pad_send_event()` on a sinkpad will call the event function on
the pad. If the event function returns success, the sticky event is
stored in the sticky event array and the event is marked for update.

When the pad is flushing, the `_send_event()` function returns FALSE
immediately.

When the next data item is pushed, the pending events are pushed first.

This ensures that the event function is never called for flushing pads
and that the sticky array only contains events for which the event
function returned success.

## pad link

When linking pads, the srcpad sticky events are marked for update when
they are different from the sinkpad events. The next buffer push will
push the events to the sinkpad.

## FLUSH_START/STOP

A flush event is sent both downstream and upstream to clear any pending
data from the pipeline. This might be needed to make the graph more
responsive when the normal dataflow gets interrupted by for example a
seek event.

Flushing happens in two stages.

1) a source element sends the `FLUSH_START` event to the downstream peer element.
   The downstream element starts rejecting buffers from the upstream elements. It
   sends the flush event further downstream and discards any buffers it is
   holding as well as return from the chain function as soon as possible.
   This makes sure that all upstream elements get unblocked.
   This event is not synchronized with the `STREAM_LOCK` and can be done in the
   application thread.

2) a source element sends the `FLUSH_STOP` event to indicate
   that the downstream element can accept buffers again. The downstream
   element sends the flush event to its peer elements. After this step dataflow
   continues. The `FLUSH_STOP` call is synchronized with the `STREAM_LOCK` so any
   data used by the chain function can safely freed here if needed. Any
   pending EOS events should be discarded too.

After the flush completes the second stage, data is flowing again in the
pipeline and all buffers are more recent than those before the flush.

For elements that use the pullrange function, they send both flush
events to the upstream pads in the same way to make sure that the
pullrange function unlocks and any pending buffers are cleared in the
upstream elements.

A `FLUSH_START` may instruct the pipeline to distribute a new `base_time`
to elements so that the `running_time` is reset to 0. (see
[clocks](additional/design/clocks.md) and [synchronisation](additional/design/synchronisation.md)).

## EOS

The EOS event can only be sent on a sinkpad. It is typically emitted by
the source element when it has finished sending data. This event is
mainly sent in the streaming thread but can also be sent from the
application thread.

The downstream element should forward the EOS event to its downstream
peer elements. This way the event will eventually reach the sinks which
should then post an EOS message on the bus when in PLAYING.

An element might want to flush its internally queued data before
forwarding the EOS event downstream. This flushing can be done in the
same thread as the one handling the EOS event.

For elements with multiple sink pads it might be possible to wait for
EOS on all the pads before forwarding the event.

The EOS event should always be interleaved with the data flow, therefore
the GStreamer core will take the `STREAM_LOCK`.

Sometimes the EOS event is generated by another element than the source,
for example a demuxer element can generate an EOS event before the
source element. This is not a problem, the demuxer does not send an EOS
event to the upstream element but returns `GST_FLOW_EOS`, causing the
source element to stop sending data.

An element that sends EOS on a pad should stop sending data on that pad.
Source elements typically `pause()` their task for that purpose.

By default, a GstBin collects all EOS messages from all its sinks before
posting the EOS message to its parent.

The EOS is only posted on the bus by the sink elements in the PLAYING
state. If the EOS event is received in the PAUSED state, it is queued
until the element goes to PLAYING.

A `FLUSH_STOP` event on an element flushes the EOS state and all pending
EOS messages.

## SEGMENT

A segment event is sent downstream by an element to indicate that the
following group of buffers start and end at the specified positions. The
newsegment event also contains the playback speed and the applied rate
of the stream.

Since the stream time is always set to 0 at start and after a seek, a 0
point for all next buffer’s timestamps has to be propagated through the
pipeline using the SEGMENT event.

Before sending buffers, an element must send a SEGMENT event. An element
is free to refuse buffers if they were not preceded by a SEGMENT event.

Elements that sync to the clock should store the SEGMENT start and end
values and subtract the start value from the buffer timestamp before
comparing it against the stream time (see [clocks](additional/design/clocks.md)).

An element is allowed to send out buffers with the SEGMENT start time
already subtracted from the timestamp. If it does so, it needs to send a
corrected SEGMENT downstream, ie, one with start time 0.

A SEGMENT event should be generated as soon as possible in the pipeline
and is usually generated by a demuxer or source. The event is generated
before pushing the first buffer and after a seek, right before pushing
the new buffer.

The SEGMENT event should be sent from the streaming thread and should be
serialized with the buffers.

Buffers should be clipped within the range indicated by the newsegment
event start and stop values. Sinks must drop buffers with timestamps out
of the indicated segment range.

## TAG

The tag event is sent downstream when an element has discovered metadata
tags in a media file. Encoders can use this event to adjust their
tagging system. A tag is serialized with buffers.

## BUFFERSIZE

> **Note**
>
> This event is not yet implemented.

An element can suggest a buffersize for downstream elements. This is
typically done by elements that produce data on multiple source pads
such as demuxers.

## QOS

A QOS, or quality of service message, is generated in an element to
report to the upstream elements about the current quality of real-time
performance of the stream. This is typically done by the sinks that
measure the amount of framedrops they have. (see [qos](additional/design/qos.md))

## SEEK

A seek event is issued by the application to configure the playback
range of a stream. It is called form the application thread and travels
upstream.

The seek event contains the new start and stop position of playback
after the seek is performed. Optionally the stop position can be left at
-1 to continue playback to the end of the stream. The seek event also
contains the new playback rate of the stream, 1.0 is normal playback,
2.0 double speed and negative values mean backwards playback.

A seek usually flushes the graph to minimize latency after the seek.
This behaviour is triggered by using the `SEEK_FLUSH` flag on the seek
event.

The seek event usually starts from the sink elements and travels
upstream from element to element until it reaches an element that can
perform the seek. No intermediate element is allowed to assume that a
seek to this location will happen. It is allowed to modify the start and
stop times if it needs to do so. this is typically the case if a seek is
requested for a non-time position.

The actual seek is performed in the application thread so that success
or failure can be reported as a return value of the seek event. It is
therefore important that before executing the seek, the element acquires
the `STREAM_LOCK` so that the streaming thread and the seek get
serialized.

The general flow of executing the seek with FLUSH is as follows:

1) unblock the streaming threads, they could be blocked in a chain
   function. This is done by sending a `FLUSH_START` on all srcpads or by pausing
   the streaming task, depending on the seek FLUSH flag.
   The flush will make sure that all downstream elements unlock and
   that control will return to this element chain/loop function.
   We cannot lock the `STREAM_LOCK` before doing this since it might
   cause a deadlock.

2) acquire the `STREAM_LOCK`. This will work since the chain/loop function
   was unlocked/paused in step 1).

3) perform the seek. since the `STREAM_LOCK` is held, the streaming thread
   will wait for the seek to complete. Most likely, the stream thread
   will pause because the peer elements are flushing.

4) send a `FLUSH_STOP` event to all peer elements to allow streaming again.

5) create a SEGMENT event to signal the new buffer timestamp base time.
   This event must be queued to be sent by the streaming thread.

6) start stopped tasks and unlock the `STREAM_LOCK`, dataflow will continue
   now from the new position.

More information about the different seek types can be found in
[seeking](additional/design/seeking.md).

## NAVIGATION

A navigation event is generated by a sink element to signal the elements
of a navigation event such as a mouse movement or button click.
Navigation events travel upstream.

## LATENCY

A latency event is used to configure a certain latency in the pipeline.
It contains a single GstClockTime with the required latency. The latency
value is calculated by the pipeline and distributed to all sink elements
before they are set to PLAYING. The sinks will add the configured
latency value to the timestamps of the buffer in order to delay their
presentation. (See also [latency](additional/design/latency.md)).

## DRAIN

> **Note**
>
> This event is not yet implemented.

Drain event indicates that upstream is about to perform a real-time
event, such as pausing to present an interactive menu or such, and needs
to wait for all data it has sent to be played-out in the sink.

Drain should only be used by live elements, as it may otherwise occur
during prerolling.

Usually after draining the pipeline, an element either needs to modify
timestamps, or FLUSH to prevent subsequent data being discarded at the
sinks for arriving late (only applies during playback scenarios).