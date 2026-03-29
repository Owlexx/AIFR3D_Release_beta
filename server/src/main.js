const express = require("express");
const cors = require("cors");
const path = require("path");
const http = require("http");
const { WebSocketServer } = require("ws");

// Import our custom modules for 2.2.4
const { PORT, STORAGE_DIR } = require("./config/constants");
const { ensureDirectoryExists } = require("./utils/helpers");

/**
 * MAIN_SERVER (2.2.4): This is the "brain" of the backend for the 2.2.4 version.
 * It's the central file that starts the server and connects all the different parts.
 */

const app = express();
const server = http.createServer(app);

// Initialize WebSocket server for real-time features
const wss = new WebSocketServer({ noServer: true });

/**
 * MIDDLEWARE: This section sets up general "helper" code that runs for every request.
 * - CORS: Allows your website or VST to talk to the server even if they are on different addresses.
 * - JSON Parser: Lets the server understand data sent in "JSON" format.
 * - Static Files: Lets the server share images or other files from a specific folder.
 */
app.use(cors());
app.use(express.json());
app.use("/assets", express.static(path.join(__dirname, "../../assets")));

/**
 * WEBSOCKET_UPGRADE: Handles the transition from HTTP to WebSocket for real-time DSP data.
 * This is the "express lane" for live audio metrics from the VST.
 */
server.on('upgrade', (request, socket, head) => {
  const pathname = new URL(request.url, `http://${request.headers.host}`).pathname;

  if (pathname === '/ws/vst') {
    wss.handleUpgrade(request, socket, head, (ws) => {
      wss.emit('connection', ws, request);
    });
  } else {
    socket.destroy();
  }
});

/**
 * WEBSOCKET_CONNECTION: Manages live connections from the VST.
 * It receives real-time DSP data and makes it available for the Aifred Brain.
 */
wss.on('connection', (ws) => {
  console.log('VST Connected via WebSocket');

  ws.on('message', (message) => {
    try {
      const data = JSON.parse(message);
      if (data.type === 'DSP_METRICS') {
        // This is where live DSP data from the VST enters the backend.
        // It can be used to provide context-aware AI feedback.
        app.set('latest_dsp_metrics', data.payload);
      }
    } catch (e) {
      console.error('Error parsing VST message:', e);
    }
  });

  ws.on('close', () => {
    console.log('VST Disconnected');
  });
});

/**
 * HEALTH_CHECK: A simple doorway to check if the server is alive and running.
 * It's like a "heartbeat" for the system.
 */
app.get("/api/v1/health", (req, res) => {
  res.json({
    status: "online",
    version: "2.2.4-beta",
    uptime: process.uptime(),
    timestamp: new Date().toISOString(),
    websocket: "enabled"
  });
});

/**
 * SERVER_START: This final block starts the server and begins listening for requests.
 * It also ensures that all necessary data folders exist so nothing crashes.
 */
ensureDirectoryExists(STORAGE_DIR);

server.listen(PORT, () => {
  console.log(`========================================`);
  console.log(` AudioSynth VST 2.2.4-beta Backend RUNNING! `);
  console.log(` Port: ${PORT}                          `);
  console.log(` Mode: Cleanly Wired & Canonical       `);
  console.log(` Real-time WebSocket: Active           `);
  console.log(`========================================`);
});

module.exports = { app, server, wss };
