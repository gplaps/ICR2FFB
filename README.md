# FFB for ICR2 – BETA 0.1  
**USE AT YOUR OWN RISK**

This is a custom Force Feedback application for the classic racing simulator **IndyCar Racing II** by Papyrus.

This app works by reading memory from the game (speed, tire loads, etc.) and uses that to send DirectInput force effects to your wheel.

Its all a lot of guesswork and workarounds bandaided together but it feels ok!

This is my first ever big program, so I am sure it could be done better in almost every aspect, let me know if theres an obvious mistake!

Big thanks to SK and Eric H for all their help in bringing this to life.

---

## DISCLAIMER – USE AT YOUR OWN RISK

I have tried my best to make sure this app can't hurt you or your wheel but **no guarantees**.  
I’ve been using it without issues on my own hardware, but your mileage may vary.

---

## Installation

1. **Download** the app. Get the latest version from Releases
2. **Open `ffb.ini`** and edit the following:
    - `Device` — Must match your device name **exactly** as seen in Windows "Game Controllers".
    - `Game` — Set to either `indycar` or `cart` depending on your executable name.
    - `Version` — Set to `REND32A` for Rendition builds, or `DOS4G` for standard DOS builds.  
      _(Windy not supported yet.)_
    - `Force` — Controls force scale (default is 25%).  
      **Be careful** — although the code limits input, always test with low force first.

3. **Run the app**, then launch the game.  
   After a moment, the window should begin showing telemetry.

> You may need to run the app in **Admin mode** so it can access IndyCar II’s memory.

---

## Changing Settings

You can close the app, edit `ffb.ini`, and reopen it while ICR2 is still running.  
To avoid sudden force application, **pause the game first** before restarting the app.

---

## Version History

### Betas

**0.2 (2025-08-08)**  
- Fixed issues with Thrustmaster wheels not working with constant force due to pausing effects

**0.1 (2025-08-07)**  
- Initial beta pushed to Git

---

### Alphas

**0.7 (2025-08-07)**  
- App now detects all DOSBox versions, not just specific titles  
- Improved slip calculations for oversteer detection  
- Reduced display flicker while running  
- Full error logging to `log.txt`

**0.6 (2025-08-07)**  
- Added `invert` toggle to `ffb.ini`  
- Improved force direction compatibility (e.g. for Moza wheels)

**0.5 (2025-08-06)**  
- Major code cleanup and comments  
- Increased polling rate to 60 Hz  
- Added example EXEs to unify setups

**0.4 (2025-08-06)**  
- Created this README  
- Moved version history here  
- No longer requires Visual Studio  
- Forces only apply after telemetry starts

**0.3 (2025-08-05)**  
- Improved slip calculation for more detailed constant force

**0.2 (2025-08-05)**  
- Corrected support for `DOS4G` executables

**0.1 (2025-08-05)**  
- First working version

---