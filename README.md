<img src="docs/metek_logo.png" width="300"/><br><br>

<span style="font-family:Helvetica; font-size:30pt;">Door Control System</span>
---

- [Introduction](#introduction)
  - [How It Works](#how-it-works)
  - [Summary of System Behavior](#summary-of-system-behavior)
    - [Main System States](#main-system-states)
    - [Common Events](#common-events)
    - [What Happens in Case of Errors?](#what-happens-in-case-of-errors)
  - [User Tips](#user-tips)
- [Command Line Interface (CLI)](#command-line-interface-cli)
  - [Installation and Setup](#installation-and-setup)
    - [Step 1: Install Arduino IDE](#step-1-install-arduino-ide)
    - [Step 2: Connect Your Device](#step-2-connect-your-device)
    - [Step 3: Open the Serial Monitor](#step-3-open-the-serial-monitor)
  - [Using the Command Line Interface (CLI)](#using-the-command-line-interface-cli)
    - [1. **info** — Get Software Information](#1-info--get-software-information)
    - [2. **log** — Set Log Level](#2-log--set-log-level)
    - [3. **timer** — Set the Timer](#3-timer--set-the-timer)
    - [4. **dbc** — Set Debounce Time](#4-dbc--set-debounce-time)
    - [5. **inputs** — Get Input State](#5-inputs--get-input-state)
    - [6. **help** — Show Help](#6-help--show-help)
    - [Common Errors](#common-errors)


# Introduction

This Door Control System is designed to control the state of two doors in a way that ensures security and proper operation. The diagram below outlines how the system moves between different states depending on user actions or events related to the doors.

## How It Works

The system goes through different states based on the status of the doors (whether they are locked, unlocked, open, or closed). These states are triggered by specific events, such as a door being unlocked or an error occurring.

## Summary of System Behavior

1. **Idle** - The system starts by waiting for action. This is indicated by both door leds being white.
2. **Unlock** - The system moves to an "unlocked" state when a door is unlocked. This is indicated by the door led blink in red (for the closed door) and green (for the open door).
3. **Open** - Once unlocked, if the door is opened, the system moves to an "open" state. This is indicated by the door led blink in red (for the closed door) and green (for the open door).
4. **Fault** - If something goes wrong (like both doors opening simultaneously), the system moves to a "fault" state. This is indicated by both door leds blinking in magenta.

### Main System States

![](docs/state_diagram.jpeg "State Diagram")

1. **INIT (Initialization State)**  
   The system starts here when it is first turned on. It checks that both doors are initially closed before moving to the next state.

2. **IDLE (Waiting State)**  
   Once the doors are closed, the system moves into IDLE, where it waits for further events. From here, the system can respond to several actions, such as unlocking or opening a door.

3. **DOOR_1_UNLOCKED**  
   When Door 1 is unlocked, the system transitions to this state. It waits for Door 1 to either open or relock.

4. **DOOR_1_OPEN**  
   If Door 1 is unlocked and then opened, the system enters this state. Once the door closes, it will return to the IDLE state. If the door remains open for too long, the system will move to a FAULT state.

5. **DOOR_2_UNLOCKED**  
   Similar to Door 1, when Door 2 is unlocked, the system moves to this state.

6. **DOOR_2_OPEN**  
   When Door 2 is unlocked and opened, the system transitions here. It will wait for the door to close before returning to the IDLE state. If the door remains open for too long, the system will move to a FAULT state.

7. **FAULT (Error State)**  
   If there is an issue, such as both doors being open at the same time, the system moves to the FAULT state. From here, it waits until the issue is resolved (i.e., both doors are closed) before returning to IDLE.

### Common Events

- **EVENT_DOOR_1_UNLOCK / EVENT_DOOR_2_UNLOCK:** These events trigger when either Door 1 or Door 2 is unlocked.
- **EVENT_DOOR_1_OPEN / EVENT_DOOR_2_OPEN:** These events occur when a door is opened.
- **EVENT_DOOR_1_CLOSE / EVENT_DOOR_2_CLOSE:** These events occur when a door is closed.
- **EVENT_DOOR_1_UNLOCK_TIMEOUT / EVENT_DOOR_2_UNLOCK_TIMEOUT:** The system moves to a timeout state if a door is left unlocked for too long without being opened.

### What Happens in Case of Errors?

If both doors are open at the same time, or if there’s a problem closing the doors, the system will enter the FAULT state. This means there is a potential security issue or malfunction that needs to be resolved. The system will remain in the FAULT state until both doors are properly closed.

## User Tips

- Ensure doors are closed properly to prevent the system from entering the FAULT state.
- The system automatically moves back to IDLE after normal door operations (unlocking and opening).
- In case of issues (FAULT state), check that both doors are closed and wait for the system to return to IDLE.


# Command Line Interface (CLI)

This manual will guide you through installing the required software and using the command line interface (CLI) to control different features of the system. No prior knowledge of computers or IT is required.

---

## Installation and Setup

### Step 1: Install Arduino IDE

To operate the CLI, you'll need to install the **Arduino IDE**. This software allows you to communicate with the system using a **Serial Monitor**.

Follow these steps to install Arduino IDE:

1. **Download the Arduino IDE:**
   - Visit the official [Arduino website](https://www.arduino.cc/en/software) and download the version for your operating system (Windows, macOS, or Linux).

2. **Install the Arduino IDE:**
   - Once the download is complete, follow the installation steps for your operating system:
     - On **Windows**: Double-click the installer file (`.exe`) and follow the on-screen instructions.
     - On **macOS**: Open the downloaded `.dmg` file and drag the Arduino icon to the Applications folder.
     - On **Linux**: Extract the downloaded archive and follow the installation instructions provided on the Arduino website.

3. **Launch Arduino IDE:**
   - After installation, open the **Arduino IDE** by double-clicking the icon.

---

### Step 2: Connect Your Device

1. **Connect Your Device to the Computer:**
   - Use a USB cable to connect your hardware (e.g., Arduino board) to your computer. The hardware is where the CLI will run.

2. **Select the Board:**
   - In the Arduino IDE, go to **Tools > Board** and select the correct board type (e.g., Arduino Uno, Nano, etc.).

3. **Select the Port:**
   - Go to **Tools > Port** and select the port to which your device is connected.

---

### Step 3: Open the Serial Monitor

Once the Arduino IDE is set up, the **Serial Monitor** will allow you to interact with the CLI. Follow these steps to open it:

1. **Open the Serial Monitor:**
   - In the Arduino IDE, click on the **magnifying glass** icon in the top right or navigate to **Tools > Serial Monitor**.

2. **Set the Baud Rate:**
   - In the Serial Monitor, set the **baud rate** to the correct value. This is typically **115200** but check your system settings if unsure.

3. **Select "Newline" Option:**
   - In the Serial Monitor, set the dropdown option to **Newline** (this ensures that commands are sent correctly).

4. **Now you're ready to use the CLI!**
   - You can type commands into the Serial Monitor and see the system's responses.

---

## Using the Command Line Interface (CLI)

The following commands are available. Simply type the command into the Serial Monitor and press **Enter** to execute it.

### 1. **info** — Get Software Information
Use this command to see information about the software.
- **Command:** `info`

**Example:**
```
info
```

### 2. **log** — Set Log Level
Use this command to set the "log level," which controls the amount of information recorded by the software. The log level can be set to a number between 0 and 6.
- **Command:** `log <level>`

Where `<level>` can be:
- 0: No logs
- 1: Error messages only
- 2: Warnings and errors
- 3: Informational messages, warnings, and errors
- 4-6: Increasingly detailed logs

**Example:**
```
log 3
```

### 3. **timer** — Set the Timer
This command allows you to set different timers that control how the software behaves in certain situations.
- **Command:** `timer -u <unlock timeout> -o <open timeout> -b <blink interval>`

Where:
- `<unlock timeout>` is the time (in seconds) before a door unlocks.
- `<open timeout>` is the time (in minutes) before a door stays open.
- `<blink interval>` is the time (in milliseconds) between LED blinks.

**Example:**
```
timer -u 5 -o 2 -b 500
```

### 4. **dbc** — Set Debounce Time
This command sets the "debounce" time for inputs (such as buttons). Debounce time ensures that accidental multiple presses are ignored.
- **Command:** `dbc -i <input index> -t <debounce time>`

Where:
- `<input index>` is the number of the input (from 0 to 3).
- `<debounce time>` is the time (in milliseconds) for which the input should be stable.

**Example:**
```
dbc -i 1 -t 100
```

### 5. **inputs** — Get Input State
This command shows the current state of all inputs (like buttons or switches).
- **Command:** `inputs`

**Example:**
```
inputs
```

### 6. **help** — Show Help
If you need to see all the available commands and what they do, use this command.
- **Command:** `help`

**Example:**
```
help
```

---

### Common Errors
If you enter a command incorrectly, the system will display an error message. Double-check your spelling and make sure you include all the necessary arguments (e.g., numbers or letters that go with the command).