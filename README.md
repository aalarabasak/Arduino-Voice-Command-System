Voice Command System

Voice Command System is a smart automation project designed to control a DC Fan and RGB LED using voice commands via Bluetooth. The system focuses on energy efficiency and user-friendly interaction, utilizing EEPROM for non-volatile memory and Timer Interrupts for precise background timing.

Key Features: Voice Control: Recognizes commands like turn on fan or blue light via the Arduino Bluetooth Voice Control Android app. Smart Memory (EEPROM): The system remembers the last fan speed, direction, and LED color even after a power loss or reset. It resumes exactly where it left off. Energy Efficiency (Auto-Shutdown): Fan: Automatically turns off after a period of inactivity (managed by millis() function). LED: Automatically turns off after 10 seconds using Timer1 interrupts to ensure non-blocking operation. Visual Effects: Includes a Random Lights Mode that cycles through colors automatically. Real-Time Feedback: Displays status updates on a 16x2 LCD screen.

Hardware Components: Microcontroller: Arduino Uno Communication: HC-05 Bluetooth Module Motor Driver: L298N (controls Fan speed & direction) Actuators: DC Fan & RGB LED (Common Anode) Display: 16x2 LCD (Hitachi HD44780 controller)

Pin Configuration: HC-05 TX connected to RX (Pin 0) HC-05 RX connected to TX (Pin 1) Fan ENA (PWM) connected to Pin 3 Fan IN1 / IN2 connected to Pin 13 / 12 RGB Red connected to Pin 4 RGB Green connected to Pin 5 RGB Blue connected to Pin 6 LCD RS / E connected to Pin 8 / 11 LCD Data connected to Pins A2, A3, A4, A5

How to Run:

Hardware: Connect the circuit components as shown in the diagram.
Software: Upload the .ino file to your Arduino Uno.
App: Install Arduino Bluetooth Voice Control on your Android device.
Pairing: Connect to HC-05 via Bluetooth settings (Default pin: 1234 or 0000).
Usage: Open the app and say a command (e.g., turn on fan, red light).
