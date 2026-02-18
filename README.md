# VAX TWEAKER — Free Edition

```
██╗   ██╗ █████╗ ██╗  ██╗
██║   ██║██╔══██╗╚██╗██╔╝
██║   ██║███████║ ╚███╔╝
╚██╗ ██╔╝██╔══██║ ██╔██╗
 ╚████╔╝ ██║  ██║██╔╝ ██╗
  ╚═══╝  ╚═╝  ╚═╝╚═╝  ╚═╝
       T W E A K E R
```

A lightweight Windows system tweaker for FPS optimization, network latency reduction, and system cleanup. Built in C++ with a modern terminal UI.

> **Your system. Your rules. Maximum performance.**

## Features

### 🎮 FPS & Rendering (30+ tweaks)
- **Display & Compositor** — Fullscreen Optimizations, Multi-Plane Overlay, Transparency, DWM tuning, Visual Effects, Window Animations
- **Game Bar & Overlays** — Game Bar/DVR, Snap Layouts, Aero Shake, Game Mode, Edge Swipe
- **GPU & Scheduling** — Hardware-Accelerated GPU Scheduling, System DPI Awareness
- **Input & Mouse** — Mouse Acceleration, Frame Pre-Render Queue, VRR Management, Menu Delay, Input Queue
- **CPU & Power** — Power Throttling, Background Apps, Startup Delay, Kernel in RAM, Hung Tasks, PCIe ASPM, Fast Startup, Timer Distribution
- **Services & Maintenance** — Disable Non-Microsoft Services

### 🌐 Network & Latency
- **NIC Low Level** — Interrupt Moderation, Flow Control, Energy Efficient Ethernet, RSC, LSO
- **TCP/IP Stack** — Nagle Algorithm, System TCP Optimizations
- **Protocols & DNS** — NetBIOS, DNS presets (Cloudflare, Google, Quad9)
- **Reset & Maintenance** — Winsock Reset, TCP/IP Reset, DNS Cache Flush

### 🧹 System Cleaner (33 operations)
- **Windows Cache** — Temp files, Prefetch, Thumbnail cache, Icon cache, Font cache, Windows Update cache, DNS cache
- **System Files** — Error reports, Memory dumps, Delivery Optimization, Windows logs, Old installations
- **Application Cache** — Browser caches (Chrome, Firefox, Edge), Discord, Spotify, Steam, Teams
- **Developer Cache** — npm, pip, NuGet, Unity/Gradle
- **Media & Gaming** — Shader caches (NVIDIA, AMD, DirectX)

### Safety & Transparency
- **System Restore Point** creation before applying tweaks
- **Automatic registry backup** with crash recovery
- **Drift Detection** — alerts when Windows or updates revert your tweaks
- **Risk levels** on every tweak (Safe, Moderate, Advanced, Risky)
- **Confirmation prompts** before every operation

## Requirements

- Windows 10 / Windows 11 (x64)
- Administrator privileges (required for registry and system modifications)
- Visual Studio 2022+ with C++ Desktop workload (for building)

## Build

1. Open `Vax Tweaker Free Version.slnx` in Visual Studio
2. Select **Release | x64**
3. Build the solution (`Ctrl+Shift+B`)

The executable will be in `x64/Release/`.

## Project Structure

```
src/
├── Core/            Application, Admin, SystemProfile, Compatibility, Types
├── Modules/         IModule, BaseModule, FpsModule, NetworkModule, CleanerModule
├── Safety/          SafetyGuard (confirmation flow)
├── System/          Registry, Logger, DriftDetector, RestorePoint, PowerPlanManager, ProcessUtils
└── UI/              Console, Renderer, Theme
```

## Premium Edition

Looking for more modules and advanced tweaks? Check out the full version:

🔗 [**VAX TWEAKER Premium**](https://github.com/vaxead/vax-tweaker)

## Community

- 💬 Discord: [discord.gg/myYrWkn7](https://discord.gg/9Wu2EYPxrw)
- 🐛 Issues: [GitHub Issues](https://github.com/vaxead/vax-tweaker-free/issues)

## License

This project is licensed under the [MIT License](LICENSE).
