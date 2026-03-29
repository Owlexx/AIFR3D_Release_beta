const DEFAULT_SECURITY_HEADERS = {
  "X-Content-Type-Options": "nosniff",
  "X-Frame-Options": "DENY",
  "Referrer-Policy": "strict-origin-when-cross-origin",
  "Permissions-Policy": "geolocation=(), microphone=(), camera=()",
  "Cross-Origin-Opener-Policy": "same-origin"
};

function json(payload, init = {}) {
  const headers = new Headers(init.headers || {});
  headers.set("Content-Type", "application/json; charset=utf-8");
  for (const [key, value] of Object.entries(DEFAULT_SECURITY_HEADERS)) {
    headers.set(key, value);
  }
  return new Response(JSON.stringify(payload), { ...init, headers });
}

function clean(value, max = 512) {
  return String(value || "").trim().slice(0, max);
}

function nowIso() {
  return new Date().toISOString();
}

function toBase64Url(raw) {
  return btoa(raw).replace(/\+/g, "-").replace(/\//g, "_").replace(/=+$/g, "");
}

function fromBase64Url(raw) {
  const base = String(raw || "").replace(/-/g, "+").replace(/_/g, "/");
  const padded = base + "=".repeat((4 - (base.length % 4)) % 4);
  return atob(padded);
}

function hashHex(input) {
  const data = new TextEncoder().encode(String(input || ""));
  return crypto.subtle.digest("SHA-256", data).then((buf) =>
    Array.from(new Uint8Array(buf), (b) => b.toString(16).padStart(2, "0")).join("")
  );
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
  return Array.from(new Uint8Array(sig), (b) => b.toString(16).padStart(2, "0")).join("");
}

async function signToken(secret, payload) {
  const payloadRaw = JSON.stringify(payload || {});
  const sig = await hmacHex(secret, payloadRaw);
  return `${toBase64Url(payloadRaw)}.${sig}`;
}

async function verifyToken(secret, token) {
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
  const expected = await hmacHex(secret, payloadRaw);
  if (expected !== sig) {
    return null;
  }
  try {
    return JSON.parse(payloadRaw);
  } catch (_error) {
    return null;
  }
}

function routeDownloadAsset(assetId) {
  const id = clean(assetId, 120).toLowerCase();
  const map = {
    "dawai-vst3": "software/dawai-vst3",
    "dawai-standalone": "software/dawai-standalone",
    "dawai-windows-installer.zip": "software/dawai-windows-installer.zip",
    "dawai-linux-amd64.deb": "software/dawai-linux-amd64.deb",
    "dawai-linux-arch-x86_64.pkg.tar.zst": "software/dawai-linux-arch-x86_64.pkg.tar.zst"
  };
  return map[id] || "";
}

async function rateLimit(env, request, bucket, maxRequests, windowSec) {
  const ip = clean(request.headers.get("CF-Connecting-IP") || "unknown", 128);
  const key = `rl:${bucket}:${ip}`;
  const current = Number((await env.DOWNLOAD_NONCES.get(key)) || "0") || 0;
  if (current >= maxRequests) {
    return { ok: false, retryAfter: windowSec };
  }
  await env.DOWNLOAD_NONCES.put(key, String(current + 1), { expirationTtl: windowSec });
  return { ok: true };
}

async function hasEntitlement(env, userHash, assetId) {
  if (!env.ENTITLEMENTS) {
    return false;
  }
  const key = `ent:${userHash}:${assetId}`;
  const wildcard = `ent:${userHash}:*`;
  const exact = await env.ENTITLEMENTS.get(key);
  const any = await env.ENTITLEMENTS.get(wildcard);
  return Boolean(exact || any);
}

async function verifyPaypalWebhook(env, headers, eventBody) {
  if (!env.PAYPAL_CLIENT_ID || !env.PAYPAL_CLIENT_SECRET || !env.PAYPAL_WEBHOOK_ID) {
    return { ok: false, error: "paypal webhook secrets missing" };
  }
  const mode = clean(env.PAYPAL_ENV || "sandbox", 16).toLowerCase() === "live" ? "live" : "sandbox";
  const base = mode === "live" ? "https://api-m.paypal.com" : "https://api-m.sandbox.paypal.com";

  const oauth = await fetch(`${base}/v1/oauth2/token`, {
    method: "POST",
    headers: {
      Authorization: `Basic ${btoa(`${env.PAYPAL_CLIENT_ID}:${env.PAYPAL_CLIENT_SECRET}`)}`,
      "Content-Type": "application/x-www-form-urlencoded"
    },
    body: "grant_type=client_credentials"
  });
  const oauthJson = await oauth.json().catch(() => null);
  const accessToken = clean(oauthJson?.access_token || "", 4096);
  if (!oauth.ok || !accessToken) {
    return { ok: false, error: "paypal oauth failed" };
  }

  const verifyPayload = {
    auth_algo: clean(headers.get("paypal-auth-algo") || "", 120),
    cert_url: clean(headers.get("paypal-cert-url") || "", 500),
    transmission_id: clean(headers.get("paypal-transmission-id") || "", 200),
    transmission_sig: clean(headers.get("paypal-transmission-sig") || "", 2000),
    transmission_time: clean(headers.get("paypal-transmission-time") || "", 120),
    webhook_id: clean(env.PAYPAL_WEBHOOK_ID || "", 200),
    webhook_event: eventBody
  };

  const verify = await fetch(`${base}/v1/notifications/verify-webhook-signature`, {
    method: "POST",
    headers: {
      Authorization: `Bearer ${accessToken}`,
      "Content-Type": "application/json"
    },
    body: JSON.stringify(verifyPayload)
  });
  const verifyJson = await verify.json().catch(() => null);
  const status = clean(verifyJson?.verification_status || "", 40);
  return {
    ok: verify.ok && status === "SUCCESS",
    error: verify.ok ? "" : "paypal signature verify failed"
  };
}

async function handleCreateDownloadSession(request, env) {
  const body = await request.json().catch(() => ({}));
  const assetId = clean(body.assetId || body.asset_id, 120).toLowerCase();
  const grantToken = clean(body.grantToken || body.grant_token || body.source_token, 8192);
  if (!assetId || !grantToken) {
    return json({ ok: false, error: "assetId and grantToken are required" }, { status: 400 });
  }
  const signingSecret = clean(env.SIGNING_SECRET || "", 4096);
  if (!signingSecret) {
    return json({ ok: false, error: "gateway misconfigured" }, { status: 500 });
  }

  const grant = await verifyToken(signingSecret, grantToken);
  if (!grant || Date.now() > Number(grant.exp || 0)) {
    return json({ ok: false, error: "invalid or expired grant token" }, { status: 403 });
  }

  const scope = Array.isArray(grant.scope) ? grant.scope.map((x) => clean(x, 120).toLowerCase()) : [];
  if (!scope.includes(assetId) && !scope.includes("bundle") && !scope.includes("*")) {
    return json({ ok: false, error: "grant token not entitled for asset" }, { status: 403 });
  }

  const userHash = clean(grant.user_hash || "", 128);
  if (userHash) {
    const entitled = await hasEntitlement(env, userHash, assetId);
    if (!entitled) {
      return json({ ok: false, error: "entitlement not found" }, { status: 403 });
    }
  }

  const ttl = Math.max(300, Math.min(900, Number(env.DOWNLOAD_TOKEN_TTL_SECONDS || "600") || 600));
  const jti = crypto.randomUUID();
  const uaHash = await hashHex(clean(request.headers.get("user-agent") || "", 512));
  const session = {
    kind: "download_session",
    asset_id: assetId,
    user_hash: userHash,
    ua_hash: uaHash,
    jti,
    iat: Date.now(),
    exp: Date.now() + ttl * 1000
  };
  await env.DOWNLOAD_NONCES.put(`dl_jti:${jti}`, "0", { expirationTtl: ttl + 60 });
  const token = await signToken(signingSecret, session);
  return json({
    ok: true,
    assetId,
    expiresAt: new Date(session.exp).toISOString(),
    downloadUrl: `/download/${encodeURIComponent(assetId)}?token=${encodeURIComponent(token)}`
  });
}

async function handleDownload(request, env, assetId) {
  const rateWindow = Math.max(10, Number(env.RATE_LIMIT_WINDOW_SECONDS || "60") || 60);
  const rateMax = Math.max(5, Number(env.RATE_LIMIT_MAX_REQUESTS || "30") || 30);
  const rate = await rateLimit(env, request, `download:${assetId}`, rateMax, rateWindow);
  if (!rate.ok) {
    return json({ ok: false, error: "rate_limited" }, { status: 429, headers: { "Retry-After": String(rate.retryAfter) } });
  }

  const signingSecret = clean(env.SIGNING_SECRET || "", 4096);
  const token = clean(new URL(request.url).searchParams.get("token") || "", 8192);
  if (!signingSecret || !token) {
    return json({ ok: false, error: "token required" }, { status: 401 });
  }
  const payload = await verifyToken(signingSecret, token);
  if (!payload || payload.kind !== "download_session" || Date.now() > Number(payload.exp || 0)) {
    return json({ ok: false, error: "invalid or expired token" }, { status: 401 });
  }
  if (clean(payload.asset_id || "", 120).toLowerCase() !== clean(assetId, 120).toLowerCase()) {
    return json({ ok: false, error: "token asset mismatch" }, { status: 403 });
  }

  const nonceKey = `dl_jti:${clean(payload.jti || "", 120)}`;
  const usedCount = Number((await env.DOWNLOAD_NONCES.get(nonceKey)) || "0") || 0;
  if (usedCount >= 1) {
    return json({ ok: false, error: "token already used" }, { status: 429 });
  }

  const requestUaHash = await hashHex(clean(request.headers.get("user-agent") || "", 512));
  if (clean(payload.ua_hash || "", 128) && clean(payload.ua_hash || "", 128) !== requestUaHash) {
    return json({ ok: false, error: "token client mismatch" }, { status: 403 });
  }

  const objectKey = routeDownloadAsset(assetId);
  if (!objectKey) {
    return json({ ok: false, error: "unknown asset" }, { status: 404 });
  }

  const object = await env.DOWNLOADS.get(objectKey);
  if (!object) {
    return json({ ok: false, error: "asset not found" }, { status: 404 });
  }

  await env.DOWNLOAD_NONCES.put(nonceKey, String(usedCount + 1), { expirationTtl: 60 * 60 });
  const headers = new Headers();
  headers.set("Content-Type", object.httpMetadata?.contentType || "application/octet-stream");
  headers.set("Content-Disposition", `attachment; filename="${objectKey.split("/").pop() || "download.bin"}"`);
  headers.set("Cache-Control", "no-store");
  for (const [k, v] of Object.entries(DEFAULT_SECURITY_HEADERS)) {
    headers.set(k, v);
  }
  return new Response(object.body, { status: 200, headers });
}

async function handlePaypalWebhook(request, env) {
  const body = await request.json().catch(() => null);
  if (!body) {
    return json({ ok: false, error: "invalid json" }, { status: 400 });
  }

  const verify = await verifyPaypalWebhook(env, request.headers, body);
  if (!verify.ok) {
    return json({ ok: false, error: verify.error || "webhook verification failed" }, { status: 401 });
  }

  const eventId = clean(body.id || "", 120);
  if (!eventId) {
    return json({ ok: false, error: "missing event id" }, { status: 400 });
  }

  const replayKey = `paypal_event:${eventId}`;
  if (await env.WEBHOOK_EVENTS.get(replayKey)) {
    return json({ ok: true, replay: true, id: eventId });
  }
  await env.WEBHOOK_EVENTS.put(replayKey, nowIso(), { expirationTtl: 60 * 60 * 24 * 30 });

  return json({
    ok: true,
    id: eventId,
    eventType: clean(body.event_type || "", 120),
    note: "Verified webhook accepted. Entitlement write hook should run here."
  });
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    const pathname = url.pathname;

    if (request.method === "OPTIONS") {
      return new Response(null, {
        status: 204,
        headers: {
          "Access-Control-Allow-Origin": "*",
          "Access-Control-Allow-Methods": "GET,POST,OPTIONS",
          "Access-Control-Allow-Headers": "Content-Type, Authorization"
        }
      });
    }

    if (pathname === "/api/health" && request.method === "GET") {
      return json({
        ok: true,
        service: "security-gateway",
        timestamp: nowIso(),
        has_signing_secret: Boolean(clean(env.SIGNING_SECRET || "", 8)),
        has_r2: Boolean(env.DOWNLOADS),
        has_entitlements_kv: Boolean(env.ENTITLEMENTS)
      });
    }

    if (pathname === "/api/security-headers" && request.method === "GET") {
      return json({ ok: true, headers: DEFAULT_SECURITY_HEADERS });
    }

    if (pathname === "/api/create-download-session" && request.method === "POST") {
      return handleCreateDownloadSession(request, env);
    }

    if (pathname === "/api/paypal/webhook" && request.method === "POST") {
      return handlePaypalWebhook(request, env);
    }

    if (pathname.startsWith("/download/") && request.method === "GET") {
      const assetId = decodeURIComponent(pathname.replace(/^\/download\//, ""));
      return handleDownload(request, env, assetId);
    }

    return json({ ok: false, error: "not found" }, { status: 404 });
  }
};
