# ArduinoStairLights
Use 2 laser curtains and Arduino to illuminate stairs without needing to install overhead lighting

# Purpose
In order to illuminate the stairs of a cabin I'm building, I did not want to resort to a big overhead light that would brighten a loft
upstairs unnecessarily.  I found this Instructables (https://www.instructables.com/id/LED-Stair-Lighting/) that was an excellent starting
point for my project.  Basically I pulled out the motion sensors (too wide field of view, I assumed, also 5v) for lasers (narrow field of
view, and 3.3 native).  I don't think I used any of their code, but implemented a simple state machine instead.  Given how much time I put
into it, I probably should have!

# Materials
- 2 Laser Detectors (https://www.amazon.com/gp/product/B07F3RH7TC/ref=ppx_yo_dt_b_asin_title_o00_s00?ie=UTF8&psc=1)
- 1 Arduino (I used a Teensy 3.2 https://www.pjrc.com/store/teensy32.html)
- Breadboard / mounting surface
- Set of NeoPixels (https://www.amazon.com/gp/product/B07FVR6W71/ref=ppx_yo_dt_b_asin_title_o09_s00?ie=UTF8&psc=1)
- Power Supply (https://www.amazon.com/gp/product/B01M0KLECZ/ref=ppx_od_dt_b_asin_title_s02?ie=UTF8&psc=1)
- .. more as  I get it (wire, connectors, etc)

