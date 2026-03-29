const fs = require("fs");
const path = require("path");

/**
 * nowIso: This function returns the current date and time in a standard format (ISO 8601).
 * It's used for timestamping events, logs, and database updates.
 */
const nowIso = () => new Date().toISOString();

/**
 * ensureDirectoryExists: This function checks if a folder exists on the computer.
 * If the folder doesn't exist, it creates it.
 */
const ensureDirectoryExists = (dirPath) => {
  if (!fs.existsSync(dirPath)) {
    fs.mkdirSync(dirPath, { recursive: true });
  }
};

/**
 * safeJsonRead: This function reads a JSON file and turns it into a JavaScript object.
 * If the file is missing or contains broken data, it returns a default value instead of crashing.
 */
const safeJsonRead = (filePath, defaultValue = {}) => {
  try {
    if (fs.existsSync(filePath)) {
      const data = fs.readFileSync(filePath, "utf8");
      return JSON.parse(data);
    }
  } catch (error) {
    console.error(`Error reading JSON from ${filePath}:`, error.message);
  }
  return defaultValue;
};

/**
 * safeJsonWrite: This function saves a JavaScript object into a JSON file.
 * It uses 'pretty-printing' (indentation) to make the file easy for humans to read.
 */
const safeJsonWrite = (filePath, data) => {
  try {
    ensureDirectoryExists(path.dirname(filePath));
    fs.writeFileSync(filePath, JSON.stringify(data, null, 2), "utf8");
    return true;
  } catch (error) {
    console.error(`Error writing JSON to ${filePath}:`, error.message);
    return false;
  }
};

module.exports = {
  nowIso,
  ensureDirectoryExists,
  safeJsonRead,
  safeJsonWrite,
};
