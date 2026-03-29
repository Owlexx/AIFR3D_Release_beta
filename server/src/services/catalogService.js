const fs = require("fs");
const path = require("path");
const { DATABASE_FILES, ROOT_DIR } = require("../config/constants");
const { safeJsonRead, safeJsonWrite } = require("../utils/helpers");

/**
 * CATALOG_SERVICE: This service manages the "Beat Catalog" database for 2.2.4.
 * It handles the reading and writing of tracks, ensuring your music store is always up to date.
 */

const getCatalog = () => {
  /**
   * getCatalog: This function reads the entire beat catalog from the database file.
   * If the file doesn't exist yet, it returns an empty list.
   */
  return safeJsonRead(DATABASE_FILES.CATALOG, []);
};

const saveCatalog = (catalogData) => {
  /**
   * saveCatalog: This function saves the updated beat catalog back to the database file.
   */
  return safeJsonWrite(DATABASE_FILES.CATALOG, catalogData);
};

const findTrackByKey = (key) => {
  /**
   * findTrackByKey: This function searches for a specific track in the catalog using its unique key.
   */
  const catalog = getCatalog();
  return catalog.find(track => track.key === key);
};

module.exports = {
  getCatalog,
  saveCatalog,
  findTrackByKey,
};
