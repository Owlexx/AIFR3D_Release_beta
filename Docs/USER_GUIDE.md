# AudioSuite System - User Guide
**Version**: 2.5.0-beta (Arch/Garuda Optimized)
**Author**: Manus AI

## **1. Welcome to AudioSuite**
Welcome to the AudioSuite Production Suite, a powerful collection of tools designed to bridge the gap between the technicalities of audio engineering and the intuitive flow of musical creation. This guide will help you navigate the core components: the AudioSuite VST, the Standalone Vocal Recorder, and the AudioSuite Admin App.

## **2. AudioSuite VST: Mixing and Analysis Plugin**

The AudioSuite VST is a dual-mode plugin offering both a legacy "Discovery" interface and a modern "Production" interface, complete with advanced metering and AI-driven feedback.

### **2.1. Installation (Arch/Garuda Linux)**
1.  **Download**: Obtain the `audiosuite_installer.run` executable from the GitHub release.
2.  **Execute**: Open your terminal, navigate to the download directory, and run:
    ```bash
    chmod +x audiosuite_installer.run
    ./audiosuite_installer.run
    ```
3.  **Follow Prompts**: The installer will guide you through dependency installation (using `pacman`) and VST3 placement.
4.  **Audio Group**: Ensure your user is part of the `audio` group for low-latency performance. If not, run `sudo usermod -a -G audio $USER` and reboot.

### **2.2. Interface Overview**
*   **TopBar**: Contains global controls, including the version toggle and theme settings.
*   **Main View**: The primary display area for the Halo meters and other visual feedback.

### **2.3. Dual-Mode Operation**
*   **Version Toggle**: Located in the `TopBar`, this switch allows you to instantly switch between:
    *   **Legacy Mode (2.2.4)**: The original "Discovery" interface, ideal for initial sound exploration.
    *   **Production Mode (2.2.5)**: The advanced interface with dual-halo compare mode and candlestick session tracking.

### **2.4. Meter Interpretation**

#### **A. Approval Halo Meters**
These circular meters provide a holistic view of your mix's characteristics.

| Metric | Scale | Description | Plain English Summary |
| :--- | :--- | :--- | :--- |
| **Tone** | **20Hz - 20kHz** | Frequency balance across the audible spectrum. | How bright or dark your mix sounds. |
| **Loudness** | **-60dBFS - 0dBFS** | Overall perceived volume of your audio. | How loud your mix is without clipping. |
| **Stereo** | **-1 (Mono) - +1 (Wide)** | Correlation between left and right channels. | How wide or narrow your mix sounds. |

#### **B. Candlestick Meters (Production Mode Only)**
These meters track the dynamic evolution of your mix over time.

| Metric | Scale | Description | Plain English Summary |
| :--- | :--- | :--- | :--- |
| **Punch** | **Dynamic Range (dB)** | The difference between the loudest and quietest parts. | How much "impact" or "movement" your mix has. |
| **Width** | **Stereo Correlation** | Consistency of stereo image over time. | How stable your stereo image is. |

### **2.5. Compare Mode (Production Mode Only)**
Activate the "Compare Mode" toggle in the `TopBar` to display two identical Halo meters side-by-side. This allows for direct A/B comparison of two different mixes or two different stages of your current mix.

### **2.6. Theme Customization**
Click the "Theme Settings" button in the `TopBar` to open a panel where you can:
*   **Color Palette**: Adjust the colors of the meters, background, and other UI elements.
*   **Layout**: Rearrange window panels to create your preferred workflow.

## **3. AudioSuite Standalone Vocal Recorder**

This application is specifically designed for professional vocal recording, leveraging AI intelligence for real-time processing.

### **3.1. Installation (Arch/Garuda Linux)**
Follow the same installation steps as the VST. The standalone application will be installed alongside the VST.

### **3.2. Key Features**
*   **AI Auto-Tune**: Automatically corrects pitch in real-time based on the context of your recording.
*   **Context-Aware FX**: Applies subtle effects (e.g., compression, reverb) that are automatically appreciated by the AI to enhance your vocal performance.
*   **Professional GUI**: Smooth, accurate readings with customizable meters.

## **4. AudioSuite Admin App (Android)**

Manage your AudioSuite backend directly from your Android device.

### **4.1. Installation**
1.  **Download**: Obtain the `audiosuite_admin.apk` from the GitHub release.
2.  **Install**: Transfer the APK to your Android device and install it.

### **4.2. Terminal Access**
*   **Remote Control**: Execute commands on your AudioSuite backend server directly from your phone, similar to Termux.
*   **Catalog Management**: Upload new beats, manage existing ones, and monitor server status.

### **4.3. Media Management**
*   **Seamless Uploads**: Upload new audio files to your beat catalog without interrupting playback.
*   **Playback Stability**: The app ensures that playback routes are robust and independent of local files.

## **5. Troubleshooting & Support**
*   **Installation Issues**: Refer to the `BUILD_CHECKLIST.md` and `ARCH_SPEC.md` for detailed technical guidance.
*   **Audio Problems**: Ensure your audio interface is correctly configured and your user is in the `audio` group.
*   **Backend Connectivity**: Verify your server is running and accessible via the configured port.

**Thank you for choosing AudioSuite. We hope this guide enhances your creative workflow!**
