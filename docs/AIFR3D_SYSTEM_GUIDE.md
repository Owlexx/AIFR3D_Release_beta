# AIFR3D System Guide & Tutorials

## **1. AIFR3D VST Plugin (Mix & Master)**
The AIFR3D VST is a dual-mode analysis engine designed to translate audio physics into human perception.

### **How to Use the VST**
- **Mode Toggle**: Use the button in the **TopBar** to switch between **V2.2.4 (Discovery)** and **V2.2.5 (Production)**.
  - *2.2.4 Mode*: Focuses on the diagnostic Halo for broad tonal analysis.
  - *2.2.5 Mode*: Features the high-precision Candlestick session tracking.
- **Compare Mode**: Enable the **Compare** toggle to see two side-by-side Halos. This allows you to compare **Mix A** (Reference) against **Mix B** (Your Mix) in real-time.
- **Reading the Meters**:
  - **Tone (Hz)**: The Halo's radius represents frequency balance from 20Hz to 20kHz.
  - **Punch (DR)**: The vertical Candlestick height shows Dynamic Range.
  - **Width (Correlation)**: The Halo's glow intensity indicates stereo correlation (Phase).
  - **Loudness (dBFS)**: The outer ring represents integrated loudness.

---

## **2. Standalone Vocal Recorder (Recording Suite)**
A specialized application designed specifically for vocal tracking with AI-driven processing.

### **Tutorial: Recording Your First Vocal**
1. **Setup**: Select your microphone in the **Audio Settings**.
2. **AI Processing**: Enable the **AI Vocal Processor**. It automatically analyzes your voice's context and applies:
   - **Auto-Tune**: Gentle pitch correction based on detected key.
   - **Context FX**: Automatic EQ and compression tailored to your vocal tone.
3. **Recording**: Press **Record** to capture your performance. The standalone app saves high-quality WAV files directly to your session folder.
4. **Theme**: Customize the interface colors in the **Theme Settings** to match your studio's vibe.

---

## **3. Android Admin App (Remote Control)**
A powerful mobile tool for managing your music business and server from anywhere.

### **Tutorial: Remote Management**
- **Terminal Access**: The **Command** tab features a professional terminal. Type commands (like `ls`, `curl`, or custom scripts) and press **Enter** to execute them on your server.
- **Beat Catalog**:
  - **Open/Close**: Use the **Open Catalog** button to view your tracks.
  - **Upload**: In the **Upload** tab, select a file from your phone to add it to your website's catalog. Playback is protected and will not break during the upload process.
- **Sales & Inquiries**: Monitor real-time sales and customer messages directly from the dashboard.

---

## **4. Developer Architecture Map (Plain English)**
- **Backend (`server/src/`)**: The modular brain. It handles AI chat, file uploads, and security.
- **Frontend (`apps/website/`)**: The public face. It streams your catalog securely and blocks unauthorized downloads.
- **Security**: API tokens are stored in the backend to prevent theft. Rate limiting prevents bot abuse.
- **Real-time**: WebSockets provide instant feedback between the VST and the Aifred AI Brain.

---

### **Routing & Build Checklist**
- [ ] **VST**: Ensure `Source/DSP_v25` is included in the build for 2.2.5 mode.
- [ ] **Standalone**: Verify `AIVocalProcessor` is initialized before audio starts.
- [ ] **Website**: Check `app.js` to ensure `withPlaybackCacheBust` is active for smooth streaming.
- [ ] **Admin**: Ensure `MainActivity.kt` is pointed to your production server URL.
