# Ping Pong Machine — Table Tennis Serving Robot

A fully custom table tennis serving robot designed for serve-receive practice. Features dual-axis spin control (pitch and yaw), a printed spindexer, and a Bluetooth mobile app for wireless control. Powered entirely from a single USB-C connection.

## Features

- **Dual-axis spin control** — independent pitch and yaw adjustment for realistic serve variation
- **Printed spindexer** — feeds and spins balls for consistent delivery
- **Bluetooth control** via a custom mobile app (Expo/React Native) for easy wireless operation
- **USB-C powered** — single cable for full system power
- **Modular electronics design:**
  - USB-C PD power negotiation board
  - Motor and microcontroller breakout board
  - Buck voltage regulation board
- Designed and fabricated with **JLCPCB** for PCB production

## Repository Structure

```
Ping_Pong_Machine/
├── Kicad/               # KiCad PCB projects (power, motor breakout, buck reg)
├── Code/                # Microcontroller firmware and Bluetooth app code
├── CAD/                 # 3D-printable parts (spindexer, frame, wheels)
├── Sims/                # Simulations for board fit and assembly
└── ul_PTPS543021DRLR/   # Custom KiCad footprint
```

## Electronics

| Board | Function |
|-------|----------|
| USB-C PD board | Negotiates input voltage from USB-C PD source |
| Motor breakout | Drives launch and spin motors |
| MCU breakout | Hosts microcontroller and Bluetooth module |
| Buck regulator | Steps down voltage for logic-level components |

## Mobile App

Controlled via a custom **React Native (Expo)** app that connects over Bluetooth. The app allows users to adjust spin type, speed, and frequency from their phone—no physical controls on the machine required.

## Tools Used

- **KiCad** — PCB schematic and layout
- **JLCPCB** — PCB fabrication and SMT assembly
- **Fusion360 / SolidWorks** — Mechanical CAD (frame and launcher)
- **Expo (React Native)** — Mobile app development

## Author

**Jackson Barber** — [github.com/jacksterb1234](https://github.com/jacksterb1234)
