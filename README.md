# MoveW - Window Mover Utility

MoveW is a command-line utility for Windows designed to find the first visible window belonging to a specified process and move it to the desired screen coordinates.

## Usage

The program is run from the command line with the following syntax:

```cmd
movew -x<coord> -y<coord> -p<process_name> [-u]
```

**Parameters:**

*   `-x<coord>`: (Required) The target X coordinate for the top-left corner of the window.
*   `-y<coord>`: (Required) The target Y coordinate for the top-left corner of the window.
*   `-p<process_name>`: (Required) The name of the executable file of the target process (e.g., `notepad.exe`, `chrome.exe`).
*   `-u`: (Optional) If specified, the program will wait indefinitely for a window belonging to the target process to appear before attempting to move it. If omitted, the program will only check for currently existing windows and exit if none are found.


**Examples:**

1.  Move the first found Notepad window to coordinates (1024, 0):
    ```cmd
    movew -x1024 -y0 -pnotepad.exe
    ```
2.  Wait for a Notepad window to appear and then move it to coordinates (0, 0):
    ```cmd
    movew -x0 -y0 -pnotepad.exe -u
    ```

## Building from Source

To build `movew.exe` from the provided `main.cpp` source code:

1.  **Prerequisites:**
    *   Microsoft Visual Studio (with the C++ toolchain installed). The `build.bat` script specifically calls `vcvars32.bat` from Visual Studio 2022 Community Edition, but may work with other versions.
    *   (Optional) `mpress` executable packer (used in `build.bat` for compression).

2.  **Build Steps:**
    *   Open a Developer Command Prompt for Visual Studio (or ensure `cl.exe` and `rc.exe` are in your PATH).
    *   Navigate to the directory containing `main.cpp`, `version.rc`, and `build.bat`.
    *   Run the build script:
        ```cmd
        build.bat
        ```
    *   This will compile the resource script, compile the C++ code and link them.

## Notes

*   The program's console output messages are in Portuguese.
*   The program attempts to preserve the original size of the window when moving it.
*   Copyright (C) 2018 MAPENO.pt
