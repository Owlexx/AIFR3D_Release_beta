const path = require("express");
const fs = require("fs");
const crypto = require("crypto");

/**
 * ROOT_DIR: The base directory of the entire project.
 * This is used as a reference point for all other paths.
 */
const ROOT_DIR = path.resolve(__dirname, "../../..");

/**
 * STORAGE_DIR: Path to the server's persistent storage.
 * This is where JSON databases, logs, and uploads are kept.
 */
const STORAGE_DIR = path.join(ROOT_DIR, "server/storage");

/**
 * DATABASE_FILES: A collection of paths to JSON files that act as our simple database.
 * Each file stores specific types of data like catalog items, sessions, or admin settings.
 */
const DATABASE_FILES = {
  CONTENT: path.join(STORAGE_DIR, "content.json"),
  CATALOG: path.join(STORAGE_DIR, "catalog.json"),
  SOUNDPACKS: path.join(STORAGE_DIR, "soundpacks.json"),
  SESSIONS: path.join(STORAGE_DIR, "sessions.json"),
  ADMIN_CREDENTIALS: path.join(STORAGE_DIR, "admin_credentials.json"),
  PROMO_CODES: path.join(STORAGE_DIR, "promo_codes.json"),
  INQUIRIES: path.join(STORAGE_DIR, "inquiries.json"),
  SALES: path.join(STORAGE_DIR, "sales.json"),
  OPENAI_CONFIG: path.join(STORAGE_DIR, "openai_config.json"),
  BRAIN_CONFIG: path.join(ROOT_DIR, "assets/aifr3d_brain/brain_config.json"),
};

/**
 * PORT: The network port the server will listen on.
 * Defaults to 8787 if not specified in environment variables.
 */
const PORT = Number(process.env.PORT || 8787);

/**
 * API_TOKEN: A secret key used to authorize requests to the API.
 */
const API_TOKEN = String(process.env.DAWAI_API_TOKEN || "").trim();

/**
 * SECRETS: Various keys used for signing receipts, sessions, and tokens.
 */
const FALLBACK_RUNTIME_SECRET = crypto.randomBytes(32).toString("hex");
const SECRETS = {
  RECEIPT_SIGNING: String(process.env.RECEIPT_SIGNING_SECRET || API_TOKEN || FALLBACK_RUNTIME_SECRET).trim(),
  ADMIN_SESSION: String(process.env.ADMIN_SESSION_SECRET || API_TOKEN || FALLBACK_RUNTIME_SECRET).trim(),
  DOWNLOAD_TOKEN: String(process.env.DOWNLOAD_TOKEN_SECRET || API_TOKEN || FALLBACK_RUNTIME_SECRET).trim(),
};

/**
 * OPENAI_CONFIG: Configuration for AI features using OpenAI's API.
 */
const OPENAI_CONFIG = {
  PRIMARY_MODEL: "gpt-5.2",
  BASE_URL: "https://api.openai.com/v1/responses",
  KEY_FILE: "/home/north3rnlight3r/Documents/API_KEY_INDEX/OpenAI API Key.txt",
};

module.exports = {
  ROOT_DIR,
  STORAGE_DIR,
  DATABASE_FILES,
  PORT,
  API_TOKEN,
  SECRETS,
  OPENAI_CONFIG,
};
