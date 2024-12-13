# Pipboy Mini

Pipboy ESP32 Smartwatch

I love Fallout’s Pipboy (wrist computer), but the truth is it’s too big and clunky to actually be a daily worn device. I wanted to see if I could make a smartwatch with a similar vibe, but in a form factor small enough to be worn daily in the real world.

I decided to build this around Adafruit’s ESP32-S2 Reverse TFT board. This board is fairly tiny, and it has built-in features that I needed:

The way the watch works, is that on reset it gets the time and date from online NTP servers while connected to wifi. Then the ESP32-S2’s internal clock handles keeping track of time until the time is updated via NTP servers again. This seems to work well. (Traditionally, you would use a Real Time Clock or GPS module to keep accurate time, but that would have made the watch bulkier.) Likewise, I use OpenWeatherMap’s API to get weather data to the watch. I plan to add additional functionally in the future.

See pictures and video here: https://austindixon.com/pipboy-mini

EDIT: I have now added a Binary Watchface option and a "Pipboy Green" color scheme option, both selectable via buttons.
