# 3D-printing ALLBOT parts on a Prusa

Velleman ships the ALLBOT kits with pre-moulded plastic, but if you are reproducing a kit from scratch, replacing a broken swing arm, or designing your own backbone/bracket variants, you can print the parts on a Prusa Mini or MK4. Adapted from the workflow taught at [geekedu.eu — 3D printing on Prusa](https://www.geekedu.eu/cesty/fyzicke-pocitani/u/3d-printing-prusa/).

## Preparing the model

### 1. Create or open a 3D model and export as STL

Model the part in one of:

- **BlocksCAD** — browser-based, block-programming style; good for teaching.
- **Tinkercad** — browser-based, drag-and-drop solids; good for quick custom brackets.
- **Unity** — if you already have geometry in a game engine and want to export it.

Once the model is done, export it as an `.stl` file — that is the format the slicer consumes.

### 2. (Alternative) Download a ready-made model from Printables

[Printables.com](https://www.printables.com/) is Prusa Research's free community library. Search for "ALLBOT" or the specific kit code (e.g. VR408 part X) and download the `.stl` directly. This skips the modelling step entirely — useful when you only need a replacement for a broken part.

## Printing the model

### 3. Slice the STL with PrusaSlicer

Open the `.stl` in [PrusaSlicer](https://www.prusa3d.com/page/prusaslicer_424/) and pick:

- The **printer profile** (Mini / MK4 / etc.) — picks bed dimensions and firmware flavour.
- The **filament** — PLA is fine for ALLBOT swing arms and brackets; PETG if you want more heat and impact resistance on the knee joints.
- **Layer height** — 0.2 mm is a good default for structural parts; 0.15 mm if you care about surface finish on the visible backbone.
- **Supports** — enable for overhangs in the swing arms; the servo bracket is usually flat enough to print without.
- **Infill** — 20 % is plenty for the chassis; go to 40 %+ for the knee horn that directly transfers torque.

Slice and export as `.gcode`.

### 4. Print over the network with OctoPrint

[OctoPrint](https://octoprint.org/) gives the Prusa a web interface. Upload the sliced `.gcode`, then start the print from the browser. Useful features when you're printing the 20-odd plastic parts for an ALLBOT:

- **Webcam + timelapse** — leave it unattended and still catch a layer-shift the moment it happens.
- **Print queue** — stack several parts back-to-back without sitting at the printer to hit "start" each time.

If you're only printing one or two parts, SD card + front-panel is faster — OctoPrint earns its keep when you're churning out a full kit's worth of brackets and arms.
