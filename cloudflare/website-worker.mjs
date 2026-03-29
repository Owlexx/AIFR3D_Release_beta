const SESSION_TTL_MS = 24 * 60 * 60 * 1000;
const WEBSITE_ROOT = "apps/website";
const WEBSITE_INDEX_PATH = `${WEBSITE_ROOT}/index.html`;
const WEBSITE_DATA_CATALOG_PATH = `${WEBSITE_ROOT}/assets/data/beat_catalog.json`;

const DEFAULT_CONTENT = {
  services: [
    {
      title: "AIFR3D Diagnostics Report",
      description: "Per-track analysis with tonal balance, stereo field, loudness, and translation notes turned into direct next moves.",
      price: "$49.99 / track"
    },
    {
      title: "Mix Review + Reference Matching",
      description: "Live review session focused on alignment, corrective priorities, and faster mix decisions against the right target.",
      price: "$120/hr"
    },
    {
      title: "Release Translation Pass",
      description: "Final loudness, stereo, and tonal translation pass designed to tighten the release-ready version without guesswork.",
      price: "$80"
    }
  ],
  products: [
    {
      sku: "software_vst3",
      title: "AIFR3D VST3 Plugin",
      description: "Live mix-intelligence plugin with tonal diagnostics, stereo field analysis, reference matching, and Aifred Mix Tips.",
      price: "Free to try for a limited time | $149.99 after Beta 2.2.4",
      availability_label: "Free to try for a limited time",
      future_price_label: "$149.99 after Beta 2.2.4",
      beta_free_trial: true,
      buy_url: ""
    },
    {
      sku: "software_standalone",
      title: "AIFR3D Standalone App",
      description: "Standalone AIFR3D workspace for live diagnostics, reference behavior, stereo intelligence, and guided correction outside the DAW.",
      price: "Free to try for a limited time | $149.99 after Beta 2.2.4",
      availability_label: "Free to try for a limited time",
      future_price_label: "$149.99 after Beta 2.2.4",
      beta_free_trial: true,
      buy_url: ""
    }
  ]
};

const GITHUB_RAW_BASE = "https://raw.githubusercontent.com/kaeganscott26/AifredVst-2.2.4-beta/Aifred-Vst-Beta-2.2.4";
const DOWNLOAD_REDIRECTS = {
  "/downloads/install-vst-standalone.sh": `${GITHUB_RAW_BASE}/scripts/install_local_linux.sh`,
  "/downloads/install-vst.sh": `${GITHUB_RAW_BASE}/apps/website/downloads/plugin-linux-installer.sh`,
  "/downloads/install-standalone.sh": `${GITHUB_RAW_BASE}/apps/website/downloads/standalone-linux-installer.sh`
};

const DEFAULT_PROMO_CODES = [];
const PAYMENT_UNLOCK_BUNDLE = Object.freeze({
  build: "2.2.4 Beta",
  entitlement_type: "beta_free_trial",
  payment_verified: false,
  payment_bypass: false,
  lifetime: true,
  session_limits: { analyze: 0, compare: 0, reference: 0 },
  products: ["vst", "standalone"],
  note: "AIFR3D 2.2.4 Beta software installers are free to try for a limited time."
});

const DEFAULT_STORE_PRODUCTS = [
  {
    sku: "software_vst3",
    name: "AIFR3D VST3 Plugin",
    description: "Live mix-intelligence plugin with tonal diagnostics, stereo field analysis, reference matching, and Aifred Mix Tips.",
    priceUSD: "0.00",
    currency: "USD",
    object_key: "products/dawai-vst3.zip",
    download_url: "/downloads/install-vst.sh",
    unlock_products: ["vst"],
    beta_free_trial: true,
    availability_label: "Free to try for a limited time",
    future_price_label: "$149.99 after Beta 2.2.4",
    active: true
  },
  {
    sku: "software_standalone",
    name: "AIFR3D Standalone App",
    description: "Standalone AIFR3D workspace for live diagnostics, reference behavior, stereo intelligence, and guided correction outside the DAW.",
    priceUSD: "0.00",
    currency: "USD",
    object_key: "products/dawai-standalone.zip",
    download_url: "/downloads/install-standalone.sh",
    unlock_products: ["standalone"],
    beta_free_trial: true,
    availability_label: "Free to try for a limited time",
    future_price_label: "$149.99 after Beta 2.2.4",
    active: true
  }
];

const DEFAULT_OPENAI_PRIMARY_MODEL = "gpt-5.2";
const DEFAULT_OPENAI_MODEL_ALLOWLIST = Object.freeze([DEFAULT_OPENAI_PRIMARY_MODEL]);
const CHAT_TRANSPORT_MODES = Object.freeze(["websocket", "http"]);
const CHAT_REASONING_EFFORTS = Object.freeze(["minimal", "low", "medium", "high"]);
const CHAT_VERBOSITY_LEVELS = Object.freeze(["low", "medium", "high"]);
const CHAT_TONE_PRESETS = Object.freeze(["direct", "calm", "technical", "executive", "creative"]);
const CHAT_WEBHOOK_EVENTS = Object.freeze(["chat.completed", "chat.failed"]);
const ADMIN_SECRET_HINT = "set ADMIN_USERNAME and ADMIN_PASSWORD in the worker environment or local .dev.vars";
const DEFAULT_ADMIN_USERNAME = "North3rnLight3r";
const DEFAULT_ADMIN_PASSWORD = "Poohbe@r2009$0826";
const DEFAULT_ADMIN_SESSION_SECRET = "north3rnlight3r_aifr3d_admin_session_2_2_4";
const PROMO_CODES_DISABLED_MESSAGE = "Promo codes are disabled for AIFR3D 2.2.4 Beta.";
const BETA_FREE_TRIAL_MESSAGE = "AIFR3D 2.2.4 Beta is free to try for a limited time. Use the installer download instead of checkout.";

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
    guidance_only: true,
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
  model: DEFAULT_OPENAI_PRIMARY_MODEL
};

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

const API_LIMITS = {
  updated_at: nowIso(),
  chat: {
    provider: "openai",
    max_prompt_chars: 4000,
    max_tokens: 900,
    memory_window_items: 20,
    retry_models_max: 0
  },
  command: {
    simulated_only: false
  },
  uploads: {
    mode: "form-data",
    max_items_kept_in_memory: 200
  },
  payments: {
    provider: "paypal",
    token_ttl_seconds: 86400,
    order_record_ttl_seconds: 604800
  }
};

const APP_STATE = {
  memory: new Map(),
  memorySummary: new Map(),
  content: structuredClone(DEFAULT_CONTENT),
  catalog: [],
  soundpacks: [],
  referenceUploads: [],
  promoCodes: structuredClone(DEFAULT_PROMO_CODES),
  openai: null,
  brain: DEFAULT_AIFR3D_BRAIN,
  chatSettings: null,
  inquiries: [],
  sales: [],
  receipts: new Map(),
  paymentOrders: new Map(),
  paymentEvents: [],
  logs: [],
  fileOverrides: new Map(),
  binaryOverrides: new Map(),
  activity: {
    page_views: 0,
    api_hits: 0,
    media_streams: 0,
    downloads: 0,
    last_request_at: "",
    recent: []
  },
  seeded: false
};
let promoCodesHydratedFromEnv = false;

function json(value, init = {}) {
  const headers = new Headers(init.headers || {});
  headers.set("Content-Type", "application/json");
  return new Response(JSON.stringify(value), { ...init, headers });
}

function nowIso() {
  return new Date().toISOString();
}

function randomTokenHex(byteLen = 24) {
  const bytes = new Uint8Array(byteLen);
  crypto.getRandomValues(bytes);
  return Array.from(bytes, (b) => b.toString(16).padStart(2, "0")).join("");
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

function normalizeSku(raw) {
  return String(raw || "")
    .trim()
    .toLowerCase()
    .replace(/[^a-z0-9._-]/g, "")
    .slice(0, 80);
}

function normalizeCurrencyCode(raw) {
  const code = String(raw || "USD")
    .trim()
    .toUpperCase()
    .replace(/[^A-Z]/g, "")
    .slice(0, 3);
  return code || "USD";
}

function normalizeUsdAmount(raw, fallback = "0.00") {
  const parsed = Number(String(raw || "").replace(/[^0-9.]/g, ""));
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

function sitePrivateMode(env) {
  return envFlag(env.WEBSITE_PRIVATE_MODE ?? env.SITE_PRIVATE_MODE ?? env.MAINTENANCE_MODE, false);
}

function paymentMode(env) {
  return "paypal";
}

function checkoutFallbackUrl(env, itemName, amount, currency = "USD") {
  return fallbackPaypalUrl(env, itemName, amount, currency);
}

function inquiryTargetEmail(env) {
  return cleanText(env.INQUIRY_TO_EMAIL || "north3rnlight3rofficial@outlook.com", 220);
}

function paymentSecret(env) {
  return cleanText(env.DOWNLOAD_TOKEN_SECRET || env.APP_SECRET || sessionSecret(env), 260);
}

function hasKvBinding(binding) {
  return Boolean(binding && typeof binding.get === "function" && typeof binding.put === "function");
}

function hasR2Binding(binding) {
  return Boolean(binding && typeof binding.get === "function" && typeof binding.head === "function");
}

function paypalMode(env) {
  const mode = cleanText(env.PAYPAL_ENV || "sandbox", 24).toLowerCase();
  return mode === "live" ? "live" : "sandbox";
}

function paypalBase(env) {
  return paypalMode(env) === "live" ? "https://api-m.paypal.com" : "https://api-m.sandbox.paypal.com";
}

function paypalReady(env) {
  return Boolean(cleanText(env.PAYPAL_CLIENT_ID || "", 260) && cleanText(env.PAYPAL_CLIENT_SECRET || "", 260));
}

function paypalReceiverEmail(env) {
  return cleanText(env.PAYPAL_BUSINESS_EMAIL || "north3rnlight3rofficial@outlook.com", 220);
}

function fallbackPaypalUrl(env, itemName, amount, currency = "USD") {
  const params = new URLSearchParams({
    cmd: "_xclick",
    business: paypalReceiverEmail(env),
    item_name: cleanText(itemName || "North3rnLight3r Product", 220),
    currency_code: normalizeCurrencyCode(currency)
  });
  const amountText = normalizeUsdAmount(amount, "");
  if (amountText) {
    params.set("amount", amountText);
  }
  return `https://www.paypal.com/cgi-bin/webscr?${params.toString()}`;
}

function normalizeStoreProduct(entry) {
  const sku = normalizeSku(entry?.sku || "");
  if (!sku) {
    return null;
  }
  return {
    sku,
    name: cleanText(entry?.name || entry?.title || "Product", 220),
    description: cleanText(entry?.description || "", 360),
    priceUSD: normalizeUsdAmount(entry?.priceUSD || entry?.price || "0"),
    currency: normalizeCurrencyCode(entry?.currency || "USD"),
    object_key: String(entry?.object_key || entry?.objectKey || "").trim().slice(0, 500),
    download_url: String(entry?.download_url || entry?.downloadUrl || "").trim().slice(0, 700),
    unlock_products: Array.isArray(entry?.unlock_products)
      ? entry.unlock_products.map((item) => normalizeSku(item)).filter(Boolean)
      : [],
    beta_free_trial: entry?.beta_free_trial === true,
    availability_label: cleanText(entry?.availability_label || "", 120),
    future_price_label: cleanText(entry?.future_price_label || "", 120),
    active: entry?.active !== false
  };
}

async function loadStoreProducts(env) {
  const fallback = DEFAULT_STORE_PRODUCTS.map((item) => normalizeStoreProduct(item)).filter(Boolean);
  if (!hasKvBinding(env.PRODUCTS) || typeof env.PRODUCTS.list !== "function") {
    return fallback;
  }
  try {
    const listed = await env.PRODUCTS.list({ prefix: "sku:" });
    if (!listed || !Array.isArray(listed.keys) || listed.keys.length === 0) {
      return fallback;
    }
    const output = [];
    for (const key of listed.keys) {
      const raw = await env.PRODUCTS.get(key.name);
      if (!raw) {
        continue;
      }
      const parsed = JSON.parse(raw);
      const normalized = normalizeStoreProduct(parsed);
      if (normalized && normalized.active) {
        output.push(normalized);
      }
    }
    return output.length > 0 ? output : fallback;
  } catch (_error) {
    return fallback;
  }
}

async function loadStoreProductBySku(env, sku) {
  const needle = normalizeSku(sku);
  if (!needle) {
    return null;
  }
  if (hasKvBinding(env.PRODUCTS)) {
    try {
      const raw = await env.PRODUCTS.get(`sku:${needle}`);
      if (raw) {
        const parsed = JSON.parse(raw);
        const normalized = normalizeStoreProduct(parsed);
        if (normalized && normalized.active) {
          return normalized;
        }
      }
    } catch (_error) {
    }
  }
  return DEFAULT_STORE_PRODUCTS.map((item) => normalizeStoreProduct(item)).find((item) => item && item.sku === needle) || null;
}

async function storePaymentOrder(env, orderId, value, ttlSeconds = 7 * 24 * 60 * 60) {
  const key = `order:${normalizeSku(orderId)}`;
  if (!key || key === "order:") {
    return;
  }
  if (hasKvBinding(env.ORDERS)) {
    await env.ORDERS.put(key, JSON.stringify(value), { expirationTtl: Math.max(300, Number(ttlSeconds) || 300) });
    return;
  }
  APP_STATE.paymentOrders.set(key, value);
}

async function readPaymentOrder(env, orderId) {
  const key = `order:${normalizeSku(orderId)}`;
  if (!key || key === "order:") {
    return null;
  }
  if (hasKvBinding(env.ORDERS)) {
    const raw = await env.ORDERS.get(key);
    if (!raw) {
      return null;
    }
    try {
      return JSON.parse(raw);
    } catch (_error) {
      return null;
    }
  }
  return APP_STATE.paymentOrders.get(key) || null;
}

function pushPaymentEvent(type, payload = {}) {
  APP_STATE.paymentEvents.unshift({
    ts: nowIso(),
    type: cleanText(type || "payment.event", 120),
    payload
  });
  if (APP_STATE.paymentEvents.length > 1000) {
    APP_STATE.paymentEvents = APP_STATE.paymentEvents.slice(0, 1000);
  }
}

async function paypalAccessToken(env) {
  const auth = btoa(`${cleanText(env.PAYPAL_CLIENT_ID || "", 260)}:${cleanText(env.PAYPAL_CLIENT_SECRET || "", 260)}`);
  const response = await fetch(`${paypalBase(env)}/v1/oauth2/token`, {
    method: "POST",
    headers: {
      Authorization: `Basic ${auth}`,
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

async function signDownloadToken(env, data) {
  const payloadRaw = JSON.stringify(data || {});
  const sig = await hmacHex(paymentSecret(env), payloadRaw);
  return `${toBase64Url(payloadRaw)}.${sig}`;
}

async function verifyDownloadToken(env, token) {
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
  const expected = await hmacHex(paymentSecret(env), payloadRaw);
  if (expected !== sig) {
    return null;
  }
  try {
    const payload = JSON.parse(payloadRaw);
    if (!payload?.exp || Number(payload.exp) < Math.floor(Date.now() / 1000)) {
      return null;
    }
    return payload;
  } catch (_error) {
    return null;
  }
}

function inferContentType(fileName) {
  const lower = String(fileName || "").toLowerCase();
  if (lower.endsWith(".mp3")) return "audio/mpeg";
  if (lower.endsWith(".wav")) return "audio/wav";
  if (lower.endsWith(".ogg")) return "audio/ogg";
  if (lower.endsWith(".m4a")) return "audio/mp4";
  if (lower.endsWith(".flac")) return "audio/flac";
  if (lower.endsWith(".zip")) return "application/zip";
  if (lower.endsWith(".vst3")) return "application/octet-stream";
  if (lower.endsWith(".deb")) return "application/vnd.debian.binary-package";
  if (lower.endsWith(".pkg.tar.zst")) return "application/zstd";
  return "application/octet-stream";
}

function parseRange(rangeHeader, totalSize) {
  const header = String(rangeHeader || "").trim();
  const total = Number(totalSize || 0);
  if (!header || !header.startsWith("bytes=") || total <= 0) {
    return null;
  }
  const match = header.match(/^bytes=(\d*)-(\d*)$/i);
  if (!match) {
    return { invalid: true };
  }
  const startRaw = match[1];
  const endRaw = match[2];
  let start = startRaw ? Number(startRaw) : 0;
  let end = endRaw ? Number(endRaw) : total - 1;
  if (!Number.isFinite(start) || !Number.isFinite(end)) {
    return { invalid: true };
  }
  if (!startRaw && endRaw) {
    const suffixLength = Number(endRaw);
    if (!Number.isFinite(suffixLength) || suffixLength <= 0) {
      return { invalid: true };
    }
    start = Math.max(0, total - suffixLength);
    end = total - 1;
  }
  if (start < 0 || end < 0 || start > end || start >= total) {
    return { invalid: true };
  }
  end = Math.min(end, total - 1);
  return {
    start,
    end,
    length: end - start + 1
  };
}

async function streamR2Object(env, objectKey, request, options = {}) {
  if (!hasR2Binding(env.R2_BUCKET)) {
    return null;
  }
  const key = String(objectKey || "").trim().slice(0, 700);
  if (!key) {
    return null;
  }

  const head = await env.R2_BUCKET.head(key);
  if (!head) {
    return null;
  }

  const totalSize = Number(head.size || 0);
  const range = parseRange(request.headers.get("range"), totalSize);
  if (range?.invalid) {
    return new Response(null, {
      status: 416,
      headers: {
        "Content-Range": `bytes */${totalSize}`,
        "Accept-Ranges": "bytes"
      }
    });
  }

  const getOptions = range
    ? {
        range: {
          offset: range.start,
          length: range.length
        }
      }
    : undefined;

  const object = await env.R2_BUCKET.get(key, getOptions);
  if (!object) {
    return null;
  }

  const headers = new Headers();
  headers.set("Accept-Ranges", "bytes");
  headers.set("Content-Type", head.httpMetadata?.contentType || inferContentType(key));
  headers.set("Cache-Control", options.noStore ? "no-store" : "public, max-age=300");

  if (range) {
    headers.set("Content-Range", `bytes ${range.start}-${range.end}/${totalSize}`);
    headers.set("Content-Length", String(range.length));
  } else if (totalSize > 0) {
    headers.set("Content-Length", String(totalSize));
  }

  if (options.downloadName) {
    headers.set("Content-Disposition", `attachment; filename="${String(options.downloadName).replace(/"/g, "_")}"`);
  }

  return new Response(object.body, {
    status: range ? 206 : 200,
    headers
  });
}

function decodePathSegment(value) {
  try {
    return decodeURIComponent(String(value || ""));
  } catch (_error) {
    return String(value || "");
  }
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
  const base = `/media/catalog/${encodeURIComponent(String(fileName || "").trim())}`;
  return appendVersionQuery(base, versionToken);
}

function buildCatalogAssetPath(fileName, versionToken = "") {
  const base = `/assets/audio/catalog/${encodeURIComponent(String(fileName || "").trim())}`;
  return appendVersionQuery(base, versionToken);
}

function buildCatalogStreamUrl(trackKey, versionToken = "") {
  const safeKey = cleanText(trackKey || "", 120);
  if (!safeKey) {
    return "";
  }
  return appendVersionQuery(`/api/v1/catalog/stream/${encodeURIComponent(safeKey)}`, versionToken);
}

async function catalogAssetExists(env, fileName) {
  const safeName = cleanText(fileName || "", 260);
  if (!safeName) {
    return false;
  }
  const mediaCandidates = [`catalog/${safeName}`, `media/catalog/${safeName}`, `beats/${safeName}`, safeName];
  const virtualCatalogPath = `${WEBSITE_ROOT}/assets/audio/catalog/${safeName}`;
  if (APP_STATE.binaryOverrides.has(virtualCatalogPath)) {
    return true;
  }
  if (hasR2Binding(env.R2_BUCKET)) {
    for (const key of mediaCandidates) {
      try {
        const head = await env.R2_BUCKET.head(key);
        if (head) {
          return true;
        }
      } catch (_error) {
      }
    }
  }
  if (!env?.ASSETS || typeof env.ASSETS.fetch !== "function") {
    return false;
  }
  try {
    const response = await env.ASSETS.fetch(new Request(`https://local/assets/audio/catalog/${encodeURIComponent(safeName)}`, {
      method: "GET",
      headers: {
        Range: "bytes=0-0"
      }
    }));
    return response.ok || response.status === 206;
  } catch (_error) {
    return false;
  }
}

async function resolveCatalogAssetFileName(env, fileName) {
  const safeName = cleanText(fileName || "", 260);
  if (!safeName) {
    return "";
  }
  if (await catalogAssetExists(env, safeName)) {
    return safeName;
  }
  const dot = safeName.lastIndexOf(".");
  const stem = dot > 0 ? safeName.slice(0, dot) : safeName;
  for (const extension of [".mp3", ".wav", ".flac", ".m4a", ".ogg", ".aiff", ".aif"]) {
    const directCandidate = `${stem}${extension}`;
    if (directCandidate !== safeName && await catalogAssetExists(env, directCandidate)) {
      return directCandidate;
    }
  }
  const previewName = `${stem}.preview.mp3`;
  if (await catalogAssetExists(env, previewName)) {
    return previewName;
  }
  return "";
}

function catalogFileNameFromUrl(rawUrl) {
  const value = String(rawUrl || "").trim();
  if (!value) {
    return "";
  }
  const withoutQuery = value.split("?")[0];
  const leaf = withoutQuery.slice(withoutQuery.lastIndexOf("/") + 1);
  try {
    return cleanText(decodeURIComponent(leaf), 260);
  } catch (_error) {
    return cleanText(leaf, 260);
  }
}

async function resolvePreferredCatalogAssetFileName(track, env) {
  const candidates = [
    cleanText(track?.file_name || "", 260),
    catalogFileNameFromUrl(track?.full_song_url || ""),
    cleanText(track?.asset_file_name || "", 260),
    catalogFileNameFromUrl(track?.stream_url || ""),
    catalogFileNameFromUrl(track?.public_url || "")
  ];

  for (const candidate of candidates) {
    if (!candidate) {
      continue;
    }
    const resolved = await resolveCatalogAssetFileName(env, candidate);
    if (resolved) {
      return resolved;
    }
  }

  return "";
}

function sanitizeUploadFileName(rawName, fallbackStem = "upload") {
  const raw = String(rawName || "").trim();
  const safe = raw.replace(/[^a-zA-Z0-9._\- ]/g, "_").replace(/\s+/g, " ").trim();
  const dotIndex = safe.lastIndexOf(".");
  const stem = (dotIndex > 0 ? safe.slice(0, dotIndex) : safe).trim() || fallbackStem;
  const extension = dotIndex > 0 ? safe.slice(dotIndex).toLowerCase() : "";
  return `${Date.now()}_${stem.replace(/\s+/g, "_")}${extension}`;
}

function normalizeTrackTitle(rawName) {
  const base = String(rawName || "")
    .replace(/\.[a-z0-9]+$/i, "")
    .replace(/[_-]+/g, " ")
    .replace(/\s+/g, " ")
    .trim();
  return base || "Untitled Track";
}

const CATALOG_ARTWORK_POOL = [
  "/assets/brand/north3rnlight3r_album_art.png",
  "/assets/brand/album_art_concept.png",
  "/assets/brand/hero_mascot.png"
];

function catalogArtworkUrlFor(seedValue = "") {
  const text = cleanText(seedValue || "north3rnlight3r", 260).toLowerCase();
  if (!text) {
    return CATALOG_ARTWORK_POOL[0];
  }
  let accumulator = 0;
  for (const ch of text) {
    accumulator = (accumulator + ch.charCodeAt(0)) % CATALOG_ARTWORK_POOL.length;
  }
  return CATALOG_ARTWORK_POOL[accumulator];
}

function buildSoundpackPublicUrl(fileName) {
  return `/media/soundpacks/${encodeURIComponent(String(fileName || "").trim())}`;
}

function previewDurationSeconds(env) {
  const parsed = Number(env.PREVIEW_SECONDS || 60);
  if (!Number.isFinite(parsed)) {
    return 60;
  }
  return Math.max(30, Math.min(90, Math.floor(parsed)));
}

async function normalizeCatalogTrackForPlayback(track, env) {
  const fileName = cleanText(track?.file_name || "", 260);
  const key = cleanText(track?.key || "", 120);
  const assetFileName = await resolvePreferredCatalogAssetFileName(track, env);
  const versionToken = trackVersionToken(track);
  const existingUrl = cleanText(track?.public_url || "", 2000);
  const publicUrl = assetFileName ? buildCatalogPublicUrl(assetFileName, versionToken)
    : fileName ? buildCatalogPublicUrl(fileName, versionToken)
    : existingUrl;
  const streamUrl = buildCatalogStreamUrl(key, versionToken) || publicUrl;
  return {
    ...track,
    key,
    file_name: fileName || track?.file_name || "",
    asset_file_name: assetFileName,
    public_url: publicUrl,
    full_song_url: publicUrl,
    stream_url: streamUrl,
    artwork_url: cleanText(track?.artwork_url || catalogArtworkUrlFor(track?.title || fileName), 400),
    full_song: true,
    preview_enabled: false
  };
}

function findCatalogTrackByKey(key) {
  const safeKey = cleanText(key || "", 120);
  if (!safeKey || !Array.isArray(APP_STATE.catalog)) {
    return null;
  }
  return APP_STATE.catalog.find((track) => cleanText(track?.key || "", 120) === safeKey) || null;
}

function hashHex(input) {
  const data = new TextEncoder().encode(String(input || ""));
  return crypto.subtle.digest("SHA-256", data).then((buffer) => hex(buffer));
}

function receiptSecret(env) {
  return cleanText(env.RECEIPT_SIGNING_SECRET || env.DAWAI_API_TOKEN || env.ADMIN_SESSION_SECRET || "", 260);
}

function contentTypeForPath(pathname) {
  const lower = String(pathname || "").toLowerCase();
  if (lower.endsWith(".html")) return "text/html; charset=utf-8";
  if (lower.endsWith(".css")) return "text/css; charset=utf-8";
  if (lower.endsWith(".js")) return "application/javascript; charset=utf-8";
  if (lower.endsWith(".json")) return "application/json; charset=utf-8";
  if (lower.endsWith(".txt")) return "text/plain; charset=utf-8";
  if (lower.endsWith(".png")) return "image/png";
  if (lower.endsWith(".jpg") || lower.endsWith(".jpeg")) return "image/jpeg";
  if (lower.endsWith(".webp")) return "image/webp";
  if (lower.endsWith(".gif")) return "image/gif";
  if (lower.endsWith(".svg")) return "image/svg+xml";
  if (lower.endsWith(".mp3")) return "audio/mpeg";
  if (lower.endsWith(".wav")) return "audio/wav";
  if (lower.endsWith(".flac")) return "audio/flac";
  if (lower.endsWith(".m4a")) return "audio/mp4";
  if (lower.endsWith(".ogg")) return "audio/ogg";
  return "text/plain; charset=utf-8";
}

function assetCacheControlForPath(pathname) {
  const cleanPath = String(pathname || "").split("?")[0];
  if (!cleanPath) {
    return "";
  }
  if (cleanPath === "/" || cleanPath === "/index.html") {
    return "no-store";
  }
  if (cleanPath === "/app.js" || cleanPath === "/config.js" || cleanPath === "/styles.css") {
    return "no-store";
  }
  if (cleanPath.startsWith("/assets/data/")) {
    return "no-store";
  }
  return "";
}

function withCacheControl(response, cacheControl) {
  if (!response || !cacheControl) {
    return response;
  }
  const headers = new Headers(response.headers);
  headers.set("Cache-Control", cacheControl);
  return new Response(response.body, {
    status: response.status,
    statusText: response.statusText,
    headers
  });
}

function normalizeVirtualPath(rawPath) {
  const cleaned = String(rawPath || "").replace(/\\/g, "/").replace(/^\/+/, "");
  if (!cleaned) {
    return "";
  }
  const safe = cleaned
    .split("/")
    .filter(Boolean)
    .map((segment) => segment.replace(/[^a-zA-Z0-9._\-]/g, ""))
    .filter(Boolean)
    .join("/");
  if (!safe) {
    return "";
  }
  if (safe.startsWith(`${WEBSITE_ROOT}/`)) {
    return safe;
  }
  return `${WEBSITE_ROOT}/${safe}`;
}

function virtualPathToAssetPath(virtualPath) {
  if (!virtualPath.startsWith(`${WEBSITE_ROOT}/`)) {
    return null;
  }
  return `/${virtualPath.slice(`${WEBSITE_ROOT}/`.length)}`;
}

function assetPathToVirtualPath(pathname) {
  const clean = String(pathname || "/");
  if (clean === "/" || clean === "") {
    return WEBSITE_INDEX_PATH;
  }
  return normalizeVirtualPath(clean.slice(1));
}

function appendLog(eventType, payload = {}) {
  APP_STATE.logs.unshift({
    ts: nowIso(),
    event_type: cleanText(eventType || "event", 120),
    payload
  });
  if (APP_STATE.logs.length > 1000) {
    APP_STATE.logs = APP_STATE.logs.slice(0, 1000);
  }
}

function recordDashboardActivity(kind, routePath, payload = {}) {
  const safeKind = cleanText(kind || "event", 80);
  const safeRoute = cleanText(routePath || "/", 260);
  APP_STATE.activity.last_request_at = nowIso();
  if (safeKind === "page_view") {
    APP_STATE.activity.page_views += 1;
  } else if (safeKind === "api_hit") {
    APP_STATE.activity.api_hits += 1;
  } else if (safeKind === "media_stream") {
    APP_STATE.activity.media_streams += 1;
  } else if (safeKind === "download") {
    APP_STATE.activity.downloads += 1;
  }
  APP_STATE.activity.recent.unshift({
    ts: APP_STATE.activity.last_request_at,
    kind: safeKind,
    path: safeRoute,
    payload
  });
  if (APP_STATE.activity.recent.length > 200) {
    APP_STATE.activity.recent = APP_STATE.activity.recent.slice(0, 200);
  }
}

async function signReceipt(env, receipt) {
  const canonical = JSON.stringify({
    receipt_id: receipt.receipt_id,
    sale_id: receipt.sale_id,
    issued_at: receipt.issued_at,
    amount: receipt.amount,
    currency: receipt.currency,
    item_name: receipt.item_name,
    customer_email: receipt.customer_email
  });
  return hmacHex(receiptSecret(env), canonical);
}

async function watermarkToken(env, receiptId, issuedAt) {
  return (await hmacHex(receiptSecret(env), `${receiptId}|${issuedAt}|watermark`)).slice(0, 20);
}

function buildReceiptHtml(receipt) {
  const esc = (value) =>
    String(value || "").replace(/[&<>"]/g, (ch) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[ch] || ch));
  const wm = esc(receipt.watermark_token);
  return `<!doctype html>
<html lang="en"><head><meta charset="utf-8" /><meta name="viewport" content="width=device-width, initial-scale=1" />
<title>Receipt ${esc(receipt.receipt_id)}</title>
<style>
body{margin:0;background:#071220;color:#e8f7ff;font-family:Segoe UI,Arial,sans-serif}
.wrap{max-width:900px;margin:22px auto;padding:20px}
.card{position:relative;border:1px solid rgba(88,216,255,.52);border-radius:16px;padding:24px;overflow:hidden;background:rgba(7,20,35,.96)}
.card::before{content:\"\";position:absolute;inset:0;background:linear-gradient(180deg,rgba(18,42,64,.16),rgba(4,10,18,.04));opacity:1}
.content{position:relative;z-index:1}.line{border-top:1px solid rgba(88,216,255,.34);margin:16px 0}.mono{font-family:ui-monospace,monospace}
</style></head><body><div class="wrap"><div class="card"><div class="content">
<h1>North3rnLight3r Official Receipt</h1>
<p>Receipt ID: <span class="mono">${esc(receipt.receipt_id)}</span></p>
<p>Sale ID: <span class="mono">${esc(receipt.sale_id)}</span></p>
<p>Issued: ${esc(receipt.issued_at)}</p>
<div class="line"></div>
<p>Customer: ${esc(receipt.customer_name || "N/A")} (${esc(receipt.customer_email || "N/A")})</p>
<p>Item: ${esc(receipt.item_name)}</p>
<p>Amount: ${esc(receipt.currency)} ${esc(receipt.amount)}</p>
<p>Payment Provider: ${esc(receipt.payment_provider)}</p>
<p>Transaction: <span class="mono">${esc(receipt.payment_txn_id || "N/A")}</span></p>
<div class="line"></div>
<p>Signature</p><p class="mono">${esc(receipt.signature)}</p>
<p>Watermark</p><p class="mono">${esc(receipt.watermark_token)}</p>
</div></div></div></body></html>`;
}

async function ensureSeedData(env) {
  if (APP_STATE.seeded) {
    return;
  }
  APP_STATE.seeded = true;
  try {
    const catalogRes = await env.ASSETS.fetch("https://local/assets/data/beat_catalog.json");
    if (catalogRes.ok) {
      const tracks = await catalogRes.json().catch(() => []);
      if (Array.isArray(tracks) && tracks.length > 0) {
        APP_STATE.catalog = tracks.map((track) => {
          const next = { ...(track || {}) };
          const fileName = cleanText(next.file_name || "", 260);
          const assetFileName = cleanText(next.asset_file_name || "", 260);
          const effectiveAssetFileName = assetFileName || fileName;
          if (effectiveAssetFileName) {
            const assetUrl = buildCatalogPublicUrl(effectiveAssetFileName);
            next.public_url = assetUrl;
            next.full_song_url = assetUrl;
            next.stream_url = assetUrl;
          }
          return next;
        });
      }
    }
  } catch (_error) {
  }
}

function parseAuthToken(request) {
  const header = String(request.headers.get("authorization") || "");
  if (header.startsWith("Bearer ")) {
    return header.slice("Bearer ".length).trim();
  }
  const url = new URL(request.url);
  return String(url.searchParams.get("token") || "").trim();
}

function toBase64Url(raw) {
  return btoa(raw).replace(/\+/g, "-").replace(/\//g, "_").replace(/=+$/g, "");
}

function fromBase64Url(raw) {
  const base = raw.replace(/-/g, "+").replace(/_/g, "/");
  const pad = base.length % 4 === 0 ? "" : "=".repeat(4 - (base.length % 4));
  return atob(base + pad);
}

function hex(buffer) {
  return Array.from(new Uint8Array(buffer), (b) => b.toString(16).padStart(2, "0")).join("");
}

function adminCreds(env) {
  const username = cleanText(env.ADMIN_USERNAME || DEFAULT_ADMIN_USERNAME, 80) || DEFAULT_ADMIN_USERNAME;
  const password = cleanText(env.ADMIN_PASSWORD || DEFAULT_ADMIN_PASSWORD, 160) || DEFAULT_ADMIN_PASSWORD;
  return {
    username,
    password
  };
}

function sessionSecret(env) {
  return cleanText(env.ADMIN_SESSION_SECRET || env.DAWAI_API_TOKEN || DEFAULT_ADMIN_SESSION_SECRET, 260)
    || DEFAULT_ADMIN_SESSION_SECRET;
}

async function hmacHex(secret, message) {
  const key = await crypto.subtle.importKey(
    "raw",
    new TextEncoder().encode(secret),
    { name: "HMAC", hash: "SHA-256" },
    false,
    ["sign"]
  );
  const sig = await crypto.subtle.sign("HMAC", key, new TextEncoder().encode(message));
  return hex(sig);
}

async function createSessionToken(env, username) {
  const payload = {
    u: username,
    n: randomTokenHex(8),
    iat: Date.now(),
    exp: Date.now() + SESSION_TTL_MS
  };
  const payloadRaw = JSON.stringify(payload);
  const sig = await hmacHex(sessionSecret(env), payloadRaw);
  return {
    token: `${toBase64Url(payloadRaw)}.${sig}`,
    expires_at: new Date(payload.exp).toISOString()
  };
}

async function verifySessionToken(env, token) {
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

  let payload;
  try {
    payload = JSON.parse(payloadRaw);
  } catch (_error) {
    return null;
  }

  if (!payload?.u || !payload?.exp || payload.exp <= Date.now()) {
    return null;
  }

  const expected = await hmacHex(sessionSecret(env), payloadRaw);
  if (expected !== sig) {
    return null;
  }

  return {
    username: String(payload.u),
    expires_at: new Date(payload.exp).toISOString()
  };
}

function requireApiToken(request, env) {
  const expected = cleanText(env.DAWAI_API_TOKEN || "", 260);
  if (!expected) {
    return null;
  }
  const got = parseAuthToken(request);
  if (got !== expected) {
    return json({ ok: false, error: "unauthorized" }, { status: 401 });
  }
  return null;
}

async function requireAdmin(request, env) {
  if (!sessionSecret(env)) {
    return {
      error: json(
        {
          ok: false,
          error: "admin session secret is not configured",
          hint: "set ADMIN_SESSION_SECRET or DAWAI_API_TOKEN in the worker environment or local .dev.vars"
        },
        { status: 503 }
      )
    };
  }
  const token = parseAuthToken(request);
  const session = await verifySessionToken(env, token);
  if (!session) {
    return { error: json({ ok: false, error: "admin session invalid" }, { status: 401 }) };
  }
  return { session };
}

function isMaintenanceBypassPath(pathname) {
  return pathname === "/api/v1/health" || pathname.startsWith("/api/v1/admin/");
}

async function hasMaintenanceAccess(request, env) {
  const token = parseAuthToken(request);
  if (!token) {
    return false;
  }
  const apiToken = cleanText(env.DAWAI_API_TOKEN || "", 260);
  if (apiToken && token === apiToken) {
    return true;
  }
  const session = await verifySessionToken(env, token);
  return Boolean(session);
}

function maintenanceHtml() {
  const body = `<!doctype html>
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
  return new Response(body, {
    status: 503,
    headers: {
      "Content-Type": "text/html; charset=utf-8",
      "Cache-Control": "no-store",
      "Retry-After": "3600"
    }
  });
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
      const code = normalizePromoCode(entry?.code || "");
      const products = Array.isArray(entry?.products)
        ? entry.products.map((p) => normalizePromoProduct(p)).filter(Boolean)
        : [];
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
      if (!code || products.length === 0) {
        return null;
      }
      return {
        code,
        products,
        active: entry?.active !== false,
        max_uses: Number.isFinite(Number(entry?.max_uses)) ? Math.max(0, Number(entry.max_uses)) : 0,
        uses: Number.isFinite(Number(entry?.uses)) ? Math.max(0, Number(entry.uses)) : 0,
        note: cleanText(entry?.note || "", 180),
        entitlement_type: paymentUnlock ? "payment_unlock" : lifetime ? "lifetime" : installOnly ? "install_only" : "session_bundle",
        lifetime,
        payment_bypass: paymentBypass,
        admin_code: adminCode,
        payment_verified: paymentVerified,
        session_limits: lifetime || installOnly || paymentUnlock ? { analyze: 0, compare: 0, reference: 0 } : sessionLimits,
        created_at: cleanText(entry?.created_at || nowIso(), 80),
        last_redeemed_at: cleanText(entry?.last_redeemed_at || "", 80)
      };
    })
    .filter(Boolean);
}

function hydratePromoCodesFromEnv(env) {
  if (promoCodesHydratedFromEnv) {
    return;
  }
  promoCodesHydratedFromEnv = true;
  const raw = String(env.PROMO_CODES_JSON || "").trim();
  if (!raw) {
    return;
  }
  let parsed = null;
  try {
    parsed = JSON.parse(raw);
  } catch (_error) {
    return;
  }
  if (!Array.isArray(parsed) || parsed.length === 0) {
    return;
  }
  APP_STATE.promoCodes = normalizePromoCodes(parsed);
}

function normalizeModelName(raw) {
  return cleanText(raw || "", 220);
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

function configuredOpenAiModelList(env = {}) {
  const raw = cleanText(
    env.DAWAI_OPENAI_MODEL_ALLOWLIST ||
    env.OPENAI_MODEL_ALLOWLIST ||
    env.DAWAI_OPENAI_MODELS ||
    "",
    2000
  );
  if (!raw) {
    return [...DEFAULT_OPENAI_MODEL_ALLOWLIST];
  }
  const parsed = raw
    .split(/[,\n]/)
    .map((item) => normalizeModelName(item))
    .filter(Boolean);
  return parsed.length > 0 ? Array.from(new Set(parsed)) : [...DEFAULT_OPENAI_MODEL_ALLOWLIST];
}

function activeOpenAiModelList(env = {}) {
  return configuredOpenAiModelList(env);
}

function configuredOpenAiPrimaryModel(env = {}) {
  const requested = normalizeModelName(
    env.DAWAI_OPENAI_PRIMARY_MODEL ||
    env.OPENAI_PRIMARY_MODEL ||
    DEFAULT_OPENAI_PRIMARY_MODEL
  );
  return activeOpenAiModelList(env).includes(requested) ? requested : activeOpenAiModelList(env)[0] || DEFAULT_OPENAI_PRIMARY_MODEL;
}

function normalizeRequestedOpenAiModel(value, env = {}) {
  const model = normalizeModelName(value);
  return activeOpenAiModelList(env).includes(model) ? model : configuredOpenAiPrimaryModel(env);
}

function loadBrainConfig(env) {
  if (APP_STATE.brain && APP_STATE.brain.brain_id) {
    return APP_STATE.brain;
  }
  const raw = cleanText(env.AIFR3D_BRAIN_JSON || "", 4000);
  if (!raw) {
    APP_STATE.brain = DEFAULT_AIFR3D_BRAIN;
    return APP_STATE.brain;
  }
  try {
    const parsed = JSON.parse(raw);
    APP_STATE.brain = {
      ...DEFAULT_AIFR3D_BRAIN,
      ...(parsed && typeof parsed === "object" ? parsed : {}),
      model: configuredOpenAiPrimaryModel(env)
    };
  } catch (_error) {
    APP_STATE.brain = DEFAULT_AIFR3D_BRAIN;
  }
  return APP_STATE.brain;
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

function loadChatSettings(env) {
  if (APP_STATE.chatSettings) {
    return APP_STATE.chatSettings;
  }
  const raw = cleanMultilineText(env.DAWAI_CHAT_SETTINGS_JSON || env.CHAT_SETTINGS_JSON || "", 8000);
  if (!raw) {
    APP_STATE.chatSettings = structuredClone(DEFAULT_CHAT_SETTINGS);
    return APP_STATE.chatSettings;
  }
  try {
    APP_STATE.chatSettings = sanitizeChatSettings(JSON.parse(raw));
  } catch (_error) {
    APP_STATE.chatSettings = structuredClone(DEFAULT_CHAT_SETTINGS);
  }
  return APP_STATE.chatSettings;
}

function setChatSettings(env, value) {
  APP_STATE.chatSettings = sanitizeChatSettings({
    ...loadChatSettings(env),
    ...(value && typeof value === "object" ? value : {})
  });
  return APP_STATE.chatSettings;
}

async function emitChatWebhook(env, eventType, payload) {
  const settings = loadChatSettings(env);
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
    headers["X-AIFR3D-Webhook-Signature"] = await hmacHex(settings.webhook.secret, body);
  }
  try {
    await fetch(settings.webhook.url, {
      method: "POST",
      headers,
      body
    });
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

function buildSystemPrompt(env, personalityMode, agentModes, chatSettings = DEFAULT_CHAT_SETTINGS) {
  const brain = loadBrainConfig(env);
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
    "Use keys: summary, issues, action_suggestions, state_update.",
    prefix ? `Additional system instruction:\n${prefix}` : "",
    suffix ? `Final response constraint:\n${suffix}` : ""
  ].join("\n");
}

function openAiConfig(env) {
  if (APP_STATE.openai) {
    return APP_STATE.openai;
  }
  const apiKey = parseOpenAiApiKey(env.OPENAI_API_KEY || env.DAWAI_OPENAI_API_KEY || "");
  const cfg = {
    api_key: apiKey,
    base_url: "https://api.openai.com/v1/responses",
    model: configuredOpenAiPrimaryModel(env),
    model_list: activeOpenAiModelList(env),
    updated_at: nowIso(),
    source: "env"
  };
  cfg.ready = Boolean(apiKey);
  APP_STATE.openai = cfg;
  return cfg;
}

function setOpenAiConfig(apiKey, baseUrl) {
  const current = APP_STATE.openai || openAiConfig({});
  const nextApiKey = parseOpenAiApiKey(apiKey || current.api_key || "");
  APP_STATE.openai = {
    api_key: nextApiKey,
    base_url: "https://api.openai.com/v1/responses",
    model: current.model || DEFAULT_OPENAI_PRIMARY_MODEL,
    model_list: Array.isArray(current.model_list) && current.model_list.length > 0 ? current.model_list : [...DEFAULT_OPENAI_MODEL_ALLOWLIST],
    updated_at: nowIso(),
    source: "admin",
    ready: Boolean(nextApiKey)
  };
  return APP_STATE.openai;
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

function parseJsonFromText(text) {
  const content = String(text || "").trim();
  const start = content.indexOf("{");
  const end = content.lastIndexOf("}");
  if (start < 0 || end < 0 || end <= start) {
    return null;
  }
  try {
    return JSON.parse(content.slice(start, end + 1));
  } catch (_error) {
    return null;
  }
}

async function runOpenAiChat(env, prompt, sessionId, options = {}) {
  const cfg = openAiConfig(env);
  const settings = loadChatSettings(env);
  if (!cfg.ready) {
    await emitChatWebhook(env, "chat.failed", {
      ok: false,
      provider: "openai",
      session_id: sessionId,
      model: cfg.model || "unset",
      error: "OpenAI API key missing — AIFR3D chat unavailable"
    });
    return {
      ok: false,
      error: "OpenAI API key missing — AIFR3D chat unavailable",
      model: cfg.model || "unset"
    };
  }

  const memory = APP_STATE.memory.get(sessionId) || [];
  const summary = memory.slice(-Number(settings.context.summary_items || 6));
  const sessionState = APP_STATE.memorySummary.get(sessionId) || {};
  const brain = loadBrainConfig(env);
  const promptMaxChars = Number(settings.context.max_prompt_chars || brain?.runtime?.max_prompt_chars || 4000);
  const promptText = String(prompt || "").slice(0, promptMaxChars);
  const personalityMode = cleanText(
    options?.personality_mode || settings.prompt.personality_mode || brain?.identity?.mode || "professional_mentor",
    80
  );
  const agentModes = normalizeModelList(options?.agent_modes || []);
  const model = normalizeRequestedOpenAiModel(options?.model || cfg.model, env);
  const maxTokens = Number(settings.response.max_output_tokens || brain?.runtime?.max_tokens || 900);
  const systemPrompt = buildSystemPrompt(env, personalityMode, agentModes, settings);
  const useChatCompletions = /\/chat\/completions$/i.test(cfg.base_url);
  const previousResponseId = settings.context.use_previous_response_id
    ? cleanText(sessionState.previous_response_id || "", 200)
    : "";
  const compactedSummary = summary.slice(-Number(settings.context.compact_threshold || 12));

  const body = useChatCompletions
    ? {
        model,
        messages: [
          { role: "system", content: systemPrompt },
          {
            role: "user",
            content: JSON.stringify({
              prompt: promptText,
              session_id: sessionId,
              memory: compactedSummary,
              aifred_brain: {
                brain_id: brain?.brain_id || "aifr3d_brain_v2_2_4",
                version: brain?.version || "2.2.4",
                personality_mode: personalityMode,
                agent_modes: agentModes.length > 0 ? agentModes : ["mix_engineer", "analysis_worker", "planning_worker"]
              },
              runtime_preferences: {
                transport_mode: settings.transport_mode,
                verbosity: settings.response.verbosity,
                tone: settings.prompt.tone,
                reasoning_effort: settings.reasoning.effort
              }
            })
          }
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
            content: [
              {
                type: "input_text",
                text: JSON.stringify({
                  prompt: promptText,
                  session_id: sessionId,
                  memory: compactedSummary,
                  aifred_brain: {
                    brain_id: brain?.brain_id || "aifr3d_brain_v2_2_4",
                    version: brain?.version || "2.2.4",
                    personality_mode: personalityMode,
                    agent_modes: agentModes.length > 0 ? agentModes : ["mix_engineer", "analysis_worker", "planning_worker"]
                  },
                  runtime_preferences: {
                    transport_mode: settings.transport_mode,
                    verbosity: settings.response.verbosity,
                    tone: settings.prompt.tone,
                    reasoning_effort: settings.reasoning.effort
                  }
                })
              }
            ]
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

  try {
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
      await emitChatWebhook(env, "chat.failed", {
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
        model,
        attempted_models: [model]
      };
    }

    const parsed = parseJsonFromText(rawText) || {
      summary: rawText,
      issues: [],
      action_suggestions: [],
      state_update: {}
    };

    const normalized = {
      summary: cleanText(parsed?.summary || "AI response ready", 1800),
      issues: Array.isArray(parsed?.issues) ? parsed.issues : [],
      action_suggestions: Array.isArray(parsed?.action_suggestions) ? parsed.action_suggestions : [],
      state_update: parsed?.state_update && typeof parsed.state_update === "object" ? parsed.state_update : {}
    };

    memory.push({ t: nowIso(), role: "user", text: promptText });
    memory.push({ t: nowIso(), role: "assistant", text: normalized.summary });
    APP_STATE.memory.set(sessionId, memory.slice(-Number(settings.context.memory_window_items || brain?.runtime?.max_recent_interactions || 40)));
    const responseId = cleanText(payload?.id || "", 200);
    APP_STATE.memorySummary.set(sessionId, {
      previous_response_id: responseId,
      updated_at: nowIso()
    });
    await emitChatWebhook(env, "chat.completed", {
      ok: true,
      provider: "openai",
      session_id: sessionId,
      model,
      attempted_models: [model],
      response_id: responseId,
      summary: normalized.summary,
      issue_count: normalized.issues.length,
      action_suggestion_count: normalized.action_suggestions.length
    });

    return {
      ok: true,
      data: normalized,
      model,
      attempted_models: [model],
      response_id: responseId,
      brain_id: brain?.brain_id || "aifr3d_brain_v2_2_4"
    };
  } catch (error) {
    await emitChatWebhook(env, "chat.failed", {
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
      model,
      attempted_models: [model]
    };
  }
}

async function handleApi(request, env, url) {
  const { pathname } = url;
  await ensureSeedData(env);

  if (pathname === "/api/v1/health" && request.method === "GET") {
    const cfg = openAiConfig(env);
    const brain = loadBrainConfig(env);
    const products = await loadStoreProducts(env);
    return json({
      ok: true,
      service: "north3rnlight3r-website-worker",
      mode: "edge-worker",
      timestamp_utc: nowIso(),
      actions: ["session.save", "catalog.refresh", "analysis.run", "command.exec", "soundpack.upload"],
      chat_provider: "openai",
      openai_model: cfg.model || null,
      openai_model_list: activeOpenAiModelList(cfg),
      openai_ready: cfg.ready,
      aifred_brain_id: brain?.brain_id || "aifr3d_brain_v2_2_4",
      aifred_brain_version: brain?.version || "2.2.4",
      admin_configured: Boolean(adminCreds(env)),
      inquiries_count: APP_STATE.inquiries.length,
      sales_count: APP_STATE.sales.length,
      event_log_count: APP_STATE.logs.length,
      store_product_count: products.length,
      website_private_mode: sitePrivateMode(env),
      checkout_provider: paymentMode(env),
      paypal_ready: paypalReady(env),
      paypal_mode: paypalMode(env),
      r2_ready: hasR2Binding(env.R2_BUCKET),
      default_online_api: "https://www.north3rnlight3r.com",
      default_online_ws: "wss://www.north3rnlight3r.com/ws/chat"
    });
  }

  if (pathname === "/api/v1/brain/config" && request.method === "GET") {
    const apiGate = requireApiToken(request, env);
    if (apiGate) {
      return apiGate;
    }
    return json({ ok: true, brain: loadBrainConfig(env) });
  }

  if (pathname === "/api/v1/limits" && request.method === "GET") {
    const apiGate = requireApiToken(request, env);
    if (apiGate) {
      return apiGate;
    }
    const cfg = openAiConfig(env);
    return json({
      ok: true,
      limits: {
        ...API_LIMITS,
        chat: {
          ...API_LIMITS.chat,
          active_models: activeOpenAiModelList(cfg)
        }
      }
    });
  }

  if (pathname === "/api/v1/models/list" && request.method === "GET") {
    const cfg = openAiConfig(env);
    return json({
      ok: true,
      provider: "openai",
      active_model: cfg.model || null,
      models: activeOpenAiModelList(cfg)
    });
  }

  if (pathname === "/api/v1/chat/settings" && request.method === "GET") {
    const apiGate = requireApiToken(request, env);
    if (apiGate) {
      return apiGate;
    }
    return json({
      ok: true,
      provider: "openai",
      websocket_url: "wss://www.north3rnlight3r.com/ws/chat",
      persistence: "worker-memory-or-env",
      settings: loadChatSettings(env)
    });
  }

  if (pathname === "/api/v1/pay/products" && request.method === "GET") {
    const products = await loadStoreProducts(env);
    const publicProducts = products
      .filter((item) => item.active)
      .map((item) => ({
        sku: item.sku,
        name: item.name,
        description: item.description,
        priceUSD: item.beta_free_trial ? null : item.priceUSD,
        currency: item.currency,
        beta_free_trial: item.beta_free_trial === true,
        availability_label: item.availability_label || "",
        future_price_label: item.future_price_label || "",
        download_url: item.beta_free_trial ? item.download_url : "",
        active: item.active
      }));
    return json({
      ok: true,
      products: publicProducts,
      provider: paymentMode(env)
    });
  }

  if (pathname === "/api/v1/pay/create-order" && request.method === "POST") {
    const body = await request.json().catch(() => ({}));
    const sku = normalizeSku(body?.sku || "");
    const product = sku ? await loadStoreProductBySku(env, sku) : null;
    const itemName = cleanText(body?.item_name || product?.name || "North3rnLight3r Product", 220);
    const amount = normalizeUsdAmount(body?.amount || product?.priceUSD || "0");
    const currency = normalizeCurrencyCode(body?.currency || product?.currency || "USD");

    if (product?.beta_free_trial) {
      return json(
        {
          ok: false,
          error: BETA_FREE_TRIAL_MESSAGE,
          sku: product.sku,
          availability_label: product.availability_label || "Free to try for a limited time",
          future_price_label: product.future_price_label || "$149.99 after Beta 2.2.4",
          direct_download_url: product.download_url || ""
        },
        { status: 409 }
      );
    }

    if (!itemName || amount === "0.00") {
      return json({ ok: false, error: "valid sku or item_name+amount required" }, { status: 400 });
    }

    if (paymentMode(env) !== "paypal" || !paypalReady(env)) {
      return json(
        {
          ok: false,
          error: paymentMode(env) === "paypal" ? "paypal backend not configured" : "checkout temporarily unavailable",
          provider: paymentMode(env),
          fallback_url: checkoutFallbackUrl(env, itemName, amount, currency)
        },
        { status: 503 }
      );
    }

    const accessToken = await paypalAccessToken(env);
    const response = await fetch(`${paypalBase(env)}/v2/checkout/orders`, {
      method: "POST",
      headers: {
        Authorization: `Bearer ${accessToken}`,
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
      pushPaymentEvent("paypal.create_order.error", {
        sku,
        status: response.status,
        error: payload?.message || payload?.error || "paypal create-order failed"
      });
      return json(
        {
          ok: false,
          error: payload?.message || payload?.error || "paypal create-order failed",
          fallback_url: checkoutFallbackUrl(env, itemName, amount, currency)
        },
        { status: 502 }
      );
    }

    const approvalUrl =
      (Array.isArray(payload.links) ? payload.links.find((entry) => entry?.rel === "approve")?.href : "") ||
      checkoutFallbackUrl(env, itemName, amount, currency);
    const orderRecord = {
      order_id: payload.id,
      status: cleanText(payload.status || "CREATED", 40),
      sku: product?.sku || sku || "",
      item_name: itemName,
      amount,
      currency,
      product: product || null,
      created_at: nowIso(),
      source: "paypal-create-order"
    };
    await storePaymentOrder(env, payload.id, orderRecord);
    pushPaymentEvent("paypal.create_order.ok", {
      order_id: payload.id,
      sku: orderRecord.sku || null
    });

    return json({
      ok: true,
      provider: "paypal",
      order_id: payload.id,
      status: payload.status || "CREATED",
      approval_url: approvalUrl
    });
  }

  if (pathname === "/api/v1/pay/capture-order" && request.method === "POST") {
    const body = await request.json().catch(() => ({}));
    const orderId = cleanText(body?.order_id || body?.token || "", 120);
    if (!orderId) {
      return json({ ok: false, error: "order_id is required" }, { status: 400 });
    }

    if (paymentMode(env) !== "paypal" || !paypalReady(env)) {
      return json(
        {
          ok: false,
          error: paymentMode(env) === "paypal" ? "paypal backend not configured" : "manual checkout has no capture endpoint",
          provider: paymentMode(env)
        },
        { status: 503 }
      );
    }

    const existing = (await readPaymentOrder(env, orderId)) || {};
    const accessToken = await paypalAccessToken(env);
    const response = await fetch(`${paypalBase(env)}/v2/checkout/orders/${encodeURIComponent(orderId)}/capture`, {
      method: "POST",
      headers: {
        Authorization: `Bearer ${accessToken}`,
        "Content-Type": "application/json"
      }
    });
    const payload = await response.json().catch(() => null);

    if (!response.ok || !payload?.id) {
      pushPaymentEvent("paypal.capture.error", {
        order_id: orderId,
        status: response.status,
        error: payload?.message || payload?.error || "paypal capture failed"
      });
      return json({ ok: false, error: payload?.message || payload?.error || "paypal capture failed" }, { status: 502 });
    }

    const status = cleanText(payload.status || "", 40);
    const pu = Array.isArray(payload.purchase_units) ? payload.purchase_units[0] || {} : {};
    const capture = Array.isArray(pu?.payments?.captures) ? pu.payments.captures[0] || {} : {};
    const currency = normalizeCurrencyCode(capture?.amount?.currency_code || pu?.amount?.currency_code || existing.currency || "USD");
    const amount = normalizeUsdAmount(capture?.amount?.value || pu?.amount?.value || existing.amount || "0");
    const sku = normalizeSku(existing?.sku || pu?.custom_id || pu?.reference_id || "");
    const product = existing?.product || (sku ? await loadStoreProductBySku(env, sku) : null);
    const payerEmail = cleanText(payload?.payer?.email_address || "", 180);
    const txnId = cleanText(capture?.id || "", 180);
    const itemName = cleanText(existing?.item_name || product?.name || pu?.description || "North3rnLight3r Product", 220);

    const orderRecord = {
      order_id: payload.id,
      status,
      sku,
      product: product || null,
      item_name: itemName,
      amount,
      currency,
      payer_email: payerEmail,
      payment_txn_id: txnId,
      captured_at: nowIso(),
      source: "paypal-capture-order"
    };
    await storePaymentOrder(env, payload.id, orderRecord);

    if (status !== "COMPLETED") {
      pushPaymentEvent("paypal.capture.pending", { order_id: payload.id, status });
      return json({
        ok: false,
        error: `paypal capture status: ${status || "unknown"}`,
        order_id: payload.id,
        status
      });
    }

    const sale = {
      sale_id: randomTokenHex(8),
      item_name: itemName,
      amount,
      currency,
      customer_name: cleanText(payload?.payer?.name?.given_name || "", 120),
      customer_email: payerEmail || "unknown@paypal.customer",
      payment_provider: "paypal",
      payment_txn_id: txnId,
      created_at: nowIso(),
      recorded_by: "paypal-web-checkout"
    };

    const receipt = {
      receipt_id: randomTokenHex(8),
      sale_id: sale.sale_id,
      issued_at: nowIso(),
      customer_name: sale.customer_name,
      customer_email: sale.customer_email,
      item_name: sale.item_name,
      amount: sale.amount,
      currency: sale.currency,
      payment_provider: sale.payment_provider,
      payment_txn_id: sale.payment_txn_id
    };
    receipt.watermark_token = await watermarkToken(env, receipt.receipt_id, receipt.issued_at);
    receipt.signature = await signReceipt(env, receipt);
    receipt.receipt_url = `/receipts/${receipt.receipt_id}`;
    sale.receipt_id = receipt.receipt_id;
    sale.receipt_signature = receipt.signature;
    sale.receipt_url = receipt.receipt_url;
    APP_STATE.sales.unshift(sale);
    APP_STATE.sales = APP_STATE.sales.slice(0, 5000);
    APP_STATE.receipts.set(receipt.receipt_id, receipt);

    const exp = Math.floor(Date.now() / 1000) + 24 * 60 * 60;
    const downloadToken = await signDownloadToken(env, {
      order_id: payload.id,
      sku: product?.sku || sku || "",
      object_key: String(product?.object_key || "").trim().slice(0, 500),
      download_url: String(product?.download_url || "").trim().slice(0, 700),
      exp
    });
    const unlockProducts = Array.isArray(product?.unlock_products) ? product.unlock_products : [];
    pushPaymentEvent("paypal.capture.ok", {
      order_id: payload.id,
      sku: product?.sku || sku || null,
      txn: txnId || null
    });

    return json({
      ok: true,
      provider: "paypal",
      order_id: payload.id,
      status,
      amount,
      currency,
      download_url: `/api/v1/pay/download?token=${encodeURIComponent(downloadToken)}`,
      unlock_products: unlockProducts,
      payment_unlock: PAYMENT_UNLOCK_BUNDLE,
      receipt: {
        receipt_id: receipt.receipt_id,
        signature: receipt.signature,
        receipt_url: receipt.receipt_url
      }
    });
  }

  if (pathname === "/api/v1/pay/webhook" && request.method === "POST") {
    const event = await request.json().catch(() => ({}));
    const eventId = cleanText(event?.id || randomTokenHex(6), 120);
    const eventType = cleanText(event?.event_type || "paypal.webhook.unknown", 160);
    pushPaymentEvent(eventType, { id: eventId });
    appendLog("paypal.webhook", { id: eventId, type: eventType });
    return json({ ok: true, id: eventId, event_type: eventType });
  }

  if (pathname === "/api/v1/pay/download" && request.method === "GET") {
    const token = cleanText(url.searchParams.get("token") || "", 4000);
    if (!token) {
      return json({ ok: false, error: "token required" }, { status: 400 });
    }
    const payload = await verifyDownloadToken(env, token);
    if (!payload) {
      return json({ ok: false, error: "invalid or expired token" }, { status: 401 });
    }
    const order = await readPaymentOrder(env, payload.order_id || "");
    if (!order || cleanText(order.status || "", 40) !== "COMPLETED") {
      return json({ ok: false, error: "order not completed" }, { status: 403 });
    }

    const objectKey = String(payload.object_key || "").trim().slice(0, 500);
    if (objectKey) {
      const streamed = await streamR2Object(env, objectKey, request, {
        noStore: true,
        downloadName: objectKey.split("/").pop() || "download.bin"
      });
      if (streamed) {
        return streamed;
      }
    }

    const redirectTarget = String(payload.download_url || "").trim().slice(0, 700);
    if (redirectTarget) {
      const absoluteTarget = /^https?:\/\//i.test(redirectTarget)
        ? redirectTarget
        : `${url.origin}${redirectTarget.startsWith("/") ? redirectTarget : `/${redirectTarget}`}`;
      return Response.redirect(absoluteTarget, 302);
    }

    return json({ ok: false, error: "download target unavailable" }, { status: 404 });
  }

  if (pathname === "/api/v1/catalog/list" && request.method === "GET") {
    const tracks = Array.isArray(APP_STATE.catalog)
      ? await Promise.all(APP_STATE.catalog.map((track) => normalizeCatalogTrackForPlayback(track, env)))
      : [];
    return json({ ok: true, tracks }, {
      headers: {
        "Cache-Control": "no-store"
      }
    });
  }

  if (pathname.startsWith("/api/v1/catalog/stream/") && request.method === "GET") {
    const trackKey = cleanText(pathname.slice("/api/v1/catalog/stream/".length), 120);
    const track = findCatalogTrackByKey(trackKey);
    if (!track) {
      return json({ ok: false, error: "track not found" }, { status: 404 });
    }

    const normalized = await normalizeCatalogTrackForPlayback(track, env);
    const assetFileName = cleanText(normalized.asset_file_name || normalized.file_name || "", 260);
    if (!assetFileName) {
      return json({ ok: false, error: "catalog asset unavailable" }, { status: 404 });
    }

    const mediaRequest = new Request(`https://local${buildCatalogAssetPath(assetFileName)}`, request);
    const mediaResponse = await env.ASSETS.fetch(mediaRequest).catch(() => null);
    if (mediaResponse && mediaResponse.ok) {
      return mediaResponse;
    }

    for (const key of [`catalog/${assetFileName}`, `media/catalog/${assetFileName}`, `beats/${assetFileName}`, assetFileName]) {
      const streamed = await streamR2Object(env, key, request, { noStore: false });
      if (streamed) {
        return streamed;
      }
    }

    return json({ ok: false, error: "track stream unavailable" }, { status: 404 });
  }

  if (pathname === "/api/v1/soundpacks/list" && request.method === "GET") {
    return json({ ok: true, soundpacks: APP_STATE.soundpacks });
  }

  if (pathname === "/api/v1/content/get" && request.method === "GET") {
    return json({ ok: true, content: APP_STATE.content });
  }

  if (pathname === "/api/v1/inquiries/submit" && request.method === "POST") {
    const body = await request.json().catch(() => ({}));
    const name = cleanText(body?.name || "", 120);
    const email = cleanText(body?.email || "", 180);
    const message = String(body?.message || "").trim().slice(0, 4000);
    if (!name || !email || !message) {
      return json({ ok: false, error: "name, email, and message are required" }, { status: 400 });
    }

    const inquiry = {
      inquiry_id: randomTokenHex(8),
      name,
      email,
      message,
      created_at: nowIso(),
      status: "new"
    };
    APP_STATE.inquiries.unshift(inquiry);
    APP_STATE.inquiries = APP_STATE.inquiries.slice(0, 1000);
    appendLog("inquiry.submit", {
      inquiry_id: inquiry.inquiry_id,
      email_hash: (await hashHex(email)).slice(0, 16)
    });
    return json({
      ok: true,
      inquiry_id: inquiry.inquiry_id,
      created_at: inquiry.created_at,
      target_email: inquiryTargetEmail(env),
      relay_ready: true
    });
  }

  if (pathname === "/api/v1/admin/login" && request.method === "POST") {
    const body = await request.json().catch(() => ({}));
    const creds = adminCreds(env);
    if (!creds || !sessionSecret(env)) {
      return json(
        {
          ok: false,
          error: "admin credentials are not configured",
          hint: ADMIN_SECRET_HINT
        },
        { status: 503 }
      );
    }
    const username = cleanText(body.username || "", 80).toLowerCase();
    const password = cleanText(body.password || "", 160);

    if (username !== creds.username.toLowerCase() || password !== creds.password) {
      appendLog("admin.login.failed", { user_hash: (await hashHex(username)).slice(0, 16) });
      return json({ ok: false, error: "invalid admin credentials" }, { status: 401 });
    }

    const session = await createSessionToken(env, creds.username);
    appendLog("admin.login.success", { user: creds.username });
    return json({ ok: true, username: creds.username, session_token: session.token, expires_at: session.expires_at });
  }

  if (pathname === "/api/v1/admin/verify" && request.method === "GET") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }
    return json({
      ok: true,
      username: gate.session.username,
      expires_at: gate.session.expires_at
    });
  }

  if (pathname === "/api/v1/admin/content/save" && request.method === "POST") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }
    const body = await request.json().catch(() => ({}));
    const nextContent = body && typeof body === "object" ? body : {};
    APP_STATE.content = {
      services: Array.isArray(nextContent.services) ? nextContent.services : APP_STATE.content.services,
      products: Array.isArray(nextContent.products) ? nextContent.products : APP_STATE.content.products
    };
    appendLog("admin.content.save", { user: gate.session.username });
    return json({ ok: true, content: APP_STATE.content });
  }

  if (pathname === "/api/v1/admin/content/get" && request.method === "GET") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }
    return json({ ok: true, content: APP_STATE.content });
  }

  if (pathname === "/api/v1/admin/catalog/list" && request.method === "GET") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }
    const tracks = Array.isArray(APP_STATE.catalog)
      ? await Promise.all(APP_STATE.catalog.map((track) => normalizeCatalogTrackForPlayback(track, env)))
      : [];
    return json({ ok: true, tracks }, {
      headers: {
        "Cache-Control": "no-store"
      }
    });
  }

  if (pathname === "/api/v1/admin/catalog/upload" && request.method === "POST") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }

    const form = await request.formData().catch(() => null);
    if (!form) {
      return json({ ok: false, error: "multipart form required" }, { status: 400 });
    }

    const file = form.get("file");
    if (!file || typeof file === "string") {
      return json({ ok: false, error: "file is required" }, { status: 400 });
    }

    const storedFileName = sanitizeUploadFileName(file.name || "catalog_upload", "catalog_audio");
    const virtualPath = `${WEBSITE_ROOT}/assets/audio/catalog/${storedFileName}`;
    APP_STATE.binaryOverrides.set(virtualPath, {
      bytes: await file.arrayBuffer(),
      contentType: cleanText(file.type || "", 120) || contentTypeForPath(virtualPath)
    });

    const title = cleanText(form.get("title") || "", 220) || normalizeTrackTitle(file.name);
    const description = cleanText(form.get("description") || "", 360);
    const bpm = cleanText(form.get("bpm") || "", 24);
    const key = cleanText(form.get("key") || "", 24);
    const tempo = cleanText(form.get("tempo") || "", 24);
    const price = cleanText(form.get("price") || "", 24) || "$19.99";
    const track = {
      key: randomTokenHex(8),
      file_name: storedFileName,
      title,
      description,
      bpm,
      key_signature: key,
      tempo,
      price,
      size: Number(file.size || 0),
      uploaded_at: nowIso(),
      artwork_url: catalogArtworkUrlFor(title || storedFileName),
      public_url: buildCatalogPublicUrl(storedFileName),
      full_song: true,
      source: "cloud-admin-upload"
    };

    APP_STATE.catalog.unshift(track);
    APP_STATE.catalog = APP_STATE.catalog.slice(0, 500);
    appendLog("admin.catalog.upload", {
      key: track.key,
      title: track.title,
      file_name: track.file_name,
      user: gate.session.username
    });
    return json({ ok: true, track: await normalizeCatalogTrackForPlayback(track, env) });
  }

  if (pathname === "/api/v1/admin/catalog/remove" && request.method === "POST") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }
    const body = await request.json().catch(() => ({}));
    const key = cleanText(body?.key || "", 80);
    if (!key) {
      return json({ ok: false, error: "key is required" }, { status: 400 });
    }
    const before = APP_STATE.catalog.length;
    const removedTrack = APP_STATE.catalog.find((item) => cleanText(item?.key || "", 80) === key) || null;
    APP_STATE.catalog = APP_STATE.catalog.filter((item) => cleanText(item?.key || "", 80) !== key);
    if (APP_STATE.catalog.length === before) {
      return json({ ok: false, error: "track not found" }, { status: 404 });
    }
    if (removedTrack?.file_name) {
      APP_STATE.binaryOverrides.delete(`${WEBSITE_ROOT}/assets/audio/catalog/${removedTrack.file_name}`);
    }
    appendLog("admin.catalog.remove", { key, user: gate.session.username });
    return json({ ok: true, removed: key });
  }

  if (pathname === "/api/v1/admin/inquiries/list" && request.method === "GET") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }
    return json({ ok: true, inquiries: APP_STATE.inquiries });
  }

  if (pathname === "/api/v1/admin/dashboard/state" && request.method === "GET") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }
    return json({
      ok: true,
      snapshot_at: nowIso(),
      traffic: {
        page_views: APP_STATE.activity.page_views,
        api_hits: APP_STATE.activity.api_hits,
        media_streams: APP_STATE.activity.media_streams,
        downloads: APP_STATE.activity.downloads,
        last_request_at: APP_STATE.activity.last_request_at,
        recent: APP_STATE.activity.recent.slice(0, 50)
      },
      inquiries: {
        count: APP_STATE.inquiries.length,
        latest: APP_STATE.inquiries.slice(0, 25)
      },
      sales: {
        count: APP_STATE.sales.length,
        latest: APP_STATE.sales.slice(0, 25)
      },
      logs: {
        events: APP_STATE.logs.slice(0, 50),
        adminlog: APP_STATE.logs.filter((entry) => String(entry?.event_type || "").startsWith("admin.")).slice(0, 50)
      }
    });
  }

  if (pathname === "/api/v1/admin/logs/list" && request.method === "GET") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }
    const limit = Number(url.searchParams.get("limit") || 200);
    const bounded = Number.isFinite(limit) ? Math.max(1, Math.min(1000, limit)) : 200;
    return json({ ok: true, logs: APP_STATE.logs.slice(0, bounded) });
  }

  if (pathname === "/api/v1/admin/sales/list" && request.method === "GET") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }
    return json({ ok: true, sales: APP_STATE.sales });
  }

  if (pathname === "/api/v1/admin/sales/record" && request.method === "POST") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }
    const body = await request.json().catch(() => ({}));
    const itemName = cleanText(body?.item_name || "", 220);
    const amount = cleanText(body?.amount || "", 24);
    const currency = cleanText(body?.currency || "USD", 8).toUpperCase();
    const customerName = cleanText(body?.customer_name || "", 120);
    const customerEmail = cleanText(body?.customer_email || "", 180);
    const paymentProvider = cleanText(body?.payment_provider || "paypal", 64).toLowerCase();
    const paymentTxnId = cleanText(body?.payment_txn_id || "", 160);
    if (!itemName || !amount || !customerEmail) {
      return json({ ok: false, error: "item_name, amount, and customer_email are required" }, { status: 400 });
    }

    const sale = {
      sale_id: randomTokenHex(8),
      item_name: itemName,
      amount,
      currency,
      customer_name: customerName,
      customer_email: customerEmail,
      payment_provider: paymentProvider,
      payment_txn_id: paymentTxnId,
      created_at: nowIso(),
      recorded_by: gate.session.username
    };

    const receipt = {
      receipt_id: randomTokenHex(8),
      sale_id: sale.sale_id,
      issued_at: nowIso(),
      customer_name: customerName,
      customer_email: customerEmail,
      item_name: itemName,
      amount,
      currency,
      payment_provider: paymentProvider,
      payment_txn_id: paymentTxnId
    };
    receipt.watermark_token = await watermarkToken(env, receipt.receipt_id, receipt.issued_at);
    receipt.signature = await signReceipt(env, receipt);
    receipt.receipt_url = `/receipts/${receipt.receipt_id}`;

    sale.receipt_id = receipt.receipt_id;
    sale.receipt_signature = receipt.signature;
    sale.receipt_url = receipt.receipt_url;
    APP_STATE.sales.unshift(sale);
    APP_STATE.sales = APP_STATE.sales.slice(0, 5000);
    APP_STATE.receipts.set(receipt.receipt_id, receipt);
    appendLog("admin.sales.record", {
      sale_id: sale.sale_id,
      receipt_id: receipt.receipt_id,
      amount: sale.amount,
      currency: sale.currency
    });

    return json({
      ok: true,
      sale,
      receipt: {
        receipt_id: receipt.receipt_id,
        signature: receipt.signature,
        receipt_url: receipt.receipt_url,
        watermark_token: receipt.watermark_token
      }
    });
  }

  const openAiAdminPayload = (cfg) => ({
    ok: true,
    openai: {
      model: cfg.model,
      model_list: cfg.model_list || [],
      base_url: cfg.base_url || "https://api.openai.com/v1/responses",
      ready: cfg.ready,
      source: cfg.source,
      updated_at: cfg.updated_at
    }
  });

  if (pathname === "/api/v1/admin/openai/get" && request.method === "GET") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }
    const cfg = openAiConfig(env);
    return json(openAiAdminPayload(cfg));
  }

  if (pathname === "/api/v1/admin/openai/save" && request.method === "POST") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }

    const body = await request.json().catch(() => ({}));
    const apiKey = cleanText(body?.api_key || "", 320);
    const baseUrl = cleanText(body?.base_url || "", 320);
    const cfg = setOpenAiConfig(apiKey, baseUrl);
    if (!cfg.ready) {
      return json({ ok: false, error: "OpenAI API key missing — AIFR3D chat unavailable", ...openAiAdminPayload(cfg) }, { status: 400 });
    }
    return json(openAiAdminPayload(cfg));
  }

  if (pathname === "/api/v1/admin/chat/settings/save" && request.method === "POST") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }
    const body = await request.json().catch(() => ({}));
    return json({
      ok: true,
      provider: "openai",
      websocket_url: "wss://www.north3rnlight3r.com/ws/chat",
      persistence: "worker-memory-or-env",
      settings: setChatSettings(env, body)
    });
  }

  if (pathname === "/api/v1/admin/files/list" && request.method === "GET") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }
    const rel = normalizeVirtualPath(url.searchParams.get("path") || "");
    const base = rel || "apps/website";
    const known = new Set([
      "apps/website/index.html",
      "apps/website/app.js",
      "apps/website/styles.css",
      "apps/website/config.js",
      "apps/website/assets/data/beat_catalog.json",
      ...Array.from(APP_STATE.fileOverrides.keys()),
      ...Array.from(APP_STATE.binaryOverrides.keys())
    ]);
    const prefix = base.endsWith("/") ? base : `${base}/`;
    const entries = Array.from(known)
      .filter((item) => item === base || item.startsWith(prefix))
      .map((item) => item.slice(prefix.length))
      .filter((item) => item.length > 0)
      .map((item) => item.split("/")[0])
      .filter((value, idx, arr) => arr.indexOf(value) === idx)
      .map((name) => ({
        name,
        type: name.includes(".") ? "file" : "dir"
      }));
    return json({ ok: true, path: base, entries });
  }

  if (pathname === "/api/v1/admin/files/read" && request.method === "POST") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }
    const body = await request.json().catch(() => ({}));
    const rel = normalizeVirtualPath(body?.path || "");
    if (!rel || !rel.startsWith("apps/website/")) {
      return json({ ok: false, error: "invalid file" }, { status: 400 });
    }
    if (APP_STATE.fileOverrides.has(rel)) {
      return json({ ok: true, path: rel, content: APP_STATE.fileOverrides.get(rel) || "" });
    }
    if (APP_STATE.binaryOverrides.has(rel)) {
      return json({ ok: false, error: "binary file not editable" }, { status: 400 });
    }
    const assetPath = virtualPathToAssetPath(rel);
    const assetResp = assetPath ? await env.ASSETS.fetch(`https://local${assetPath}`) : null;
    if (!assetResp || !assetResp.ok) {
      return json({ ok: false, error: "invalid file" }, { status: 400 });
    }
    const contentType = assetResp.headers.get("content-type") || "";
    if (!contentType.includes("text") && !contentType.includes("json") && !contentType.includes("javascript")) {
      return json({ ok: false, error: "binary file not editable" }, { status: 400 });
    }
    const content = await assetResp.text();
    return json({ ok: true, path: rel, content });
  }

  if (pathname === "/api/v1/admin/files/write" && request.method === "POST") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }
    const body = await request.json().catch(() => ({}));
    const rel = normalizeVirtualPath(body?.path || "");
    const content = String(body?.content || "");
    if (!rel || !rel.startsWith("apps/website/")) {
      return json({ ok: false, error: "invalid file path" }, { status: 400 });
    }
    if (!/\.(html|css|js|json|txt)$/i.test(rel)) {
      return json({ ok: false, error: "file extension not editable in cloud mode" }, { status: 400 });
    }
    APP_STATE.fileOverrides.set(rel, content);
    appendLog("admin.files.write", {
      path: rel,
      bytes: content.length,
      user: gate.session.username
    });
    return json({ ok: true, path: rel, bytes: content.length });
  }

  if (pathname === "/api/v1/admin/files/upload" && request.method === "POST") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }
    const form = await request.formData().catch(() => null);
    if (!form) {
      return json({ ok: false, error: "multipart form required" }, { status: 400 });
    }
    const file = form.get("file");
    if (!file || typeof file === "string") {
      return json({ ok: false, error: "file is required" }, { status: 400 });
    }
    const rel = normalizeVirtualPath(form.get("path") || form.get("target_path") || "");
    if (!rel || !rel.startsWith("apps/website/")) {
      return json({ ok: false, error: "invalid upload path" }, { status: 400 });
    }
    APP_STATE.binaryOverrides.set(rel, {
      bytes: await file.arrayBuffer(),
      contentType: cleanText(file.type || "", 120) || contentTypeForPath(rel)
    });
    appendLog("admin.files.upload", {
      path: rel,
      bytes: Number(file.size || 0),
      user: gate.session.username
    });
    return json({
      ok: true,
      path: rel,
      size: Number(file.size || 0),
      file_name: rel.split("/").pop() || ""
    });
  }

  if (pathname === "/api/v1/admin/files/delete" && request.method === "POST") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }
    const body = await request.json().catch(() => ({}));
    const rel = normalizeVirtualPath(body?.path || "");
    if (!rel || !rel.startsWith("apps/website/")) {
      return json({ ok: false, error: "invalid path" }, { status: 400 });
    }
    APP_STATE.fileOverrides.delete(rel);
    APP_STATE.binaryOverrides.delete(rel);
    appendLog("admin.files.delete", {
      path: rel,
      user: gate.session.username
    });
    return json({ ok: true, deleted: rel });
  }

  if (pathname === "/api/v1/admin/reference/upload" && request.method === "POST") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }
    const form = await request.formData().catch(() => null);
    if (!form) {
      return json({ ok: false, error: "multipart form required" }, { status: 400 });
    }
    const file = form.get("file");
    if (!file || typeof file === "string") {
      return json({ ok: false, error: "file is required" }, { status: 400 });
    }
    const genre = cleanText(form.get("genre") || form.get("reference_genre") || "", 32).toLowerCase();
    if (!["rap", "hip-hop", "edm", "dubstep", "pop", "rock"].includes(genre)) {
      return json({ ok: false, error: "reference genre is required" }, { status: 400 });
    }
    const storedFileName = sanitizeUploadFileName(file.name || "reference_upload", genre);
    const virtualPath = `${WEBSITE_ROOT}/assets/reference_uploads/${genre}/${storedFileName}`;
    APP_STATE.binaryOverrides.set(virtualPath, {
      bytes: await file.arrayBuffer(),
      contentType: cleanText(file.type || "", 120) || contentTypeForPath(virtualPath)
    });
    const entry = {
      genre,
      title: cleanText(form.get("title") || "", 220) || normalizeTrackTitle(file.name),
      file_name: storedFileName,
      stored_path: virtualPath,
      uploaded_at: nowIso()
    };
    APP_STATE.referenceUploads.unshift(entry);
    APP_STATE.referenceUploads = APP_STATE.referenceUploads.slice(0, 500);
    appendLog("admin.reference.upload", {
      genre,
      title: entry.title,
      file_name: entry.file_name,
      user: gate.session.username
    });
    return json({
      ok: true,
      genre,
      title: entry.title,
      file_name: entry.file_name,
      stored_path: entry.stored_path,
      next_action: "Run action:reference.pool.rebuild after enough licensed references are uploaded."
    });
  }

  if (pathname === "/api/v1/admin/soundpacks/upload" && request.method === "POST") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }

    const form = await request.formData().catch(() => null);
    if (!form) {
      return json({ ok: false, error: "multipart form required" }, { status: 400 });
    }

    const file = form.get("file");
    if (!file || typeof file === "string") {
      return json({ ok: false, error: "file is required" }, { status: 400 });
    }

    const packType = cleanText(form.get("pack_type") || "soundpack", 32).toLowerCase();
    const title = cleanText(form.get("title") || file.name || "Pack", 220);
    const description = cleanText(form.get("description") || "", 360);
    const bpm = cleanText(form.get("bpm") || "", 24);
    const key = cleanText(form.get("key") || "", 24);
    const tempo = cleanText(form.get("tempo") || "", 24);
    const price = cleanText(form.get("price") || "", 24) || (packType === "single" ? "$2.99" : "$19.99");

    const item = {
      key: randomTokenHex(8),
      file_name: file.name,
      pack_type: packType,
      title,
      description,
      bpm,
      key_signature: key,
      tempo,
      price,
      size: Number(file.size || 0),
      uploaded_at: nowIso(),
      public_url: ""
    };

    APP_STATE.soundpacks.unshift(item);
    APP_STATE.soundpacks = APP_STATE.soundpacks.slice(0, 200);
    appendLog("admin.soundpack.upload", { key: item.key, title: item.title });

    return json({ ok: true, soundpack: item });
  }

  if (pathname === "/api/v1/admin/soundpacks/list" && request.method === "GET") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }
    return json({ ok: true, soundpacks: APP_STATE.soundpacks });
  }

  if (pathname === "/api/v1/admin/soundpacks/remove" && request.method === "POST") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }
    const body = await request.json().catch(() => ({}));
    const key = cleanText(body?.key || "", 80);
    if (!key) {
      return json({ ok: false, error: "key is required" }, { status: 400 });
    }
    const before = APP_STATE.soundpacks.length;
    APP_STATE.soundpacks = APP_STATE.soundpacks.filter((item) => cleanText(item?.key || "", 80) !== key);
    if (APP_STATE.soundpacks.length === before) {
      return json({ ok: false, error: "sound pack not found" }, { status: 404 });
    }
    appendLog("admin.soundpack.remove", { key, user: gate.session.username });
    return json({ ok: true, removed: key });
  }

  if (pathname === "/api/v1/admin/promo/list" && request.method === "GET") {
    return json({ ok: false, error: PROMO_CODES_DISABLED_MESSAGE }, { status: 410 });
  }

  if (pathname === "/api/v1/admin/promo/save" && request.method === "POST") {
    return json({ ok: false, error: PROMO_CODES_DISABLED_MESSAGE }, { status: 410 });
  }

  if (pathname === "/api/v1/promo/redeem" && request.method === "POST") {
    return json({ ok: false, error: PROMO_CODES_DISABLED_MESSAGE }, { status: 410 });
  }

  if (pathname === "/api/v1/command/run" && request.method === "POST") {
    const gate = await requireAdmin(request, env);
    if (gate.error) {
      return gate.error;
    }

    const body = await request.json().catch(() => ({}));
    const cmd = cleanText(body?.command_line || "", 2048);

    if (!cmd) {
      return json({ ok: false, error: "command_line is required" }, { status: 400 });
    }

    if (cmd.startsWith("echo ")) {
      appendLog("command.run", { ok: true, command_line: cleanText(cmd, 260), exit_code: 0, user: gate.session.username });
      return json({ ok: true, exit_code: 0, stdout: `${cmd.slice(5)}\n`, stderr: "" });
    }

    if (cmd.includes("/api/v1/health")) {
      appendLog("command.run", { ok: true, command_line: cleanText(cmd, 260), exit_code: 0, user: gate.session.username });
      return json({ ok: true, exit_code: 0, stdout: `${JSON.stringify({ ok: true, service: "north3rnlight3r-website-worker", timestamp_utc: nowIso() })}\n`, stderr: "" });
    }

    if (cmd === "ls -la") {
      appendLog("command.run", { ok: true, command_line: cleanText(cmd, 260), exit_code: 0, user: gate.session.username });
      return json({ ok: true, exit_code: 0, stdout: ".\n..\napps\napi\ndownloads\n", stderr: "" });
    }

    appendLog("command.run", { ok: false, command_line: cleanText(cmd, 260), exit_code: 1, user: gate.session.username });
    return json({ ok: false, exit_code: 1, stdout: "", stderr: "Cloud runtime does not permit shell command execution for this command." });
  }

  if (pathname === "/api/v1/chat/ask" && request.method === "POST") {
    const apiGate = requireApiToken(request, env);
    if (apiGate) {
      return apiGate;
    }

    const body = await request.json().catch(() => ({}));
    const chatSettings = loadChatSettings(env);
    const prompt = cleanText(body?.prompt || "", Number(chatSettings.context.max_prompt_chars || 4000));
    const sessionId = cleanText(body?.session_id || "default", 100);
    const requestedModel = cleanText(body?.model || "", 120);
    const personalityMode = cleanText(body?.personality_mode || "", 80);
    const agentModes = Array.isArray(body?.agent_modes) ? body.agent_modes : [];

    if (!prompt) {
      return json({ ok: false, error: "prompt is empty" }, { status: 400 });
    }

    const result = await runOpenAiChat(env, prompt, sessionId, {
      model: requestedModel,
      personality_mode: personalityMode,
      agent_modes: agentModes
    });
    if (!result.ok) {
      appendLog("chat.ask", { ok: false, session_id: sessionId, model: result.model || null });
      return json(
        { ok: false, error: result.error, provider: "openai", model: result.model, attempted_models: result.attempted_models || [] },
        { status: 502 }
      );
    }

    appendLog("chat.ask", { ok: true, session_id: sessionId, model: result.model || null });

    return json({
      ok: true,
      provider: "openai",
      model: result.model,
      attempted_models: result.attempted_models || [result.model],
      brain_id: result.brain_id || "aifr3d_brain_v2_2_4",
      session_id: sessionId,
      summary: result.data.summary,
      issues: result.data.issues,
      action_suggestions: result.data.action_suggestions,
      state_update: result.data.state_update,
      timestamp_utc: nowIso()
    });
  }

  if (pathname === "/api/v1/memory/feedback" && request.method === "POST") {
    const apiGate = requireApiToken(request, env);
    if (apiGate) {
      return apiGate;
    }
    const body = await request.json().catch(() => ({}));
    return json({
      ok: true,
      session_id: cleanText(body?.session_id || "default", 100),
      action_id: cleanText(body?.action_id || "", 200),
      accepted: Boolean(body?.accepted)
    });
  }

  if (pathname === "/api/v1/memory/clear" && request.method === "POST") {
    const apiGate = requireApiToken(request, env);
    if (apiGate) {
      return apiGate;
    }
    const body = await request.json().catch(() => ({}));
    const sessionId = cleanText(body?.session_id || "default", 100);
    APP_STATE.memory.delete(sessionId);
    APP_STATE.memorySummary.delete(sessionId);
    return json({ ok: true, session_id: sessionId, cleared: true });
  }

  if (pathname === "/api/v1/memory/state" && request.method === "GET") {
    const apiGate = requireApiToken(request, env);
    if (apiGate) {
      return apiGate;
    }
    const sessionId = cleanText(url.searchParams.get("session_id") || "default", 100);
    const memory = APP_STATE.memory.get(sessionId) || [];
    return json({ ok: true, session_id: sessionId, memory: { items: memory.slice(-10) } });
  }

  if (pathname === "/api/v1/receipts/verify" && request.method === "GET") {
    const receiptId = cleanText(url.searchParams.get("receipt_id") || "", 120);
    const signature = cleanText(url.searchParams.get("signature") || "", 260);
    if (!receiptId || !signature) {
      return json({ ok: false, error: "receipt_id and signature are required" }, { status: 400 });
    }
    const receipt = APP_STATE.receipts.get(receiptId);
    if (!receipt) {
      return json({ ok: false, error: "receipt not found" }, { status: 404 });
    }
    const expected = await signReceipt(env, receipt);
    return json({ ok: true, valid: expected === signature && signature === receipt.signature, receipt_id: receiptId });
  }

  return json({ ok: false, error: "not found" }, { status: 404 });
}

async function handleWebSocket(request, env) {
  if (request.headers.get("Upgrade") !== "websocket") {
    return new Response("Expected websocket", { status: 426 });
  }

  const pair = new WebSocketPair();
  const [client, server] = Object.values(pair);
  server.accept();

  const cfg = openAiConfig(env);
  const brain = loadBrainConfig(env);
  const chatSettings = loadChatSettings(env);
  server.send(
    JSON.stringify({
      type: "chat.ready",
      service: "north3rnlight3r-website-worker",
      provider: "openai",
      model: cfg.model || null,
      models: activeOpenAiModelList(cfg),
      transport_mode: chatSettings.transport_mode,
      brain_id: brain?.brain_id || "aifr3d_brain_v2_2_4",
      timestamp_utc: nowIso()
    })
  );

  server.addEventListener("message", async (event) => {
    let payload;
    try {
      payload = JSON.parse(String(event.data));
    } catch (_error) {
      server.send(JSON.stringify({ type: "chat.error", message: "invalid json" }));
      return;
    }

    if (payload?.type === "chat.clear") {
      const sessionId = cleanText(payload.session_id || "default", 100);
      APP_STATE.memory.delete(sessionId);
      APP_STATE.memorySummary.delete(sessionId);
      server.send(JSON.stringify({ type: "chat.cleared", session_id: sessionId }));
      return;
    }

    if (payload?.type !== "chat.send") {
      server.send(JSON.stringify({ type: "chat.error", message: "unsupported message type" }));
      return;
    }

    const liveSettings = loadChatSettings(env);
    const prompt = cleanText(payload.text || "", Number(liveSettings.context.max_prompt_chars || 4000));
    const sessionId = cleanText(payload.session_id || "default", 100);
    const personalityMode = cleanText(payload.personality_mode || "", 80);
    const agentModes = Array.isArray(payload.agent_modes) ? payload.agent_modes : [];
    if (!prompt) {
      server.send(JSON.stringify({ type: "chat.error", message: "empty prompt" }));
      return;
    }

    const result = await runOpenAiChat(env, prompt, sessionId, {
      personality_mode: personalityMode,
      agent_modes: agentModes
    });
    if (!result.ok) {
      server.send(
        JSON.stringify({
          type: "chat.error",
          message: result.error,
          provider: "openai",
          model: result.model || null,
          attempted_models: result.attempted_models || []
        })
      );
      return;
    }

    const messageId = randomTokenHex(6);
    const tokens = String(result.data.summary || "").split(/\s+/).filter(Boolean);
    for (const token of tokens) {
      server.send(JSON.stringify({ type: "chat.token", messageId, text: `${token} ` }));
    }
    server.send(
      JSON.stringify({
        type: "chat.done",
        messageId,
        model: result.model || null,
        attempted_models: result.attempted_models || [result.model],
        brain_id: result.brain_id || "aifr3d_brain_v2_2_4"
      })
    );
    for (const issue of (result.data.issues || []).slice(0, 5)) {
      server.send(JSON.stringify({ type: "issue.object", issue }));
    }
  });

  return new Response(null, { status: 101, webSocket: client });
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    if (request.method === "GET") {
      const routePath = url.pathname || "/";
      if (routePath.startsWith("/downloads") || routePath.startsWith("/download/") || routePath.startsWith("/api/v1/pay/download")) {
        recordDashboardActivity("download", routePath);
      } else if (routePath.startsWith("/media/")) {
        recordDashboardActivity("media_stream", routePath);
      } else if (routePath.startsWith("/api/")) {
        recordDashboardActivity("api_hit", routePath);
      } else if (routePath === "/" || routePath.endsWith(".html") || !routePath.slice(1).includes(".")) {
        recordDashboardActivity("page_view", routePath);
      }
    }
    if (sitePrivateMode(env)) {
      const bypass = isMaintenanceBypassPath(url.pathname);
      const allowed = bypass ? true : await hasMaintenanceAccess(request, env);
      if (!allowed) {
        if (url.pathname.startsWith("/api/") || url.pathname === "/ws/chat") {
          return json(
            {
              ok: false,
              error: "website is temporarily private for maintenance",
              maintenance_mode: true
            },
            { status: 503 }
          );
        }
        return maintenanceHtml();
      }
    }

    if (url.pathname === "/ws/chat") {
      return handleWebSocket(request, env);
    }

    if (url.pathname.startsWith("/api/")) {
      return handleApi(request, env, url);
    }

    if (url.pathname.startsWith("/receipts/")) {
      const receiptId = cleanText(url.pathname.split("/").pop() || "", 120);
      const receipt = APP_STATE.receipts.get(receiptId);
      if (!receipt) {
        return new Response("Receipt not found", { status: 404 });
      }
      return new Response(buildReceiptHtml(receipt), {
        status: 200,
        headers: {
          "Content-Type": "text/html; charset=utf-8"
        }
      });
    }

    if (url.pathname.startsWith("/media/")) {
      const rawPath = decodePathSegment(url.pathname.replace(/^\/media\//, ""));
      const safePath = rawPath
        .split("/")
        .filter(Boolean)
        .map((segment) => segment.replace(/\.\./g, "").trim())
        .filter(Boolean)
        .join("/");
      const isSoundpack = safePath.startsWith("soundpacks/");
      const isCatalog = !isSoundpack && safePath.startsWith("catalog/");
      const relativeName = isSoundpack
        ? safePath.slice("soundpacks/".length)
        : isCatalog
          ? safePath.slice("catalog/".length)
          : safePath;
      const candidates = isSoundpack
        ? [`soundpacks/${relativeName}`, `media/soundpacks/${relativeName}`]
        : [`catalog/${relativeName}`, `media/catalog/${relativeName}`, `beats/${relativeName}`, relativeName];

      for (const key of candidates) {
        const streamed = await streamR2Object(env, key, request, { noStore: false });
        if (streamed) {
          return streamed;
        }
      }

      if (!isSoundpack && relativeName) {
        const virtualCatalogPath = `${WEBSITE_ROOT}/assets/audio/catalog/${relativeName}`;
        if (APP_STATE.binaryOverrides.has(virtualCatalogPath)) {
          const asset = APP_STATE.binaryOverrides.get(virtualCatalogPath);
          if (asset?.bytes) {
            return new Response(asset.bytes.slice(0), {
              status: 200,
              headers: {
                "Content-Type": asset.contentType || contentTypeForPath(virtualCatalogPath)
              }
            });
          }
        }
        const assetUrl = `https://local/assets/audio/catalog/${encodeURIComponent(relativeName)}`;
        const assetRes = await env.ASSETS.fetch(new Request(assetUrl, request)).catch(() => null);
        if (assetRes && assetRes.ok) {
          return withCacheControl(assetRes, assetCacheControlForPath(url.pathname));
        }
      }

      const fallbackBase = cleanText(env.CATALOG_PUBLIC_BASE_URL || "", 1200).replace(/\/+$/, "");
      if (fallbackBase && !isSoundpack && relativeName) {
        return Response.redirect(`${fallbackBase}/${encodeURIComponent(relativeName)}`, 302);
      }

      return new Response("Media not found", { status: 404 });
    }

    if (Object.prototype.hasOwnProperty.call(DOWNLOAD_REDIRECTS, url.pathname)) {
      const target = DOWNLOAD_REDIRECTS[url.pathname];
      return Response.redirect(target, 302);
    }

    if (url.pathname === "/downloads/dawai-admin-android.apk") {
      return new Response("Android app is admin-only and distributed privately.", { status: 403 });
    }

    await ensureSeedData(env);
    const virtualPath = assetPathToVirtualPath(url.pathname);
    if (APP_STATE.binaryOverrides.has(virtualPath)) {
      const asset = APP_STATE.binaryOverrides.get(virtualPath);
      if (asset?.bytes) {
        return new Response(asset.bytes.slice(0), {
          status: 200,
          headers: {
            "Content-Type": asset.contentType || contentTypeForPath(virtualPath)
          }
        });
      }
    }
    if (APP_STATE.fileOverrides.has(virtualPath)) {
      const content = APP_STATE.fileOverrides.get(virtualPath) || "";
      const contentType = contentTypeForPath(virtualPath);
      return new Response(content, {
        status: 200,
        headers: {
          "Content-Type": contentType
        }
      });
    }

    if (url.pathname.startsWith("/assets/audio/catalog/")) {
      const assetResponse = await env.ASSETS.fetch(request).catch(() => null);
      if (assetResponse && assetResponse.ok) {
        return withCacheControl(assetResponse, assetCacheControlForPath(url.pathname));
      }
      const requestedFileName = decodePathSegment(url.pathname.split("/").pop() || "");
      const safeName = cleanText(requestedFileName || "", 260);
      const dot = safeName.lastIndexOf(".");
      const stem = dot > 0 ? safeName.slice(0, dot) : safeName;
      const extensionFallbacks = [".mp3", ".wav", ".ogg", ".m4a", ".flac", ".aiff", ".aif"]
        .map((extension) => (stem ? `${stem}${extension}` : ""))
        .filter((candidate) => candidate && candidate !== safeName);
      for (const candidate of extensionFallbacks) {
        const candidatePath = `/assets/audio/catalog/${encodeURIComponent(candidate)}`;
        const candidateVirtualPath = assetPathToVirtualPath(candidatePath);
        if (APP_STATE.binaryOverrides.has(candidateVirtualPath)) {
          const asset = APP_STATE.binaryOverrides.get(candidateVirtualPath);
          if (asset?.bytes) {
            return new Response(asset.bytes.slice(0), {
              status: 200,
              headers: {
                "Content-Type": asset.contentType || contentTypeForPath(candidateVirtualPath)
              }
            });
          }
        }
        const candidateResponse = await env.ASSETS.fetch(`https://local${candidatePath}`, request).catch(() => null);
        if (candidateResponse && candidateResponse.ok) {
          return withCacheControl(candidateResponse, assetCacheControlForPath(candidatePath));
        }
      }
      const previewName = stem ? `${stem}.preview.mp3` : "";
      if (previewName && previewName !== safeName) {
        const previewPath = `/assets/audio/catalog/${encodeURIComponent(previewName)}`;
        const previewVirtualPath = assetPathToVirtualPath(previewPath);
        if (APP_STATE.binaryOverrides.has(previewVirtualPath)) {
          const asset = APP_STATE.binaryOverrides.get(previewVirtualPath);
          if (asset?.bytes) {
            return new Response(asset.bytes.slice(0), {
              status: 200,
              headers: {
                "Content-Type": asset.contentType || contentTypeForPath(previewVirtualPath)
              }
            });
          }
        }
        const previewResponse = await env.ASSETS.fetch(`https://local${previewPath}`, request).catch(() => null);
        if (previewResponse && previewResponse.ok) {
          return withCacheControl(previewResponse, assetCacheControlForPath(previewPath));
        }
      }
      if (assetResponse) {
        return withCacheControl(assetResponse, assetCacheControlForPath(url.pathname));
      }
      return new Response("Not found", { status: 404 });
    }

    return withCacheControl(await env.ASSETS.fetch(request), assetCacheControlForPath(url.pathname));
  }
};
