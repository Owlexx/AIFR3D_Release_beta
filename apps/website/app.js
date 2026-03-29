const config = window.CORESYNTH_CONFIG || {};
const API_BASE = String(config.apiBase || "https://www.north3rnlight3r.com").replace(/\/+$/, "");
const API_TOKEN = String(config.apiToken || "").trim();
const SITE_TEXT_DEFAULTS = Object.freeze({
  brandTitle: "NORTH3RNLIGHT3R",
  brandSubtitle: "AudioSuite | Mix Intelligence | Licensing",
  heroEyebrow: "North3rnLight3r Official",
  heroTitle: "Professional mix analysis, direct licensing, and premium releases in one system.",
  heroCopy: "AudioSuite combines live diagnostics, reference matching, stereo field intelligence, tonal balance insight, loudness control, and AudioSynth Mix Tips inside one focused North3rnLight3r workflow for artists, producers, and engineers.",
  heroPrimaryCta: "Launch Beat Player",
  heroSecondaryCta: "Book Diagnostics",
  catalogEyebrow: "Beat Catalog",
  catalogTitle: "Audition the catalog through one stable player",
  catalogCopy: "Every beat now opens inside the on-page player so playback, transport, visualizer, and checkout all stay in one controlled signal path."
});
const SITE_TEXT = {
  ...SITE_TEXT_DEFAULTS,
  ...(config.siteText && typeof config.siteText === "object" ? config.siteText : {})
};
const VENMO_HANDLE = String(config.venmoHandle || config.venmoUsername || "").trim().replace(/^@+/, "");
const CHECKOUT_CONTACT_EMAIL = String(config.checkoutContactEmail || "north3rnlight3rofficial@outlook.com").trim();
const INQUIRY_CONTACT_EMAIL = String(config.inquiryEmail || "north3rnlight3rofficial@outlook.com").trim();
const CATALOG_PREVIEW_SECONDS = 0;
const BRAND_ARTWORK_POOL = [
  "assets/brand/north3rnlight3r_album_art.png",
  "assets/brand/album_art_concept.png",
  "assets/brand/hero_mascot.png"
];
const INSTALLATION_DOC_URL = "/assets/docs/installation-instructions-v2.2.4.txt";
const README_DOC_URL = "/assets/docs/readme-v2.2.4-beta.txt";
const MINIMUM_PC_DOC_URL = "/assets/docs/minimum-pc-requirements-v2.2.4.txt";
const RELEASE_LOG_DOC_URL = "/assets/docs/release-notes-v2.2.4.txt";
const VST_GUIDE_DOC_URL = "/assets/docs/vst-user-guide-v2.2.4.txt";
const STANDALONE_GUIDE_DOC_URL = "/assets/docs/standalone-user-guide-v2.2.4.txt";
const METRIC_CATALOG_URL = "/assets/data/analysis_metric_catalog.json";
const FREE_TRIAL_UNLOCK_SOURCE = "free_trial";
const BETA_FREE_TRIAL_LABEL = "Free to try for a limited time";
const POST_BETA_PRICE_LABEL = "$149.99 after Beta 2.2.4";
const BETA_SOFTWARE_SKUS = Object.freeze(["software_vst3", "software_standalone"]);
const BETA_INSTALL_PRODUCTS = Object.freeze({
  software_vst3: {
    assetId: "install-vst.sh",
    productKey: "vst"
  },
  software_standalone: {
    assetId: "install-standalone.sh",
    productKey: "standalone"
  }
});

const PACK_DEFAULT_PRICES = {
  soundpack: "$19.99",
  midipack: "$19.99",
  drumpack: "$19.99",
  samplepack: "$19.99",
  single: "$2.99"
};

const storeGrid = document.getElementById("store-grid");
const servicesGrid = document.getElementById("services-grid");
const installGrid = document.getElementById("install-grid");
const checkoutStatus = document.getElementById("checkout-status");
const catalogToggle = document.getElementById("catalog-toggle");
const catalogWrap = document.getElementById("catalog-wrap");
const catalogList = document.getElementById("catalog-list");
const soundpackList = document.getElementById("soundpack-list");
const nowTitle = document.getElementById("now-title");
const nowMeta = document.getElementById("now-meta");
const buyTrack = document.getElementById("buy-track");
const prevTrack = document.getElementById("prev-track");
const nextTrack = document.getElementById("next-track");
const toggleTrack = document.getElementById("toggle-track");
const stopTrack = document.getElementById("stop-track");
const audioPlayer = document.getElementById("audio-player");
const audioProgress = document.getElementById("audio-progress");
const audioCurrentTime = document.getElementById("audio-current-time");
const audioDuration = document.getElementById("audio-duration");
const audioSpectrum = document.getElementById("audio-spectrum");
const servicesCountEl = document.getElementById("count-services");
const beatCatalogCountEl = document.getElementById("count-beat-catalog");
const productsCountEl = document.getElementById("count-products");
const supportForm = document.getElementById("support-form");
const supportStatus = document.getElementById("support-status");
const educationTabs = document.getElementById("education-tabs");
const educationTitle = document.getElementById("education-title");
const educationDescription = document.getElementById("education-description");
const educationBody = document.getElementById("education-body");
const yearEl = document.getElementById("year");
const enableNotificationsButton = document.getElementById("enable-notifications");
const brandTitleEl = document.getElementById("brand-title");
const brandSubEl = document.getElementById("brand-sub");
const heroEyebrowEl = document.getElementById("hero-eyebrow");
const heroTitleEl = document.getElementById("hero-title");
const heroCopyEl = document.getElementById("hero-copy");
const heroPrimaryCtaEl = document.getElementById("hero-primary-cta");
const heroSecondaryCtaEl = document.getElementById("hero-secondary-cta");
const catalogEyebrowEl = document.getElementById("catalog-eyebrow");
const catalogTitleEl = document.getElementById("catalog-title");
const catalogCopyEl = document.getElementById("catalog-copy");

let tracks = [];
let metricCatalog = {};
let activeEducationMetric = "frequency_balance";
const unlockState = {
  vst: false,
  standalone: false
};
const unlockExpiresAt = {
  vst: 0,
  standalone: 0
};
const unlockSource = {
  vst: "none",
  standalone: "none"
};
const downloadAccessTokens = {
  vst: "",
  standalone: ""
};
const counterState = {
  services: 0,
  beatCatalog: 0,
  products: 0
};

function setCheckoutStatus(message = "") {
  if (!checkoutStatus) {
    return;
  }
  checkoutStatus.textContent = String(message || "");
}

function educationMetricOrder() {
  return ["frequency_balance", "integrated_loudness", "dynamic_range", "stereo_width"];
}

function educationMetricLabel(metricId) {
  switch (String(metricId || "")) {
    case "integrated_loudness":
      return "Integrated Loudness";
    case "dynamic_range":
      return "Dynamic Range";
    case "stereo_width":
      return "Stereo Width";
    case "frequency_balance":
    default:
      return "Frequency Balance";
  }
}

function formatMetricName(value) {
  return String(value || "").replace(/_/g, " ").replace(/\b\w/g, (match) => match.toUpperCase());
}

function appendEducationSection(parent, title, bodyNode) {
  if (!parent || !bodyNode) {
    return;
  }
  const section = document.createElement("section");
  section.className = "education-section";
  const heading = document.createElement("h4");
  heading.textContent = title;
  section.appendChild(heading);
  section.appendChild(bodyNode);
  parent.appendChild(section);
}

function appendEducationList(parent, title, items) {
  if (!Array.isArray(items) || items.length === 0) {
    return;
  }
  const list = document.createElement("ul");
  list.className = "education-list";
  items.forEach((item) => {
    const li = document.createElement("li");
    li.textContent = typeof item === "string"
      ? item
      : `${item.track || item.width || item.value || "Reference"}${item.note ? ` — ${item.note}` : ""}`;
    list.appendChild(li);
  });
  appendEducationSection(parent, title, list);
}

function renderEducationMetric(metricId) {
  const metric = metricCatalog?.[metricId];
  if (!metric || !educationBody || !educationTitle || !educationDescription) {
    return;
  }

  activeEducationMetric = metricId;
  educationTitle.textContent = String(metric.display_name || educationMetricLabel(metricId));
  educationDescription.textContent = String(metric.description || "");
  educationBody.innerHTML = "";

  if (Array.isArray(metric.related_metrics) && metric.related_metrics.length > 0) {
    const related = document.createElement("p");
    related.className = "muted";
    related.textContent = `Related metrics: ${metric.related_metrics.map(formatMetricName).join(", ")}`;
    appendEducationSection(educationBody, "Related Metrics", related);
  }

  const targets = metric.reference_targets || {};
  if (targets && Object.keys(targets).length > 0) {
    const grid = document.createElement("div");
    grid.className = "education-target-grid";
    Object.entries(targets).forEach(([groupName, groupValue]) => {
      const card = document.createElement("article");
      card.className = "education-target-card";
      const heading = document.createElement("strong");
      heading.textContent = formatMetricName(groupName);
      card.appendChild(heading);
      const lines = document.createElement("div");
      lines.className = "education-kv-list";
      if (groupValue && typeof groupValue === "object" && !Array.isArray(groupValue)) {
        Object.entries(groupValue).forEach(([key, value]) => {
          const row = document.createElement("p");
          row.textContent = `${formatMetricName(key)}: ${value}`;
          lines.appendChild(row);
        });
      }
      card.appendChild(lines);
      grid.appendChild(card);
    });
    appendEducationSection(educationBody, "Reference Targets", grid);
  }

  const educational = metric.educational_content || {};
  if (educational && Object.keys(educational).length > 0) {
    const grid = document.createElement("div");
    grid.className = "education-topic-grid";
    Object.entries(educational).forEach(([key, value]) => {
      const card = document.createElement("article");
      card.className = "education-topic-card";
      const heading = document.createElement("strong");
      heading.textContent = formatMetricName(key);
      card.appendChild(heading);
      if (value && typeof value === "object") {
        Object.entries(value).forEach(([field, text]) => {
          const row = document.createElement("p");
          row.textContent = `${formatMetricName(field)}: ${text}`;
          card.appendChild(row);
        });
      } else {
        const row = document.createElement("p");
        row.textContent = String(value || "");
        card.appendChild(row);
      }
      grid.appendChild(card);
    });
    appendEducationSection(educationBody, "Educational Content", grid);
  }

  const tips = metric.mixing_tips || {};
  if (tips && Object.keys(tips).length > 0) {
    const grid = document.createElement("div");
    grid.className = "education-topic-grid";
    Object.entries(tips).forEach(([key, list]) => {
      const card = document.createElement("article");
      card.className = "education-topic-card";
      const heading = document.createElement("strong");
      heading.textContent = formatMetricName(key);
      card.appendChild(heading);
      const ul = document.createElement("ul");
      ul.className = "education-list";
      (Array.isArray(list) ? list : []).forEach((item) => {
        const li = document.createElement("li");
        li.textContent = String(item || "");
        ul.appendChild(li);
      });
      card.appendChild(ul);
      grid.appendChild(card);
    });
    appendEducationSection(educationBody, "Mixing Tips", grid);
  }

  const examples = metric.examples || {};
  if (examples && Object.keys(examples).length > 0) {
    const grid = document.createElement("div");
    grid.className = "education-topic-grid";
    Object.entries(examples).forEach(([key, entries]) => {
      const card = document.createElement("article");
      card.className = "education-topic-card";
      const heading = document.createElement("strong");
      heading.textContent = formatMetricName(key);
      card.appendChild(heading);
      const ul = document.createElement("ul");
      ul.className = "education-list";
      (Array.isArray(entries) ? entries : []).forEach((entry) => {
        const li = document.createElement("li");
        if (typeof entry === "string") {
          li.textContent = entry;
        } else {
          const parts = [entry.track || "", entry.value != null ? `(${entry.value})` : "", entry.width != null ? `(${entry.width})` : "", entry.note || ""].filter(Boolean);
          li.textContent = parts.join(" — ").replace(" — (", " (");
        }
        ul.appendChild(li);
      });
      card.appendChild(ul);
      grid.appendChild(card);
    });
    appendEducationSection(educationBody, "Reference Examples", grid);
  }

  if (educationTabs) {
    Array.from(educationTabs.querySelectorAll("button")).forEach((button) => {
      button.classList.toggle("active", button.dataset.metricId === metricId);
    });
  }
}

function renderEducationTabs() {
  if (!educationTabs) {
    return;
  }
  educationTabs.innerHTML = "";
  educationMetricOrder().forEach((metricId) => {
    if (!metricCatalog?.[metricId]) {
      return;
    }
    const button = document.createElement("button");
    button.type = "button";
    button.className = "btn ghost education-tab";
    button.dataset.metricId = metricId;
    button.textContent = educationMetricLabel(metricId);
    button.addEventListener("click", () => {
      renderEducationMetric(metricId);
    });
    educationTabs.appendChild(button);
  });
}

async function loadMetricCatalog() {
  try {
    const response = await fetch(METRIC_CATALOG_URL, { method: "GET", cache: "no-store" });
    const payload = await response.json().catch(() => ({}));
    metricCatalog = payload?.metrics && typeof payload.metrics === "object" ? payload.metrics : {};
  } catch (_error) {
    metricCatalog = {};
  }
  renderEducationTabs();
  const firstMetric = educationMetricOrder().find((metricId) => metricCatalog?.[metricId]) || "";
  if (firstMetric) {
    renderEducationMetric(firstMetric);
  }
}

function syncCounters() {
  if (servicesCountEl) {
    servicesCountEl.textContent = String(counterState.services || 0);
  }
  if (beatCatalogCountEl) {
    beatCatalogCountEl.textContent = String(counterState.beatCatalog || 0);
  }
  if (productsCountEl) {
    productsCountEl.textContent = String(counterState.products || 0);
  }
}

function applySiteTextConfig() {
  const mappings = [
    [brandTitleEl, SITE_TEXT.brandTitle],
    [brandSubEl, SITE_TEXT.brandSubtitle],
    [heroEyebrowEl, SITE_TEXT.heroEyebrow],
    [heroTitleEl, SITE_TEXT.heroTitle],
    [heroCopyEl, SITE_TEXT.heroCopy],
    [heroPrimaryCtaEl, SITE_TEXT.heroPrimaryCta],
    [heroSecondaryCtaEl, SITE_TEXT.heroSecondaryCta],
    [catalogEyebrowEl, SITE_TEXT.catalogEyebrow],
    [catalogTitleEl, SITE_TEXT.catalogTitle],
    [catalogCopyEl, SITE_TEXT.catalogCopy]
  ];
  mappings.forEach(([node, value]) => {
    if (node && String(value || "").trim()) {
      node.textContent = String(value).trim();
    }
  });
}

function apiUrl(pathname) {
  return `${API_BASE}${pathname.startsWith("/") ? pathname : `/${pathname}`}`;
}

function normalizePlayableUrl(rawUrl) {
  const value = String(rawUrl || "").trim();
  if (!value) {
    return "";
  }
  if (/^https?:\/\//i.test(value)) {
    return value;
  }
  return apiUrl(value);
}

function buildInquiryMailto(name, email, message, targetEmail = INQUIRY_CONTACT_EMAIL) {
  const safeTarget = String(targetEmail || INQUIRY_CONTACT_EMAIL).trim() || INQUIRY_CONTACT_EMAIL;
  const subject = encodeURIComponent(`Website Inquiry - ${name}`);
  const body = encodeURIComponent(`Name: ${name}\nEmail: ${email}\n\nMessage:\n${message}`);
  return `mailto:${encodeURIComponent(safeTarget)}?subject=${subject}&body=${body}`;
}

function authHeaders() {
  if (!API_TOKEN) {
    return {};
  }
  return {
    Authorization: `Bearer ${API_TOKEN}`
  };
}

function parsePriceAmount(priceText) {
  const cleaned = String(priceText || "").replace(/[^0-9.]/g, "").trim();
  if (!cleaned) {
    return "";
  }
  const value = Number(cleaned);
  if (!Number.isFinite(value) || value <= 0) {
    return "";
  }
  return value.toFixed(2);
}

function defaultPriceForPackType(packType) {
  const key = String(packType || "soundpack").toLowerCase().trim();
  return PACK_DEFAULT_PRICES[key] || "$19.99";
}

function artworkSeed(value = "") {
  return Array.from(String(value || "north3rnlight3r")).reduce((sum, ch) => sum + ch.charCodeAt(0), 0);
}

function fallbackArtworkUrl(index = 0) {
  const safeIndex = Math.abs(Number(index || 0));
  return BRAND_ARTWORK_POOL[safeIndex % BRAND_ARTWORK_POOL.length];
}

function resolveArtworkUrl(item, index = 0) {
  const explicit = String(item?.artwork_url || item?.artworkUrl || "").trim();
  if (explicit) {
    return explicit;
  }
  return fallbackArtworkUrl(artworkSeed(item?.title || item?.file_name || index));
}

function createArtworkNode(title, artworkUrl, index = 0) {
  const wrap = document.createElement("div");
  wrap.className = "track-art";
  const img = document.createElement("img");
  img.src = resolveArtworkUrl({ artwork_url: artworkUrl }, index);
  img.alt = `${String(title || "North3rnLight3r")} artwork`;
  img.loading = "lazy";
  img.decoding = "async";
  img.addEventListener("error", () => {
    img.src = fallbackArtworkUrl(index + 1);
  });
  wrap.appendChild(img);
  return wrap;
}

function loadUnlockState() {
  try {
    const raw = localStorage.getItem("coresynth_install_unlock");
    if (!raw) {
      return;
    }
    const parsed = JSON.parse(raw);
    unlockState.vst = Boolean(parsed?.vst);
    unlockState.standalone = Boolean(parsed?.standalone);
    unlockExpiresAt.vst = Number(parsed?.expires_at?.vst || 0) || 0;
    unlockExpiresAt.standalone = Number(parsed?.expires_at?.standalone || 0) || 0;
    unlockSource.vst = String(parsed?.source?.vst || "").trim().toLowerCase();
    unlockSource.standalone = String(parsed?.source?.standalone || "").trim().toLowerCase();
    if (!unlockSource.vst) {
      unlockSource.vst = unlockState.vst ? FREE_TRIAL_UNLOCK_SOURCE : "none";
    }
    if (!unlockSource.standalone) {
      unlockSource.standalone = unlockState.standalone ? FREE_TRIAL_UNLOCK_SOURCE : "none";
    }
    downloadAccessTokens.vst = String(parsed?.tokens?.vst || "").trim();
    downloadAccessTokens.standalone = String(parsed?.tokens?.standalone || "").trim();
  } catch (_error) {
  }
}

function saveUnlockState() {
  localStorage.setItem(
    "coresynth_install_unlock",
    JSON.stringify({
      vst: unlockState.vst,
      standalone: unlockState.standalone,
      expires_at: {
        vst: Number(unlockExpiresAt.vst || 0),
        standalone: Number(unlockExpiresAt.standalone || 0)
      },
      source: {
        vst: String(unlockSource.vst || "none"),
        standalone: String(unlockSource.standalone || "none")
      },
      tokens: {
        vst: String(downloadAccessTokens.vst || ""),
        standalone: String(downloadAccessTokens.standalone || "")
      }
    })
  );
}

function setUnlock(productKey, enabled, source = "none", expiresAt = 0) {
  if (productKey !== "vst" && productKey !== "standalone") {
    return;
  }
  unlockState[productKey] = Boolean(enabled);
  unlockSource[productKey] = String(source || "none").trim().toLowerCase() || "none";
  unlockExpiresAt[productKey] = unlockState[productKey] ? Math.max(0, Number(expiresAt || 0) || 0) : 0;
  if (!unlockState[productKey]) {
    unlockExpiresAt[productKey] = 0;
    unlockSource[productKey] = "none";
    downloadAccessTokens[productKey] = "";
  }
}

function applyUnlockExpiry() {
  const now = Date.now();
  let changed = false;
  for (const productKey of ["vst", "standalone"]) {
    const expiresAt = Number(unlockExpiresAt[productKey] || 0);
    if (expiresAt > 0 && now > expiresAt) {
      setUnlock(productKey, false, "none", 0);
      changed = true;
    }
  }
  if (changed) {
    saveUnlockState();
  }
  return changed;
}

function isUnlocked(productKey) {
  applyUnlockExpiry();
  return productKey === "vst" ? unlockState.vst : productKey === "standalone" ? unlockState.standalone : false;
}

function buildPaypalUrl(itemName, priceText = "") {
  const amount = parsePriceAmount(priceText);
  const safeName = String(itemName || "North3rnLight3r Product").trim();
  const params = new URLSearchParams({
    cmd: "_xclick",
    business: CHECKOUT_CONTACT_EMAIL,
    item_name: safeName,
    currency_code: "USD"
  });
  if (amount) {
    params.set("amount", amount);
  }
  return `https://www.paypal.com/cgi-bin/webscr?${params.toString()}`;
}

function resolvePaypalBuyUrl(itemName, priceText = "", rawUrl = "") {
  const candidate = String(rawUrl || "").trim();
  if (candidate && /^https?:\/\//i.test(candidate) && !/paypal\.com/i.test(candidate)) {
    return candidate;
  }
  return buildPaypalUrl(itemName, priceText);
}

function normalizeSku(value) {
  return String(value || "")
    .trim()
    .toLowerCase()
    .replace(/[^a-z0-9._-]/g, "")
    .slice(0, 80);
}

function inferStoreSku(title) {
  const lowered = String(title || "").toLowerCase();
  if (lowered.includes("vst3")) {
    return "software_vst3";
  }
  if (lowered.includes("standalone")) {
    return "software_standalone";
  }
  return "";
}

function isBetaSoftwareSku(sku) {
  return BETA_SOFTWARE_SKUS.includes(normalizeSku(sku));
}

function betaInstallInfoForSku(sku) {
  return BETA_INSTALL_PRODUCTS[normalizeSku(sku)] || null;
}

function ensureBetaTrialAccess() {
  setUnlock("vst", true, FREE_TRIAL_UNLOCK_SOURCE, 0);
  setUnlock("standalone", true, FREE_TRIAL_UNLOCK_SOURCE, 0);
}

async function beginManagedCheckout({ sku = "", itemName = "", amount = "", currency = "USD", fallbackUrl = "" } = {}) {
  const safeItem = String(itemName || sku || "North3rnLight3r Product").trim() || "North3rnLight3r Product";
  const safeAmount = String(amount || "").trim();
  const target = String(fallbackUrl || buildPaypalUrl(safeItem, safeAmount)).trim();
  if (!target) {
    setCheckoutStatus("PayPal checkout link unavailable.");
    notifyWebsite("Checkout Error", "PayPal checkout link unavailable.");
    return;
  }
  setCheckoutStatus("Opening PayPal checkout...");
  const popup = window.open(target, "_blank", "noopener,noreferrer");
  if (!popup) {
    window.location.href = target;
  }
}

async function finalizeManagedCheckoutFromUrl() {
  const params = new URLSearchParams(window.location.search);
  const orderId = String(params.get("token") || params.get("order_id") || "").trim();
  if (!orderId) {
    return;
  }

  setCheckoutStatus("Finalizing payment...");
  try {
    const response = await fetch(apiUrl("/api/v1/pay/capture-order"), {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
        ...authHeaders()
      },
      body: JSON.stringify({ order_id: orderId })
    });
    const payload = await response.json().catch(() => null);
    if (!response.ok || !payload?.ok) {
      const message = String(payload?.error || "Payment capture failed.");
      setCheckoutStatus(message);
      notifyWebsite("Checkout Error", message);
      return;
    }

    const unlockProducts = Array.isArray(payload?.unlock_products) ? payload.unlock_products.map((p) => String(p || "").toLowerCase()) : [];
    if (unlockProducts.includes("vst")) {
      setUnlock("vst", true, "purchase", 0);
    }
    if (unlockProducts.includes("standalone")) {
      setUnlock("standalone", true, "purchase", 0);
    }
    const tokens = payload?.download_tokens && typeof payload.download_tokens === "object" ? payload.download_tokens : {};
    if (tokens.vst) {
      downloadAccessTokens.vst = String(tokens.vst).trim();
    }
    if (tokens.standalone) {
      downloadAccessTokens.standalone = String(tokens.standalone).trim();
    }
    saveUnlockState();
    renderInstall();

    const downloadUrl = String(payload?.download_url || "").trim();
    const paymentUnlock = payload?.payment_unlock && typeof payload.payment_unlock === "object" ? payload.payment_unlock : null;
    if (downloadUrl) {
      setCheckoutStatus(paymentUnlock ? "Payment complete. Secure download ready. Payment proof bundled with installer." : "Payment complete. Secure download ready.");
      notifyWebsite("Purchase Complete", paymentUnlock ? "Download is ready. Payment proof bundled." : "Download is ready.");
      window.open(downloadUrl, "_blank", "noopener,noreferrer");
    } else {
      setCheckoutStatus(paymentUnlock ? "Payment complete. Payment proof bundled with installer." : "Payment complete.");
      notifyWebsite("Purchase Complete", paymentUnlock ? "Payment processed. Proof bundled." : "Payment processed.");
    }
  } catch (_error) {
    setCheckoutStatus("Payment captured but confirmation failed. Contact support with your payment reference.");
    notifyWebsite("Checkout Warning", "Capture confirmation failed.");
  } finally {
    const cleanUrl = `${window.location.origin}${window.location.pathname}${window.location.hash || ""}`;
    window.history.replaceState({}, "", cleanUrl);
  }
}

function pickDownloadSourceToken(preferredProduct = "") {
  const product = String(preferredProduct || "").toLowerCase().trim();
  if (product === "vst" && downloadAccessTokens.vst) {
    return { product: "vst", token: downloadAccessTokens.vst };
  }
  if (product === "standalone" && downloadAccessTokens.standalone) {
    return { product: "standalone", token: downloadAccessTokens.standalone };
  }
  if (downloadAccessTokens.vst) {
    return { product: "vst", token: downloadAccessTokens.vst };
  }
  if (downloadAccessTokens.standalone) {
    return { product: "standalone", token: downloadAccessTokens.standalone };
  }
  return null;
}

async function openProtectedDownload(assetId, preferredProduct = "") {
  applyUnlockExpiry();
  const requiredProduct = String(preferredProduct || "").toLowerCase().trim();
  if ((requiredProduct === "vst" || requiredProduct === "standalone") && !isUnlocked(requiredProduct)) {
    setCheckoutStatus("Beta installer access is temporarily unavailable.");
    notifyWebsite("Install Locked", "Beta installer access is temporarily unavailable.");
    renderInstall();
    return;
  }

  const source = pickDownloadSourceToken(preferredProduct);
  if (!source) {
    if (requiredProduct && isUnlocked(requiredProduct) && String(assetId || "").trim()) {
      const directUrl = `${API_BASE}/downloads/${String(assetId || "").trim()}`;
      setCheckoutStatus("Opening install download...");
      window.open(directUrl, "_blank", "noopener,noreferrer");
      return;
    }
    setCheckoutStatus("Download token missing. Opening direct beta installer instead.");
    notifyWebsite("Download Locked", "Falling back to the direct beta installer.");
    if (String(assetId || "").trim()) {
      window.open(`${API_BASE}/downloads/${String(assetId || "").trim()}`, "_blank", "noopener,noreferrer");
    }
    return;
  }
  setCheckoutStatus("Preparing secure download session...");
  try {
    const response = await fetch(apiUrl("/api/create-download-session"), {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
        ...authHeaders()
      },
      body: JSON.stringify({
        asset_id: String(assetId || "").trim(),
        source_token: source.token
      })
    });
    const payload = await response.json().catch(() => null);
    if (!response.ok || !payload?.ok || !payload?.download_url) {
      const fallbackPath = `/downloads/${String(assetId || "").trim()}`;
      if (String(assetId || "").trim()) {
        const fallbackUrl = `${API_BASE}${fallbackPath}?access_token=${encodeURIComponent(source.token)}`;
        setCheckoutStatus("Secure session endpoint unavailable. Opening direct download.");
        window.open(fallbackUrl, "_blank", "noopener,noreferrer");
        return;
      }
      const message = String(payload?.error || "Unable to create download session.");
      setCheckoutStatus(message);
      notifyWebsite("Download Error", message);
      return;
    }
    const absoluteUrl = String(payload.download_url).startsWith("http")
      ? String(payload.download_url)
      : `${API_BASE}${String(payload.download_url).startsWith("/") ? "" : "/"}${String(payload.download_url)}`;
    setCheckoutStatus("Download session created.");
    window.open(absoluteUrl, "_blank", "noopener,noreferrer");
  } catch (_error) {
    setCheckoutStatus("Network error while creating download session.");
    notifyWebsite("Download Error", "Network error while creating download session.");
  }
}

function normalizeTrackMeta(track) {
  const title = String(track?.title || track?.file_name || "Untitled").trim();
  const description = String(track?.description || "Full beat stream").trim();
  const bpm = String(track?.bpm || track?.tempo_bpm || "N/A").trim();
  const price = "$19.99";
  return { title, description, bpm, price };
}

function formatPlayerTime(value) {
  const seconds = Number(value || 0);
  if (!Number.isFinite(seconds) || seconds < 0) {
    return "0:00";
  }
  const whole = Math.floor(seconds);
  const minutes = Math.floor(whole / 60);
  const remainder = whole % 60;
  return `${minutes}:${String(remainder).padStart(2, "0")}`;
}

function isPreviewableAudio(url) {
  const value = String(url || "").toLowerCase();
  return value.includes("/api/v1/catalog/stream/") || /\.(mp3|wav|ogg|m4a|flac|aiff|aif)(\?|$)/i.test(value);
}

function resolveTrackPlaybackUrl(track) {
  const streamUrl = normalizePlayableUrl(track?.stream_url || "");
  if (streamUrl) {
    return streamUrl;
  }
  const routedUrl = normalizePlayableUrl(track?.full_song_url || track?.public_url || "");
  if (routedUrl) {
    return routedUrl;
  }
  const fileName = String(track?.asset_file_name || track?.file_name || "").trim();
  if (fileName) {
    return apiUrl(`/assets/audio/catalog/${encodeURIComponent(fileName)}`);
  }
  return "";
}

function notificationsSupported() {
  return typeof window !== "undefined" && "Notification" in window;
}

function notificationsPermissionText() {
  if (!notificationsSupported()) {
    return "Not supported";
  }
  if (Notification.permission === "granted") {
    return "On";
  }
  if (Notification.permission === "denied") {
    return "Blocked";
  }
  return "Off";
}

function refreshNotificationsButton() {
  if (!enableNotificationsButton) {
    return;
  }
  enableNotificationsButton.textContent = `Notifications: ${notificationsPermissionText()}`;
}

async function requestNotificationsPermission() {
  if (!notificationsSupported()) {
    refreshNotificationsButton();
    return;
  }
  if (Notification.permission === "default") {
    await Notification.requestPermission();
  }
  refreshNotificationsButton();
}

function notifyWebsite(title, body) {
  if (!notificationsSupported()) {
    return;
  }
  if (Notification.permission !== "granted") {
    return;
  }
  try {
    const note = new Notification(String(title || "North3rnLight3r"), {
      body: String(body || "").slice(0, 220),
      icon: "assets/brand/hero_mascot.png"
    });
    setTimeout(() => note.close(), 6000);
  } catch (_error) {
  }
}

class WebsiteAudioEngine {
  constructor(audioElement, titleElement, metaElement, buttonElement, buyElement, stopElement, progressElement, currentTimeElement, durationElement, canvasElement) {
    this.audio = audioElement;
    this.titleEl = titleElement;
    this.metaEl = metaElement;
    this.buttonEl = buttonElement;
    this.buyEl = buyElement;
    this.stopEl = stopElement;
    this.progressEl = progressElement;
    this.currentTimeEl = currentTimeElement;
    this.durationEl = durationElement;
    this.canvasEl = canvasElement;
    this.queue = [];
    this.index = -1;
    this.currentTrack = null;
    this.audioContext = null;
    this.sourceNode = null;
    this.analyser = null;
    this.freqData = null;
    this.visualizerFrame = 0;

    this.audio.preload = "auto";
    this.audio.controls = true;
    this.audio.crossOrigin = "anonymous";

    this.audio.addEventListener("play", async () => {
      this.buttonEl.textContent = "Pause";
      await this.ensureVisualizer();
      this.startVisualizer();
    });

    this.audio.addEventListener("pause", () => {
      this.buttonEl.textContent = "Play";
      if (this.audio.ended || this.audio.currentTime === 0) {
        this.resetVisualizer();
      }
    });

    this.audio.addEventListener("ended", () => {
      this.buttonEl.textContent = "Play";
      this.resetVisualizer();
    });

    this.audio.addEventListener("loadedmetadata", () => {
      this.updateProgress();
    });

    this.audio.addEventListener("canplay", () => {
      this.updateProgress();
    });

    this.audio.addEventListener("timeupdate", () => {
      this.updateProgress();
    });

    this.audio.addEventListener("error", () => {
      this.buttonEl.textContent = "Play";
      this.resetVisualizer();
      const title = String(this.currentTrack?.title || this.titleEl.textContent || "Track");
      notifyWebsite("Playback Error", `${title} could not be loaded from the active stream route.`);
    });

    if (this.progressEl) {
      this.progressEl.addEventListener("input", () => {
        const duration = Number(this.audio.duration || 0);
        const value = Number(this.progressEl.value || 0);
        if (Number.isFinite(duration) && duration > 0) {
          this.audio.currentTime = (Math.max(0, Math.min(1000, value)) / 1000) * duration;
        }
        this.updateProgress();
      });
    }

    if (this.stopEl) {
      this.stopEl.addEventListener("click", () => {
        this.stop();
      });
    }

    this.buyEl.addEventListener("click", (event) => {
      event.preventDefault();
      const itemName = String(this.buyEl.dataset.itemName || this.titleEl.textContent || "North3rnLight3r Beat");
      const itemPrice = String(this.buyEl.dataset.itemPrice || "$19.99").trim() || "$19.99";
      beginManagedCheckout({
        itemName,
        amount: itemPrice,
        fallbackUrl: String(this.buyEl.href || buildPaypalUrl(itemName, itemPrice))
      });
    });

    this.resetVisualizer();
    this.updateProgress();
  }

  async ensureVisualizer() {
    if (this.analyser) {
      if (this.audioContext?.state === "suspended") {
        await this.audioContext.resume().catch(() => {});
      }
      return;
    }
    const AudioContextCtor = window.AudioContext || window.webkitAudioContext;
    if (!AudioContextCtor) {
      return;
    }
    this.audioContext = new AudioContextCtor();
    this.sourceNode = this.audioContext.createMediaElementSource(this.audio);
    this.analyser = this.audioContext.createAnalyser();
    this.analyser.fftSize = 2048;
    this.analyser.smoothingTimeConstant = 0.82;
    this.freqData = new Uint8Array(this.analyser.frequencyBinCount);
    this.sourceNode.connect(this.analyser);
    this.analyser.connect(this.audioContext.destination);
    if (this.audioContext.state === "suspended") {
      await this.audioContext.resume().catch(() => {});
    }
  }

  resetVisualizer() {
    if (!this.canvasEl) {
      return;
    }
    const ctx = this.canvasEl.getContext("2d");
    if (!ctx) {
      return;
    }
    const width = this.canvasEl.width;
    const height = this.canvasEl.height;
    ctx.clearRect(0, 0, width, height);
    const bg = ctx.createLinearGradient(0, 0, 0, height);
    bg.addColorStop(0, "rgba(16, 42, 68, 0.85)");
    bg.addColorStop(1, "rgba(2, 8, 16, 0.98)");
    ctx.fillStyle = bg;
    ctx.fillRect(0, 0, width, height);
    ctx.strokeStyle = "rgba(71, 215, 255, 0.18)";
    ctx.lineWidth = 1;
    for (let i = 1; i <= 4; i += 1) {
      const y = (height / 5) * i;
      ctx.beginPath();
      ctx.moveTo(0, y);
      ctx.lineTo(width, y);
      ctx.stroke();
    }
  }

  startVisualizer() {
    if (!this.analyser || !this.canvasEl || !this.freqData) {
      return;
    }
    const ctx = this.canvasEl.getContext("2d");
    if (!ctx) {
      return;
    }
    const width = this.canvasEl.width;
    const height = this.canvasEl.height;
    const draw = () => {
      this.visualizerFrame = window.requestAnimationFrame(draw);
      this.analyser.getByteFrequencyData(this.freqData);
      this.resetVisualizer();
      const barCount = 72;
      const step = Math.max(1, Math.floor(this.freqData.length / barCount));
      const gap = 4;
      const barWidth = Math.max(4, (width / barCount) - gap);
      const gradient = ctx.createLinearGradient(0, 0, 0, height);
      gradient.addColorStop(0, "rgba(184, 255, 98, 0.95)");
      gradient.addColorStop(0.45, "rgba(71, 215, 255, 0.9)");
      gradient.addColorStop(1, "rgba(19, 95, 180, 0.55)");
      ctx.fillStyle = gradient;
      for (let index = 0; index < barCount; index += 1) {
        const value = this.freqData[index * step] || 0;
        const magnitude = Math.max(6, (value / 255) * (height - 24));
        const x = index * (barWidth + gap);
        const y = height - magnitude;
        ctx.fillRect(x, y, barWidth, magnitude);
      }
    };
    window.cancelAnimationFrame(this.visualizerFrame);
    draw();
  }

  updateProgress() {
    const current = Number(this.audio.currentTime || 0);
    const duration = Number(this.audio.duration || 0);
    if (this.progressEl) {
      this.progressEl.value = Number.isFinite(duration) && duration > 0
        ? String(Math.max(0, Math.min(1000, Math.round((current / duration) * 1000))))
        : "0";
    }
    if (this.currentTimeEl) {
      this.currentTimeEl.textContent = formatPlayerTime(current);
    }
    if (this.durationEl) {
      this.durationEl.textContent = formatPlayerTime(duration);
    }
  }

  setQueue(queue) {
    this.queue = Array.isArray(queue) ? queue.slice() : [];
    if (this.queue.length === 0) {
      this.index = -1;
      this.currentTrack = null;
      this.titleEl.textContent = "Select a beat";
      this.metaEl.textContent = "Full stream ready.";
      this.buyEl.href = buildPaypalUrl("North3rnLight3r Beat", "$19.99");
      this.buyEl.dataset.itemName = "North3rnLight3r Beat";
      this.buyEl.dataset.itemPrice = "$19.99";
      this.audio.removeAttribute("src");
      this.audio.load();
      this.buttonEl.textContent = "Play";
      this.updateProgress();
      this.resetVisualizer();
      return;
    }
    if (this.index < 0 || this.index >= this.queue.length) {
      this.index = 0;
    }
    this.setSelectedTrack(this.queue[this.index]);
  }

  setSelectedTrack(track) {
    const meta = normalizeTrackMeta(track);
    this.currentTrack = track;
    this.titleEl.textContent = meta.title;
    this.metaEl.textContent = `BPM ${meta.bpm}`;
    this.buyEl.href = buildPaypalUrl(meta.title, meta.price);
    this.buyEl.dataset.itemName = meta.title;
    this.buyEl.dataset.itemPrice = meta.price;
  }

  async play(index = this.index >= 0 ? this.index : 0) {
    if (index < 0 || index >= this.queue.length) {
      return;
    }
    const track = this.queue[index];
    const playbackUrl = resolveTrackPlaybackUrl(track);
    if (!playbackUrl) {
      notifyWebsite("Playback Error", "No stream URL is available for this beat.");
      return;
    }
    this.index = index;
    this.setSelectedTrack(track);
    if (this.audio.src !== playbackUrl) {
      this.audio.src = playbackUrl;
      this.audio.load();
    }
    await this.ensureVisualizer();
    try {
      await this.audio.play();
      notifyWebsite("Now Playing", normalizeTrackMeta(track).title);
    } catch (_error) {
      this.buttonEl.textContent = "Play";
    }
  }

  async previewExternal(title, details, sourceUrl, priceText = "$19.99") {
    const playbackUrl = String(sourceUrl || "").trim();
    if (!isPreviewableAudio(playbackUrl)) {
      this.stop();
      return;
    }
    this.index = -1;
    this.currentTrack = null;
    this.titleEl.textContent = title;
    this.metaEl.textContent = details;
    this.buyEl.href = buildPaypalUrl(title, priceText);
    this.buyEl.dataset.itemName = title;
    this.buyEl.dataset.itemPrice = String(priceText || "$19.99");
    this.audio.src = playbackUrl;
    this.audio.load();
    await this.ensureVisualizer();
    try {
      await this.audio.play();
      notifyWebsite("Now Playing", title);
    } catch (_error) {
      this.buttonEl.textContent = "Play";
    }
  }

  async toggle() {
    if (!this.audio.src && this.queue.length > 0) {
      await this.play(this.index >= 0 ? this.index : 0);
      return;
    }
    if (this.audio.paused) {
      await this.ensureVisualizer();
      await this.audio.play().catch(() => {});
    } else {
      this.audio.pause();
    }
  }

  async next() {
    if (this.queue.length === 0) {
      return;
    }
    const nextIndex = this.index >= this.queue.length - 1 ? 0 : this.index + 1;
    await this.play(nextIndex);
  }

  async prev() {
    if (this.queue.length === 0) {
      return;
    }
    const prevIndex = this.index <= 0 ? this.queue.length - 1 : this.index - 1;
    await this.play(prevIndex);
  }

  stop() {
    this.audio.pause();
    try {
      this.audio.currentTime = 0;
    } catch (_error) {
    }
    this.updateProgress();
    this.resetVisualizer();
  }
}

const audioEngine = new WebsiteAudioEngine(audioPlayer, nowTitle, nowMeta, toggleTrack, buyTrack, stopTrack, audioProgress, audioCurrentTime, audioDuration, audioSpectrum);

function renderStore(content) {
  const productCards = (content?.products || []).map((item) => ({
    sku: normalizeSku(item?.sku || inferStoreSku(item?.title || "")),
    title: String(item?.title || "Product"),
    description: String(item?.description || ""),
    price: String(item?.price || ""),
    placeholder: Boolean(item?.placeholder),
    availabilityLabel: String(item?.availability_label || item?.availabilityLabel || ""),
    futurePriceLabel: String(item?.future_price_label || item?.futurePriceLabel || ""),
    buyUrl: resolvePaypalBuyUrl(String(item?.title || "Product"), String(item?.price || ""), String(item?.buy_url || ""))
  }));

  storeGrid.innerHTML = "";
  productCards.forEach((card) => {
    const node = document.createElement("article");
    node.className = "store-card";

    const title = document.createElement("h3");
    title.textContent = card.title;

    const desc = document.createElement("p");
    desc.className = "muted";
    desc.textContent = card.description;

    const price = document.createElement("p");
    if (card.placeholder) {
      price.textContent = card.futurePriceLabel || POST_BETA_PRICE_LABEL;
      node.append(title, desc, price);
    } else if (isBetaSoftwareSku(card.sku)) {
      const installInfo = betaInstallInfoForSku(card.sku);
      price.textContent = `${card.availabilityLabel || BETA_FREE_TRIAL_LABEL} | ${card.futurePriceLabel || POST_BETA_PRICE_LABEL}`;
      const downloadButton = document.createElement("button");
      downloadButton.className = "btn";
      downloadButton.type = "button";
      downloadButton.textContent = "Download Free Beta";
      downloadButton.addEventListener("click", () => {
        if (!installInfo) {
          return;
        }
        openProtectedDownload(installInfo.assetId, installInfo.productKey);
      });
      node.append(title, desc, price, downloadButton);
    } else if (card.sku) {
      price.textContent = card.price;
      const buyButton = document.createElement("button");
      buyButton.className = "btn";
      buyButton.type = "button";
      buyButton.textContent = "Buy License";
      buyButton.addEventListener("click", () => {
        beginManagedCheckout({
          sku: card.sku,
          itemName: card.title,
          amount: card.price,
          fallbackUrl: card.buyUrl
        });
      });
      node.append(title, desc, price, buyButton);
    } else {
      price.textContent = card.price;
      const buyButton = document.createElement("button");
      buyButton.className = "btn";
      buyButton.type = "button";
      buyButton.textContent = "Buy License";
      buyButton.addEventListener("click", () => {
        beginManagedCheckout({
          itemName: card.title,
          amount: card.price,
          fallbackUrl: card.buyUrl
        });
      });
      node.append(title, desc, price, buyButton);
    }
    storeGrid.appendChild(node);
  });

  if (productCards.length === 0) {
    const empty = document.createElement("p");
    empty.className = "muted";
    empty.textContent = "No products listed.";
    storeGrid.appendChild(empty);
  }
}

function renderServices(content) {
  servicesGrid.innerHTML = "";
  const services = Array.isArray(content?.services) ? content.services : [];
  counterState.services = services.length;
  syncCounters();

  services.forEach((service) => {
    const node = document.createElement("article");
    node.className = "store-card";

    const title = document.createElement("h3");
    title.textContent = String(service?.title || "Service");

    const desc = document.createElement("p");
    desc.className = "muted";
    desc.textContent = String(service?.description || "");

    const price = document.createElement("p");
    price.textContent = String(service?.price || "");

    const buyButton = document.createElement("button");
    buyButton.className = "btn";
    buyButton.type = "button";
    buyButton.textContent = "Book Service";
    buyButton.addEventListener("click", () => {
      beginManagedCheckout({
        itemName: String(service?.title || "Service"),
        amount: String(service?.price || ""),
        fallbackUrl: buildPaypalUrl(String(service?.title || "Service"), String(service?.price || ""))
      });
    });

    node.append(title, desc, price, buyButton);
    servicesGrid.appendChild(node);
  });

  if (services.length === 0) {
    const empty = document.createElement("p");
    empty.className = "muted";
    empty.textContent = "No services listed.";
    servicesGrid.appendChild(empty);
  }
}

function renderInstall() {
  const installCards = [
    {
      title: "Plugin Linux Installer 2.2.4 Beta",
      description: "Linux VST3 installer for the current AudioSuite beta with live diagnostics, stereo field analysis, reference matching, and AudioSynth Mix Tips.",
      price: BETA_FREE_TRIAL_LABEL,
      assetId: "install-vst.sh",
      productKey: "vst"
    },
    {
      title: "Standalone Linux Installer 2.2.4 Beta",
      description: "Linux standalone build for the current AudioSuite beta with the same analysis and advisory workflow as the plugin.",
      price: BETA_FREE_TRIAL_LABEL,
      assetId: "install-standalone.sh",
      productKey: "standalone"
    },
    {
      title: "Windows Build Instructions",
      description: "Current Windows build guidance for producers who want to compile the plugin and standalone locally.",
      price: "",
      ctaLabel: "Open Instructions",
      url: INSTALLATION_DOC_URL
    },
    {
      title: "2.2.4 Beta README",
      description: "Release summary, current scope, and build expectations for the active beta.",
      price: "",
      ctaLabel: "Open README",
      url: README_DOC_URL
    },
    {
      title: "Minimum PC Requirements",
      description: "Minimum Windows and Linux targets for stable AudioSuite playback, analysis, and UI performance.",
      price: "",
      ctaLabel: "Open Requirements",
      url: MINIMUM_PC_DOC_URL
    },
    {
      title: "Detailed Update Log",
      description: "Detailed engineering log for the current release candidate surface.",
      price: "",
      ctaLabel: "Open Log",
      url: RELEASE_LOG_DOC_URL
    },
    {
      title: "VST Guide",
      description: "VST setup and live analysis workflow for the 2.2.4 Beta plugin.",
      price: "",
      ctaLabel: "Open Guide",
      url: VST_GUIDE_DOC_URL
    },
    {
      title: "Standalone Guide",
      description: "Standalone app setup and workflow for the 2.2.4 Beta build.",
      price: "",
      ctaLabel: "Open Guide",
      url: STANDALONE_GUIDE_DOC_URL
    }
  ];

  counterState.products = 2;
  syncCounters();
  installGrid.innerHTML = "";

  installCards.forEach((card) => {
    const node = document.createElement("article");
    node.className = "store-card";

    const title = document.createElement("h3");
    title.textContent = card.title;

    const desc = document.createElement("p");
    desc.className = "muted";
    desc.textContent = card.description;

    const price = document.createElement("p");
    price.textContent = card.price;

    const actions = document.createElement("div");
    actions.className = "row";

    if (card.productKey && card.assetId) {
      const installButton = document.createElement("button");
      installButton.className = "btn";
      installButton.type = "button";
      installButton.textContent = "Download Free Beta";
      installButton.addEventListener("click", () => {
        openProtectedDownload(card.assetId, card.productKey);
      });
      actions.append(installButton);
    } else if (card.url) {
      const openButton = document.createElement("button");
      openButton.className = "btn ghost";
      openButton.type = "button";
      openButton.textContent = card.ctaLabel || "Open";
      openButton.addEventListener("click", () => {
        const target = String(card.url || "").trim();
        const popup = window.open(target, "_blank", "noopener,noreferrer");
        if (!popup) {
          window.location.href = target;
        }
      });
      actions.append(openButton);
    }

    node.append(title, desc, price, actions);
    installGrid.appendChild(node);
  });
}

function renderCatalog() {
  catalogList.innerHTML = "";
  tracks.forEach((track, index) => {
    const meta = normalizeTrackMeta(track);
    const genre = String(track?.genre || "").trim();

    const card = document.createElement("article");
    card.className = "track-card";
    const artwork = createArtworkNode(meta.title, track?.artwork_url, index);

    const title = document.createElement("h4");
    title.textContent = meta.title;

    const details = document.createElement("p");
    details.className = "track-meta";
    details.textContent = `BPM ${meta.bpm}${genre ? ` · ${genre}` : ""}`;

    const play = document.createElement("button");
    play.className = "btn";
    play.type = "button";
    play.textContent = "Play";
    play.addEventListener("click", () => audioEngine.play(index));

    const buy = document.createElement("button");
    buy.className = "btn ghost";
    buy.type = "button";
    buy.textContent = "Buy";
    buy.addEventListener("click", () => {
      beginManagedCheckout({
        itemName: meta.title,
        amount: meta.price,
        fallbackUrl: buildPaypalUrl(meta.title, meta.price)
      });
    });

    const actionRow = document.createElement("div");
    actionRow.className = "row";
    actionRow.append(play, buy);

    card.append(artwork, title, details, actionRow);
    catalogList.appendChild(card);
  });
}

function renderSoundpacks(items) {
  soundpackList.innerHTML = "";

  if (!Array.isArray(items) || items.length === 0) {
    const empty = document.createElement("p");
    empty.className = "muted";
    empty.textContent = "No packs uploaded.";
    soundpackList.appendChild(empty);
    return;
  }

  items.forEach((item) => {
    const card = document.createElement("article");
    card.className = "track-card";

    const titleText = String(item?.title || item?.file_name || "Pack").trim();
    const packType = String(item?.pack_type || "soundpack").trim();
    const description = String(item?.description || "").trim();
    const bpm = String(item?.bpm || "N/A").trim();
    const key = String(item?.key || item?.key_signature || "N/A").trim();
    const tempo = String(item?.tempo || item?.bpm || "N/A").trim();
    const price = String(item?.price || defaultPriceForPackType(packType)).trim();

    const artwork = createArtworkNode(titleText, item?.artwork_url, titleText.length);
    const title = document.createElement("h4");
    title.textContent = titleText;

    const details = document.createElement("p");
    details.className = "track-meta";
    details.textContent = `${packType} | ${description || "Pack"} | BPM ${bpm} | Key ${key} | Tempo ${tempo} | ${price}`;

    const actionRow = document.createElement("div");
    actionRow.className = "row";

    const preview = document.createElement("button");
    preview.className = "btn";
    preview.type = "button";
    preview.textContent = "Preview";
    preview.addEventListener("click", () => {
      audioEngine.previewExternal(
        `${titleText} (${packType})`,
        `${description || "Pack"} | BPM ${bpm}${key !== "N/A" ? ` | Key ${key}` : ""}${tempo !== "N/A" ? ` | Tempo ${tempo}` : ""}`,
        String(item?.public_url || ""),
        price
      );
    });

    const buy = document.createElement("button");
    buy.className = "btn";
    buy.textContent = "Buy";
    buy.type = "button";
    buy.addEventListener("click", () => {
      beginManagedCheckout({
        itemName: `${titleText} (${packType})`,
        amount: price,
        fallbackUrl: buildPaypalUrl(`${titleText} (${packType})`, price)
      });
    });

    actionRow.append(preview, buy);
    card.append(artwork, title, details, actionRow);
    soundpackList.appendChild(card);
  });
}

async function fetchJson(pathname, headers = {}) {
  const response = await fetch(apiUrl(pathname), {
    cache: "no-store",
    headers
  });
  const payload = await response.json().catch(() => null);
  return { response, payload };
}

async function loadCatalog() {
  const { response, payload } = await fetchJson("/api/v1/catalog/list", {
    ...authHeaders()
  });
  let rawTracks = Array.isArray(payload?.tracks) ? payload.tracks : [];
  if (!response.ok || !payload?.ok || rawTracks.length === 0) {
    const seedResponse = await fetch("assets/data/beat_catalog.json", { cache: "no-store" }).catch(() => null);
    const seedPayload = seedResponse ? await seedResponse.json().catch(() => null) : null;
    rawTracks = Array.isArray(seedPayload) ? seedPayload : [];
  }

  const seenTracks = new Set();
  tracks = rawTracks
    .map((track, index) => ({
      ...track,
      artwork_url: resolveArtworkUrl(track, index),
      price: String(track?.price || "$19.99").trim() || "$19.99"
    }))
    .filter((track) => {
      const dedupeKey = String(track?.key || track?.file_name || "").trim().toLowerCase();
      if (!dedupeKey || seenTracks.has(dedupeKey)) {
        return false;
      }
      seenTracks.add(dedupeKey);
      return true;
    });
  counterState.beatCatalog = tracks.length;
  syncCounters();
  audioEngine.setQueue(tracks);
  renderCatalog();
}

async function loadSoundpacks() {
  const { response, payload } = await fetchJson("/api/v1/soundpacks/list", {
    ...authHeaders()
  });
  if (!response.ok || !payload?.ok) {
    renderSoundpacks([]);
    return;
  }
  renderSoundpacks(payload.soundpacks || []);
}

async function loadStore() {
  const { response, payload } = await fetchJson("/api/v1/content/get", {
    ...authHeaders()
  });

  if (!response.ok || !payload?.ok) {
    renderStore(null);
    renderServices(null);
    return;
  }

  const content = payload.content || null;
  renderStore(content);
  renderServices(content);
}

function bindCatalog() {
  catalogToggle.addEventListener("click", () => {
    const expanded = catalogToggle.getAttribute("aria-expanded") === "true";
    catalogToggle.setAttribute("aria-expanded", expanded ? "false" : "true");
    catalogToggle.textContent = expanded ? "Open Catalog" : "Close Catalog";
    catalogWrap.hidden = expanded;
  });

  prevTrack.addEventListener("click", () => {
    audioEngine.prev();
  });

  nextTrack.addEventListener("click", () => {
    audioEngine.next();
  });

  toggleTrack.addEventListener("click", () => {
    audioEngine.toggle();
  });
}

function setupSupportForm() {
  supportForm.addEventListener("submit", async (event) => {
    event.preventDefault();

    const name = String(document.getElementById("support-name").value || "").trim();
    const email = String(document.getElementById("support-email").value || "").trim();
    const message = String(document.getElementById("support-message").value || "").trim();
    if (!name || !email || !message) {
      supportStatus.textContent = "Name, email, and message are required.";
      return;
    }

    supportStatus.textContent = "Sending inquiry...";
    try {
      const response = await fetch(apiUrl("/api/v1/inquiries/submit"), {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
          ...authHeaders()
        },
        body: JSON.stringify({ name, email, message })
      });
      const payload = await response.json().catch(() => null);
      if (!response.ok || !payload?.ok) {
        throw new Error(String(payload?.error || "inquiry submit failed"));
      }
      supportStatus.textContent = `Inquiry saved for ${String(payload?.target_email || INQUIRY_CONTACT_EMAIL)} · ID ${payload.inquiry_id}`;
      notifyWebsite("Inquiry Received", `Saved for ${String(payload?.target_email || INQUIRY_CONTACT_EMAIL)}`);
      supportForm.reset();
    } catch (error) {
      supportStatus.textContent = `Inquiry save failed. Use ${INQUIRY_CONTACT_EMAIL}.`;
      notifyWebsite("Inquiry Fallback", String(error?.message || "Email fallback required."));
      window.location.href = buildInquiryMailto(name, email, message, INQUIRY_CONTACT_EMAIL);
    }
  });
}

function setupNotifications() {
  refreshNotificationsButton();
  if (enableNotificationsButton) {
    enableNotificationsButton.addEventListener("click", () => {
      requestNotificationsPermission();
    });
  }
}

async function init() {
  if (yearEl) {
    yearEl.textContent = new Date().getFullYear().toString();
  }

  applySiteTextConfig();
  setCheckoutStatus("");
  setupNotifications();
  bindCatalog();
  setupSupportForm();
  loadUnlockState();
  ensureBetaTrialAccess();
  saveUnlockState();
  await loadMetricCatalog();
  renderInstall();
  await finalizeManagedCheckoutFromUrl();

  await loadStore();
  await loadCatalog();
  await loadSoundpacks();
}

init();
