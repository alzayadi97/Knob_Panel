README
Purpose of the Project
The objective of this project is to ensure that the ESP32-C3-LCDKit is capable of:
Producing audio at specific levels.
Adjusting the screen brightness to predefined levels: 25%, 50%, 75%, and 100%.
Synchronizing audio output with brightness changes to ensure functionality at each brightness level.
Modifications Made
Memory Allocation Adjustment
File Modified: lv_conf.h
Change Made: Increased memory buffer allocation for the LVGL library to handle graphical and audio-related tasks simultaneously without running out of memory.
Updated LV_MEM_SIZE or custom memory allocation macros.
Ensured LV_MEM_CUSTOM is set appropriately for custom memory allocation functions.
Also modified files: audio_main.c, audio_app.c, and ui_light_2color.c
Changes made: These were the three source files majority of the configuring was made and was done so to ensure audio playback and ensure levels of brightness to each desired level
Updated  the shared variable brightness_level to reflect the current brightness.
Signaled the Voice Announcement Task using xEventGroupSetBits when  never the brightness level changed.

Brightness and Audio Functionality
Brightness Control:
Integrated functions to adjust the brightness levels of the display to 25%, 50%, 75%, and 100%.
Added menu options for selecting these brightness levels.
Audio Output:
Ensured audio functionality is active at each brightness level.
Verified the audio levels are consistent across brightness adjustments.
Debugging Enhancements
Enabled logging in the lv_conf.h file to capture error messages and system information during runtime.
Also adjusted audio_app.c to synchronize audio and brightness levels to state level of brightness at certain levels
Configured audio_app.c to ensure synchronization of menu
Configured LV_USE_LOG and adjusted logging levels for better troubleshooting.
Synchronization
Developed a mechanism to synchronize audio playback with brightness adjustments.
Utilized mutexes to avoid conflicts between brightness changes and audio tasks.
Building and Flashing the Firmware
Setup ESP-IDF: Ensure the ESP-IDF environment is installed and configured.
Navigate to the Project Directory:
cd C:\Users\mahdifalouji\Downloads\esp-dev-kits-master\esp-dev-kits-master\examples\esp32-c3-lcdkit\examples\knob_panel
Build the Project:
idf.py build
Flash the Firmware:
idf.py -p COM3 flash
Monitor the Output:
idf.py monitor
Use Ctrl] to exit the monitor.
Usage Instructions
Brightness Adjustment:
Navigate through the menu to select the desired brightness level (25%, 50%, 75%, 100%).
Confirm the brightness change via the LCD interface.
Audio Playback:
Ensure the audio output is functional at all brightness levels.
Test audio consistency while switching between brightness levels.
Debugging:
Use the monitor to capture logs and verify functionality.

Link to videos: https://youtube.com/shorts/kUZfBaw7eGg   https://youtube.com/shorts/5BYDeOocCew?si=t0-2jY9foKie7xZC
