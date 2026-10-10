# ETS2HeadTracker 1.0
> *Lightweight Head-Tracking for Euro Truck Simulator 2 via Mobile Browser Offloading*

---

## Overview
ETS2HeadTracker 1.0 is an open-source head-tracking utility designed specifically for Euro Truck Simulator 2 (ETS2). It bridges mobile web technologies with the game’s telemetry architecture to deliver smooth, responsive in-cabin camera rotation. The project addresses hardware performance bottlenecks where lower-spec host PCs struggle to run conventional computer vision and head-tracking suites simultaneously with modern game titles. By offloading intensive computer vision and head-orientation calculations directly to an Android device via Google Chrome, this tool eliminates host computational overhead entirely.

The system consists of two primary layers: a native C++ telemetry integration and a lightweight browser client. On the PC side, the tracker functions as a dynamic library plugin (`ETS2_HeadTracker.dll`) hooked directly into SCS Software’s native Telemetry SDK. It hosts a local server instance bound to port 47991, communicating tracking packets without competing for desktop CPU and GPU threads.

On the client side, the Android phone runs a minimalist, performance-optimized web UI rendered inside Google Chrome. The mobile browser captures facial landmarks, computes real-time head rotation coordinates, and transmits camera transform packets back over the local network via secure protocols.

---

## Setup
1. Copy `ETS2_HeadTracker.dll` into the plugins directory located within your ETS2 installation path (`bin/win_x64/plugins`).
2. Start ETS2 and enable head tracking ingame.
3. Identify your PC’s IPv4 address using Command Prompt (`ipconfig` command).
4. Open Google Chrome on your Android phone and navigate to `https://<PC_IPv4_Address>:47991`. Both devices must reside on the same Wi-Fi subnet or connect via mobile portable hotspot.
5. To overcome Chrome’s restrictions on camera usage from insecure origins, enable a Chrome experimental flag via: `chrome://flags/#unsafely-treat-insecure-origin-as-secure`. Add your IP with port 47991 here (example: `http://192.168.1.2:47991`) and relaunch Chrome on the smartphone.
6. Initialize the tracking interface directly from the browser window.

---

## Requirements
- **Android OS** with Google Chrome installed.
- **OpenTrack:** Ensure OpenTrack is installed on the host PC. Inside the application, navigate to the Output settings, set the protocol to **facetrack 2.0 Enhanced**, and select **Both** from the dropdown menu (next to the Output settings).
