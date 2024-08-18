# Pico Sequencer Synth
A polyphonic, multitimbral DDS (Direct Digital Synthesis) library for Raspberry Pi Pico, equipped with a sequencer supporting up to 8 channels.

Demo video: [YouTube link](https://www.youtube.com/watch?v=MUQ7loauSa0)

Works with RP2040 and RP2350.


### Features
- I²S or PWM audio output
- Available waveforms: NOISE, SQUARE, SAW, TRIANGLE, SINE, WAVE (custom waveform)
- ADSR amp envelope
- Polyphony up to 8 voices, each one with individual waveform, ADSR, and volume settings
- 44.100 kHz default sample rate
- Multitrack sequencer able to start and stop playback of multiple (non-concurrent) sequences
- A note name to pitch map, covering notes from B0 to D#8
- Chiptune-ready!


### Usage
A full example program is included. Please see [example/example.c](/example/example.c).

To specify which audio output to use, leave uncommented only one of the two definitions in your CMakeLists.txt
```cmake
target_compile_definitions(${PROJECT_NAME} PRIVATE
        # USE_AUDIO_PWM=1
        USE_AUDIO_I2S=1
    )
```

### A note about PWM audio
The audio quality of the PWM output is greatly inferior to the I²S one. It's also very noisy if unfiltered, and for this reason you might want to pair it with a DAC circuit to smooth the signal. There are several designs that will work, but my research led me to the one I used for [Dodepan](https://github.com/TuriSc/Dodepan), which also provides some noise filtering and DC offset removal. 


### Credits
This library is the C port of a [C++ demo by Pimoroni](https://github.com/pimoroni/pimoroni-pico/tree/main/examples/pico_audio), adapted to output audio via I²S or PWM. The original depencency of 'Pico Extras' SDK libraries has been removed and I²S functionality is now provided by a PIO-driven I²S driver based on the work of [Ricardo Massaro](https://github.com/moefh/).


### Version history
- 2024.01.13 - Initial release
