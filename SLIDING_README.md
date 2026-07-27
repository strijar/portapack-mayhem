# Sliding-frequency Audio RX work in progress

This branch ports the old `origin/sliding_freq` Audio RX functionality onto
the current `next` code base.  It intentionally keeps the current Mayhem
colour scheme and existing Zoom UI.

## Current status

Tested on hardware:

- AM/NFM Audio RX can be tuned inside a fixed spectrum window without
  immediately retuning the RF centre.
- The displayed frequency follows the encoder.
- The green/yellow channel-filter cursor follows the selected audio
  frequency; the red marker remains the hardware centre.
- Crossing the sliding limit moves the hardware centre and keeps tuning
  continuous.
- Normal spectrum width is 192 kHz. AM Zoom x2 remains available and shows
  96 kHz.
- The USB/LSB audio image about 10--12 kHz away is now strongly suppressed.
- M4 load is about 60% on the tested unit and `M4 miss` does not increase.
- Audio sounds usable off-air, but its final quality has not yet been
  measured with a clean generated signal.

The last tested firmware before this checkpoint was built with GCC 9.3.1
(the project recommends 9.2.1) and had SHA-256:

`f6babf5e5d76a866bfd3120db549755dd5030d5a6a8278c7c9a026b50793db95`

## UI and tuning changes

- Added `AudioDDCConfigMessage` and `baseband::set_audio_ddc_frequency()`.
- Added an atomic receiver-model operation which stores the displayed
  frequency while applying a hidden RF-centre offset.
- AM and NFM use sliding tuning within approximately +/-48 kHz.
- The RF centre is moved only when the selected frequency reaches the
  sliding boundary.
- Spectrum messages now carry the channel-filter frequency offset.
- Spectrum rendering moves the useful-band and transition-band cursors to
  the selected audio channel.
- Frequency-field and waterfall encoder handling were adapted for live
  tuning.

## Current DSP chain

The 192 kHz display and audio processing use separate branches after the
first decimator:

```text
3.072 MHz complex int8
        |
        v
specialised FS/4 FIR, /4
        |
        +-----------------------> 768 kHz display branch
        |                           sparse snapshots
        |                           BlockDecimator /4
        |                           192 kHz FFT
        |
        v
16-tap SIMD FIR, /2
384 kHz complex int16
        |
        v
32-tap frequency-translating complex FIR, /8
        |
        v
interpolated SIMD NCO at 48 kHz
        |
        v
48 kHz complex int16
        |
        +--> AM: FIR /4 -> 12 kHz -> channel filter -> demodulator
        |
        `--> NFM: channel FIR /2 -> 24 kHz -> demodulator
```

The translating FIR modulates its 32 coefficients only when the tuning
offset changes. It combines channel translation with the 384-to-48 kHz
decimation. A residual phase rotation is performed at only 48 kHz. FIR
history is retained when tuning changes.

The NCO uses a 32-bit phase accumulator, a 256-entry Q15 sine table, linear
interpolation, and two packed ARM SIMD multiplies per complex sample.

## Why the old chain produced audio images

The original filter coefficients document these intended rates:

```text
3.072 MHz -> 384 kHz -> 48 kHz -> 12 kHz
```

The old sliding-frequency change widened the first stage from `/8` to `/4`
for the spectrum, but left the downstream audio filters unchanged. The
actual audio rates became:

```text
3.072 MHz -> 768 kHz -> 96 kHz -> 12 kHz
```

Consequently, the downstream filter boundaries were doubled in real
frequency and the final direct `/8` decimation produced audible copies
spaced by roughly the 12 kHz output rate. In USB the copy was heard above
the station; in LSB it was heard below it.

The current separate `/2` audio stage restores the intended 384 kHz input
rate without reducing the 192 kHz display width.

## Approaches tried

### Continuous 192 kHz channelizer

Tried:

```text
3.072M -> 768k -> 384k -> 192k -> NCO -> channel FIR
```

This used two half-band filters continuously and provided good control over
aliasing, but saturated the M4. CPU stayed at 100%, DMA misses increased,
and CW/SSB audio became hum, crackle, or metallic noise.

### Circular generic FIR optimisation

`FIRAndDecimateComplex` was changed from shifting its complete history to a
mirrored circular delay line. This is retained and reduces memory movement.
By itself it did not provide enough headroom for the continuous 192 kHz
chain.

### Sparse half-band filters

Sparse half-band FIR stages were implemented. Two continuous stages still
left the M4 at 100%. The remaining audio `/2` stage was later replaced by
the existing fixed, unrolled `FIRC16xR16x16Decim2`, which is cheaper.

### Replacement first-stage c8 FIR

A 20-tap circular SIMD c8 `/4` implementation was tested in place of
`FIRC8xR16x24FS4Decim4`. It did not reduce the measured load and could make
misses worse. The original specialised, unrolled FS/4 implementation is
used now.

### NCO before the 768-to-96 kHz decimator

Moving the NCO to 768 kHz did not remove the observed image and raised CPU
to 100%. This experiment also helped show that the important fault was the
downstream sample-rate mismatch.

### NCO at 384 kHz

After restoring the missing `/2`, a packed-SIMD table NCO at 384 kHz
suppressed the image but still used nearly all M4 time. Replacing it with
the frequency-translating `/8` FIR reduced measured CPU to about 60%.

### Non-interpolated NCO

A direct 256-phase lookup was tested to reduce load. It did not solve the
load problem while the NCO ran at 384 kHz and was a possible source of
phase-quantisation noise. Linear interpolation is restored now because the
residual NCO runs at only 48 kHz.

## Spectrum compromise

The display branch retains the old lightweight behaviour: it processes
sparse 768 kHz blocks and obtains 192 kHz samples using `BlockDecimator`
selection rather than a continuous anti-alias FIR. This keeps CPU low, but
the displayed spectrum may contain aliases.

Do not add a continuously running high-order display filter without first
measuring M4 headroom. A future improvement could capture a short
contiguous FFT frame and filter only that frame, instead of filtering every
baseband buffer.

## Firmware size

Legacy built-in `ble_rx_app` and `ble_tx_app`, including their baseband
images and navigation entries, are disabled by default through
`BUILD_LEGACY_BLE_APPS=OFF`. They are intended to be converted to SD-card
external applications later. Set the option to `ON` to restore them.

## Tests still needed

Use a calibrated or at least stable RF generator:

1. Test an unmodulated carrier in CW.
2. Test USB and LSB with a clean 1 kHz audio tone.
3. Test AM and NFM with known modulation and deviation.
4. Repeat near offset 0 and at approximately +/-10, +/-30, and +/-45 kHz
   from the red RF centre.
5. Measure wanted-to-image rejection, especially around offsets which used
   to produce the 10--12 kHz copy.
6. Check amplitude and audio-frequency accuracy while changing the DDC
   offset.
7. Check for clicks both while stationary and while stepping the encoder.
8. Compare Zoom x1 and x2 without changing the audio result.
9. Record M4 CPU and verify that `M4 miss` remains constant in every
   modulation mode.
10. Test strong adjacent signals near both edges of the 192 kHz window.

## Further ideas

- Add a temporary diagnostic mode which injects deterministic complex test
  samples before the audio channelizer. This would allow repeatable tests
  without an RF generator.
- Add host-side response tests for the translated FIR at several positive
  and negative offsets.
- Measure coefficient quantisation and image rejection of the translating
  FIR; consider higher precision during coefficient generation if needed.
- Verify phase continuity when coefficients change rapidly during encoder
  tuning.
- Profile AM and NFM separately now that there is M4 headroom.
- If the residual sound defect is real, compare samples at 384, 48, and
  12/24 kHz to isolate the stage introducing it.
- Implement filtered, burst-mode spectrum capture if display aliases prove
  objectionable.
- Convert the disabled BLE RX/TX applications to SD external applications,
  then remove the temporary build option.

## Build

The working build directory used during development is `build`:

```sh
cmake --build build --target firmware -j4
```

The generated image is:

```text
build/firmware/portapack-mayhem-firmware.bin
```

