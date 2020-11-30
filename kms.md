KMS sequence format
===================

The KMS sequence format used by the Keyboardmania games is a derivative of Standard MIDI Format with a number of noteworthy differences.

All offsets in the following information are in hexadecimal. Sizes are in decimal. All values are stored in big-endian form.

Header
------

A KMS file always starts with a header with a signature of `MThd`.

| Offset | Content | Meaning |
|--------|---------|---------|
| 0x0000 | `MThd`  | Signature |
| 0x0004 | uint32  | Total size of file |
| 0x0008 | ?int16  | Unknown |
| 0x000A | 0x0001  | Format flag (always 1) |
| 0x000C | uint16  | Number of tracks |
| 0x000E | uint16  | Ticks per quarter note (usually 480) |

Tracks
------

The header is followed by a set of tracks. There is no padding. Tracks start with a signature of `MTrk` and end with the 24-bit "Track End" event 0xFF2F00.

After the `MTrk` signature, each event has the following format:

| Offset              | Content  | Meaning |
|---------------------|----------|---------|
| 0x0000              | uint24   | Timestamp, measured in ticks since the start of the song |
| 0x0003, high nibble | enum     | Event type |
| 0x0003, low nibble  | uint4    | Channel or system message type |
| 0x0004              | 1+ bytes | Event data |

Unlike Standard MIDI Format, running status is not used.

The following events are recognized:

| Event | Name           | Data Size  | Description |
|-------|----------------|------------|-------------|
| 0x8   | Note Off       | 2          | The first byte is the note number. The second byte in SMF is a release velocity, but it appears to always be 0x40 here. |
| 0x9   | Note On        | 2, 4, or 8 | The first byte is the note number. The second byte is 0x00, 0xFF, or the attack velocity. See "Note On Events" below for more information. |
| 0xB   | Controller     | 2          | The first byte is the controller ID. The second byte is the value being assigned to the controller.  See "Controllers" below for more information. |
| 0xC   | Program Change | 1          | The parameter is the instrument ID. |
| 0xF0  | SysEx          | variable   | The data is terminated by an 0xF7 byte. The interpretation of the content is unknown. |
| 0xFF  | Meta           | variable   | See "Meta Events" below. |

Note On Events
--------------

KMS extends the normal interpretation of note velocity to add two additional states.

If the velocity is 0x00, the note is followed by a 16-bit value expressing the note length in ticks.

If the velocity is 0xFF, the note is followed by a 16-bit value of unknown meaning. Only 0xFFFF has been observed. Additionally, on channel 0x4, this is followed by an additional
24-bit value which is presumed to express the note length in ticks. Both of these forms have only been seen preceding a Track End event and may signify that it is the last note
in the track.

Controllers
-----------

The controllers used by KMS appear to align with the MIDI standard, although the precise interpretation may vary.
See [MIDI Control Change Messages](https://www.midi.org/specifications-old/item/table-3-control-change-messages-data-bytes-2) for more information.

The following controllers have been observed in use so far:

 00  06  07  0a  0b  20  47  49
 4a  5b  5d  62  63  64  65


Meta Events
-----------

The following meta events have been observed:

| Event    | Data Size | Description |
|----------|-----------|-------------|
| 0xFF03   | prefixed  | Appears to be some sort of track identifier. |
| 0xFF0601 | 1         | Measure marker. |
| 0xFF0603 | 1         | Beat marker within a measure. |
| 0xFF0605 | 5         | Unknown. |
| 0xFF51   | prefixed  | Tempo, in microseconds per quarter note. |
| 0xFF2F   | 1         | Marks the end of the track. The data should be 0x00. |

When the data size is listed as prefixed, the first byte of the data represents the number of additional data bytes that follow.
