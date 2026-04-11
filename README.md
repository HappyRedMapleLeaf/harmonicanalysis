### title: ill probably make this private eventually, but for now, im publicly working on coding projects outside of work to explore my other interests with the intent of making the 0 people reading this think that i have hobbies, which is true, but suspect that this effort will ultimately backfire and instead make these same 0 people think that im "trying too hard" and conclude that, like many others in the "tech industry", whatever the hell that means these days, i have no dreams or passions outside of getting that BAG.

exploring auditory illusions - specifically, the perception of the presence, intensity, and phase of different overtones.

started with an internet rabbit hole that went something like:\
benn jordan's speech jammer video ->\
adam neely's combination tones video ->\
psychoacoustic effects, missing fundamental, non-linearity of the structure of the ear ->\
hopelessly failing to intuitively grasp non-linear differential equations ->\
mongolian/tuvan throat singing?? (specifically, the kargyraa undertone singing style) ->\
wtf i can add a shitton of higher overtones to a sawtooth wave and make it sound an octave lower?? ->\
wtf i can then remove some of these higher overtones, adjust the amplitudes of them a bit, and then it'll suddenly sound like a root and a fifth sawtooth playing at the same time?? ->\
sawtooth waves have their harmonics at specific phases to line up and create the saw shaped discontinuity, so what if i mess around with these phases?

### step 0
.wav file generation with just a basic sawtooth\
done on apr 11 2026

### step 1
cli live playback with portaudio

### step 2
qt gui with live playback and sliders for fundamental frequency and number of overtones

### step 3
displays of what one period's waveform looks like

### step 4
visual fft

### step 5
have the 5 categories of fundamentals as separate sliders

### step 6
allow adjustment of individual phases (or in groups)
