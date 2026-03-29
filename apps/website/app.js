// AIFR3D Website Application
// Handles beat catalog, R2 storage, PayPal checkout, and UI interactions

const API_BASE = String(config.apiBase || "https://www.north3rnlight3r.com").replace(/\/+$/, "");
const R2_BUCKET_URL = "https://aifr3d.r2.cloudflarestorage.com";
const LOCAL_SERVER_URL = "http://127.0.0.1:3333";
const PAYPAL_CLIENT_ID = "YOUR_PAYPAL_CLIENT_ID";

// DOM Elements
const catalogToggle = document.getElementById("catalog-toggle");
const catalogWrap = document.getElementById("catalog-wrap");
const catalogList = document.getElementById("catalog-list");
const audioPlayer = document.getElementById("audio-player");
const audioProgress = document.getElementById("audio-progress");
const audioCurrentTime = document.getElementById("audio-current-time");
const audioDuration = document.getElementById("audio-duration");
const audioSpectrum = document.getElementById("audio-spectrum");
const nowTitle = document.getElementById("now-title");
const nowMeta = document.getElementById("now-meta");
const buyTrack = document.getElementById("buy-track");
const prevTrack = document.getElementById("prev-track");
const nextTrack = document.getElementById("next-track");
const toggleTrack = document.getElementById("toggle-track");
const stopTrack = document.getElementById("stop-track");
const paypalModal = document.getElementById("paypal-modal");
const modalClose = document.getElementById("modal-close");
const paypalBeatName = document.getElementById("paypal-beat-name");
const paypalBeatPrice = document.getElementById("paypal-beat-price");
const yearEl = document.getElementById("year");

// State
let beats = [];
let currentBeatIndex = 0;
let isPlaying = false;
let audioContext = null;
let analyser = null;
let animationId = null;

/**
 * Plain English Summary: Initialize the website on page load
 */
async function initializeWebsite() {
  console.log("🚀 Initializing AIFR3D Website");
  
  // Set current year
  yearEl.textContent = new Date().getFullYear();
  
  // Load beats from R2 storage
  await loadBeatsFromR2();
  
  // Setup event listeners
  setupEventListeners();
  
  // Initialize audio context for spectrum analyzer
  initializeAudioContext();
  
  console.log("✅ Website initialized");
}

/**
 * Plain English Summary: Load beats from Cloudflare R2 storage
 */
async function loadBeatsFromR2() {
  try {
    console.log("📥 Loading beats from R2 storage...");
    
    // Try local server first
    const response = await fetch(`${LOCAL_SERVER_URL}/api/r2/beats/list`);
    
    if (!response.ok) {
      throw new Error(`Failed to load beats: ${response.status}`);
    }
    
    const data = await response.json();
    beats = data.beats || [];
    
    console.log(`✅ Loaded ${beats.length} beats from R2`);
    
    // Render catalog
    renderCatalog();
  } catch (error) {
    console.error("❌ Error loading beats:", error);
    
    // Fallback to demo beats
    beats = [
      {
        id: "beat-001",
        name: "Ambient Vibes",
        url: `${R2_BUCKET_URL}/beats/ambient-vibes.mp3`,
        duration: 240,
        bpm: 90,
        price: "$2.99"
      },
      {
        id: "beat-002",
        name: "Hip Hop Flow",
        url: `${R2_BUCKET_URL}/beats/hiphop-flow.mp3`,
        duration: 180,
        bpm: 95,
        price: "$2.99"
      },
      {
        id: "beat-003",
        name: "Lo-Fi Chill",
        url: `${R2_BUCKET_URL}/beats/lofi-chill.mp3`,
        duration: 200,
        bpm: 85,
        price: "$2.99"
      }
    ];
    
    renderCatalog();
  }
}

/**
 * Plain English Summary: Render the beat catalog in the UI
 */
function renderCatalog() {
  catalogList.innerHTML = "";
  
  beats.forEach((beat, index) => {
    const beatElement = document.createElement("div");
    beatElement.className = "catalog-item";
    beatElement.innerHTML = `
      <div class="beat-info">
        <h4>${beat.name}</h4>
        <p class="beat-meta">${beat.bpm} BPM • ${beat.duration}s</p>
      </div>
      <div class="beat-actions">
        <button class="btn ghost" onclick="selectBeat(${index})">Select</button>
      </div>
    `;
    catalogList.appendChild(beatElement);
  });
  
  // Update counter
  document.getElementById("count-beat-catalog").textContent = beats.length;
}

/**
 * Plain English Summary: Select a beat and load it into the player
 */
function selectBeat(index) {
  currentBeatIndex = index;
  const beat = beats[index];
  
  // Update now playing
  nowTitle.textContent = beat.name;
  nowMeta.textContent = `${beat.bpm} BPM • ${beat.duration}s`;
  
  // Load audio
  audioPlayer.src = beat.url;
  audioPlayer.load();
  
  console.log(`🎵 Selected: ${beat.name}`);
}

/**
 * Plain English Summary: Setup all event listeners
 */
function setupEventListeners() {
  // Catalog toggle
  catalogToggle.addEventListener("click", toggleCatalog);
  
  // Player controls
  toggleTrack.addEventListener("click", togglePlayPause);
  stopTrack.addEventListener("click", stopPlayback);
  prevTrack.addEventListener("click", previousBeat);
  nextTrack.addEventListener("click", nextBeat);
  buyTrack.addEventListener("click", openPayPalCheckout);
  
  // Audio player events
  audioPlayer.addEventListener("timeupdate", updateProgress);
  audioPlayer.addEventListener("loadedmetadata", updateDuration);
  audioPlayer.addEventListener("ended", nextBeat);
  audioProgress.addEventListener("input", seekAudio);
  
  // Modal close
  modalClose.addEventListener("click", closePayPalCheckout);
  paypalModal.addEventListener("click", (e) => {
    if (e.target === paypalModal) closePayPalCheckout();
  });
}

/**
 * Plain English Summary: Toggle the catalog visibility
 */
function toggleCatalog() {
  const isHidden = catalogWrap.hidden;
  catalogWrap.hidden = !isHidden;
  catalogToggle.setAttribute("aria-expanded", !isHidden);
  catalogToggle.textContent = isHidden ? "Close Catalog" : "Open Catalog";
}

/**
 * Plain English Summary: Toggle play/pause
 */
function togglePlayPause() {
  if (audioPlayer.src === "") {
    if (beats.length > 0) {
      selectBeat(0);
    } else {
      alert("No beats available");
      return;
    }
  }
  
  if (isPlaying) {
    audioPlayer.pause();
    isPlaying = false;
    toggleTrack.textContent = "Play";
  } else {
    audioPlayer.play();
    isPlaying = true;
    toggleTrack.textContent = "Pause";
    startSpectrum();
  }
}

/**
 * Plain English Summary: Stop playback
 */
function stopPlayback() {
  audioPlayer.pause();
  audioPlayer.currentTime = 0;
  isPlaying = false;
  toggleTrack.textContent = "Play";
  stopSpectrum();
}

/**
 * Plain English Summary: Play previous beat
 */
function previousBeat() {
  currentBeatIndex = (currentBeatIndex - 1 + beats.length) % beats.length;
  selectBeat(currentBeatIndex);
  if (isPlaying) audioPlayer.play();
}

/**
 * Plain English Summary: Play next beat
 */
function nextBeat() {
  currentBeatIndex = (currentBeatIndex + 1) % beats.length;
  selectBeat(currentBeatIndex);
  if (isPlaying) audioPlayer.play();
}

/**
 * Plain English Summary: Update progress bar
 */
function updateProgress() {
  if (audioPlayer.duration) {
    audioProgress.value = (audioPlayer.currentTime / audioPlayer.duration) * 1000;
    audioCurrentTime.textContent = formatTime(audioPlayer.currentTime);
  }
}

/**
 * Plain English Summary: Update duration display
 */
function updateDuration() {
  if (audioPlayer.duration) {
    audioDuration.textContent = formatTime(audioPlayer.duration);
  }
}

/**
 * Plain English Summary: Seek to position in audio
 */
function seekAudio() {
  const seekTime = (audioProgress.value / 1000) * audioPlayer.duration;
  audioPlayer.currentTime = seekTime;
}

/**
 * Plain English Summary: Format time in MM:SS format
 */
function formatTime(seconds) {
  if (!seconds || isNaN(seconds)) return "0:00";
  const mins = Math.floor(seconds / 60);
  const secs = Math.floor(seconds % 60);
  return `${mins}:${secs.toString().padStart(2, "0")}`;
}

/**
 * Plain English Summary: Initialize audio context for spectrum analyzer
 */
function initializeAudioContext() {
  try {
    const AudioContext = window.AudioContext || window.webkitAudioContext;
    audioContext = new AudioContext();
    analyser = audioContext.createAnalyser();
    analyser.fftSize = 256;
    
    // Connect audio player to analyser
    const source = audioContext.createMediaElementAudioSource(audioPlayer);
    source.connect(analyser);
    analyser.connect(audioContext.destination);
    
    console.log("✅ Audio context initialized");
  } catch (error) {
    console.error("❌ Error initializing audio context:", error);
  }
}

/**
 * Plain English Summary: Start drawing spectrum analyzer
 */
function startSpectrum() {
  if (!analyser) return;
  
  const canvas = audioSpectrum;
  const ctx = canvas.getContext("2d");
  const bufferLength = analyser.frequencyBinCount;
  const dataArray = new Uint8Array(bufferLength);
  
  function draw() {
    animationId = requestAnimationFrame(draw);
    
    analyser.getByteFrequencyData(dataArray);
    
    // Clear canvas
    ctx.fillStyle = "rgba(3, 16, 28, 0.8)";
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    
    // Draw spectrum
    const barWidth = canvas.width / bufferLength;
    let x = 0;
    
    for (let i = 0; i < bufferLength; i++) {
      const barHeight = (dataArray[i] / 255) * canvas.height;
      
      // Gradient color
      const hue = (i / bufferLength) * 120; // Green spectrum
      ctx.fillStyle = `hsl(${hue}, 100%, 50%)`;
      ctx.fillRect(x, canvas.height - barHeight, barWidth, barHeight);
      
      x += barWidth + 1;
    }
  }
  
  draw();
}

/**
 * Plain English Summary: Stop drawing spectrum analyzer
 */
function stopSpectrum() {
  if (animationId) {
    cancelAnimationFrame(animationId);
  }
}

/**
 * Plain English Summary: Open PayPal checkout modal
 */
function openPayPalCheckout() {
  if (beats.length === 0 || currentBeatIndex >= beats.length) {
    alert("Please select a beat first");
    return;
  }
  
  const beat = beats[currentBeatIndex];
  paypalBeatName.textContent = `Beat: ${beat.name}`;
  paypalBeatPrice.textContent = `Price: ${beat.price || "$2.99"}`;
  
  paypalModal.hidden = false;
  
  // Load PayPal button
  loadPayPalButton(beat);
}

/**
 * Plain English Summary: Close PayPal checkout modal
 */
function closePayPalCheckout() {
  paypalModal.hidden = true;
}

/**
 * Plain English Summary: Load PayPal button
 */
function loadPayPalButton(beat) {
  const container = document.getElementById("paypal-button-container");
  container.innerHTML = "";
  
  // Create PayPal button
  const button = document.createElement("button");
  button.className = "btn";
  button.textContent = `Pay ${beat.price || "$2.99"} with PayPal`;
  button.onclick = () => {
    // In production, integrate with PayPal SDK
    alert(`Processing payment for: ${beat.name}\nPrice: ${beat.price || "$2.99"}\n\nRedirecting to PayPal...`);
    // window.location.href = `https://www.paypal.com/cgi-bin/webscr?cmd=_xclick&business=YOUR_EMAIL&item_name=${beat.name}&amount=${beat.price}`;
  };
  
  container.appendChild(button);
}

/**
 * Plain English Summary: Initialize on page load
 */
document.addEventListener("DOMContentLoaded", initializeWebsite);

// Export for testing
if (typeof module !== "undefined" && module.exports) {
  module.exports = {
    loadBeatsFromR2,
    selectBeat,
    togglePlayPause,
    openPayPalCheckout
  };
}
