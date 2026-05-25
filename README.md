# PNS Virtual Dial

Wrist-worn gestural volume controller. Surface EMG on the forearm acts as a clutch, the dial only responds to wrist rotation while the muscle is intentionally contracted. A potentiometer sets the activation threshold.

---

## [Demo video:](https://youtu.be/9u-hjNNP5dQ)

[![Watch the demo](https://img.youtube.com/vi/9u-hjNNP5dQ/maxresdefault.jpg)](https://youtu.be/9u-hjNNP5dQ)

___
## Hardware Setup

1. Attach gel pads to the EMG sensor electrodes. Gel lowers skin-electrode impedance and reduces signal noise.
2. Wipe down the belly of the flexor carpi ulnaris (pinky-side forearm, near the elbow) with an alcohol swab and let it dry.
3. Apply the EMG sensor to that site.
4. Connect the EMG sensor to the breadboard via 3.5mm TRS cable. Output feeds ADC channel 0 (GP40), 0–3.3V.
5. Equip the wrist IMU band. Ensure a snug fit to minimize mechanical noise.
6. Connect the IMU band to the breadboard via TRRS cable (I2C: SDA → GP4, SCL → GP5, 400 kHz).
7. Power the board with a 5V 2A brick. Avoids ADC noise from USB bus power fluctuations.
8. Plug the SWD debugger into the MacBook for flashing and serial output.

---

## Software Setup & Calibration

1. Flash firmware via PlatformIO.
2. Run `Main.java`. The GUI connects automatically over USB serial at 115200 baud.
3. Hold the calibration button (GP21) to run gyroscope bias calibration — 500 samples, ~1 second, sensor must be still.
4. With forearm relaxed, note the resting EMG voltage in the GUI.
5. Adjust the threshold potentiometer to ~0.5V above that resting value.
6. To operate: form a pinch (index + middle finger) or firm grip to engage the clutch, then rotate the wrist. 360° of rotation covers the full 0-100 dial range. Releasing the gesture disengages the clutch and freezes the dial.
