const express = require("express");
const cors = require("cors");
const multer = require("multer");
const bcrypt = require("bcryptjs");
const path = require("path");
const fs = require("fs");
const crypto = require("crypto");
const http = require("http");
const { spawn } = require("child_process");
const { WebSocketServer } = require("ws");

const ROOT_DIR = path.resolve(__dirname, "../..");
const WEBSITE_DIR = path.join(ROOT_DIR, "apps/website");
const WEBSITE_CATALOG_ASSET_DIR = path.join(WEBSITE_DIR, "assets/audio/catalog");
const FILE_ACCESS_ROOT = ROOT_DIR;
const CATALOG_DIR = "/home/north3rnlight3r/Music/North3rnlighter beats";
const LEGACY_CATALOG_DIR = path.join(ROOT_DIR, "server/data/North3rnlighter beats");
const HOME_DIR = String(process.env.HOME || "/home/north3rnlight3r").trim() || "/home/north3rnlight3r";
const EXTERNAL_CATALOG_DIR_CANDIDATES = Array.from(new Set([
  CATALOG_DIR,
  String(process.env.DAWAI_EXTERNAL_CATALOG_DIR || "").trim(),
  LEGACY_CATALOG_DIR,
  path.join(HOME_DIR, "Music/North3rnlight3r_Beatz"),
  path.join(HOME_DIR, "Music/North3rnlighter beats"),
  "/home/north3rnlight3r/Music/North3rnlight3r_Beatz",
  "/home/north3rnlight3r/Music/North3rnlighter beats"
].filter(Boolean)));
const EXTERNAL_CATALOG_DIRS = resolveExistingCatalogDirs(EXTERNAL_CATALOG_DIR_CANDIDATES);
const EXTERNAL_CATALOG_DIR = EXTERNAL_CATALOG_DIRS[0] || EXTERNAL_CATALOG_DIR_CANDIDATES[0];
const CATALOG_ARTWORK_POOL = [
  "/assets/brand/north3rnlight3r_album_art.png",
  "/assets/brand/album_art_concept.png",
  "/assets/brand/hero_mascot.png"
];
const CATALOG_AUDIO_EXTENSIONS = new Set([".wav", ".mp3", ".flac", ".m4a", ".aiff", ".aif", ".ogg"]);
const SOUNDPACK_DIR = path.join(ROOT_DIR, "media/soundpacks");
const REFERENCE_UPLOAD_ROOT = path.join(ROOT_DIR, "assets/reference_intake/licensed_audio");
const STORAGE_DIR = path.join(ROOT_DIR, "server/storage");
const MEMORY_DIR = path.join(STORAGE_DIR, "memory");
const UPLOAD_DIR = path.join(STORAGE_DIR, "uploads");
const CONTENT_FILE = path.join(STORAGE_DIR, "content.json");
const CATALOG_FILE = path.join(STORAGE_DIR, "catalog.json");
const CATALOG_SEED_FILE = path.join(WEBSITE_DIR, "assets/data/beat_catalog.json");
const SOUNDPACK_FILE = path.join(STORAGE_DIR, "soundpacks.json");
const SESSION_FILE = path.join(STORAGE_DIR, "sessions.json");
const ADMIN_CREDENTIALS_FILE = path.join(STORAGE_DIR, "admin_credentials.json");
const PROMO_CODES_FILE = path.join(STORAGE_DIR, "promo_codes.json");
const PAYMENT_UNLOCK_FILE = path.join(ROOT_DIR, "assets/licensing/payment_unlock_v2_2_4_beta.json");
const OPENAI_CONFIG_FILE = path.join(STORAGE_DIR, "openai_config.json");
const CHAT_SETTINGS_FILE = path.join(STORAGE_DIR, "chat_settings.json");
const INQUIRIES_FILE = path.join(STORAGE_DIR, "inquiries.json");
const SALES_FILE = path.join(STORAGE_DIR, "sales.json");
const RECEIPTS_DIR = path.join(STORAGE_DIR, "receipts");
const EVENT_LOG_FILE = path.join(STORAGE_DIR, "events.log");
const ADMIN_LOG_FILE = path.join(STORAGE_DIR, "ADMINLOG.log");
const BRAIN_CONFIG_FILE = path.join(ROOT_DIR, "assets/aifr3d_brain/brain_config.json");

const PORT = Number(process.env.PORT || 8787);
const ADMIN_USER_ENV = String(process.env.ADMIN_USER || process.env.DAWAI_ADMIN_USER || "").trim();
const ADMIN_PASS_ENV = String(process.env.ADMIN_PASS || process.env.DAWAI_ADMIN_PASS || "").trim();
const ADMIN_PASS_HASH_ENV = String(process.env.ADMIN_PASS_HASH || process.env.DAWAI_ADMIN_PASS_HASH || "").trim();
const API_TOKEN = String(process.env.DAWAI_API_TOKEN || "").trim();
const FALLBACK_RUNTIME_SECRET = crypto.randomBytes(32).toString("hex");
const RECEIPT_SIGNING_SECRET = String(process.env.RECEIPT_SIGNING_SECRET || API_TOKEN || FALLBACK_RUNTIME_SECRET).trim();
const ADMIN_SESSION_SECRET = String(process.env.ADMIN_SESSION_SECRET || API_TOKEN || FALLBACK_RUNTIME_SECRET).trim();
const DOWNLOAD_TOKEN_SECRET = String(process.env.DOWNLOAD_TOKEN_SECRET || ADMIN_SESSION_SECRET).trim();
const PROMO_TOKEN_TTL_SECONDS = Math.max(60, Math.min(3600, Number(process.env.PROMO_TOKEN_TTL_SECONDS || 600) || 600));
const SESSION_TTL_MS = 24 * 60 * 60 * 1000;

const DEFAULT_ONLINE_API = String(process.env.DAWAI_DEFAULT_API_URL || "https://www.north3rnlight3r.com").trim();
const DEFAULT_ONLINE_WS = DEFAULT_ONLINE_API.replace(/^https:\/\//i, "wss://").replace(/^http:\/\//i, "ws://") + "/ws/chat";
const LINUX_DEB_URL = String(process.env.DAWAI_LINUX_DEB_URL || "").trim();
const LINUX_ARCH_URL = String(process.env.DAWAI_LINUX_ARCH_URL || "").trim();

const DEFAULT_OPENAI_PRIMARY_MODEL = "gpt-5.2";
const DEFAULT_OPENAI_MODEL_ALLOWLIST = Object.freeze([DEFAULT_OPENAI_PRIMARY_MODEL]);
const OPENAI_PRIMARY_KEY_FILE = "/home/north3rnlight3r/Documents/API_KEY_INDEX/OpenAI API Key.txt";
const OPENAI_BASE_URL = "https://api.openai.com/v1/responses";
const BETA_FREE_TRIAL_LABEL = "Free to try for a limited time";
const POST_BETA_PRICE_LABEL = "$149.99 after Beta 2.2.4";
const PROMO_CODES_DISABLED_MESSAGE = "Promo codes are disabled for AIFR3D 2.2.4 Beta.";
const BETA_FREE_TRIAL_MESSAGE = "AIFR3D 2.2.4 Beta is free to try for a limited time. Use the installer download instead of checkout.";
const PAYPAL_CLIENT_ID = String(process.env.PAYPAL_CLIENT_ID || "").trim();
const PAYPAL_CLIENT_SECRET = String(process.env.PAYPAL_CLIENT_SECRET || "").trim();
const PAYPAL_ENV = String(process.env.PAYPAL_ENV || "sandbox").trim().toLowerCase() === "live" ? "live" : "sandbox";
const PAYPAL_BUSINESS_EMAIL = String(process.env.PAYPAL_BUSINESS_EMAIL || "north3rnlight3rofficial@outlook.com").trim();
const PAYMENT_MODE = String(process.env.PAYMENT_MODE || "paypal").trim().toLowerCase();
const INQUIRY_TARGET_EMAIL = String(process.env.INQUIRY_TO_EMAIL || "north3rnlight3rofficial@outlook.com").trim();
const PAYPAL_CREDENTIAL_FILE_CANDIDATES = [
  path.join(process.env.HOME || "", "Documents/API_KEY_INDEX/PayPalID.txt"),
  path.join(process.env.HOME || "", "Documents/API_KEY_INDEX/SecretPal.txt"),
  "/home/north3rnlight3r/Documents/API_KEY_INDEX/PayPalID.txt",
  "/home/north3rnlight3r/Documents/API_KEY_INDEX/SecretPal.txt"
].filter(Boolean);
const PROMO_CODES_FILE_ENV = String(process.env.DAWAI_PROMO_CODES_FILE || "").trim();
const LOCAL_PROMO_FILE_CANDIDATES = [
  PROMO_CODES_FILE_ENV,
  path.join(process.env.HOME || "", "Documents/AIFR3D_promo_codes_2.2.4_beta.json"),
  "/home/north3rnlight3r/Documents/AIFR3D_promo_codes_2.2.4_beta.json",
  path.join(process.env.HOME || "", "Documents/API_KEY_INDEX/promo_codes.local.json"),
  "/home/north3rnlight3r/Documents/API_KEY_INDEX/promo_codes.local.json"
].filter(Boolean);
let PAYPAL_FILE_CACHE = null;

const DEFAULT_PROMO_CODES = [];
const DASHBOARD_STATE = {
  page_views: 0,
  api_hits: 0,
  media_streams: 0,
  downloads: 0,
  last_request_at: "",
  recent: []
};

const DEFAULT_AIFR3D_BRAIN = {
  brain_id: "aifr3d_brain_v2_2_4",
  version: "2.2.4",
  identity: {
    name: "AIFR3D",
    mode: "professional_mentor",
    voice: ["direct", "calm", "systems_thinking", "macro_before_micro", "no_fluff"]
  },
  guardrails: {
    online_only: true,
    no_offline_fallback: true,
    guidance_only: false,
    require_save_before_destructive: true,
    no_judgemental_copy: true
  },
  runtime: {
    max_recent_interactions: 40,
    max_prompt_chars: 4000,
    max_tokens: 900
  },
  output_contract: {
    keys: ["summary", "issues", "action_suggestions", "state_update"]
  },
  model: configuredOpenAiPrimaryModel()
};

const CHAT_TRANSPORT_MODES = Object.freeze(["websocket", "http"]);
const CHAT_REASONING_EFFORTS = Object.freeze(["minimal", "low", "medium", "high"]);
const CHAT_VERBOSITY_LEVELS = Object.freeze(["low", "medium", "high"]);
const CHAT_TONE_PRESETS = Object.freeze(["direct", "calm", "technical", "executive", "creative"]);
const CHAT_WEBHOOK_EVENTS = Object.freeze(["chat.completed", "chat.failed"]);
const DEFAULT_CHAT_SETTINGS = Object.freeze({
  transport_mode: "websocket",
  webhook: {
    enabled: false,
    url: "",
    secret: "",
    events: [...CHAT_WEBHOOK_EVENTS]
  },
  context: {
    use_previous_response_id: true,
    memory_window_items: 40,
    summary_items: 6,
    max_prompt_chars: 4000,
    compact_threshold: 12
  },
  prompt: {
    tone: "direct",
    personality_mode: "professional_mentor",
    system_prefix: "",
    system_suffix: ""
  },
  reasoning: {
    enabled: true,
    effort: "low"
  },
  response: {
    verbosity: "low",
    max_output_tokens: 900
  }
});

function resolveExistingCatalogDirs(candidates) {
  const results = [];
  const seen = new Set();
  for (const candidate of candidates || []) {
    const normalized = String(candidate || "").trim();
    if (!normalized || seen.has(normalized)) {
      continue;
    }
    seen.add(normalized);
    try {
      if (fs.existsSync(normalized) && fs.statSync(normalized).isDirectory()) {
        results.push(normalized);
      }
    } catch (_error) {
    }
  }
  return results;
}

const API_LIMITS = {
  updated_at: nowIso(),
  chat: {
    provider: "openai",
    max_prompt_chars: 4000,
    max_tokens: 900,
    memory_window_items: 40,
    retry_models_max: 0
  },
  upload: {
    file_limit_bytes: 536870912
  },
  command: {
    timeout_ms: 15000
  }
};

const AIFR3D_BRAIN = loadAifredBrain();

const DEFAULT_CONTENT = {
  services: [
    {
      title: "Mixing and Mastering Services",
      description:"Dont pay for time, pay for quality. Get your track mixed and mastered TODAY. ",
      price: "$120/project"
    },
    {
      title: "Mix Analysis and Feedback",
      description: "Release your next track with confidence!Achieve clarity on your mix before release. Recieve objective feedback that isolates real issues and gives you actionable steps that directly relate to your mix to polish your mix to ensure release-ready quality.",
      price: "$49.99"
    },
    {
      title: "Beat Sale Catalog",
      description: "Purchase non-exclusive licensing for instrumental tracks in the Beat Catalog. Exclusive licensing is by inquiry only. Remixes are for entertainment only and are not for sale. Instrumental beats are the only catalog items intended for monetization and distribution.",
      price: "$19.99"
    }
  ],
  products: [
    {
      sku: "software_vst3",
      title: "AIFR3D VST Beta 2.2.4",
      description: "Features include 'Reference Aware Metering', 'Fix List' Feedback, 'Live Diagnostic Halo', 'Live Candle' and 'Last 10 Sessions Candle' show your mix signature's deviation live and shows your mix consistency  over time.",
      price: `${BETA_FREE_TRIAL_LABEL} | ${POST_BETA_PRICE_LABEL}`,
      availability_label: BETA_FREE_TRIAL_LABEL,
      future_price_label: POST_BETA_PRICE_LABEL,
      beta_free_trial: true,
      buy_url: ""
    }
  ]
};

const DEFAULT_STORE_PRODUCTS = [
  {
    sku: "software_vst3",
    name: "AIFR3D VST Beta 2.2.4",
    description: "Features include 'Reference Aware Metering', 'Fix List' Feedback, 'Live Diagnostic Halo', and the live/session candle views that show your mix signature deviation and consistency over time.",
    priceUSD: "0.00",
    currency: "USD",
    download_url: "/downloads/install-vst.sh",
    unlock_products: ["vst"],
    beta_free_trial: true,
    availability_label: BETA_FREE_TRIAL_LABEL,
    future_price_label: POST_BETA_PRICE_LABEL
  }
];

function catalogArtworkUrlFor(seedValue) {
  const seed = String(seedValue || "").trim().toLowerCase();
  if (!seed) {
    return CATALOG_ARTWORK_POOL[0];
  }
  let accumulator = 0;
  for (const ch of seed) {
    accumulator = (accumulator + ch.charCodeAt(0)) % CATALOG_ARTWORK_POOL.length;
  }
  return CATALOG_ARTWORK_POOL[accumulator];
}

function normalizeReferenceGenreName(value) {
  const raw = String(value || "").trim().toLowerCase();
  if (!raw) {
    return "";
  }
  if (raw === "hip hop" || raw === "hiphop") {
    return "hip-hop";
  }
  const normalized = raw.replace(/[^a-z0-9-]/g, "");
  if (["rap", "hip-hop", "edm", "dubstep", "pop", "rock"].includes(normalized)) {
    return normalized;
  }
  return "";
}

function mimeTypeForFile(fileName) {
  const lower = String(fileName || "").toLowerCase();
  if (lower.endsWith(".mp3")) {
    return "audio/mpeg";
  }
  if (lower.endsWith(".wav")) {
    return "audio/wav";
  }
  if (lower.endsWith(".ogg")) {
    return "audio/ogg";
  }
  if (lower.endsWith(".m4a")) {
    return "audio/mp4";
  }
  if (lower.endsWith(".flac")) {
    return "audio/flac";
  }
  if (lower.endsWith(".aiff") || lower.endsWith(".aif")) {
    return "audio/aiff";
  }
  return "application/octet-stream";
}

const ACTION_REGISTRY = {
  "session.save": { id: "session.save", description: "Save current session state" },
  "catalog.refresh": { id: "catalog.refresh", description: "Refresh catalog from API" },
  "mixer.channel.set_gain": { id: "mixer.channel.set_gain", description: "Set channel gain" },
  "plugin.insert": { id: "plugin.insert", description: "Insert plugin on channel" },
  "analysis.run": { id: "analysis.run", description: "Run analysis worker" },
  "command.exec": { id: "command.exec", description: "Execute backend shell command" },
  "soundpack.upload": { id: "soundpack.upload", description: "Upload sound pack" },
  "reference.pool.validate": { id: "reference.pool.validate", description: "Validate licensed reference pool sources and counts" },
  "reference.pool.rebuild": { id: "reference.pool.rebuild", description: "Rebuild licensed reference pool from licensed intake" },
  "analysis.engine.verify": { id: "analysis.engine.verify", description: "Run analysis-engine verification tests" }
};

const ACTION_COMMANDS = {
  "reference.pool.validate": "python3 scripts/validate_reference_pool_sources.py",
  "reference.pool.rebuild": "python3 scripts/build_licensed_reference_pool.py",
  "analysis.engine.verify": "ctest --test-dir build --output-on-failure -R 'profile_cache_test|genre_detector_test|aifr3d_fix_list_reactivity_smoke'"
};

function ensureDir(dirPath) {
  fs.mkdirSync(dirPath, { recursive: true });
}

function ensureFile(filePath, fallbackData) {
  if (!fs.existsSync(filePath)) {
    fs.writeFileSync(filePath, JSON.stringify(fallbackData, null, 2));
  }
}

function readJson(filePath, fallbackData) {
  try {
    const raw = fs.readFileSync(filePath, "utf8");
    return JSON.parse(raw);
  } catch (_error) {
    return fallbackData;
  }
}

function writeJson(filePath, value) {
  fs.writeFileSync(filePath, JSON.stringify(value, null, 2));
}

function nowIso() {
  return new Date().toISOString();
}

function hashHex(input) {
  return crypto.createHash("sha256").update(String(input || "")).digest("hex");
}

function hmacHex(secret, input) {
  return crypto.createHmac("sha256", String(secret || "")).update(String(input || "")).digest("hex");
}

function toBase64Url(raw) {
  return Buffer.from(String(raw || ""), "utf8").toString("base64url");
}

function fromBase64Url(raw) {
  return Buffer.from(String(raw || ""), "base64url").toString("utf8");
}

function createSignedToken(secret, payload) {
  const payloadRaw = JSON.stringify(payload || {});
  const payloadB64 = toBase64Url(payloadRaw);
  const sig = hmacHex(secret, payloadRaw);
  return `${payloadB64}.${sig}`;
}

function verifySignedToken(secret, token) {
  const [payloadB64, sig] = String(token || "").split(".");
  if (!payloadB64 || !sig) {
    return null;
  }
  let payloadRaw = "";
  try {
    payloadRaw = fromBase64Url(payloadB64);
  } catch (_error) {
    return null;
  }
  const expected = hmacHex(secret, payloadRaw);
  if (expected !== sig) {
    return null;
  }
  try {
    return JSON.parse(payloadRaw);
  } catch (_error) {
    return null;
  }
}

function appendEventLog(eventType, payload = {}) {
  try {
    const entry = {
      ts: nowIso(),
      event_type: cleanText(eventType || "event", 120),
      payload
    };
    fs.appendFileSync(EVENT_LOG_FILE, `${JSON.stringify(entry)}\n`, "utf8");
  } catch (_error) {
  }
}

function appendAdminLog(commandText, payload = {}) {
  try {
    const entry = {
      ts: nowIso(),
      command: cleanText(commandText || "", 320),
      payload
    };
    fs.appendFileSync(ADMIN_LOG_FILE, `${JSON.stringify(entry)}\n`, "utf8");
  } catch (_error) {
  }
}

function recordDashboardActivity(kind, routePath, payload = {}) {
  const safeKind = cleanText(kind || "event", 80);
  const safeRoute = cleanText(routePath || "/", 260);
  DASHBOARD_STATE.last_request_at = nowIso();
  if (safeKind === "page_view") {
    DASHBOARD_STATE.page_views += 1;
  } else if (safeKind === "api_hit") {
    DASHBOARD_STATE.api_hits += 1;
  } else if (safeKind === "media_stream") {
    DASHBOARD_STATE.media_streams += 1;
  } else if (safeKind === "download") {
    DASHBOARD_STATE.downloads += 1;
  }
  DASHBOARD_STATE.recent.unshift({
    ts: DASHBOARD_STATE.last_request_at,
    kind: safeKind,
    path: safeRoute,
    payload
  });
  if (DASHBOARD_STATE.recent.length > 200) {
    DASHBOARD_STATE.recent = DASHBOARD_STATE.recent.slice(0, 200);
  }
}

function readEventLogs(limit = 200) {
  try {
    if (!fs.existsSync(EVENT_LOG_FILE)) {
      return [];
    }
    const lines = fs
      .readFileSync(EVENT_LOG_FILE, "utf8")
      .split("\n")
      .map((line) => line.trim())
      .filter(Boolean);
    const selected = lines.slice(Math.max(0, lines.length - limit));
    return selected
      .map((line) => {
        try {
          return JSON.parse(line);
        } catch (_error) {
          return null;
        }
      })
      .filter(Boolean);
  } catch (_error) {
    return [];
  }
}

function readAdminLog(limit = 200) {
  try {
    if (!fs.existsSync(ADMIN_LOG_FILE)) {
      return [];
    }
    const lines = fs
      .readFileSync(ADMIN_LOG_FILE, "utf8")
      .split("\n")
      .map((line) => line.trim())
      .filter(Boolean);
    return lines
      .slice(Math.max(0, lines.length - limit))
      .map((line) => {
        try {
          return JSON.parse(line);
        } catch (_error) {
          return null;
        }
      })
      .filter(Boolean);
  } catch (_error) {
    return [];
  }
}

function receiptWatermarkToken(receiptId, issuedAtIso) {
  return hmacHex(RECEIPT_SIGNING_SECRET, `${receiptId}|${issuedAtIso}|watermark`).slice(0, 20);
}

function signReceipt(receipt) {
  const canonical = JSON.stringify({
    receipt_id: receipt.receipt_id,
    sale_id: receipt.sale_id,
    issued_at: receipt.issued_at,
    amount: receipt.amount,
    currency: receipt.currency,
    item_name: receipt.item_name,
    customer_email: receipt.customer_email
  });
  return hmacHex(RECEIPT_SIGNING_SECRET, canonical);
}

function buildReceiptHtml(receipt) {
  const escaped = (value) => String(value || "").replace(/[&<>"]/g, (ch) => ({
    "&": "&amp;",
    "<": "&lt;",
    ">": "&gt;",
    '"': "&quot;"
  }[ch] || ch));
  const wm = escaped(receipt.watermark_token);
  return `<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>North3rnLight3r Receipt ${escaped(receipt.receipt_id)}</title>
    <style>
      body { margin: 0; background: #071220; color: #e8f7ff; font-family: "Segoe UI", Arial, sans-serif; }
      .wrap { max-width: 900px; margin: 22px auto; padding: 20px; }
      .card {
        position: relative;
        border: 1px solid rgba(88, 216, 255, 0.52);
        border-radius: 16px;
        padding: 24px;
        overflow: hidden;
        background: rgba(7, 20, 35, 0.96);
      }
      .card::before {
        content: "";
        position: absolute;
        inset: 0;
        background: linear-gradient(180deg, rgba(16, 38, 58, 0.16), rgba(4, 10, 18, 0.04));
        filter: saturate(1.1) contrast(1.03);
      }
      .content { position: relative; z-index: 1; }
      h1 { margin: 0 0 8px; font-size: 1.7rem; letter-spacing: 0.02em; }
      p { margin: 6px 0; }
      .muted { color: #9bc6df; }
      .line { border-top: 1px solid rgba(88, 216, 255, 0.34); margin: 16px 0; }
      .mono { font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace; font-size: 0.92rem; }
      .seal { color: #4af0cc; font-weight: 700; }
    </style>
  </head>
  <body>
    <div class="wrap">
      <div class="card">
        <div class="content">
          <h1>North3rnLight3r Official Receipt</h1>
          <p class="seal">Official Watermarked Purchase Record</p>
          <p>Receipt ID: <span class="mono">${escaped(receipt.receipt_id)}</span></p>
          <p>Sale ID: <span class="mono">${escaped(receipt.sale_id)}</span></p>
          <p>Issued: ${escaped(receipt.issued_at)}</p>
          <div class="line"></div>
          <p>Customer: ${escaped(receipt.customer_name || "N/A")} (${escaped(receipt.customer_email || "N/A")})</p>
          <p>Item: ${escaped(receipt.item_name)}</p>
          <p>Amount: ${escaped(receipt.currency)} ${escaped(receipt.amount)}</p>
          <p>Payment Provider: ${escaped(receipt.payment_provider)}</p>
          <p>Transaction: <span class="mono">${escaped(receipt.payment_txn_id || "N/A")}</span></p>
          <div class="line"></div>
          <p class="muted">Verification Signature</p>
          <p class="mono">${escaped(receipt.signature)}</p>
          <p class="muted">Watermark Token</p>
          <p class="mono">${escaped(receipt.watermark_token)}</p>
        </div>
      </div>
    </div>
  </body>
</html>`;
}

function createReceiptFromSale(sale) {
  const issuedAt = nowIso();
  const receiptId = randomToken(8);
  const watermarkToken = receiptWatermarkToken(receiptId, issuedAt);
  const receipt = {
    receipt_id: receiptId,
    sale_id: sale.sale_id,
    issued_at: issuedAt,
    customer_name: sale.customer_name,
    customer_email: sale.customer_email,
    item_name: sale.item_name,
    amount: sale.amount,
    currency: sale.currency,
    payment_provider: sale.payment_provider,
    payment_txn_id: sale.payment_txn_id,
    watermark_token: watermarkToken
  };
  receipt.signature = signReceipt(receipt);
  receipt.receipt_url = `/receipts/${receipt.receipt_id}`;
  return receipt;
}

function randomToken(size = 24) {
  return crypto.randomBytes(size).toString("hex");
}

function normalizeTrackTitle(fileName) {
  return String(fileName || "").replace(/\.[^/.]+$/, "").trim();
}

function cleanText(value, maxLength = 220) {
  return String(value || "").replace(/\s+/g, " ").trim().slice(0, maxLength);
}

function cleanMultilineText(value, maxLength = 2000) {
  return String(value || "").replace(/\r/g, "").trim().slice(0, maxLength);
}

function normalizeChoice(value, allowed, fallback) {
  const normalized = String(value || "").trim().toLowerCase();
  return allowed.includes(normalized) ? normalized : fallback;
}

function normalizeInteger(value, fallback, min, max) {
  const parsed = Number(value);
  if (!Number.isFinite(parsed)) {
    return fallback;
  }
  return Math.min(max, Math.max(min, Math.round(parsed)));
}

function normalizeSku(value) {
  return String(value || "")
    .trim()
    .toLowerCase()
    .replace(/[^a-z0-9._-]/g, "")
    .slice(0, 80);
}

function normalizeCurrencyCode(value) {
  const code = String(value || "USD")
    .trim()
    .toUpperCase()
    .replace(/[^A-Z]/g, "")
    .slice(0, 3);
  return code || "USD";
}

function normalizeUsdAmount(value, fallback = "0.00") {
  const parsed = Number(String(value || "").replace(/[^0-9.]/g, ""));
  if (!Number.isFinite(parsed) || parsed <= 0) {
    return fallback;
  }
  return parsed.toFixed(2);
}

function envFlag(value, fallback = false) {
  const text = String(value ?? "").trim().toLowerCase();
  if (!text) {
    return fallback;
  }
  if (["1", "true", "yes", "on", "enabled"].includes(text)) {
    return true;
  }
  if (["0", "false", "no", "off", "disabled"].includes(text)) {
    return false;
  }
  return fallback;
}

function sitePrivateModeEnabled() {
  return envFlag(process.env.WEBSITE_PRIVATE_MODE ?? process.env.SITE_PRIVATE_MODE ?? process.env.MAINTENANCE_MODE, false);
}

function paymentProvider() {
  return "paypal";
}

function checkoutFallbackUrl(itemName, amount, currency = "USD") {
  return fallbackPaypalUrl(itemName, amount, currency);
}

function loadPayPalFileCredentials() {
  if (PAYPAL_FILE_CACHE) {
    return PAYPAL_FILE_CACHE;
  }
  const tokens = [];
  const tokensByFile = new Map();
  for (const filePath of PAYPAL_CREDENTIAL_FILE_CANDIDATES) {
    try {
      if (!filePath || !fs.existsSync(filePath)) {
        continue;
      }
      const raw = fs.readFileSync(filePath, "utf8");
      const found = raw.match(/[A-Za-z0-9._-]{40,}/g) || [];
      const normalized = [];
      for (const token of found) {
        normalized.push(token);
        if (!token.startsWith("A") && token.startsWith("id") && token.length > 42) {
          const trimmed = token.slice(2);
          if (/^[A-Za-z0-9._-]{40,}$/.test(trimmed)) {
            normalized.push(trimmed);
          }
        }
      }
      tokensByFile.set(filePath, normalized);
      for (const token of normalized) {
        if (!tokens.includes(token)) {
          tokens.push(token);
        }
      }
    } catch (_error) {
    }
  }
  let clientId = "";
  let clientSecret = "";
  const idFileTokens =
    tokensByFile.get(PAYPAL_CREDENTIAL_FILE_CANDIDATES.find((filePath) => /PayPalID\.txt$/i.test(String(filePath || ""))) || "") || [];
  const secretFileTokens =
    tokensByFile.get(PAYPAL_CREDENTIAL_FILE_CANDIDATES.find((filePath) => /SecretPal\.txt$/i.test(String(filePath || ""))) || "") || [];
  clientId = idFileTokens.find((token) => token.startsWith("A")) || tokens.find((token) => token.startsWith("A")) || "";
  clientSecret =
    secretFileTokens.find((token) => !token.startsWith("A")) ||
    tokens.find((token) => token && token !== clientId && !token.startsWith("A")) ||
    "";
  PAYPAL_FILE_CACHE = { clientId, clientSecret };
  return PAYPAL_FILE_CACHE;
}

function paypalClientId() {
  if (PAYPAL_CLIENT_ID) {
    return PAYPAL_CLIENT_ID;
  }
  return loadPayPalFileCredentials().clientId || "";
}

function paypalClientSecret() {
  if (PAYPAL_CLIENT_SECRET) {
    return PAYPAL_CLIENT_SECRET;
  }
  return loadPayPalFileCredentials().clientSecret || "";
}

function paypalBaseUrl() {
  return PAYPAL_ENV === "live" ? "https://api-m.paypal.com" : "https://api-m.sandbox.paypal.com";
}

function paypalBusinessEmail() {
  if (PAYPAL_BUSINESS_EMAIL) {
    return PAYPAL_BUSINESS_EMAIL;
  }
  for (const filePath of PAYPAL_CREDENTIAL_FILE_CANDIDATES) {
    try {
      if (!filePath || !fs.existsSync(filePath)) {
        continue;
      }
      const raw = fs.readFileSync(filePath, "utf8");
      const match = raw.match(/[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,}/i);
      if (match) {
        return match[0].trim();
      }
    } catch (_error) {
    }
  }
  return "north3rnlight3rofficial@outlook.com";
}

function paypalReady() {
  return Boolean(paypalClientId() && paypalClientSecret());
}

function fallbackPaypalUrl(itemName, amount, currency = "USD") {
  const params = new URLSearchParams({
    cmd: "_xclick",
    business: paypalBusinessEmail(),
    item_name: cleanText(itemName || "North3rnLight3r Product", 220),
    currency_code: normalizeCurrencyCode(currency)
  });
  const value = normalizeUsdAmount(amount, "");
  if (value) {
    params.set("amount", value);
  }
  return `https://www.paypal.com/cgi-bin/webscr?${params.toString()}`;
}

async function paypalAccessToken() {
  const basic = Buffer.from(`${paypalClientId()}:${paypalClientSecret()}`).toString("base64");
  const response = await fetch(`${paypalBaseUrl()}/v1/oauth2/token`, {
    method: "POST",
    headers: {
      Authorization: `Basic ${basic}`,
      "Content-Type": "application/x-www-form-urlencoded"
    },
    body: "grant_type=client_credentials"
  });
  const payload = await response.json().catch(() => null);
  if (!response.ok || !payload?.access_token) {
    throw new Error(payload?.error_description || payload?.error || "PayPal token request failed");
  }
  return payload.access_token;
}

function cleanCredentialValue(value, maxLength = 160) {
  return cleanText(value, maxLength).replace(/^['"]+|['"]+$/g, "").trim();
}

function normalizeModelName(value) {
  return cleanText(value || "", 220);
}

function normalizeModelList(input) {
  const raw = Array.isArray(input)
    ? input
    : String(input || "")
        .split(",")
        .map((item) => item.trim());
  const output = [];
  const seen = new Set();
  raw.forEach((item) => {
    const model = normalizeModelName(item);
    if (!model || seen.has(model)) {
      return;
    }
    seen.add(model);
    output.push(model);
  });
  return output;
}

function readSecretFile(filePath) {
  try {
    if (!filePath || !fs.existsSync(filePath)) {
      return "";
    }
    return String(fs.readFileSync(filePath, "utf8") || "").trim();
  } catch (_error) {
    return "";
  }
}

function parseOpenAiApiKey(raw) {
  const text = String(raw || "").trim();
  if (!text) {
    return "";
  }
  const direct = text.match(/(sk-[A-Za-z0-9_-]{20,})/);
  if (direct?.[1]) {
    return direct[1];
  }
  const line = text.match(/OPENAI_API_KEY\s*[:=]\s*["']?([^"' \n\r]+)["']?/i);
  if (line?.[1] && /^sk-[A-Za-z0-9_-]{20,}$/.test(line[1])) {
    return line[1];
  }
  return "";
}

function openAiKeyFilePath(configValue = null) {
  const configured = cleanText(configValue?.key_file_path || "", 1000);
  if (configured && configured === OPENAI_PRIMARY_KEY_FILE) {
    return OPENAI_PRIMARY_KEY_FILE;
  }
  return OPENAI_PRIMARY_KEY_FILE;
}

function resolveOpenAiApiKey(configValue = null) {
  const primary = parseOpenAiApiKey(readSecretFile(OPENAI_PRIMARY_KEY_FILE));
  if (primary) {
    return primary;
  }
  return "";
}

function configuredOpenAiModelList() {
  const raw = String(
    process.env.DAWAI_OPENAI_MODEL_ALLOWLIST ||
    process.env.OPENAI_MODEL_ALLOWLIST ||
    process.env.DAWAI_OPENAI_MODELS ||
    ""
  ).trim();
  if (!raw) {
    return [...DEFAULT_OPENAI_MODEL_ALLOWLIST];
  }
  const parsed = raw
    .split(/[,\n]/)
    .map((item) => normalizeModelName(item))
    .filter(Boolean);
  return parsed.length > 0 ? Array.from(new Set(parsed)) : [...DEFAULT_OPENAI_MODEL_ALLOWLIST];
}

function activeOpenAiModelList() {
  return configuredOpenAiModelList();
}

function configuredOpenAiPrimaryModel() {
  const requested = normalizeModelName(
    process.env.DAWAI_OPENAI_PRIMARY_MODEL ||
    process.env.OPENAI_PRIMARY_MODEL ||
    DEFAULT_OPENAI_PRIMARY_MODEL
  );
  return activeOpenAiModelList().includes(requested) ? requested : activeOpenAiModelList()[0] || DEFAULT_OPENAI_PRIMARY_MODEL;
}

function normalizeRequestedOpenAiModel(value) {
  const model = normalizeModelName(value);
  return activeOpenAiModelList().includes(model) ? model : configuredOpenAiPrimaryModel();
}

function defaultOpenAiConfig() {
  return {
    model: configuredOpenAiPrimaryModel(),
    model_list: activeOpenAiModelList(),
    key_file_path: OPENAI_PRIMARY_KEY_FILE,
    base_url: OPENAI_BASE_URL,
    updated_at: nowIso(),
    source: "file"
  };
}

function loadOpenAiConfig() {
  const fileValue = readJson(OPENAI_CONFIG_FILE, defaultOpenAiConfig());
  const apiKey = resolveOpenAiApiKey(fileValue);
  return {
    api_key: apiKey,
    model: configuredOpenAiPrimaryModel(),
    model_list: activeOpenAiModelList(),
    key_file_path: openAiKeyFilePath(fileValue),
    base_url: OPENAI_BASE_URL,
    ready: Boolean(apiKey),
    source: apiKey ? "file" : "unset",
    updated_at: cleanText(fileValue?.updated_at, 80) || nowIso()
  };
}

function saveOpenAiConfig(apiKeyInput, baseUrl, keyFilePathInput = "") {
  const current = readJson(OPENAI_CONFIG_FILE, defaultOpenAiConfig());
  const requestedPath = cleanText(keyFilePathInput || current?.key_file_path || OPENAI_PRIMARY_KEY_FILE, 1000);
  const keyFilePath = requestedPath === OPENAI_PRIMARY_KEY_FILE ? OPENAI_PRIMARY_KEY_FILE : OPENAI_PRIMARY_KEY_FILE;
  const nextApiKey = parseOpenAiApiKey(apiKeyInput || "");
  if (nextApiKey) {
    ensureDir(path.dirname(keyFilePath));
    fs.writeFileSync(keyFilePath, `${nextApiKey}\n`, "utf8");
  }
  const payload = {
    model: configuredOpenAiPrimaryModel(),
    model_list: activeOpenAiModelList(),
    key_file_path: keyFilePath,
    base_url: OPENAI_BASE_URL,
    updated_at: nowIso(),
    source: "admin"
  };
  writeJson(OPENAI_CONFIG_FILE, payload);
  return loadOpenAiConfig();
}

function loadAifredBrain() {
  try {
    if (!fs.existsSync(BRAIN_CONFIG_FILE)) {
      return DEFAULT_AIFR3D_BRAIN;
    }
    const raw = fs.readFileSync(BRAIN_CONFIG_FILE, "utf8");
    const parsed = JSON.parse(raw);
    return {
      ...DEFAULT_AIFR3D_BRAIN,
      ...(parsed && typeof parsed === "object" ? parsed : {}),
      model: configuredOpenAiPrimaryModel()
    };
  } catch (_error) {
    return DEFAULT_AIFR3D_BRAIN;
  }
}

function normalizeWebhookEvents(value) {
  const items = Array.isArray(value) ? value : [];
  const normalized = items
    .map((item) => normalizeChoice(item, CHAT_WEBHOOK_EVENTS, ""))
    .filter(Boolean);
  return normalized.length > 0 ? Array.from(new Set(normalized)) : [...DEFAULT_CHAT_SETTINGS.webhook.events];
}

function sanitizeChatSettings(value) {
  const input = value && typeof value === "object" ? value : {};
  const webhook = input.webhook && typeof input.webhook === "object" ? input.webhook : {};
  const context = input.context && typeof input.context === "object" ? input.context : {};
  const prompt = input.prompt && typeof input.prompt === "object" ? input.prompt : {};
  const reasoning = input.reasoning && typeof input.reasoning === "object" ? input.reasoning : {};
  const response = input.response && typeof input.response === "object" ? input.response : {};
  return {
    transport_mode: normalizeChoice(input.transport_mode, CHAT_TRANSPORT_MODES, DEFAULT_CHAT_SETTINGS.transport_mode),
    webhook: {
      enabled: webhook.enabled === true,
      url: cleanText(webhook.url || "", 700),
      secret: cleanText(webhook.secret || "", 260),
      events: normalizeWebhookEvents(webhook.events)
    },
    context: {
      use_previous_response_id: context.use_previous_response_id !== false,
      memory_window_items: normalizeInteger(
        context.memory_window_items,
        DEFAULT_CHAT_SETTINGS.context.memory_window_items,
        1,
        120
      ),
      summary_items: normalizeInteger(context.summary_items, DEFAULT_CHAT_SETTINGS.context.summary_items, 1, 20),
      max_prompt_chars: normalizeInteger(context.max_prompt_chars, DEFAULT_CHAT_SETTINGS.context.max_prompt_chars, 200, 24000),
      compact_threshold: normalizeInteger(
        context.compact_threshold,
        DEFAULT_CHAT_SETTINGS.context.compact_threshold,
        1,
        64
      )
    },
    prompt: {
      tone: normalizeChoice(prompt.tone, CHAT_TONE_PRESETS, DEFAULT_CHAT_SETTINGS.prompt.tone),
      personality_mode: cleanText(prompt.personality_mode || DEFAULT_CHAT_SETTINGS.prompt.personality_mode, 80),
      system_prefix: cleanMultilineText(prompt.system_prefix || "", 2000),
      system_suffix: cleanMultilineText(prompt.system_suffix || "", 2000)
    },
    reasoning: {
      enabled: reasoning.enabled !== false,
      effort: normalizeChoice(reasoning.effort, CHAT_REASONING_EFFORTS, DEFAULT_CHAT_SETTINGS.reasoning.effort)
    },
    response: {
      verbosity: normalizeChoice(response.verbosity, CHAT_VERBOSITY_LEVELS, DEFAULT_CHAT_SETTINGS.response.verbosity),
      max_output_tokens: normalizeInteger(
        response.max_output_tokens,
        DEFAULT_CHAT_SETTINGS.response.max_output_tokens,
        50,
        4000
      )
    }
  };
}

function loadChatSettings() {
  return sanitizeChatSettings(readJson(CHAT_SETTINGS_FILE, DEFAULT_CHAT_SETTINGS));
}

function saveChatSettings(value) {
  const next = sanitizeChatSettings({
    ...loadChatSettings(),
    ...(value && typeof value === "object" ? value : {})
  });
  writeJson(CHAT_SETTINGS_FILE, next);
  return next;
}

async function emitChatWebhook(eventType, payload) {
  const settings = loadChatSettings();
  if (!settings.webhook.enabled || !settings.webhook.url || !settings.webhook.events.includes(eventType)) {
    return false;
  }

  const body = JSON.stringify({
    event: eventType,
    emitted_at: nowIso(),
    ...payload
  });
  const headers = {
    "Content-Type": "application/json"
  };
  if (settings.webhook.secret) {
    headers["X-AIFR3D-Webhook-Signature"] = hmacHex(settings.webhook.secret, body);
  }

  try {
    const controller = new AbortController();
    const timer = setTimeout(() => controller.abort(), 3000);
    await fetch(settings.webhook.url, {
      method: "POST",
      headers,
      body,
      signal: controller.signal
    });
    clearTimeout(timer);
    return true;
  } catch (_error) {
    return false;
  }
}

function toneInstructions(tone) {
  switch (tone) {
    case "calm":
      return "Keep the delivery calm, steady, and reassuring without adding fluff.";
    case "technical":
      return "Keep the delivery technical, concrete, and implementation-focused.";
    case "executive":
      return "Keep the delivery concise, high-signal, and decision-oriented.";
    case "creative":
      return "Keep the delivery imaginative but still specific and operational.";
    case "direct":
    default:
      return "Keep the delivery direct, concise, and plain-spoken.";
  }
}

function buildSystemPrompt(personalityMode, agentModes, chatSettings = DEFAULT_CHAT_SETTINGS) {
  const brain = AIFR3D_BRAIN;
  const roles = normalizeModelList(agentModes).join(", ") || "mix_engineer, analysis_worker, planning_worker";
  const mode = cleanText(personalityMode || brain?.identity?.mode || "professional_mentor", 80);
  const voice = Array.isArray(brain?.identity?.voice) ? brain.identity.voice.join(", ") : "direct, calm";
  const prefix = cleanMultilineText(chatSettings?.prompt?.system_prefix || "", 2000);
  const suffix = cleanMultilineText(chatSettings?.prompt?.system_suffix || "", 2000);
  return [
    `You are ${brain?.identity?.name || "AIFR3D"}, a structured production intelligence system.`,
    `Personality mode: ${mode}.`,
    `Agent roles: ${roles}.`,
    `Voice traits: ${voice}.`,
    `Tone preset: ${chatSettings?.prompt?.tone || "direct"}.`,
    toneInstructions(chatSettings?.prompt?.tone || "direct"),
    "Use deterministic, evidence-based language. No judgemental phrasing.",
    "Return JSON only.",
    "No markdown, no prose outside JSON.",
    "Use this exact schema:",
    "{",
    '  "summary": "string",',
    '  "issues": [',
    "    {",
    '      "issue_type": "string",',
    '      "severity": "Low|Medium|High",',
    '      "summary": "string",',
    '      "why_it_matters": "string",',
    '      "try_first": ["string"],',
    '      "ignore_for_now": ["string"],',
    '      "evidence": {"metrics": {}, "pointers": ["string"]}',
    "    }",
    "  ],",
    '  "action_suggestions": [',
    "    {",
    '      "id": "string",',
    '      "label": "string",',
    '      "payload": {},',
    '      "destructive": false,',
    '      "requiresSave": false',
    "    }",
    "  ],",
    '  "state_update": {"focus_tags": ["string"], "signal_clarity": 0.0, "confidence": 0.0}',
    "}",
    prefix ? `Additional system instruction:\n${prefix}` : "",
    suffix ? `Final response constraint:\n${suffix}` : ""
  ].join("\n");
}

function normalizeMusicMeta(input) {
  return {
    description: cleanText(input?.description || "", 360),
    bpm: cleanText(input?.bpm || input?.tempo_bpm || "", 24),
    key: cleanText(input?.key || input?.musical_key || "", 24),
    tempo: cleanText(input?.tempo || input?.bpm || input?.tempo_bpm || "", 24),
    price: cleanText(input?.price || "", 32),
    pack_type: cleanText(input?.pack_type || "soundpack", 24).toLowerCase()
  };
}

function defaultPackPrice(packType) {
  const key = cleanText(packType || "soundpack", 24).toLowerCase();
  if (key === "single") {
    return "$2.99";
  }
  if (["soundpack", "midipack", "drumpack", "samplepack"].includes(key)) {
    return "$19.99";
  }
  return "$19.99";
}

function normalizePromoCode(raw) {
  return String(raw || "").trim().toUpperCase().slice(0, 64);
}

function normalizePromoProduct(raw) {
  const value = String(raw || "").trim().toLowerCase();
  if (value === "vst" || value === "standalone") {
    return value;
  }
  return "";
}

function normalizePromoCodes(input) {
  const list = Array.isArray(input) ? input : [];
  return list
    .map((entry) => {
      const code = normalizePromoCode(entry?.code);
      const products = Array.isArray(entry?.products)
        ? entry.products.map((item) => normalizePromoProduct(item)).filter(Boolean)
        : ["vst", "standalone"];
      const active = entry?.active !== false;
      const maxUses = Number.isFinite(Number(entry?.max_uses)) ? Number(entry.max_uses) : 0;
      const uses = Number.isFinite(Number(entry?.uses)) ? Number(entry.uses) : 0;
      const note = cleanText(entry?.note || "", 180);
      const rawEntitlementType = cleanText(entry?.entitlement_type || "", 40).toLowerCase();
      const paymentUnlock = rawEntitlementType === "payment_unlock";
      const lifetime = Boolean(entry?.lifetime) || rawEntitlementType === "lifetime" || paymentUnlock;
      const installOnly = rawEntitlementType === "install_only";
      const paymentBypass = Boolean(entry?.payment_bypass);
      const adminCode = Boolean(entry?.admin_code);
      const paymentVerified = Boolean(entry?.payment_verified) || paymentUnlock;
      const sessionLimits = {
        analyze: Math.max(0, Math.floor(Number(entry?.session_limits?.analyze || 0) || 0)),
        compare: Math.max(0, Math.floor(Number(entry?.session_limits?.compare || 0) || 0)),
        reference: Math.max(0, Math.floor(Number(entry?.session_limits?.reference || 0) || 0))
      };
      if (!code) {
        return null;
      }
      return {
        code,
        products: products.length > 0 ? products : ["vst", "standalone"],
        active,
        max_uses: Math.max(0, Math.floor(maxUses)),
        uses: Math.max(0, Math.floor(uses)),
        note,
        entitlement_type: paymentUnlock ? "payment_unlock" : lifetime ? "lifetime" : installOnly ? "install_only" : "session_bundle",
        lifetime,
        payment_bypass: paymentBypass,
        admin_code: adminCode,
        payment_verified: paymentVerified,
        session_limits: lifetime || installOnly || paymentUnlock ? { analyze: 0, compare: 0, reference: 0 } : sessionLimits,
        created_at: cleanText(entry?.created_at, 80) || nowIso(),
        last_redeemed_at: cleanText(entry?.last_redeemed_at, 80) || ""
      };
    })
    .filter(Boolean);
}

function loadPaymentUnlockBundle() {
  const fallback = {
    build: "2.2.4 Beta",
    entitlement_type: "beta_free_trial",
    payment_verified: false,
    payment_bypass: false,
    lifetime: true,
    session_limits: { analyze: 0, compare: 0, reference: 0 },
    products: ["vst", "standalone"],
    note: "AIFR3D 2.2.4 Beta software installers are free to try for a limited time."
  };
  try {
    if (!fs.existsSync(PAYMENT_UNLOCK_FILE)) {
      return fallback;
    }
    const parsed = JSON.parse(fs.readFileSync(PAYMENT_UNLOCK_FILE, "utf8"));
    return {
      ...fallback,
      ...parsed,
      products: Array.isArray(parsed?.products)
        ? parsed.products.map((item) => normalizePromoProduct(item)).filter(Boolean)
        : fallback.products,
      session_limits: {
        analyze: Math.max(0, Math.floor(Number(parsed?.session_limits?.analyze || 0) || 0)),
        compare: Math.max(0, Math.floor(Number(parsed?.session_limits?.compare || 0) || 0)),
        reference: Math.max(0, Math.floor(Number(parsed?.session_limits?.reference || 0) || 0))
      }
    };
  } catch (_error) {
    return fallback;
  }
}

function loadPromoCodes() {
  for (const promoPath of LOCAL_PROMO_FILE_CANDIDATES) {
    try {
      if (!promoPath || !fs.existsSync(promoPath)) {
        continue;
      }
      const raw = fs.readFileSync(promoPath, "utf8");
      const parsed = normalizePromoCodes(JSON.parse(raw));
      if (parsed.length > 0) {
        return parsed;
      }
    } catch (_error) {
    }
  }
  const defaults = normalizePromoCodes(DEFAULT_PROMO_CODES);
  const loaded = normalizePromoCodes(readJson(PROMO_CODES_FILE, defaults));
  return loaded.length > 0 ? loaded : defaults;
}

function savePromoCodes(codes) {
  writeJson(PROMO_CODES_FILE, normalizePromoCodes(codes));
}

function promoProductDownloadPath(product) {
  if (product === "vst") {
    return "/downloads/dawai-vst3";
  }
  if (product === "standalone") {
    return "/downloads/dawai-standalone";
  }
  return "";
}

function createPromoDownloadToken(product, entry = null) {
  const payload = {
    kind: "promo_download",
    product: normalizePromoProduct(product),
    nonce: randomToken(8),
    iat: Date.now(),
    exp: Date.now() + (PROMO_TOKEN_TTL_SECONDS * 1000)
  };
  if (entry && typeof entry === "object") {
    payload.promo_code = normalizePromoCode(entry.code || "");
    payload.entitlement_type = cleanText(entry.entitlement_type || "session_bundle", 40);
    payload.lifetime = Boolean(entry.lifetime);
    payload.payment_bypass = Boolean(entry.payment_bypass);
    payload.admin_code = Boolean(entry.admin_code);
    payload.payment_verified = Boolean(entry.payment_verified);
    payload.session_limits = {
      analyze: Math.max(0, Math.floor(Number(entry?.session_limits?.analyze || 0) || 0)),
      compare: Math.max(0, Math.floor(Number(entry?.session_limits?.compare || 0) || 0)),
      reference: Math.max(0, Math.floor(Number(entry?.session_limits?.reference || 0) || 0))
    };
  }
  return createSignedToken(DOWNLOAD_TOKEN_SECRET, payload);
}

function verifyPromoDownloadToken(token, expectedProduct) {
  const payload = verifySignedToken(DOWNLOAD_TOKEN_SECRET, token);
  if (!payload || payload.kind !== "promo_download") {
    return null;
  }
  if (Date.now() > Number(payload.exp || 0)) {
    return null;
  }
  const product = normalizePromoProduct(payload.product || "");
  if (!product || product !== normalizePromoProduct(expectedProduct || "")) {
    return null;
  }
  return payload;
}

const DOWNLOAD_SESSION_TTL_MS = Math.max(5 * 60 * 1000, Math.min(15 * 60 * 1000, Number(process.env.DOWNLOAD_SESSION_TTL_MS || (10 * 60 * 1000))));
const downloadSessionNonceStore = new Map();

function normalizeAssetId(value) {
  return String(value || "").trim().toLowerCase().replace(/[^a-z0-9._-]/g, "");
}

function downloadAssetPolicy(assetId) {
  const id = normalizeAssetId(assetId);
  const table = {
    "dawai-vst3": { products: ["vst"] },
    "dawai-standalone": { products: ["standalone"] },
    "dawai-vst3-a": { products: ["vst"] },
    "dawai-vst3-b": { products: ["vst"] },
    "dawai-vst3-c": { products: ["vst"] },
    "dawai-vst3-d": { products: ["vst"] },
    "dawai-standalone-a": { products: ["standalone"] },
    "dawai-standalone-b": { products: ["standalone"] },
    "dawai-standalone-c": { products: ["standalone"] },
    "dawai-standalone-d": { products: ["standalone"] },
    "dawai-linux-amd64.deb": { products: ["vst", "standalone"] },
    "dawai-linux-arch-x86_64.pkg.tar.zst": { products: ["vst", "standalone"] },
    "install-vst-standalone.sh": { products: ["vst", "standalone"] },
    "install-vst.sh": { products: ["vst"] },
    "install-standalone.sh": { products: ["standalone"] }
  };
  return table[id] || null;
}

function hashUserAgent(value) {
  return hashHex(String(value || "")).slice(0, 32);
}

function createDownloadSessionToken(payload) {
  return createSignedToken(DOWNLOAD_TOKEN_SECRET, {
    kind: "download_session",
    ...payload
  });
}

function verifyDownloadSessionToken(token, expectedAssetId) {
  const payload = verifySignedToken(DOWNLOAD_TOKEN_SECRET, token);
  if (!payload || payload.kind !== "download_session") {
    return null;
  }
  if (Date.now() > Number(payload.exp || 0)) {
    return null;
  }
  if (normalizeAssetId(payload.asset_id || "") !== normalizeAssetId(expectedAssetId || "")) {
    return null;
  }
  return payload;
}

function resolveDownloadAsset(assetId) {
  const id = normalizeAssetId(assetId);
  const vstVariant = id.match(/^dawai-vst3-([abcd])$/i);
  if (vstVariant) {
    const suffix = vstVariant[1].toLowerCase();
    const target = resolveFirstExisting(buildVstVariantCandidates(suffix)) || resolveFirstExisting(buildVstCandidates());
    return target
      ? { path: target, fileName: `DawAI_v2.2.4${suffix}.vst3`, contentType: "application/octet-stream" }
      : null;
  }
  const standaloneVariant = id.match(/^dawai-standalone-([abcd])$/i);
  if (standaloneVariant) {
    const suffix = standaloneVariant[1].toLowerCase();
    const target =
      resolveFirstExisting(buildStandaloneVariantCandidates(suffix)) || resolveFirstExisting(buildStandaloneCandidates());
    return target
      ? {
          path: target,
          fileName: target.endsWith(".exe") ? `dawai-standalone-v2.2.4${suffix}.exe` : `dawai-standalone-v2.2.4${suffix}`,
          contentType: "application/octet-stream"
        }
      : null;
  }
  if (id === "install-vst-standalone.sh") {
    const filePath = path.join(ROOT_DIR, "scripts/install_local_linux.sh");
    return fs.existsSync(filePath)
      ? { path: filePath, fileName: "install-vst-standalone.sh", contentType: "text/x-shellscript" }
      : null;
  }
  if (id === "install-vst.sh") {
    const filePath = path.join(ROOT_DIR, "apps/website/downloads/plugin-linux-installer.sh");
    return fs.existsSync(filePath)
      ? { path: filePath, fileName: "install-vst.sh", contentType: "text/x-shellscript" }
      : null;
  }
  if (id === "install-standalone.sh") {
    const filePath = path.join(ROOT_DIR, "apps/website/downloads/standalone-linux-installer.sh");
    return fs.existsSync(filePath)
      ? { path: filePath, fileName: "install-standalone.sh", contentType: "text/x-shellscript" }
      : null;
  }
  if (id === "dawai-standalone") {
    const target = resolveFirstExisting(buildStandaloneCandidates());
    return target
      ? {
          path: target,
          fileName: target.endsWith(".exe") ? "dawai-standalone.exe" : "dawai-standalone",
          contentType: "application/octet-stream"
        }
      : null;
  }
  if (id === "dawai-vst3") {
    const target = resolveFirstExisting(buildVstCandidates());
    return target
      ? { path: target, fileName: "DawAI.vst3", contentType: "application/octet-stream" }
      : null;
  }
  if (id === "dawai-linux-amd64.deb") {
    if (LINUX_DEB_URL) {
      return { redirect: LINUX_DEB_URL };
    }
    const target = resolveFirstExisting(buildLinuxDebCandidates());
    return target
      ? { path: target, fileName: "dawai-linux-amd64.deb", contentType: "application/vnd.debian.binary-package" }
      : null;
  }
  if (id === "dawai-linux-arch-x86_64.pkg.tar.zst") {
    if (LINUX_ARCH_URL) {
      return { redirect: LINUX_ARCH_URL };
    }
    const target = resolveFirstExisting(buildLinuxArchCandidates());
    return target
      ? { path: target, fileName: "dawai-linux-arch-x86_64.pkg.tar.zst", contentType: "application/zstd" }
      : null;
  }
  return null;
}

function normalizeContent(input) {
  const services = Array.isArray(input?.services) ? input.services : [];
  const products = Array.isArray(input?.products) ? input.products : [];

  const cleanItems = (items) =>
    items
      .map((item) => ({
        title: cleanText(item?.title, 140),
        description: cleanText(item?.description, 300),
        price: cleanText(item?.price, 64),
        buy_url: cleanText(item?.buy_url, 700)
      }))
      .filter((item) => item.title && item.description);

  const normalized = {
    services: cleanItems(services),
    products: cleanItems(products)
  };

  if (normalized.services.length === 0) {
    normalized.services = DEFAULT_CONTENT.services;
  }
  if (normalized.products.length === 0) {
    normalized.products = DEFAULT_CONTENT.products;
  }

  return normalized;
}

function defaultAdminCredentials() {
  const releaseUsername = "North3rnLight3r";
  const releasePassword = "Poohbe@r2009$0826";
  const username = cleanCredentialValue(ADMIN_USER_ENV, 80);
  const hash = cleanCredentialValue(ADMIN_PASS_HASH_ENV, 260);
  const plain = cleanCredentialValue(ADMIN_PASS_ENV, 160);
  if (username && hash) {
    return {
      username,
      password_hash: hash,
      updated_at: nowIso(),
      source: "env:hash"
    };
  }
  if (username && plain) {
    return {
      username,
      password_hash: bcrypt.hashSync(plain, 12),
      updated_at: nowIso(),
      source: "env:plain->hashed"
    };
  }
  return {
    username: releaseUsername,
    password_hash: bcrypt.hashSync(releasePassword, 12),
    updated_at: nowIso(),
    source: "release-default"
  };
}

function loadAdminCredentials() {
  const fallback = defaultAdminCredentials();
  const value = readJson(ADMIN_CREDENTIALS_FILE, fallback);
  const username = cleanCredentialValue(value?.username || fallback.username, 80);
  const passwordHash = cleanCredentialValue(
    value?.password_hash || value?.passwordHash || "",
    260
  );
  const legacyPassword = cleanCredentialValue(value?.password || "", 160);

  if (!username) {
    return fallback;
  }

  if (passwordHash.startsWith("$2")) {
    return {
      username,
      password_hash: passwordHash,
      updated_at: cleanText(value?.updated_at, 80) || nowIso(),
      source: cleanText(value?.source, 80) || "file"
    };
  }

  if (legacyPassword) {
    const migrated = {
      username,
      password_hash: bcrypt.hashSync(legacyPassword, 12),
      updated_at: nowIso(),
      source: "migrated:plaintext"
    };
    writeJson(ADMIN_CREDENTIALS_FILE, migrated);
    return migrated;
  }

  if (fallback.password_hash) {
    return fallback;
  }

  return {
    username,
    password_hash: "",
    updated_at: cleanText(value?.updated_at, 80) || nowIso(),
    source: cleanText(value?.source, 80) || "file"
  };
}

function saveAdminCredentials(username, password, source) {
  const cleanUser = cleanCredentialValue(username, 80);
  const cleanPass = cleanCredentialValue(password, 160);
  if (!cleanUser || !cleanPass) {
    return null;
  }
  const payload = {
    username: cleanUser,
    password_hash: bcrypt.hashSync(cleanPass, 12),
    updated_at: nowIso(),
    source: cleanText(source || "uploaded", 120)
  };
  writeJson(ADMIN_CREDENTIALS_FILE, payload);
  return payload;
}

function parseAdminCredentialText(content) {
  const raw = String(content || "").trim();
  if (!raw) {
    return null;
  }

  try {
    const parsed = JSON.parse(raw);
    const username = cleanCredentialValue(parsed?.username || parsed?.user || parsed?.login, 80);
    const password = cleanCredentialValue(parsed?.password || parsed?.pass, 160);
    if (username && password) {
      return { username, password, source: "json" };
    }
  } catch (_error) {
  }

  const lines = raw
    .split(/\r?\n/)
    .map((line) => line.trim())
    .filter((line) => line && !line.startsWith("#") && !line.startsWith("//"));

  let username = "";
  let password = "";

  lines.forEach((line) => {
    const pair = line.match(/^([a-zA-Z0-9 _\-.]+)\s*[:=]\s*(.+)$/);
    if (!pair) {
      return;
    }
    const key = pair[1].toLowerCase().replace(/\s+/g, "");
    const value = cleanCredentialValue(pair[2], 160);

    if (["username", "user", "login", "adminuser", "adminusername"].includes(key)) {
      username = cleanCredentialValue(value, 80);
    }
    if (["password", "pass", "adminpass", "adminpassword"].includes(key)) {
      password = cleanCredentialValue(value, 160);
    }
  });

  if ((!username || !password) && lines.length >= 2) {
    const possibleUser = cleanCredentialValue(lines[0], 80);
    const possiblePass = cleanCredentialValue(lines[1], 160);
    if (possibleUser && possiblePass) {
      username = username || possibleUser;
      password = password || possiblePass;
    }
  }

  if (!username || !password) {
    return null;
  }

  return { username, password, source: "text" };
}

function parseAuthToken(req) {
  const header = String(req.headers.authorization || "");
  if (header.startsWith("Bearer ")) {
    return header.slice("Bearer ".length).trim();
  }
  if (typeof req.query.token === "string") {
    return req.query.token.trim();
  }
  return "";
}

function parseCookieHeader(rawCookie) {
  const out = {};
  String(rawCookie || "")
    .split(";")
    .map((entry) => entry.trim())
    .filter(Boolean)
    .forEach((entry) => {
      const idx = entry.indexOf("=");
      if (idx <= 0) {
        return;
      }
      const key = entry.slice(0, idx).trim();
      const value = entry.slice(idx + 1).trim();
      out[key] = decodeURIComponent(value);
    });
  return out;
}

function parseAdminSessionToken(req) {
  const headerToken = parseAuthToken(req);
  if (headerToken) {
    return headerToken;
  }
  const cookies = parseCookieHeader(req.headers.cookie);
  return cleanCredentialValue(cookies.dawai_admin_session || "", 260);
}

function isMaintenanceBypassPath(pathname) {
  return pathname === "/api/v1/health" || pathname.startsWith("/api/v1/admin/");
}

function hasMaintenanceAccess(req) {
  const token = parseAdminSessionToken(req);
  if (!token) {
    return false;
  }
  if (API_TOKEN && token === API_TOKEN) {
    return true;
  }
  return Boolean(findSession(token));
}

function maintenanceHtml() {
  return `<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>North3rnLight3r - Maintenance</title>
  <style>
    :root { color-scheme: dark; }
    html, body {
      margin: 0;
      min-height: 100%;
      background: radial-gradient(circle at top, #0e1f2b 0%, #050a0f 58%, #03060a 100%);
      color: #d7ecff;
      font-family: "Segoe UI", "Helvetica Neue", Arial, sans-serif;
    }
    main {
      min-height: 100vh;
      display: grid;
      place-items: center;
      padding: 24px;
    }
    section {
      max-width: 620px;
      border: 1px solid rgba(90, 211, 255, 0.4);
      border-radius: 14px;
      padding: 24px;
      background: rgba(4, 12, 20, 0.88);
      box-shadow: 0 10px 30px rgba(0, 0, 0, 0.45);
    }
    h1 {
      margin: 0 0 12px;
      font-size: 1.5rem;
      color: #8ceaff;
    }
    p {
      margin: 8px 0;
      line-height: 1.45;
    }
    .muted { color: #9db6c9; font-size: 0.95rem; }
  </style>
</head>
<body>
  <main>
    <section>
      <h1>Site Temporarily Private</h1>
      <p>North3rnLight3r.com is in maintenance mode while checkout and inquiry routing are being finalized.</p>
      <p class="muted">Admin and API maintenance services remain active.</p>
    </section>
  </main>
</body>
</html>`;
}

function isSecureRequest(req) {
  if (req.secure) {
    return true;
  }
  const proto = String(req.headers["x-forwarded-proto"] || "").toLowerCase();
  return proto === "https";
}

function setAdminSessionCookie(res, token, req) {
  const maxAgeSec = Math.floor(SESSION_TTL_MS / 1000);
  const secure = isSecureRequest(req) ? "; Secure" : "";
  res.setHeader(
    "Set-Cookie",
    `dawai_admin_session=${encodeURIComponent(token)}; HttpOnly; Path=/; Max-Age=${maxAgeSec}; SameSite=Lax${secure}`
  );
}

function requireApiToken(req, res, next) {
  if (!API_TOKEN) {
    next();
    return;
  }
  const token = parseAuthToken(req);
  if (token !== API_TOKEN) {
    res.status(401).json({ ok: false, error: "unauthorized" });
    return;
  }
  next();
}

function loadSessions() {
  return readJson(SESSION_FILE, []);
}

function saveSessions(sessions) {
  writeJson(SESSION_FILE, sessions);
}

function createSession(username) {
  const now = Date.now();
  const sessions = loadSessions()
    .filter((entry) => Date.parse(entry.expires_at || "") > now)
    .slice(-63);
  const session = {
    token: randomToken(),
    username,
    created_at: nowIso(),
    expires_at: new Date(Date.now() + SESSION_TTL_MS).toISOString()
  };
  sessions.push(session);
  saveSessions(sessions);
  return session;
}

function findSession(token) {
  const sessions = loadSessions();
  const now = Date.now();
  const alive = sessions.filter((entry) => Date.parse(entry.expires_at || "") > now);
  if (alive.length !== sessions.length) {
    saveSessions(alive);
  }
  return alive.find((entry) => entry.token === token) || null;
}

function requireAdmin(req, res, next) {
  const token = parseAdminSessionToken(req);
  const session = findSession(token);
  if (!session) {
    res.status(401).json({ ok: false, error: "admin session invalid" });
    return;
  }
  req.admin = session;
  next();
}

function runCommand(commandLine) {
  return new Promise((resolve) => {
    const cmd = String(commandLine || "").trim();
    if (!cmd) {
      resolve({ ok: false, exit_code: 2, stdout: "", stderr: "command is empty" });
      return;
    }

    const normalizedAction = cmd.replace(/^action:/i, "").trim();
    if (ACTION_REGISTRY[normalizedAction]) {
      const mappedCommand = ACTION_COMMANDS[normalizedAction];
      if (mappedCommand) {
        runCommand(mappedCommand).then((result) => {
          const stdoutParts = [
            `registered action executed: ${normalizedAction}`,
            ACTION_REGISTRY[normalizedAction].description,
            String(result.stdout || "").trim()
          ].filter(Boolean);
          resolve({
            ...result,
            stdout: stdoutParts.join("\n\n"),
            action_id: normalizedAction
          });
        });
        return;
      }
      resolve({
        ok: true,
        exit_code: 0,
        stdout: `registered action accepted: ${normalizedAction}\n${ACTION_REGISTRY[normalizedAction].description}`,
        stderr: "",
        action_id: normalizedAction
      });
      return;
    }

    const shellExecutable = process.platform === "win32"
      ? String(process.env.ComSpec || "C:\\Windows\\System32\\cmd.exe")
      : "/bin/bash";
    const shellArgs = process.platform === "win32"
      ? ["/d", "/s", "/c", cmd]
      : ["-lc", cmd];

    const child = spawn(shellExecutable, shellArgs, {
      cwd: ROOT_DIR,
      env: process.env,
      windowsHide: true
    });

    let stdout = "";
    let stderr = "";
    let settled = false;

    const timeout = setTimeout(() => {
      if (settled) {
        return;
      }
      settled = true;
      if (process.platform === "win32") {
        child.kill();
      } else {
        child.kill("SIGKILL");
      }
      resolve({ ok: false, exit_code: -1, stdout, stderr: `${stderr}\ncommand timed out` });
    }, 120000);

    child.stdout.on("data", (data) => {
      stdout += data.toString();
    });

    child.stderr.on("data", (data) => {
      stderr += data.toString();
    });

    child.on("error", (error) => {
      if (settled) {
        return;
      }
      settled = true;
      clearTimeout(timeout);
      resolve({ ok: false, exit_code: -1, stdout, stderr: String(error.message || error) });
    });

    child.on("close", (code) => {
      if (settled) {
        return;
      }
      settled = true;
      clearTimeout(timeout);
      resolve({ ok: code === 0, exit_code: Number(code), stdout, stderr });
    });
  });
}

function secureRootPath(relativePath) {
  const normalized = String(relativePath || "").replace(/\\/g, "/").replace(/^\/+/, "");
  const resolved = path.resolve(FILE_ACCESS_ROOT, normalized);
  const allowedRoot = `${FILE_ACCESS_ROOT}${path.sep}`;
  if (!(resolved + path.sep).startsWith(allowedRoot) && resolved !== FILE_ACCESS_ROOT) {
    return null;
  }
  return resolved;
}

function appendVersionQuery(url, versionToken) {
  const base = String(url || "").trim();
  const token = String(versionToken || "").trim();
  if (!base || !token) {
    return base;
  }
  return `${base}${base.includes("?") ? "&" : "?"}v=${encodeURIComponent(token)}`;
}

function trackVersionToken(track) {
  const parsedTime = Date.parse(track?.uploaded_at || track?.updated_at || "");
  const size = Number(track?.size || 0);
  if (Number.isFinite(parsedTime) && parsedTime > 0) {
    return `${parsedTime}-${Math.max(0, Math.round(size))}`;
  }

  const fallback = cleanText(track?.key || track?.file_name || "", 80).replace(/[^a-zA-Z0-9_-]/g, "");
  if (fallback) {
    return `${fallback.slice(0, 32)}-${Math.max(0, Math.round(size))}`;
  }

  return "";
}

function buildCatalogPublicUrl(fileName, versionToken = "") {
  const safeName = findCatalogAssetFileName(fileName) || safeCatalogFileName(fileName);
  if (!safeName) {
    return "";
  }
  return appendVersionQuery(`/assets/audio/catalog/${encodeURIComponent(safeName)}`, versionToken);
}

function buildCatalogStreamUrl(trackKey, versionToken = "") {
  const safeKey = cleanText(trackKey || "", 80);
  if (!safeKey) {
    return "";
  }
  return appendVersionQuery(`/api/v1/catalog/stream/${encodeURIComponent(safeKey)}`, versionToken);
}

function safeCatalogFileName(fileName) {
  const raw = String(fileName || "").trim();
  const safeName = path.basename(raw);
  if (!safeName || safeName !== raw) {
    return "";
  }
  return safeName;
}

function isCatalogAudioFile(fileName) {
  return CATALOG_AUDIO_EXTENSIONS.has(path.extname(String(fileName || "")).toLowerCase());
}

function findCatalogAssetFileName(fileName) {
  const safeName = safeCatalogFileName(fileName);
  if (!safeName) {
    return "";
  }

  const exact = path.join(WEBSITE_CATALOG_ASSET_DIR, safeName);
  try {
    if (fs.existsSync(exact) && fs.statSync(exact).isFile()) {
      return safeName;
    }
  } catch (_error) {
  }

  const stem = path.parse(safeName).name.toLowerCase();
  try {
    const matches = fs.readdirSync(WEBSITE_CATALOG_ASSET_DIR)
      .filter((entry) => isCatalogAudioFile(entry))
      .filter((entry) => {
        const parsed = path.parse(entry);
        const entryStem = parsed.name.toLowerCase();
        return entryStem === `${stem}.preview` || entryStem === stem;
      })
      .sort((left, right) => {
        const leftStem = path.parse(left).name.toLowerCase();
        const rightStem = path.parse(right).name.toLowerCase();
        const leftRank = leftStem === stem ? 0 : 1;
        const rightRank = rightStem === stem ? 0 : 1;
        if (leftRank !== rightRank) {
          return leftRank - rightRank;
        }
        return left.localeCompare(right);
      });
    return matches[0] || "";
  } catch (_error) {
    return "";
  }
}

function catalogTrackKey(fileName) {
  const safeName = safeCatalogFileName(fileName);
  return safeName ? hashHex(safeName.toLowerCase()).slice(0, 16) : "";
}

function listCatalogFilesIn(dirPath) {
  if (!dirPath) {
    return [];
  }
  try {
    return fs.readdirSync(dirPath)
      .filter((name) => isCatalogAudioFile(name))
      .map((name) => path.join(dirPath, name))
      .filter((filePath) => {
        try {
          return fs.statSync(filePath).isFile();
        } catch (_error) {
          return false;
        }
      })
      .sort((left, right) => left.localeCompare(right));
  } catch (_error) {
    return [];
  }
}

function loadCatalogMetadataEntries() {
  const runtimeCatalog = readJson(CATALOG_FILE, []);
  const seedCatalog = readJson(CATALOG_SEED_FILE, []);
  const entries = [];
  if (Array.isArray(seedCatalog)) {
    entries.push(...seedCatalog);
  }
  if (Array.isArray(runtimeCatalog)) {
    entries.push(...runtimeCatalog);
  }
  return entries.filter((entry) => entry && typeof entry === "object");
}

function buildCatalogMetadataMap() {
  const metaByFileName = new Map();
  for (const entry of loadCatalogMetadataEntries()) {
    const safeName = safeCatalogFileName(entry.file_name || "");
    if (!safeName) {
      continue;
    }
    metaByFileName.set(safeName.toLowerCase(), entry);
  }
  return metaByFileName;
}

function buildCatalogTrackFromSource(sourcePath, metaByFileName, managedSource = false) {
  const fileName = safeCatalogFileName(path.basename(sourcePath));
  if (!fileName) {
    return null;
  }

  let stats = null;
  try {
    stats = fs.statSync(sourcePath);
  } catch (_error) {
    return null;
  }

  const meta = metaByFileName.get(fileName.toLowerCase()) || {};
  const artworkUrl = cleanText(meta.artwork_url || catalogArtworkUrlFor(meta.title || fileName), 400);
  return {
    key: cleanText(meta.key || catalogTrackKey(fileName), 80),
    file_name: fileName,
    title: cleanText(meta.title || normalizeTrackTitle(fileName), 220),
    description: cleanText(meta.description || "North3rnLight3r Beat", 360),
    bpm: cleanText(meta.bpm || meta.tempo_bpm || "", 24),
    key_signature: cleanText(meta.key_signature || meta.musical_key || "", 24),
    tempo: cleanText(meta.tempo || meta.bpm || meta.tempo_bpm || "", 24),
    genre: cleanText(meta.genre || "", 64),
    price: cleanText(meta.price || "$19.99", 32),
    size: Number(meta.size || stats.size || 0),
    uploaded_at: meta.uploaded_at || meta.updated_at || (stats.mtime ? stats.mtime.toISOString() : nowIso()),
    artwork_url: artworkUrl,
    full_song: true,
    source_path: sourcePath,
    managed_source: managedSource
  };
}

function normalizeCatalogTrackForPlayback(track) {
  const fileName = safeCatalogFileName(track?.file_name || "");
  const key = cleanText(track?.key || catalogTrackKey(fileName), 80);
  const versionToken = trackVersionToken(track);
  const streamUrl = buildCatalogStreamUrl(key, versionToken);
  const seededAssetFileName = safeCatalogFileName(track?.asset_file_name || "");
  const assetFileName = findCatalogAssetFileName(fileName) || seededAssetFileName;
  const publicUrl = buildCatalogPublicUrl(assetFileName || fileName, versionToken);
  return {
    key,
    file_name: fileName,
    asset_file_name: assetFileName,
    title: cleanText(track?.title || normalizeTrackTitle(fileName), 220),
    description: cleanText(track?.description || "North3rnLight3r Beat", 360),
    bpm: cleanText(track?.bpm || track?.tempo_bpm || "", 24),
    key_signature: cleanText(track?.key_signature || track?.musical_key || "", 24),
    tempo: cleanText(track?.tempo || track?.bpm || track?.tempo_bpm || "", 24),
    genre: cleanText(track?.genre || "", 64),
    price: cleanText(track?.price || "$19.99", 32),
    size: Number(track?.size || 0),
    uploaded_at: track?.uploaded_at || nowIso(),
    artwork_url: cleanText(track?.artwork_url || catalogArtworkUrlFor(track?.title || fileName), 400),
    stream_url: streamUrl,
    public_url: publicUrl,
    full_song_url: publicUrl,
    full_song: true
  };
}

function loadCatalogData() {
  const metaByFileName = buildCatalogMetadataMap();
  const files = [...EXTERNAL_CATALOG_DIRS, CATALOG_DIR].flatMap((directory) => listCatalogFilesIn(directory));

  const seen = new Set();
  const tracks = [];
  for (const sourcePath of files) {
    const fileName = safeCatalogFileName(path.basename(sourcePath));
    const dedupeKey = fileName.toLowerCase();
    if (!dedupeKey || seen.has(dedupeKey)) {
      continue;
    }
    const managedSource = sourcePath.startsWith(CATALOG_DIR + path.sep);
    const track = buildCatalogTrackFromSource(sourcePath, metaByFileName, managedSource);
    if (!track) {
      continue;
    }
    seen.add(dedupeKey);
    tracks.push(track);
  }

  if (tracks.length > 0) {
    return tracks;
  }

  const fallback = [];
  for (const entry of loadCatalogMetadataEntries()) {
    const fileName = safeCatalogFileName(entry.file_name || "");
    const sourcePath = resolveCatalogSourcePath(fileName);
    if (!fileName || !sourcePath || seen.has(fileName.toLowerCase())) {
      continue;
    }
    const track = buildCatalogTrackFromSource(sourcePath, metaByFileName, sourcePath.startsWith(CATALOG_DIR + path.sep));
    if (!track) {
      continue;
    }
    seen.add(fileName.toLowerCase());
    fallback.push(track);
  }
  return fallback;
}

function findCatalogTrackByKey(key) {
  const safeKey = cleanText(key || "", 80);
  if (!safeKey) {
    return null;
  }
  return loadCatalogData().find((track) => track.key === safeKey) || null;
}

function resolveCatalogSourcePath(fileName) {
  const safeName = safeCatalogFileName(fileName);
  if (!safeName) {
    return null;
  }

  const candidates = [...EXTERNAL_CATALOG_DIRS, EXTERNAL_CATALOG_DIR, CATALOG_DIR]
    .filter(Boolean)
    .map((directory) => path.join(directory, safeName));

  for (const candidate of candidates) {
    try {
      if (candidate && fs.existsSync(candidate) && fs.statSync(candidate).isFile()) {
        return candidate;
      }
    } catch (_error) {
    }
  }

  return null;
}

function buildSoundpackPublicUrl(fileName) {
  return `/media/soundpacks/${encodeURIComponent(fileName)}`;
}

function memoryFile(sessionId) {
  const safe = String(sessionId || "default").replace(/[^a-zA-Z0-9._\-]/g, "_");
  return path.join(MEMORY_DIR, `${safe}.json`);
}

function loadMemory(sessionId) {
  const file = memoryFile(sessionId);
  return readJson(file, {
    session_id: sessionId,
    updated_at: nowIso(),
    openai_previous_response_id: "",
    topic_weights: {},
    accepted_actions: {},
    rejected_actions: {},
    interactions: []
  });
}

function clearMemory(sessionId) {
  const file = memoryFile(sessionId);
  try {
    if (fs.existsSync(file)) {
      fs.rmSync(file, { force: true });
    }
    return true;
  } catch (_error) {
    return false;
  }
}

function saveMemory(sessionId, memory) {
  const file = memoryFile(sessionId);
  const settings = loadChatSettings();
  memory.updated_at = nowIso();
  memory.interactions = Array.isArray(memory.interactions)
    ? memory.interactions.slice(-Number(settings.context.memory_window_items || 40))
    : [];
  writeJson(file, memory);
}

function updateMemoryFromChat(memory, prompt, structured) {
  const next = { ...memory };
  const tags = Array.isArray(structured?.state_update?.focus_tags)
    ? structured.state_update.focus_tags
    : [];

  tags.forEach((tag) => {
    const key = String(tag || "").trim().toLowerCase();
    if (!key) {
      return;
    }
    next.topic_weights[key] = Number(next.topic_weights[key] || 0) + 1;
  });

  const interaction = {
    ts: nowIso(),
    prompt: String(prompt || "").slice(0, 500),
    summary: String(structured.summary || "").slice(0, 1200),
    issue_count: Array.isArray(structured.issues) ? structured.issues.length : 0,
    suggestion_count: Array.isArray(structured.action_suggestions)
      ? structured.action_suggestions.length
      : 0
  };

  next.interactions = Array.isArray(next.interactions) ? next.interactions.concat(interaction) : [interaction];
  return next;
}

function applyMemoryFeedback(memory, actionId, accepted) {
  const next = { ...memory };
  const key = String(actionId || "").trim();
  if (!key) {
    return next;
  }

  if (accepted) {
    next.accepted_actions[key] = Number(next.accepted_actions[key] || 0) + 1;
  } else {
    next.rejected_actions[key] = Number(next.rejected_actions[key] || 0) + 1;
  }

  return next;
}

function summarizeMemory(memory, summaryItems = 6) {
  const topics = Object.entries(memory.topic_weights || {})
    .sort((a, b) => Number(b[1]) - Number(a[1]))
    .slice(0, 8)
    .map(([k, v]) => `${k}:${v}`)
    .join(", ");

  const accepted = Object.entries(memory.accepted_actions || {})
    .sort((a, b) => Number(b[1]) - Number(a[1]))
    .slice(0, 6)
    .map(([k, v]) => `${k}:${v}`)
    .join(", ");

  const rejected = Object.entries(memory.rejected_actions || {})
    .sort((a, b) => Number(b[1]) - Number(a[1]))
    .slice(0, 6)
    .map(([k, v]) => `${k}:${v}`)
    .join(", ");

  return {
    topics,
    accepted,
    rejected,
    recent: (memory.interactions || []).slice(-Math.max(1, Number(summaryItems) || 6))
  };
}

function extractTextResult(payload) {
  const result = payload?.result;
  if (typeof result?.response === "string") {
    return result.response;
  }
  if (Array.isArray(result?.choices) && typeof result.choices[0]?.message?.content === "string") {
    return result.choices[0].message.content;
  }
  return "";
}

function parseJsonFromText(text) {
  const content = String(text || "").trim();
  const start = content.indexOf("{");
  const end = content.lastIndexOf("}");
  if (start < 0 || end < 0 || end <= start) {
    return null;
  }
  const jsonBlock = content.slice(start, end + 1);
  try {
    return JSON.parse(jsonBlock);
  } catch (_error) {
    return null;
  }
}

function validateStructuredResponse(value) {
  if (!value || typeof value !== "object") {
    return { ok: false, error: "structured response missing" };
  }

  if (typeof value.summary !== "string" || !value.summary.trim()) {
    return { ok: false, error: "summary missing" };
  }

  if (!Array.isArray(value.issues) || !Array.isArray(value.action_suggestions)) {
    return { ok: false, error: "issues/action_suggestions missing" };
  }

  for (const issue of value.issues) {
    if (!issue || typeof issue !== "object") {
      return { ok: false, error: "invalid issue object" };
    }
    if (typeof issue.summary !== "string" || typeof issue.issue_type !== "string") {
      return { ok: false, error: "issue fields missing" };
    }
  }

  for (const suggestion of value.action_suggestions) {
    if (!suggestion || typeof suggestion !== "object") {
      return { ok: false, error: "invalid action suggestion" };
    }
    if (typeof suggestion.id !== "string" || typeof suggestion.label !== "string") {
      return { ok: false, error: "action suggestion fields missing" };
    }
  }

  return { ok: true };
}

function extractOpenAiText(payload) {
  if (!payload || typeof payload !== "object") {
    return "";
  }
  if (typeof payload.output_text === "string" && payload.output_text.trim()) {
    return payload.output_text.trim();
  }
  if (Array.isArray(payload.output_text)) {
    const joined = payload.output_text.join("\n").trim();
    if (joined) {
      return joined;
    }
  }
  if (Array.isArray(payload.choices)) {
    const message = payload.choices[0]?.message;
    if (typeof message?.content === "string") {
      return message.content;
    }
    if (Array.isArray(message?.content)) {
      const joined = message.content
        .map((item) => (typeof item?.text === "string" ? item.text : ""))
        .filter(Boolean)
        .join("\n")
        .trim();
      if (joined) {
        return joined;
      }
    }
  }
  if (Array.isArray(payload.output)) {
    const joined = payload.output
      .map((item) =>
        Array.isArray(item?.content)
          ? item.content.map((c) => (typeof c?.text === "string" ? c.text : "")).filter(Boolean).join("\n")
          : ""
      )
      .filter(Boolean)
      .join("\n")
      .trim();
    if (joined) {
      return joined;
    }
  }
  return "";
}

async function openaiStructuredChat(prompt, sessionId, memorySummary, options = {}) {
  const cfg = loadOpenAiConfig();
  const settings = loadChatSettings();
  if (!cfg.ready) {
    await emitChatWebhook("chat.failed", {
      ok: false,
      provider: "openai",
      session_id: sessionId,
      model: cfg.model || "unset",
      error: "OpenAI API key missing — AIFR3D chat unavailable"
    });
    return {
      ok: false,
      error: "OpenAI API key missing — AIFR3D chat unavailable",
      provider: "openai",
      model: cfg.model || "unset",
      attempted_models: activeOpenAiModelList()
    };
  }

  const model = normalizeRequestedOpenAiModel(options?.model || cfg.model);
  const sessionMemory = loadMemory(sessionId);
  const personalityMode = cleanText(
    options?.personality_mode || settings.prompt.personality_mode || AIFR3D_BRAIN?.identity?.mode || "professional_mentor",
    80
  );
  const agentModes = normalizeModelList(options?.agent_modes || []);
  const promptMaxChars = Number(settings.context.max_prompt_chars || AIFR3D_BRAIN?.runtime?.max_prompt_chars || 4000);
  const promptText = String(prompt || "").slice(0, promptMaxChars);
  const maxTokens = Number(settings.response.max_output_tokens || AIFR3D_BRAIN?.runtime?.max_tokens || 900);
  const systemPrompt = buildSystemPrompt(personalityMode, agentModes, settings);
  const compactedRecent = Array.isArray(memorySummary?.recent)
    ? memorySummary.recent.slice(-Number(settings.context.compact_threshold || 12))
    : [];

  const userPayload = {
    prompt: promptText,
    session_id: sessionId,
    memory: {
      ...(memorySummary && typeof memorySummary === "object" ? memorySummary : {}),
      recent: compactedRecent
    },
    aifred_brain: {
      brain_id: AIFR3D_BRAIN?.brain_id || "aifr3d_brain_v2_2_4",
      version: AIFR3D_BRAIN?.version || "2.2.4",
      personality_mode: personalityMode,
      agent_modes: agentModes.length > 0 ? agentModes : ["mix_engineer", "analysis_worker", "planning_worker"]
    },
    constraints: {
      online_only: true,
      no_fallback_phrases: true,
      deterministic_schema: true,
      keep_cpu_ram_low: true
    },
    runtime_preferences: {
      transport_mode: settings.transport_mode,
      verbosity: settings.response.verbosity,
      tone: settings.prompt.tone,
      reasoning_effort: settings.reasoning.effort
    }
  };

  const useChatCompletions = /\/chat\/completions$/i.test(cfg.base_url);
  const previousResponseId = settings.context.use_previous_response_id
    ? cleanText(sessionMemory?.openai_previous_response_id || "", 200)
    : "";
  try {
    const body = useChatCompletions
      ? {
          model,
          messages: [
            { role: "system", content: systemPrompt },
            { role: "user", content: JSON.stringify(userPayload) }
          ],
          max_tokens: maxTokens,
          response_format: { type: "json_object" }
        }
      : {
          model,
          input: [
            {
              role: "system",
              content: [{ type: "input_text", text: systemPrompt }]
            },
            {
              role: "user",
              content: [{ type: "input_text", text: JSON.stringify(userPayload) }]
            }
          ],
          max_output_tokens: maxTokens,
          store: settings.context.use_previous_response_id || settings.webhook.enabled,
          text: {
            format: { type: "json_object" },
            verbosity: settings.response.verbosity
          }
        };
    if (!useChatCompletions && settings.reasoning.enabled) {
      body.reasoning = {
        effort: settings.reasoning.effort
      };
    }
    if (!useChatCompletions && previousResponseId) {
      body.previous_response_id = previousResponseId;
    }
    const response = await fetch(cfg.base_url, {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
        Authorization: `Bearer ${cfg.api_key}`
      },
      body: JSON.stringify(body)
    });
    const payload = await response.json().catch(() => null);
    const rawText = extractOpenAiText(payload);
    if (!response.ok || !rawText) {
      await emitChatWebhook("chat.failed", {
        ok: false,
        provider: "openai",
        session_id: sessionId,
        model,
        attempted_models: [model],
        error: payload?.error?.message || payload?.message || "OpenAI request failed — AIFR3D chat unavailable"
      });
      return {
        ok: false,
        error: payload?.error?.message || payload?.message || "OpenAI request failed — AIFR3D chat unavailable",
        provider: "openai",
        model,
        attempted_models: [model]
      };
    }
    const parsed = parseJsonFromText(rawText);
    const validation = validateStructuredResponse(parsed);
    if (!validation.ok) {
      await emitChatWebhook("chat.failed", {
        ok: false,
        provider: "openai",
        session_id: sessionId,
        model,
        attempted_models: [model],
        error: `OpenAI structured output invalid: ${validation.error}`
      });
      return {
        ok: false,
        error: `OpenAI structured output invalid: ${validation.error}`,
        provider: "openai",
        model,
        attempted_models: [model]
      };
    }
    const responseId = cleanText(payload?.id || "", 200);
    await emitChatWebhook("chat.completed", {
      ok: true,
      provider: "openai",
      session_id: sessionId,
      model,
      attempted_models: [model],
      response_id: responseId,
      summary: parsed.summary,
      issue_count: Array.isArray(parsed.issues) ? parsed.issues.length : 0,
      action_suggestion_count: Array.isArray(parsed.action_suggestions) ? parsed.action_suggestions.length : 0
    });
    return {
      ok: true,
      provider: "openai",
      model,
      attempted_models: [model],
      response_id: responseId,
      data: parsed
    };
  } catch (error) {
    await emitChatWebhook("chat.failed", {
      ok: false,
      provider: "openai",
      session_id: sessionId,
      model,
      attempted_models: [model],
      error: String(error?.message || error) || "OpenAI request failed — AIFR3D chat unavailable"
    });
    return {
      ok: false,
      error: String(error?.message || error) || "OpenAI request failed — AIFR3D chat unavailable",
      provider: "openai",
      model,
      attempted_models: [model]
    };
  }
}

function buildAndroidApkCandidates() {
  const configured = cleanText(process.env.DAWAI_ANDROID_APK_PATH || "", 1000);
  const candidates = [
    configured,
    path.join(ROOT_DIR, "apps/android_admin/app/build/outputs/apk/release/app-release.apk"),
    path.join(ROOT_DIR, "apps/android_admin/app/build/outputs/apk/debug/app-debug.apk")
  ].filter(Boolean);
  return candidates;
}

function buildStandaloneCandidates() {
  const configured = cleanText(process.env.DAWAI_STANDALONE_PATH || "", 1000);
  return [
    configured,
    path.join(ROOT_DIR, "dist/dawai_standalone.exe"),
    path.join(ROOT_DIR, "build/Release/dawai_standalone.exe"),
    path.join(ROOT_DIR, "build/dawai_standalone"),
    path.join(ROOT_DIR, "build/dawai_standalone.exe")
  ].filter(Boolean);
}

function buildVstCandidates() {
  const configured = cleanText(process.env.DAWAI_VST_PATH || "", 1000);
  return [
    configured,
    path.join(ROOT_DIR, "dist/DawAI.vst3"),
    path.join(ROOT_DIR, "build/Release/DawAI.vst3"),
    path.join(ROOT_DIR, "build/DawAI.vst3"),
    path.join(ROOT_DIR, "build/dawai_vst3_stub"),
    path.join(ROOT_DIR, "build/Release/dawai_vst3_stub.exe"),
    path.join(ROOT_DIR, "build/dawai_vst3_stub.exe")
  ].filter(Boolean);
}

function normalizeVariantSuffix(value) {
  const v = String(value || "").trim().toLowerCase();
  if (v === "a" || v === "b" || v === "c" || v === "d") {
    return v;
  }
  return "";
}

function buildStandaloneVariantCandidates(variant) {
  const suffix = normalizeVariantSuffix(variant);
  if (!suffix) {
    return [];
  }
  const envKey = `DAWAI_STANDALONE_PATH_${suffix.toUpperCase()}`;
  const configured = cleanText(process.env[envKey] || "", 1000);
  return [
    configured,
    path.join(ROOT_DIR, `dist/dawai_standalone_${suffix}.exe`),
    path.join(ROOT_DIR, `dist/dawai_standalone_${suffix}`),
    path.join(ROOT_DIR, `build/dawai_standalone_${suffix}.exe`),
    path.join(ROOT_DIR, `build/dawai_standalone_${suffix}`)
  ].filter(Boolean);
}

function buildVstVariantCandidates(variant) {
  const suffix = normalizeVariantSuffix(variant);
  if (!suffix) {
    return [];
  }
  const envKey = `DAWAI_VST_PATH_${suffix.toUpperCase()}`;
  const configured = cleanText(process.env[envKey] || "", 1000);
  return [
    configured,
    path.join(ROOT_DIR, `dist/DawAI_${suffix.toUpperCase()}.vst3`),
    path.join(ROOT_DIR, `dist/DawAI_${suffix}.vst3`),
    path.join(ROOT_DIR, `build/DawAI_${suffix.toUpperCase()}.vst3`),
    path.join(ROOT_DIR, `build/DawAI_${suffix}.vst3`)
  ].filter(Boolean);
}

function buildLinuxDebCandidates() {
  const configured = cleanText(process.env.DAWAI_LINUX_DEB_PATH || "", 1000);
  return [
    configured,
    path.join(ROOT_DIR, "dist/dawai-linux-amd64.deb"),
    path.join(WEBSITE_DIR, "downloads/dawai-linux-amd64.deb")
  ].filter(Boolean);
}

function buildLinuxArchCandidates() {
  const configured = cleanText(process.env.DAWAI_LINUX_ARCH_PATH || "", 1000);
  return [
    configured,
    path.join(ROOT_DIR, "dist/dawai-linux-arch-x86_64.pkg.tar.zst"),
    path.join(WEBSITE_DIR, "downloads/dawai-linux-arch-x86_64.pkg.tar.zst")
  ].filter(Boolean);
}

function resolveFirstExisting(candidates) {
  return candidates.find((candidate) => fs.existsSync(candidate) && fs.statSync(candidate).isFile()) || null;
}

function sendDownloadFile(res, filePath, fileName, contentType) {
  if (!filePath) {
    res.status(404).json({ ok: false, error: `${fileName} not found on server` });
    return;
  }

  if (contentType) {
    res.setHeader("Content-Type", contentType);
  }
  res.setHeader("Content-Disposition", `attachment; filename=${fileName}`);
  res.sendFile(filePath);
}

ensureDir(STORAGE_DIR);
ensureDir(UPLOAD_DIR);
ensureDir(CATALOG_DIR);
ensureDir(SOUNDPACK_DIR);
ensureDir(REFERENCE_UPLOAD_ROOT);
ensureDir(MEMORY_DIR);
ensureDir(RECEIPTS_DIR);
ensureFile(CONTENT_FILE, DEFAULT_CONTENT);
ensureFile(CATALOG_FILE, []);
ensureFile(SOUNDPACK_FILE, []);
ensureFile(SESSION_FILE, []);
ensureFile(ADMIN_CREDENTIALS_FILE, defaultAdminCredentials());
ensureFile(PROMO_CODES_FILE, normalizePromoCodes(DEFAULT_PROMO_CODES));
ensureFile(OPENAI_CONFIG_FILE, defaultOpenAiConfig());
ensureFile(INQUIRIES_FILE, []);
ensureFile(SALES_FILE, []);
if (!fs.existsSync(EVENT_LOG_FILE)) {
  fs.writeFileSync(EVENT_LOG_FILE, "", "utf8");
}
if (!fs.existsSync(ADMIN_LOG_FILE)) {
  fs.writeFileSync(ADMIN_LOG_FILE, "", "utf8");
}
const initialCatalog = readJson(CATALOG_FILE, []);
if (Array.isArray(initialCatalog) && initialCatalog.length === 0) {
  const seedCatalog = readJson(CATALOG_SEED_FILE, []);
  if (Array.isArray(seedCatalog) && seedCatalog.length > 0) {
    writeJson(CATALOG_FILE, seedCatalog);
  }
}

const app = express();
const server = http.createServer(app);
const wss = new WebSocketServer({ noServer: true });

app.use(cors());
app.use(express.json({ limit: "25mb" }));
app.use(express.urlencoded({ extended: true }));
app.use((req, res, next) => {
  if (!sitePrivateModeEnabled()) {
    next();
    return;
  }
  if (isMaintenanceBypassPath(req.path) || hasMaintenanceAccess(req)) {
    next();
    return;
  }
  if (req.path.startsWith("/api/") || req.path === "/ws/chat") {
    res.status(503).json({
      ok: false,
      error: "website is temporarily private for maintenance",
      maintenance_mode: true
    });
    return;
  }
  res.status(503).set("Cache-Control", "no-store").set("Retry-After", "3600").type("html").send(maintenanceHtml());
});

app.use((req, res, next) => {
  if (req.method !== "GET") {
    next();
    return;
  }
  const routePath = String(req.path || "/").trim() || "/";
  res.on("finish", () => {
    if (res.statusCode >= 400) {
      return;
    }
    if (routePath.startsWith("/downloads") || routePath.startsWith("/download/") || routePath.startsWith("/api/v1/pay/download")) {
      recordDashboardActivity("download", routePath);
      return;
    }
    if (routePath.startsWith("/media/")) {
      recordDashboardActivity("media_stream", routePath);
      return;
    }
    if (routePath.startsWith("/api/")) {
      recordDashboardActivity("api_hit", routePath);
      return;
    }
    if (routePath === "/" || routePath.endsWith(".html") || !routePath.slice(1).includes(".")) {
      recordDashboardActivity("page_view", routePath);
    }
  });
  next();
});

const upload = multer({
  storage: multer.diskStorage({
    destination: (_req, _file, cb) => cb(null, CATALOG_DIR),
    filename: (_req, file, cb) => {
      const stamp = Date.now();
      const safe = file.originalname.replace(/[^a-zA-Z0-9._\- ]/g, "_");
      cb(null, `${stamp}_${safe}`);
    }
  }),
  limits: {
    fileSize: 1024 * 1024 * 512
  }
});

const soundpackUpload = multer({
  storage: multer.diskStorage({
    destination: (_req, _file, cb) => cb(null, SOUNDPACK_DIR),
    filename: (_req, file, cb) => {
      const stamp = Date.now();
      const safe = file.originalname.replace(/[^a-zA-Z0-9._\- ]/g, "_");
      cb(null, `${stamp}_${safe}`);
    }
  }),
  limits: {
    fileSize: 1024 * 1024 * 512
  }
});

const referenceUpload = multer({
  storage: multer.diskStorage({
    destination: (req, _file, cb) => {
      const genre = normalizeReferenceGenreName(req.body?.genre || req.body?.reference_genre || "");
      if (!genre) {
        cb(new Error("reference genre is required"));
        return;
      }
      const targetDir = path.join(REFERENCE_UPLOAD_ROOT, genre);
      ensureDir(targetDir);
      cb(null, targetDir);
    },
    filename: (_req, file, cb) => {
      const stamp = Date.now();
      const safe = file.originalname.replace(/[^a-zA-Z0-9._\- ]/g, "_");
      cb(null, `${stamp}_${safe}`);
    }
  }),
  limits: {
    fileSize: 1024 * 1024 * 512
  }
});

const genericUpload = multer({
  storage: multer.diskStorage({
    destination: (_req, _file, cb) => cb(null, UPLOAD_DIR),
    filename: (_req, file, cb) => {
      const stamp = Date.now();
      const safe = file.originalname.replace(/[^a-zA-Z0-9._\- ]/g, "_");
      cb(null, `${stamp}_${safe}`);
    }
  }),
  limits: {
    fileSize: 1024 * 1024 * 512
  }
});

app.get("/api/v1/health", (_req, res) => {
  const adminCreds = loadAdminCredentials();
  const openai = loadOpenAiConfig();
  const models = activeOpenAiModelList(openai);
  const inquiries = readJson(INQUIRIES_FILE, []);
  const sales = readJson(SALES_FILE, []);
  const logs = readEventLogs(200);
  res.json({
    ok: true,
    service: "dawai-ecosystem",
    mode: "http+websocket",
    timestamp_utc: nowIso(),
    actions: Object.keys(ACTION_REGISTRY),
    chat_provider: "openai",
    openai_model: openai.model || null,
    openai_model_list: activeOpenAiModelList(openai),
    openai_ready: openai.ready,
    chat_model_list: models,
    aifred_brain_id: AIFR3D_BRAIN?.brain_id || "aifr3d_brain_v2_2_4",
    aifred_brain_version: AIFR3D_BRAIN?.version || "2.2.4",
    file_access_root: path.relative(ROOT_DIR, FILE_ACCESS_ROOT) || ".",
    admin_configured: Boolean(adminCreds.username && adminCreds.password_hash),
    promo_codes_enabled: false,
    inquiries_count: Array.isArray(inquiries) ? inquiries.length : 0,
    sales_count: Array.isArray(sales) ? sales.length : 0,
    event_log_count: logs.length,
    website_private_mode: sitePrivateModeEnabled(),
    checkout_provider: paymentProvider(),
    paypal_ready: paypalReady(),
    paypal_mode: PAYPAL_ENV,
    default_online_api: DEFAULT_ONLINE_API,
    default_online_ws: DEFAULT_ONLINE_WS
  });
});

app.get("/api/v1/registry/actions", (_req, res) => {
  res.json({ ok: true, actions: Object.values(ACTION_REGISTRY) });
});

app.get("/api/v1/brain/config", requireApiToken, (_req, res) => {
  res.json({
    ok: true,
    brain: AIFR3D_BRAIN
  });
});

app.get("/api/v1/limits", requireApiToken, (_req, res) => {
  const cfg = loadOpenAiConfig();
  res.json({
    ok: true,
    limits: {
      ...API_LIMITS,
      chat: {
        ...API_LIMITS.chat,
        active_models: activeOpenAiModelList(cfg)
      }
    }
  });
});

app.get("/api/v1/models/list", (_req, res) => {
  const cfg = loadOpenAiConfig();
  res.json({
    ok: true,
    provider: "openai",
    active_model: cfg.model || null,
    models: activeOpenAiModelList(cfg)
  });
});

app.get("/api/v1/chat/settings", requireApiToken, (_req, res) => {
  res.json({
    ok: true,
    provider: "openai",
    websocket_url: DEFAULT_ONLINE_WS,
    settings: loadChatSettings()
  });
});

app.get("/api/v1/pay/products", (_req, res) => {
  res.json({
    ok: true,
    provider: paymentProvider(),
    products: DEFAULT_STORE_PRODUCTS.map((item) => ({
      ...item,
      priceUSD: item.beta_free_trial ? null : item.priceUSD,
      beta_free_trial: item.beta_free_trial === true,
      availability_label: item.availability_label || "",
      future_price_label: item.future_price_label || "",
      download_url: item.beta_free_trial ? item.download_url : ""
    }))
  });
});

app.post("/api/v1/pay/create-order", async (req, res) => {
  const sku = normalizeSku(req.body?.sku || "");
  const product = DEFAULT_STORE_PRODUCTS.find((item) => item.sku === sku) || null;
  const itemName = cleanText(req.body?.item_name || product?.name || "North3rnLight3r Product", 220);
  const amount = normalizeUsdAmount(req.body?.amount || product?.priceUSD || "0");
  const currency = normalizeCurrencyCode(req.body?.currency || product?.currency || "USD");

  if (product?.beta_free_trial) {
    res.status(409).json({
      ok: false,
      error: BETA_FREE_TRIAL_MESSAGE,
      beta_free_trial: true,
      download_url: product.download_url || ""
    });
    return;
  }

  if (!itemName || amount === "0.00") {
    res.status(400).json({ ok: false, error: "valid sku or item_name+amount required" });
    return;
  }

  if (!paypalReady()) {
    res.status(503).json({
      ok: false,
      error: "paypal backend not configured",
      provider: paymentProvider(),
      fallback_url: checkoutFallbackUrl(itemName, amount, currency)
    });
    return;
  }

  try {
    const token = await paypalAccessToken();
    const response = await fetch(`${paypalBaseUrl()}/v2/checkout/orders`, {
      method: "POST",
      headers: {
        Authorization: `Bearer ${token}`,
        "Content-Type": "application/json"
      },
      body: JSON.stringify({
        intent: "CAPTURE",
        purchase_units: [
          {
            reference_id: sku || "direct",
            custom_id: sku || "",
            description: itemName,
            amount: {
              currency_code: currency,
              value: amount
            }
          }
        ],
        application_context: {
          brand_name: "North3rnLight3r",
          user_action: "PAY_NOW"
        }
      })
    });
    const payload = await response.json().catch(() => null);
    if (!response.ok || !payload?.id) {
      res.status(502).json({
        ok: false,
        error: payload?.message || payload?.error || "paypal create-order failed",
        fallback_url: checkoutFallbackUrl(itemName, amount, currency)
      });
      return;
    }

    const approvalUrl = Array.isArray(payload.links) ? payload.links.find((entry) => entry?.rel === "approve")?.href : "";
    res.json({
      ok: true,
      provider: "paypal",
      order_id: payload.id,
      status: payload.status || "CREATED",
      approval_url: approvalUrl || checkoutFallbackUrl(itemName, amount, currency)
    });
  } catch (error) {
    res.status(502).json({
      ok: false,
      error: cleanText(error?.message || "paypal create-order failed", 260),
      fallback_url: checkoutFallbackUrl(itemName, amount, currency)
    });
  }
});

app.post("/api/v1/pay/capture-order", async (req, res) => {
  const orderId = cleanText(req.body?.order_id || req.body?.token || "", 120);
  if (!orderId) {
    res.status(400).json({ ok: false, error: "order_id is required" });
    return;
  }

  if (!paypalReady()) {
    res.status(503).json({
      ok: false,
      error: "paypal backend not configured",
      provider: paymentProvider()
    });
    return;
  }

  try {
    const token = await paypalAccessToken();
    const response = await fetch(`${paypalBaseUrl()}/v2/checkout/orders/${encodeURIComponent(orderId)}/capture`, {
      method: "POST",
      headers: {
        Authorization: `Bearer ${token}`,
        "Content-Type": "application/json"
      }
    });
    const payload = await response.json().catch(() => null);
    if (!response.ok || !payload?.id) {
      res.status(502).json({
        ok: false,
        error: payload?.message || payload?.error || "paypal capture failed"
      });
      return;
    }

    const purchaseUnit = Array.isArray(payload.purchase_units) ? payload.purchase_units[0] || {} : {};
    const sku = normalizeSku(purchaseUnit?.custom_id || purchaseUnit?.reference_id || "");
    const product = DEFAULT_STORE_PRODUCTS.find((item) => item.sku === sku) || null;
    const status = cleanText(payload.status || "", 40);

    const unlockProducts = Array.isArray(product?.unlock_products)
      ? product.unlock_products.map((item) => normalizePromoProduct(item)).filter(Boolean)
      : [];
    const downloadTokens = {};
    const downloadUrls = {};
    unlockProducts.forEach((item) => {
      const tokenValue = createPromoDownloadToken(item);
      const basePath = promoProductDownloadPath(item);
      if (basePath) {
        downloadTokens[item] = tokenValue;
        downloadUrls[item] = `${basePath}?access_token=${encodeURIComponent(tokenValue)}`;
      }
    });

    res.json({
      ok: status === "COMPLETED",
      provider: "paypal",
      order_id: payload.id,
      status,
      unlock_products: unlockProducts,
      download_url: downloadUrls.vst || downloadUrls.standalone || "",
      download_tokens: downloadTokens,
      download_urls: downloadUrls,
      payment_unlock: loadPaymentUnlockBundle(),
      receipt: null
    });
  } catch (error) {
    res.status(502).json({
      ok: false,
      error: cleanText(error?.message || "paypal capture failed", 260)
    });
  }
});

app.post("/api/v1/pay/webhook", express.json({ limit: "2mb" }), (req, res) => {
  const eventType = cleanText(req.body?.event_type || "paypal.webhook.unknown", 160);
  const eventId = cleanText(req.body?.id || randomToken(8), 120);
  appendEventLog("paypal.webhook", { id: eventId, type: eventType });
  res.json({ ok: true, id: eventId, event_type: eventType });
});

app.post("/api/v1/command/run", requireAdmin, async (req, res) => {
  const result = await runCommand(req.body?.command_line || "");
  appendAdminLog(req.body?.command_line || "", {
    ok: result.ok,
    exit_code: result.exit_code,
    user: req.admin?.username || "admin",
    action_id: result.action_id || ""
  });
  appendEventLog("command.run", {
    ok: result.ok,
    exit_code: result.exit_code,
    command_line: cleanText(req.body?.command_line || "", 260),
    user: req.admin?.username || "admin"
  });
  res.status(result.ok ? 200 : 500).json(result);
});

app.post("/api/v1/chat/ask", requireApiToken, async (req, res) => {
  const prompt = String(req.body?.prompt || "").trim();
  const sessionId = String(req.body?.session_id || "default").trim();
  const requestedModel = String(req.body?.model || "").trim();
  const personalityMode = String(req.body?.personality_mode || "").trim();
  const agentModes = Array.isArray(req.body?.agent_modes) ? req.body.agent_modes : [];

  if (!prompt) {
    res.status(400).json({ ok: false, error: "prompt is empty" });
    return;
  }

  const memory = loadMemory(sessionId);
  const summary = summarizeMemory(memory, loadChatSettings().context.summary_items);
  const inference = await openaiStructuredChat(prompt, sessionId, summary, {
    model: requestedModel,
    personality_mode: personalityMode,
    agent_modes: agentModes
  });

  if (!inference.ok) {
    res.status(502).json({
      ok: false,
      error: inference.error,
      provider: inference.provider,
      model: inference.model,
      attempted_models: inference.attempted_models || []
    });
    return;
  }

  const updated = updateMemoryFromChat(memory, prompt, inference.data);
  updated.openai_previous_response_id = inference.response_id || updated.openai_previous_response_id || "";
  saveMemory(sessionId, updated);
  appendEventLog("chat.ask", {
    ok: true,
    session_id: sessionId,
    model: inference.model
  });

  res.json({
    ok: true,
    provider: inference.provider,
    model: inference.model,
    attempted_models: inference.attempted_models || [inference.model],
    brain_id: AIFR3D_BRAIN?.brain_id || "aifr3d_brain_v2_2_4",
    session_id: sessionId,
    summary: inference.data.summary,
    issues: inference.data.issues,
    action_suggestions: inference.data.action_suggestions,
    state_update: inference.data.state_update || {},
    timestamp_utc: nowIso()
  });
});

app.post("/api/v1/memory/feedback", requireApiToken, (req, res) => {
  const sessionId = String(req.body?.session_id || "default").trim();
  const actionId = String(req.body?.action_id || "").trim();
  const accepted = Boolean(req.body?.accepted);

  if (!actionId) {
    res.status(400).json({ ok: false, error: "action_id required" });
    return;
  }

  const memory = loadMemory(sessionId);
  const updated = applyMemoryFeedback(memory, actionId, accepted);
  saveMemory(sessionId, updated);
  res.json({ ok: true, session_id: sessionId, action_id: actionId, accepted });
});

app.post("/api/v1/memory/clear", requireApiToken, (req, res) => {
  const sessionId = String(req.body?.session_id || "default").trim();
  clearMemory(sessionId);
  res.json({ ok: true, session_id: sessionId });
});

app.get("/api/v1/memory/state", requireApiToken, (req, res) => {
  const sessionId = String(req.query.session_id || "default").trim();
  const memory = loadMemory(sessionId);
  res.json({ ok: true, session_id: sessionId, memory: summarizeMemory(memory, loadChatSettings().context.summary_items) });
});

app.post("/api/v1/files/upload", requireApiToken, genericUpload.single("file"), (req, res) => {
  if (!req.file) {
    res.status(400).json({ ok: false, error: "file is required" });
    return;
  }

  res.json({
    ok: true,
    name: req.file.filename,
    stored_path: path.relative(ROOT_DIR, req.file.path),
    size: req.file.size,
    timestamp_utc: nowIso()
  });
  appendEventLog("files.upload", {
    name: req.file.filename,
    size: req.file.size
  });
});

app.get("/api/v1/catalog/list", (_req, res) => {
  const tracks = loadCatalogData();
  const normalizedTracks = Array.isArray(tracks) ? tracks.map((track) => normalizeCatalogTrackForPlayback(track)) : [];
  res.set("Cache-Control", "no-store");
  res.json({ ok: true, tracks: normalizedTracks });
});

app.get("/api/v1/catalog/stream/:key", (req, res) => {
  const target = findCatalogTrackByKey(req.params.key || "");
  if (!target?.source_path || !fs.existsSync(target.source_path)) {
    res.status(404).json({ ok: false, error: "track not found" });
    return;
  }
  const stat = fs.statSync(target.source_path);
  const totalSize = stat.size;
  const contentType = mimeTypeForFile(target.source_path);
  const rangeHeader = String(req.headers.range || "").trim();

  res.setHeader("Content-Type", contentType);
  res.setHeader("Accept-Ranges", "bytes");
  res.setHeader("Cache-Control", "public, max-age=3600");

  if (!rangeHeader) {
    res.setHeader("Content-Length", totalSize);
    fs.createReadStream(target.source_path).pipe(res);
    return;
  }

  const match = /^bytes=(\d*)-(\d*)$/i.exec(rangeHeader);
  if (!match) {
    res.status(416).setHeader("Content-Range", `bytes */${totalSize}`).end();
    return;
  }

  const start = match[1] ? Number(match[1]) : 0;
  const end = match[2] ? Number(match[2]) : totalSize - 1;
  if (!Number.isFinite(start) || !Number.isFinite(end) || start < 0 || end < start || start >= totalSize) {
    res.status(416).setHeader("Content-Range", `bytes */${totalSize}`).end();
    return;
  }

  const safeEnd = Math.min(end, totalSize - 1);
  const chunkSize = safeEnd - start + 1;
  res.status(206);
  res.setHeader("Content-Length", chunkSize);
  res.setHeader("Content-Range", `bytes ${start}-${safeEnd}/${totalSize}`);
  fs.createReadStream(target.source_path, { start, end: safeEnd }).pipe(res);
});

app.get("/api/v1/soundpacks/list", (_req, res) => {
  const packs = readJson(SOUNDPACK_FILE, []);
  res.json({ ok: true, soundpacks: packs });
});

app.get("/api/v1/content/get", (_req, res) => {
  const content = normalizeContent(readJson(CONTENT_FILE, DEFAULT_CONTENT));
  res.json({ ok: true, content });
});

app.post("/api/v1/inquiries/submit", (req, res) => {
  const name = cleanText(req.body?.name || "", 120);
  const email = cleanText(req.body?.email || "", 180);
  const message = String(req.body?.message || "").trim().slice(0, 4000);
  if (!name || !email || !message) {
    res.status(400).json({ ok: false, error: "name, email, and message are required" });
    return;
  }

  const inquiries = readJson(INQUIRIES_FILE, []);
  const inquiry = {
    inquiry_id: randomToken(8),
    name,
    email,
    message,
    created_at: nowIso(),
    status: "new"
  };
  inquiries.unshift(inquiry);
  writeJson(INQUIRIES_FILE, Array.isArray(inquiries) ? inquiries.slice(0, 1000) : [inquiry]);
  appendEventLog("inquiry.submit", {
    inquiry_id: inquiry.inquiry_id,
    email_hash: hashHex(email).slice(0, 16)
  });
  res.json({
    ok: true,
    inquiry_id: inquiry.inquiry_id,
    created_at: inquiry.created_at,
    target_email: INQUIRY_TARGET_EMAIL,
    relay_ready: false
  });
});

app.get("/api/v1/receipts/verify", (req, res) => {
  const receiptId = cleanText(req.query.receipt_id || "", 120);
  const signature = cleanText(req.query.signature || "", 260);
  if (!receiptId || !signature) {
    res.status(400).json({ ok: false, error: "receipt_id and signature are required" });
    return;
  }
  const receiptPath = path.join(RECEIPTS_DIR, `${receiptId}.json`);
  if (!fs.existsSync(receiptPath)) {
    res.status(404).json({ ok: false, error: "receipt not found" });
    return;
  }
  const receipt = readJson(receiptPath, null);
  if (!receipt) {
    res.status(500).json({ ok: false, error: "receipt load failed" });
    return;
  }
  const expected = signReceipt(receipt);
  const valid = expected === signature && signature === receipt.signature;
  res.json({ ok: true, valid, receipt_id: receiptId });
});

app.post("/api/v1/promo/redeem", (req, res) => {
  res.status(410).json({ ok: false, error: PROMO_CODES_DISABLED_MESSAGE });
});

app.post("/api/create-download-session", (req, res) => {
  const assetId = normalizeAssetId(req.body?.asset_id || req.body?.assetId || "");
  const sourceToken = cleanText(req.body?.source_token || req.body?.access_token || "", 4096);
  const policy = downloadAssetPolicy(assetId);
  if (!assetId || !policy) {
    res.status(400).json({ ok: false, error: "invalid asset_id" });
    return;
  }
  if (!sourceToken) {
    res.status(401).json({ ok: false, error: "source_token required" });
    return;
  }

  let matchedProduct = "";
  for (const product of policy.products) {
    if (verifyPromoDownloadToken(sourceToken, product)) {
      matchedProduct = product;
      break;
    }
  }
  if (!matchedProduct) {
    res.status(403).json({ ok: false, error: "token not entitled for requested asset" });
    return;
  }

  const jti = randomToken(10);
  const uaHash = hashUserAgent(req.headers["user-agent"] || "");
  const payload = {
    asset_id: assetId,
    product: matchedProduct,
    ua_hash: uaHash,
    jti,
    iat: Date.now(),
    exp: Date.now() + DOWNLOAD_SESSION_TTL_MS
  };
  downloadSessionNonceStore.set(jti, {
    uses: 0,
    exp: payload.exp
  });
  const token = createDownloadSessionToken(payload);
  res.json({
    ok: true,
    asset_id: assetId,
    expires_at: new Date(payload.exp).toISOString(),
    download_url: `/download/${encodeURIComponent(assetId)}?token=${encodeURIComponent(token)}`
  });
});

app.get("/download/:assetId", (req, res) => {
  const assetId = normalizeAssetId(req.params.assetId || "");
  const policy = downloadAssetPolicy(assetId);
  const token = cleanText(req.query?.token || "", 4096);
  if (!assetId || !policy || !token) {
    res.status(401).json({ ok: false, error: "valid token required" });
    return;
  }

  const payload = verifyDownloadSessionToken(token, assetId);
  if (!payload) {
    res.status(401).json({ ok: false, error: "invalid or expired download token" });
    return;
  }

  const nonce = downloadSessionNonceStore.get(payload.jti);
  if (!nonce || Date.now() > Number(nonce.exp || 0)) {
    res.status(401).json({ ok: false, error: "download session expired" });
    return;
  }

  const requestUaHash = hashUserAgent(req.headers["user-agent"] || "");
  if (payload.ua_hash && payload.ua_hash !== requestUaHash) {
    res.status(403).json({ ok: false, error: "download token does not match this client" });
    return;
  }

  if (Number(nonce.uses || 0) >= 1) {
    res.status(429).json({ ok: false, error: "download token already used" });
    return;
  }

  const resolved = resolveDownloadAsset(assetId);
  if (!resolved) {
    res.status(404).json({ ok: false, error: "asset unavailable" });
    return;
  }

  nonce.uses = Number(nonce.uses || 0) + 1;
  downloadSessionNonceStore.set(payload.jti, nonce);

  if (resolved.redirect) {
    res.redirect(resolved.redirect);
    return;
  }
  sendDownloadFile(res, resolved.path, resolved.fileName, resolved.contentType);
});

app.post("/api/v1/admin/login", (req, res) => {
  const username = cleanCredentialValue(req.body?.username, 80);
  const password = cleanCredentialValue(req.body?.password, 160);
  const creds = loadAdminCredentials();

  if (!creds.password_hash) {
    res.status(503).json({ ok: false, error: "admin credentials not configured" });
    return;
  }

  const userMatch = username.toLowerCase() === cleanCredentialValue(creds.username, 80).toLowerCase();
  const passMatch = bcrypt.compareSync(password, creds.password_hash);

  if (!userMatch || !passMatch) {
    appendEventLog("admin.login.failed", {
      user_hash: hashHex(username).slice(0, 16)
    });
    res.status(401).json({ ok: false, error: "invalid admin credentials" });
    return;
  }

  const session = createSession(creds.username);
  setAdminSessionCookie(res, session.token, req);
  appendEventLog("admin.login.success", {
    user: session.username
  });
  res.json({ ok: true, username: session.username, session_token: session.token, expires_at: session.expires_at });
});

app.get("/api/v1/admin/verify", requireAdmin, (req, res) => {
  res.json({ ok: true, username: req.admin.username, expires_at: req.admin.expires_at });
});

app.post("/api/v1/admin/credentials/upload", requireAdmin, genericUpload.single("file"), (req, res) => {
  if (!req.file) {
    res.status(400).json({ ok: false, error: "credentials file is required" });
    return;
  }

  const content = fs.readFileSync(req.file.path, "utf8");
  const parsed = parseAdminCredentialText(content);
  fs.unlinkSync(req.file.path);

  if (!parsed) {
    res.status(400).json({
      ok: false,
      error: "credentials file must provide username and password (JSON or key:value text)"
    });
    return;
  }

  const saved = saveAdminCredentials(parsed.username, parsed.password, `upload:${parsed.source}`);
  if (!saved) {
    res.status(400).json({ ok: false, error: "invalid credentials payload" });
    return;
  }
  saveSessions([]);
  const session = createSession(saved.username);
  setAdminSessionCookie(res, session.token, req);

  res.json({
    ok: true,
    username: saved.username,
    source: saved.source,
    updated_at: saved.updated_at,
    session_token: session.token,
    expires_at: session.expires_at
  });
});

app.get("/api/v1/admin/catalog/list", requireAdmin, (_req, res) => {
  const tracks = loadCatalogData();
  const normalizedTracks = Array.isArray(tracks) ? tracks.map((track) => normalizeCatalogTrackForPlayback(track)) : [];
  res.set("Cache-Control", "no-store");
  res.json({ ok: true, tracks: normalizedTracks });
});

app.post("/api/v1/admin/catalog/upload", requireAdmin, upload.single("file"), (req, res) => {
  if (!req.file) {
    res.status(400).json({ ok: false, error: "file is required" });
    return;
  }

  const meta = normalizeMusicMeta(req.body || {});
  const tracks = readJson(CATALOG_FILE, []);
  const key = randomToken(8);
  const track = {
    key,
    file_name: req.file.filename,
    title: cleanText(req.body?.title, 220) || normalizeTrackTitle(req.file.originalname),
    description: meta.description,
    bpm: meta.bpm,
    key_signature: meta.key,
    tempo: meta.tempo,
    price: meta.price || "$19.99",
    size: req.file.size,
    uploaded_at: nowIso(),
    artwork_url: catalogArtworkUrlFor(req.body?.title || req.file.originalname),
    full_song: true
  };

  tracks.push(track);
  writeJson(CATALOG_FILE, tracks);
  res.json({ ok: true, track: normalizeCatalogTrackForPlayback(track) });
});

app.post("/api/v1/admin/catalog/remove", requireAdmin, (req, res) => {
  const key = String(req.body?.key || "").trim();
  if (!key) {
    res.status(400).json({ ok: false, error: "key is required" });
    return;
  }

  const catalogEntries = readJson(CATALOG_FILE, []);
  const liveTracks = loadCatalogData();
  const target = liveTracks.find((item) => item.key === key);
  if (!target) {
    res.status(404).json({ ok: false, error: "track not found" });
    return;
  }

  if (target.managed_source && target.source_path && fs.existsSync(target.source_path)) {
    fs.unlinkSync(target.source_path);
  }

  const nextTracks = Array.isArray(catalogEntries)
    ? catalogEntries.filter((item) => cleanText(item?.key || "", 80) !== key)
    : [];
  writeJson(CATALOG_FILE, nextTracks);
  appendEventLog("admin.catalog.remove", { key, file_name: target.file_name, managed_source: !!target.managed_source });
  res.json({ ok: true, removed: key });
});

app.get("/api/v1/admin/soundpacks/list", requireAdmin, (_req, res) => {
  const packs = readJson(SOUNDPACK_FILE, []);
  res.json({ ok: true, soundpacks: packs });
});

app.post("/api/v1/admin/soundpacks/upload", requireAdmin, soundpackUpload.single("file"), (req, res) => {
  if (!req.file) {
    res.status(400).json({ ok: false, error: "file is required" });
    return;
  }

  const meta = normalizeMusicMeta(req.body || {});
  const packs = readJson(SOUNDPACK_FILE, []);
  const key = randomToken(8);
  const soundpack = {
    key,
    file_name: req.file.filename,
    pack_type: meta.pack_type || "soundpack",
    title: cleanText(req.body?.title, 220) || normalizeTrackTitle(req.file.originalname),
    description: meta.description,
    bpm: meta.bpm,
    key_signature: meta.key,
    tempo: meta.tempo,
    price: meta.price || defaultPackPrice(meta.pack_type),
    size: req.file.size,
    uploaded_at: nowIso(),
    artwork_url: catalogArtworkUrlFor(req.body?.title || req.file.originalname),
    public_url: buildSoundpackPublicUrl(req.file.filename)
  };

  packs.push(soundpack);
  writeJson(SOUNDPACK_FILE, packs);
  res.json({ ok: true, soundpack });
});

app.post("/api/v1/admin/reference/upload", requireAdmin, referenceUpload.single("file"), (req, res) => {
  if (!req.file) {
    res.status(400).json({ ok: false, error: "file is required" });
    return;
  }

  const genre = normalizeReferenceGenreName(req.body?.genre || req.body?.reference_genre || "");
  if (!genre) {
    res.status(400).json({ ok: false, error: "reference genre is required" });
    return;
  }

  const title = cleanText(req.body?.title, 220) || normalizeTrackTitle(req.file.originalname);
  const referencePath = path.join("assets/reference_intake/licensed_audio", genre, req.file.filename);
  appendEventLog("admin.reference.upload", {
    genre,
    title,
    file_name: req.file.filename,
    user: req.admin.username
  });
  res.json({
    ok: true,
    genre,
    title,
    file_name: req.file.filename,
    stored_path: referencePath,
    next_action: "Run action:reference.pool.rebuild after enough licensed references are uploaded."
  });
});

app.post("/api/v1/admin/soundpacks/remove", requireAdmin, (req, res) => {
  const key = String(req.body?.key || "").trim();
  if (!key) {
    res.status(400).json({ ok: false, error: "key is required" });
    return;
  }

  const packs = readJson(SOUNDPACK_FILE, []);
  const target = packs.find((item) => item.key === key);
  if (!target) {
    res.status(404).json({ ok: false, error: "sound pack not found" });
    return;
  }

  const filePath = path.join(SOUNDPACK_DIR, target.file_name);
  if (fs.existsSync(filePath)) {
    fs.unlinkSync(filePath);
  }

  const nextPacks = packs.filter((item) => item.key !== key);
  writeJson(SOUNDPACK_FILE, nextPacks);
  appendEventLog("admin.soundpack.remove", { key, file_name: target.file_name });
  res.json({ ok: true, removed: key });
});

app.get("/api/v1/admin/content/get", requireAdmin, (_req, res) => {
  const content = normalizeContent(readJson(CONTENT_FILE, DEFAULT_CONTENT));
  res.json({ ok: true, content });
});

app.post("/api/v1/admin/content/save", requireAdmin, (req, res) => {
  const content = normalizeContent(req.body || {});
  writeJson(CONTENT_FILE, content);
  appendEventLog("admin.content.save", { user: req.admin?.username || "admin" });
  res.json({ ok: true, content });
});

app.get("/api/v1/admin/inquiries/list", requireAdmin, (_req, res) => {
  const inquiries = readJson(INQUIRIES_FILE, []);
  res.json({ ok: true, inquiries: Array.isArray(inquiries) ? inquiries : [] });
});

app.get("/api/v1/admin/dashboard/state", requireAdmin, (_req, res) => {
  const inquiries = readJson(INQUIRIES_FILE, []);
  const sales = readJson(SALES_FILE, []);
  res.json({
    ok: true,
    snapshot_at: nowIso(),
    traffic: {
      page_views: DASHBOARD_STATE.page_views,
      api_hits: DASHBOARD_STATE.api_hits,
      media_streams: DASHBOARD_STATE.media_streams,
      downloads: DASHBOARD_STATE.downloads,
      last_request_at: DASHBOARD_STATE.last_request_at,
      recent: DASHBOARD_STATE.recent.slice(0, 50)
    },
    inquiries: {
      count: Array.isArray(inquiries) ? inquiries.length : 0,
      latest: Array.isArray(inquiries) ? inquiries.slice(0, 25) : []
    },
    sales: {
      count: Array.isArray(sales) ? sales.length : 0,
      latest: Array.isArray(sales) ? sales.slice(0, 25) : []
    },
    logs: {
      events: readEventLogs(50),
      adminlog: readAdminLog(50)
    }
  });
});

app.get("/api/v1/admin/logs/list", requireAdmin, (req, res) => {
  const limit = Number(req.query.limit || 200);
  const bounded = Number.isFinite(limit) ? Math.max(1, Math.min(1000, limit)) : 200;
  res.json({ ok: true, logs: readEventLogs(bounded), adminlog: readAdminLog(bounded) });
});

app.get("/api/v1/admin/sales/list", requireAdmin, (_req, res) => {
  const sales = readJson(SALES_FILE, []);
  res.json({ ok: true, sales: Array.isArray(sales) ? sales : [] });
});

app.post("/api/v1/admin/sales/record", requireAdmin, (req, res) => {
  const itemName = cleanText(req.body?.item_name || "", 220);
  const amount = cleanText(req.body?.amount || "", 24);
  const currency = cleanText(req.body?.currency || "USD", 8).toUpperCase();
  const customerName = cleanText(req.body?.customer_name || "", 120);
  const customerEmail = cleanText(req.body?.customer_email || "", 180);
  const paymentProvider = cleanText(req.body?.payment_provider || "paypal", 64).toLowerCase();
  const paymentTxnId = cleanText(req.body?.payment_txn_id || "", 160);

  if (!itemName || !amount || !customerEmail) {
    res.status(400).json({ ok: false, error: "item_name, amount, and customer_email are required" });
    return;
  }

  const sales = readJson(SALES_FILE, []);
  const sale = {
    sale_id: randomToken(8),
    item_name: itemName,
    amount,
    currency,
    customer_name: customerName,
    customer_email: customerEmail,
    payment_provider: paymentProvider,
    payment_txn_id: paymentTxnId,
    created_at: nowIso(),
    recorded_by: req.admin?.username || "admin"
  };
  const receipt = createReceiptFromSale(sale);
  sale.receipt_id = receipt.receipt_id;
  sale.receipt_signature = receipt.signature;
  sale.receipt_url = receipt.receipt_url;

  sales.unshift(sale);
  writeJson(SALES_FILE, Array.isArray(sales) ? sales.slice(0, 5000) : [sale]);

  const receiptJsonPath = path.join(RECEIPTS_DIR, `${receipt.receipt_id}.json`);
  const receiptHtmlPath = path.join(RECEIPTS_DIR, `${receipt.receipt_id}.html`);
  writeJson(receiptJsonPath, receipt);
  fs.writeFileSync(receiptHtmlPath, buildReceiptHtml(receipt), "utf8");

  appendEventLog("admin.sales.record", {
    sale_id: sale.sale_id,
    receipt_id: receipt.receipt_id,
    item_name: sale.item_name,
    amount: sale.amount,
    currency: sale.currency
  });

  res.json({
    ok: true,
    sale,
    receipt: {
      receipt_id: receipt.receipt_id,
      signature: receipt.signature,
      receipt_url: receipt.receipt_url,
      watermark_token: receipt.watermark_token
    }
  });
});

app.get("/api/v1/admin/promo/list", requireAdmin, (_req, res) => {
  res.status(410).json({ ok: false, error: PROMO_CODES_DISABLED_MESSAGE });
});

app.post("/api/v1/admin/promo/save", requireAdmin, (req, res) => {
  res.status(410).json({ ok: false, error: PROMO_CODES_DISABLED_MESSAGE });
});

function buildOpenAiAdminPayload(cfg) {
  return {
    ok: true,
    openai: {
      model: cfg.model,
      model_list: cfg.model_list || [],
      key_file_path: cfg.key_file_path || OPENAI_PRIMARY_KEY_FILE,
      base_url: cfg.base_url || OPENAI_BASE_URL,
      ready: cfg.ready,
      source: cfg.source,
      updated_at: cfg.updated_at
    }
  };
}

app.get("/api/v1/admin/openai/get", requireAdmin, (_req, res) => {
  const cfg = loadOpenAiConfig();
  res.json(buildOpenAiAdminPayload(cfg));
});

app.post("/api/v1/admin/openai/save", requireAdmin, (req, res) => {
  const baseUrl = req.body?.base_url;
  const apiKey = req.body?.api_key || "";
  const keyFilePath = req.body?.key_file_path || "";
  const cfg = saveOpenAiConfig(apiKey, baseUrl, keyFilePath);
  if (!cfg.ready) {
    res.status(400).json({ ok: false, error: "OpenAI API key missing — AIFR3D chat unavailable", ...buildOpenAiAdminPayload(cfg) });
    return;
  }
  res.json(buildOpenAiAdminPayload(cfg));
});

app.post("/api/v1/admin/chat/settings/save", requireAdmin, (req, res) => {
  res.json({
    ok: true,
    provider: "openai",
    websocket_url: DEFAULT_ONLINE_WS,
    settings: saveChatSettings(req.body)
  });
});

app.post("/api/v1/admin/files/upload", requireAdmin, genericUpload.single("file"), (req, res) => {
  if (!req.file) {
    res.status(400).json({ ok: false, error: "file is required" });
    return;
  }

  const rel = String(req.body?.path || req.body?.target_path || "").trim();
  const full = secureRootPath(rel);
  if (!full) {
    fs.unlinkSync(req.file.path);
    res.status(400).json({ ok: false, error: "invalid upload path" });
    return;
  }

  ensureDir(path.dirname(full));
  fs.copyFileSync(req.file.path, full);
  fs.unlinkSync(req.file.path);
  appendEventLog("admin.files.upload", {
    path: path.relative(FILE_ACCESS_ROOT, full),
    bytes: req.file.size,
    user: req.admin.username
  });
  appendAdminLog(`upload:${path.relative(FILE_ACCESS_ROOT, full)}`, {
    ok: true,
    bytes: req.file.size,
    user: req.admin.username
  });
  res.json({
    ok: true,
    path: path.relative(FILE_ACCESS_ROOT, full),
    size: req.file.size,
    file_name: path.basename(full),
    uploaded_at: nowIso()
  });
});

app.get("/api/v1/admin/files/list", requireAdmin, (req, res) => {
  const rel = String(req.query.path || "").trim();
  const full = secureRootPath(rel);
  if (!full || !fs.existsSync(full) || !fs.statSync(full).isDirectory()) {
    res.status(400).json({ ok: false, error: "invalid directory" });
    return;
  }

  const entries = fs.readdirSync(full, { withFileTypes: true }).map((entry) => ({
    name: entry.name,
    type: entry.isDirectory() ? "dir" : "file"
  }));

  res.json({ ok: true, path: path.relative(FILE_ACCESS_ROOT, full), entries });
});

app.post("/api/v1/admin/files/read", requireAdmin, (req, res) => {
  const rel = String(req.body?.path || "").trim();
  const full = secureRootPath(rel);
  if (!full || !fs.existsSync(full) || !fs.statSync(full).isFile()) {
    res.status(400).json({ ok: false, error: "invalid file" });
    return;
  }

  const content = fs.readFileSync(full, "utf8");
  res.json({ ok: true, path: path.relative(FILE_ACCESS_ROOT, full), content });
});

app.post("/api/v1/admin/files/write", requireAdmin, (req, res) => {
  const rel = String(req.body?.path || "").trim();
  const content = String(req.body?.content || "");
  const full = secureRootPath(rel);
  if (!full) {
    res.status(400).json({ ok: false, error: "invalid file path" });
    return;
  }

  ensureDir(path.dirname(full));
  fs.writeFileSync(full, content, "utf8");
  appendEventLog("admin.files.write", {
    path: path.relative(FILE_ACCESS_ROOT, full),
    bytes: Buffer.byteLength(content)
  });
  res.json({ ok: true, path: path.relative(FILE_ACCESS_ROOT, full), bytes: Buffer.byteLength(content) });
});

app.post("/api/v1/admin/files/delete", requireAdmin, (req, res) => {
  const rel = String(req.body?.path || "").trim();
  const full = secureRootPath(rel);
  if (!full || !fs.existsSync(full)) {
    res.status(400).json({ ok: false, error: "invalid path" });
    return;
  }

  const stat = fs.statSync(full);
  if (stat.isDirectory()) {
    fs.rmSync(full, { recursive: true, force: true });
  } else {
    fs.unlinkSync(full);
  }

  appendEventLog("admin.files.delete", {
    path: path.relative(FILE_ACCESS_ROOT, full),
    type: stat.isDirectory() ? "dir" : "file"
  });
  res.json({ ok: true, deleted: path.relative(FILE_ACCESS_ROOT, full) });
});

app.get("/receipts/:receiptId", (req, res) => {
  const receiptId = cleanText(req.params.receiptId || "", 120);
  const receiptHtmlPath = path.join(RECEIPTS_DIR, `${receiptId}.html`);
  if (!receiptId || !fs.existsSync(receiptHtmlPath)) {
    res.status(404).send("Receipt not found");
    return;
  }
  res.setHeader("Content-Type", "text/html; charset=utf-8");
  res.sendFile(receiptHtmlPath);
});

function requireDownloadAccess(products) {
  const allowed = Array.isArray(products)
    ? products.map((item) => normalizePromoProduct(item)).filter(Boolean)
    : [];
  return (req, res, next) => {
    const token = cleanText(req.query?.access_token || "", 4096);
    if (!token) {
      res.status(401).json({ ok: false, error: "access_token required" });
      return;
    }
    const granted = allowed.some((product) => Boolean(verifyPromoDownloadToken(token, product)));
    if (!granted) {
      res.status(403).json({ ok: false, error: "invalid or expired access token" });
      return;
    }
    next();
  };
}

app.get("/downloads/install-vst-standalone.sh", requireDownloadAccess(["vst", "standalone"]), (_req, res) => {
  const scriptPath = path.join(ROOT_DIR, "scripts/install_local_linux.sh");
  sendDownloadFile(res, fs.existsSync(scriptPath) ? scriptPath : null, "install-vst-standalone.sh", "text/x-shellscript");
});

app.get("/downloads/install-vst.sh", requireDownloadAccess(["vst"]), (_req, res) => {
  const scriptPath = path.join(ROOT_DIR, "apps/website/downloads/plugin-linux-installer.sh");
  sendDownloadFile(res, fs.existsSync(scriptPath) ? scriptPath : null, "install-vst.sh", "text/x-shellscript");
});

app.get("/downloads/install-standalone.sh", requireDownloadAccess(["standalone"]), (_req, res) => {
  const scriptPath = path.join(ROOT_DIR, "apps/website/downloads/standalone-linux-installer.sh");
  sendDownloadFile(res, fs.existsSync(scriptPath) ? scriptPath : null, "install-standalone.sh", "text/x-shellscript");
});

app.get("/downloads/dawai-standalone", requireDownloadAccess(["standalone"]), (_req, res) => {
  const target = resolveFirstExisting(buildStandaloneCandidates());
  sendDownloadFile(res, target, target && target.endsWith(".exe") ? "dawai-standalone.exe" : "dawai-standalone", "application/octet-stream");
});

app.get("/downloads/dawai-standalone-:variant([a-dA-D])", requireDownloadAccess(["standalone"]), (req, res) => {
  const suffix = String(req.params.variant || "").toLowerCase();
  const target = resolveFirstExisting(buildStandaloneVariantCandidates(suffix)) || resolveFirstExisting(buildStandaloneCandidates());
  sendDownloadFile(
    res,
    target,
    target && target.endsWith(".exe") ? `dawai-standalone-v2.2.4${suffix}.exe` : `dawai-standalone-v2.2.4${suffix}`,
    "application/octet-stream"
  );
});

app.get("/downloads/dawai-vst3", requireDownloadAccess(["vst"]), (_req, res) => {
  const target = resolveFirstExisting(buildVstCandidates());
  sendDownloadFile(res, target, "DawAI.vst3", "application/octet-stream");
});

app.get("/downloads/dawai-vst3-:variant([a-dA-D])", requireDownloadAccess(["vst"]), (req, res) => {
  const suffix = String(req.params.variant || "").toLowerCase();
  const target = resolveFirstExisting(buildVstVariantCandidates(suffix)) || resolveFirstExisting(buildVstCandidates());
  sendDownloadFile(res, target, `DawAI_v2.2.4${suffix}.vst3`, "application/octet-stream");
});

app.get("/downloads/dawai-linux-amd64.deb", requireDownloadAccess(["vst", "standalone"]), (_req, res) => {
  if (LINUX_DEB_URL) {
    res.redirect(LINUX_DEB_URL);
    return;
  }
  const target = resolveFirstExisting(buildLinuxDebCandidates());
  sendDownloadFile(res, target, "dawai-linux-amd64.deb", "application/vnd.debian.binary-package");
});

app.get("/downloads/dawai-linux-arch-x86_64.pkg.tar.zst", requireDownloadAccess(["vst", "standalone"]), (_req, res) => {
  if (LINUX_ARCH_URL) {
    res.redirect(LINUX_ARCH_URL);
    return;
  }
  const target = resolveFirstExisting(buildLinuxArchCandidates());
  sendDownloadFile(res, target, "dawai-linux-arch-x86_64.pkg.tar.zst", "application/zstd");
});

app.get("/downloads/dawai-admin-android.apk", requireAdmin, (req, res) => {
  const target = resolveFirstExisting(buildAndroidApkCandidates());
  appendEventLog("admin.android.apk.download", {
    user: req.admin?.username || "admin",
    file_name: target ? path.basename(target) : ""
  });
  sendDownloadFile(res, target, "dawai-admin-android.apk", "application/vnd.android.package-archive");
});

server.on("upgrade", (request, socket, head) => {
  const host = request.headers.host || `localhost:${PORT}`;
  const reqUrl = new URL(request.url || "/", `http://${host}`);

  if (reqUrl.pathname !== "/ws/chat") {
    socket.destroy();
    return;
  }

  if (sitePrivateModeEnabled()) {
    const auth = String(request.headers.authorization || "");
    const queryToken = String(reqUrl.searchParams.get("token") || "").trim();
    const bearer = auth.startsWith("Bearer ") ? auth.slice("Bearer ".length).trim() : "";
    const token = bearer || queryToken;
    const hasApiAccess = API_TOKEN ? token === API_TOKEN : false;
    const hasAdminAccess = token ? Boolean(findSession(token)) : false;
    if (!hasApiAccess && !hasAdminAccess) {
      socket.destroy();
      return;
    }
  }

  if (API_TOKEN) {
    const auth = String(request.headers.authorization || "");
    const queryToken = String(reqUrl.searchParams.get("token") || "");
    const token = auth.startsWith("Bearer ") ? auth.slice("Bearer ".length).trim() : queryToken;
    if (token !== API_TOKEN) {
      socket.destroy();
      return;
    }
  }

  wss.handleUpgrade(request, socket, head, (ws) => {
    wss.emit("connection", ws, request);
  });
});

wss.on("connection", (ws) => {
  const openai = loadOpenAiConfig();
  const chatSettings = loadChatSettings();
  ws.send(
    JSON.stringify({
      type: "chat.ready",
      service: "dawai-ecosystem",
      provider: "openai",
      model: openai.model || null,
      models: activeOpenAiModelList(openai),
      transport_mode: chatSettings.transport_mode,
      brain_id: AIFR3D_BRAIN?.brain_id || "aifr3d_brain_v2_2_4",
      timestamp_utc: nowIso()
    })
  );

  ws.on("message", async (raw) => {
    let payload;
    try {
      payload = JSON.parse(String(raw));
    } catch (_error) {
      ws.send(JSON.stringify({ type: "chat.error", message: "invalid json" }));
      return;
    }

    if (payload?.type === "chat.clear") {
      const sessionId = String(payload.session_id || "default").trim();
      clearMemory(sessionId);
      ws.send(JSON.stringify({ type: "chat.cleared", session_id: sessionId }));
      return;
    }

    if (payload?.type !== "chat.send") {
      ws.send(JSON.stringify({ type: "chat.error", message: "unsupported message type" }));
      return;
    }

    const prompt = String(payload.text || "").trim();
    const sessionId = String(payload.session_id || "default").trim();
    const personalityMode = String(payload.personality_mode || "").trim();
    const agentModes = Array.isArray(payload.agent_modes) ? payload.agent_modes : [];

    if (!prompt) {
      ws.send(JSON.stringify({ type: "chat.error", message: "empty prompt" }));
      return;
    }

    const memory = loadMemory(sessionId);
    const summary = summarizeMemory(memory, loadChatSettings().context.summary_items);
    const inference = await openaiStructuredChat(prompt, sessionId, summary, {
      personality_mode: personalityMode,
      agent_modes: agentModes
    });

    if (!inference.ok) {
      ws.send(
        JSON.stringify({
          type: "chat.error",
          message: inference.error,
          provider: inference.provider,
          model: inference.model,
          attempted_models: inference.attempted_models || []
        })
      );
      return;
    }

    const updated = updateMemoryFromChat(memory, prompt, inference.data);
    updated.openai_previous_response_id = inference.response_id || updated.openai_previous_response_id || "";
    saveMemory(sessionId, updated);

    const messageId = randomToken(6);
    const tokens = String(inference.data.summary || "").split(/\s+/).filter(Boolean);

    let index = 0;
    const timer = setInterval(() => {
      if (ws.readyState !== ws.OPEN) {
        clearInterval(timer);
        return;
      }

      if (index < tokens.length) {
        ws.send(JSON.stringify({ type: "chat.token", messageId, text: `${tokens[index]} ` }));
        index += 1;
        return;
      }

      clearInterval(timer);

      (inference.data.issues || []).forEach((issue) => {
        ws.send(JSON.stringify({ type: "issue.object", issue }));
      });

      (inference.data.action_suggestions || []).forEach((suggestion) => {
        ws.send(JSON.stringify({ type: "action.suggest", suggestion }));
      });

      ws.send(
        JSON.stringify({
          type: "chat.done",
          messageId,
          provider: inference.provider,
          model: inference.model,
          attempted_models: inference.attempted_models || [inference.model],
          brain_id: AIFR3D_BRAIN?.brain_id || "aifr3d_brain_v2_2_4",
          session_id: sessionId
        })
      );
    }, 35);
  });
});

app.use("/media/soundpacks", express.static(SOUNDPACK_DIR));
app.get("/media/:fileName", (req, res, next) => {
  const fileName = decodeURIComponent(String(req.params.fileName || "")).trim();
  const streamUrl = buildCatalogPublicUrl(fileName);
  if (!streamUrl) {
    next();
    return;
  }
  res.redirect(302, streamUrl);
});
app.use("/media", express.static(CATALOG_DIR));
app.use("/downloads", express.static(path.join(WEBSITE_DIR, "downloads")));
app.use(express.static(WEBSITE_DIR));

app.get("*", (_req, res) => {
  res.sendFile(path.join(WEBSITE_DIR, "index.html"));
});

server.listen(PORT, "0.0.0.0", () => {
  console.log(`[dawai] server listening on :${PORT}`);
});
