Raspberry Pi Image Processing HAT
A custom Raspberry Pi 4 Model B HAT designed for image capture, processing, and display using an ArduCAM OV2640 camera, ST7735 TFT display, and an LM2596S-3.3 based switching power supply.
<p align="center">
  <img src="images/hat_project_overview.jpg" alt="Raspberry Pi Image Processing HAT" width="850">
</p>

Project Overview
The aim of this project is to develop a compact embedded vision platform that integrates image acquisition, processing, visualization, and power management on a custom Raspberry Pi HAT PCB.
The system is built around three main hardware blocks:
- Image Capture: ArduCAM Mini OV2640
- Display: ST7735 TFT
- Power Management: LM2596S-3.3 based SMPS
The Raspberry Pi 4 Model B acts as the main processing and control unit.
System Architecture
The camera communicates with the Raspberry Pi through:
- I²C for sensor configuration
- SPI for image data transfer
The ST7735 TFT display also uses SPI for command and pixel data transfer, with additional GPIO pins for display control and reset.
ArduCAM OV2640
      │
      ├── I²C → Sensor Configuration
      └── SPI → Image Data Transfer
      │
      ▼
 Raspberry Pi 4
      │
      ├── JPEG Data Acquisition
      ├── Buffer Processing
      ├── JPEG Decoding
      └── RGB565 Conversion
      │
      ▼
 ST7735 TFT Display
Hardware Design
The custom HAT was designed in KiCad EDA and includes:
- Raspberry Pi 40-pin GPIO interface
- ArduCAM OV2640 connector
- ST7735 TFT connector
- LM2596S-3.3 switching regulator
- SPI and I²C signal routing
- Dedicated power distribution
- Two-layer PCB layout with a ground plane
The PCB was manufactured and assembled with both SMD and THT components before being tested on the Raspberry Pi platform.
Software
The software side includes C/C++ implementations for:
- ArduCAM OV2640 configuration
- SPI image acquisition
- I²C communication
- JPEG handling and decoding
- RGB565 conversion
- ST7735 display control
- Hardware and module test routines
Build & Run
The project includes a Makefile for compiling the camera, display, and hardware test applications on Raspberry Pi.
Build Requirements
The build configuration uses:
- gcc for C sources
- g++ with GNU++20 for the camera-to-display application
- /usr/local/include for additional headers
- /usr/local/lib for external libraries
- rpidisplaygl
- lgpio
- librt
Makefile Commands
Running make without a target displays the available build commands:
make
Clean generated object files, executables, and captured image files:
make clean
Build the ArduChip SPI test:
make test_arduchip
Build the OV2640 initialization / HAL test:
make test_hal
Build the JPEG image capture application:
make capture_app
Build the JPEG-to-RGB565 conversion test:
make decode_test
Build the complete camera-to-TFT pipeline:
make capture_screen
Run Applications
Hardware-access applications are executed with elevated privileges through the Makefile:
make run_test_arduchip
make run_test_hal
make run_capture_app
make run_decode_test
make run_capture_screen
The full capture_screen target combines the OV2640 camera pipeline with the TFT display application.
Testing
The project was tested in multiple stages:
- Independent camera tests
- Independent TFT display tests
- SPI and I²C communication tests
- JPEG image capture and storage
- RGB565 display tests
- PCB electrical and functional tests
- Final hardware/software integration
The completed system successfully captured image data from the camera, processed it on the Raspberry Pi, and displayed the output on the TFT screen.
Technologies & Tools
Raspberry Pi 4 C C++ SPI I²C GPIO KiCad PCB Design ArduCAM OV2640 ST7735 LM2596S-3.3
Documentation
Project reports and technical documents are available in the [`rapor`](rapor/) directory.
Project Context
Developed as part of the MÜHTAS-1 Engineering Design Project in the Department of Electronics and Communication Engineering at Kocaeli University.
Author
Sena Sümeyye Durarslan
Electronics and Communication Engineering
Kocaeli University
