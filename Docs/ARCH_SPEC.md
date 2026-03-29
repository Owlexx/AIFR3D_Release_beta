# AudioSuite System - Developer Architecture Specification
**Version**: 2.5.0-beta (Arch/Garuda Optimized)
**Author**: Manus AI

## **1. Introduction**
This document details the architectural design of the AudioSuite Production Suite, a comprehensive system comprising a VST plugin, a standalone vocal recorder, an Android administration application, and a unified backend. The architecture emphasizes modularity, real-time performance, security, and a clear separation of concerns, all while providing a professional-grade user experience.

## **2. System Overview**
The AudioSuite system is structured as a "Translation Layer" that processes raw audio physics into perceptually meaningful information, enabling AI-driven feedback and creative control. It consists of the following interconnected components:

*   **CoreSynth VST/Standalone**: The client-side audio processing and user interface, built with JUCE.
*   **CoreSynth Backend**: A Node.js server providing API services and WebSocket communication.
*   **AudioSuite Admin App**: An Android application for remote administration and content management.

### **2.1. Component Diagram**

```mermaid
graph TD
    subgraph User Interface
        VST[AudioSuite VST Plugin]
        Standalone[AudioSuite Standalone Vocal Recorder]
        AdminApp[AudioSuite Android Admin App]
    end

    subgraph Backend Services
        Backend[CoreSynth Backend (Node.js)]
        WebSocket[WebSocket Gateway]
        APIRoutes[API Routes]
        ChatService[Chat Service (AI Brain)]
        CatalogService[Catalog Service]
        PaymentService[Payment Service]
    end

    subgraph Data & Storage
        DSPData[Real-time DSP Data]
        UserData[User Profiles & Settings]
        BeatCatalog[Beat Catalog & Assets]
        Logs[System Logs]
    end

    VST -- DSP Metrics --> WebSocket
    Standalone -- DSP Metrics --> WebSocket
    WebSocket -- Real-time Feedback --> VST
    WebSocket -- Real-time Feedback --> Standalone

    VST -- API Requests --> APIRoutes
    Standalone -- API Requests --> APIRoutes
    AdminApp -- API Requests --> APIRoutes

    APIRoutes -- Data Access --> ChatService
    APIRoutes -- Data Access --> CatalogService
    APIRoutes -- Data Access --> PaymentService

    ChatService -- AI Processing --> DSPData
    ChatService -- AI Processing --> UserData
    CatalogService -- Asset Management --> BeatCatalog
    PaymentService -- Transaction Processing --> UserData

    Backend -- Logging --> Logs
```

## **3. CoreSynth VST/Standalone Architecture**

### **3.1. Dual-Mode UI**
The VST features a dynamic UI that can switch between two distinct modes:
*   **Legacy Mode (2.2.4)**: The original interface, primarily for diagnostic and exploratory use.
*   **Production Mode (2.2.5)**: The advanced interface with dual-halo compare mode, candlestick meters, and enhanced visual feedback.

This is managed by a `versionToggle` in the `TopBar` and handled by the `CoreSynthAudioProcessorEditor` which dynamically instantiates and manages the visibility of UI components (e.g., `MainView` vs. `MainView_v25`).

### **3.2. DSP Pipeline**
*   **`CoreSynthAudioProcessor`**: The central audio processing unit. It handles input/output, manages DSP algorithms, and pushes `AnalysisFrame` data to a FIFO for UI and WebSocket consumption.
*   **`Source/DSP/` & `Source/DSP_v25/`**: Contains the core DSP algorithms. These are isolated to ensure bit-perfect matching between the 2.2.4 and 2.2.5 modes. Key components include `LoudnessEBUR128` and `FeatureExtractor`.

### **3.3. Real-time DSP Feedback**
*   **WebSocket Client**: Integrated into `CoreSynthAudioProcessorEditor` and `StandaloneVocalRecorder`, establishing a real-time WebSocket connection to the backend (`/ws/vst`).
*   **DSP Metric Streaming**: `AnalysisFrame` data, containing live DSP metrics, is serialized and streamed to the backend via WebSocket, providing context for the AI Brain.

### **3.4. Theme Customization**
*   **`Theme` Class**: A singleton class managing color palettes and layout preferences. It acts as a `ChangeBroadcaster`.
*   **`ThemeSettingsComponent`**: A dedicated UI panel for users to customize colors and rearrange elements.
*   **`CoreSynthAudioProcessorEditor`**: Acts as a `ChangeListener` to the `Theme` class, dynamically updating UI elements upon theme changes.

## **4. CoreSynth Backend Architecture**

### **4.1. Modular Structure**
The Node.js backend follows a modular, service-oriented architecture:
*   **`main.js`**: The entry point, responsible for initializing the Express app, HTTP server, and WebSocket server. It also handles the WebSocket upgrade mechanism.
*   **`config/constants.js`**: Centralized configuration for ports, storage directories, and API keys.
*   **`middleware/`**: Contains authentication (`auth.js`) and rate-limiting logic.
*   **`routes/`**: Defines API endpoints, categorized by functionality (e.g., `catalogRoutes.js`, `chatRoutes.js`, `paymentRoutes.js`).
*   **`services/`**: Encapsulates business logic for specific domains (e.g., `chatService.js` for AI interaction, `catalogService.js` for beat management).
*   **`utils/`**: Helper functions for common tasks like file I/O and data validation.

### **4.2. WebSocket Gateway**
*   **Endpoint**: `/ws/vst` for real-time communication with VST/Standalone clients.
*   **Data Flow**: Receives `DSP_METRICS` payloads from clients, which are then used by the `chatService` to provide context-aware AI feedback.

### **4.3. Security**
*   **API Token Management**: API tokens are stored securely in backend configuration and never exposed on the frontend.
*   **Rate Limiting**: Implemented in middleware to prevent abuse of API endpoints.
*   **Logging**: Comprehensive logging of API requests and WebSocket messages for auditing and debugging.

## **5. AudioSuite Admin App Architecture (Android)**

### **5.1. Terminal Access**
*   **`TerminalComponent.kt`**: A dedicated UI component providing a command-line interface within the app.
*   **Remote Execution**: Sends commands to the CoreSynth Backend via API requests, allowing remote server administration.

### **5.2. Media Management**
*   **Robust Uploads**: Utilizes backend services for seamless media uploads to the beat catalog, ensuring playback stability during the process.
*   **Playback Routes**: Designed to be independent of local file paths, relying on streaming from the backend.

## **6. Build System & Deployment**

### **6.1. Linux (Arch/Garuda) Installation**
*   **`audiosuite_installer.run`**: A self-extracting executable containing the entire `AudioSuite_System` and an `install_logic.sh` script.
*   **`install_logic.sh`**: Handles dependency installation via `pacman`, VST3 placement (`~/.vst3`), and desktop shortcut creation (`.desktop` file).
*   **Obfuscated Install Path**: Installs the suite into `$HOME/.AudioSuite_Production_Suite` to prevent conflicts and maintain a clean user home directory.

### **6.2. Windows Installation**
*   **`AIFR3D_Installer.ps1`**: A PowerShell script (placeholder for this build) to handle Windows-specific installations.

## **7. Future Considerations**
*   **Cross-Platform Build Automation**: Implement CI/CD pipelines for automated builds across Linux, Windows, and Android.
*   **Advanced AI Integration**: Explore more sophisticated AI models for enhanced vocal processing and mixing assistance.
*   **Cloud Deployment**: Optimize backend for cloud-native deployment (e.g., Docker, Kubernetes).

**This architecture provides a robust, scalable, and maintainable foundation for the AudioSuite Production Suite.**
