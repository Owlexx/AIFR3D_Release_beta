package com.coresynth.admin

import android.util.Log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.RequestBody.Companion.toRequestBody
import org.json.JSONObject
import java.io.File

/**
 * Plain English Summary: This class manages direct access to Cloudflare R2 storage
 * for uploading, downloading, and listing files from the admin app.
 */
class R2StorageManager(
    private val localServerUrl: String = "http://127.0.0.1:3333",
    private val r2BucketUrl: String = "https://aifr3d.r2.cloudflarestorage.com"
) {
    private val client = OkHttpClient()

    companion object {
        private const val TAG = "R2StorageManager"
    }

    /**
     * Plain English Summary: Lists all beats from R2 storage
     */
    suspend fun listBeats(): Result<List<BeatFile>> = withContext(Dispatchers.IO) {
        try {
            val request = Request.Builder()
                .url("$localServerUrl/api/r2/beats/list")
                .get()
                .build()

            val response = client.newCall(request).execute()
            if (!response.isSuccessful) {
                return@withContext Result.failure(Exception("Failed to list beats: ${response.code}"))
            }

            val body = response.body?.string() ?: return@withContext Result.failure(Exception("Empty response"))
            val json = JSONObject(body)
            val beatsArray = json.getJSONArray("beats")

            val beats = mutableListOf<BeatFile>()
            for (i in 0 until beatsArray.length()) {
                val beatJson = beatsArray.getJSONObject(i)
                beats.add(
                    BeatFile(
                        id = beatJson.getString("id"),
                        name = beatJson.getString("name"),
                        url = beatJson.getString("url"),
                        duration = beatJson.getInt("duration"),
                        bpm = beatJson.getInt("bpm")
                    )
                )
            }

            Log.d(TAG, "Listed ${beats.size} beats from R2")
            Result.success(beats)
        } catch (e: Exception) {
            Log.e(TAG, "Error listing beats: ${e.message}", e)
            Result.failure(e)
        }
    }

    /**
     * Plain English Summary: Gets a signed URL for a file in R2 storage
     */
    suspend fun getSignedUrl(fileName: String, expiresIn: Int = 3600): Result<String> =
        withContext(Dispatchers.IO) {
            try {
                val requestBody = JSONObject().apply {
                    put("fileName", fileName)
                    put("expiresIn", expiresIn)
                }.toString().toRequestBody()

                val request = Request.Builder()
                    .url("$localServerUrl/api/r2/signed-url")
                    .post(requestBody)
                    .addHeader("Content-Type", "application/json")
                    .build()

                val response = client.newCall(request).execute()
                if (!response.isSuccessful) {
                    return@withContext Result.failure(Exception("Failed to get signed URL: ${response.code}"))
                }

                val body = response.body?.string() ?: return@withContext Result.failure(Exception("Empty response"))
                val json = JSONObject(body)
                val url = json.getString("url")

                Log.d(TAG, "Got signed URL for: $fileName")
                Result.success(url)
            } catch (e: Exception) {
                Log.e(TAG, "Error getting signed URL: ${e.message}", e)
                Result.failure(e)
            }
        }

    /**
     * Plain English Summary: Uploads a file to R2 storage
     */
    suspend fun uploadFile(filePath: String, fileName: String = File(filePath).name): Result<String> =
        withContext(Dispatchers.IO) {
            try {
                val file = File(filePath)
                if (!file.exists()) {
                    return@withContext Result.failure(Exception("File not found: $filePath"))
                }

                val fileBytes = file.readBytes()
                val requestBody = fileBytes.toRequestBody()

                val request = Request.Builder()
                    .url("$r2BucketUrl/upload/$fileName")
                    .put(requestBody)
                    .addHeader("Content-Type", "application/octet-stream")
                    .build()

                val response = client.newCall(request).execute()
                if (!response.isSuccessful) {
                    return@withContext Result.failure(Exception("Upload failed: ${response.code}"))
                }

                val uploadUrl = "$r2BucketUrl/$fileName"
                Log.d(TAG, "File uploaded successfully: $uploadUrl")
                Result.success(uploadUrl)
            } catch (e: Exception) {
                Log.e(TAG, "Error uploading file: ${e.message}", e)
                Result.failure(e)
            }
        }

    /**
     * Plain English Summary: Downloads a file from R2 storage
     */
    suspend fun downloadFile(beatId: String, destinationPath: String): Result<File> =
        withContext(Dispatchers.IO) {
            try {
                val request = Request.Builder()
                    .url("$localServerUrl/api/r2/beats/stream/$beatId")
                    .get()
                    .build()

                val response = client.newCall(request).execute()
                if (!response.isSuccessful) {
                    return@withContext Result.failure(Exception("Download failed: ${response.code}"))
                }

                val destinationFile = File(destinationPath)
                destinationFile.parentFile?.mkdirs()
                destinationFile.writeBytes(response.body?.bytes() ?: byteArrayOf())

                Log.d(TAG, "File downloaded to: $destinationPath")
                Result.success(destinationFile)
            } catch (e: Exception) {
                Log.e(TAG, "Error downloading file: ${e.message}", e)
                Result.failure(e)
            }
        }

    /**
     * Plain English Summary: Deletes a file from R2 storage
     */
    suspend fun deleteFile(fileName: String): Result<Boolean> = withContext(Dispatchers.IO) {
        try {
            val request = Request.Builder()
                .url("$r2BucketUrl/$fileName")
                .delete()
                .build()

            val response = client.newCall(request).execute()
            val success = response.isSuccessful

            Log.d(TAG, "File deletion ${if (success) "successful" else "failed"}: $fileName")
            Result.success(success)
        } catch (e: Exception) {
            Log.e(TAG, "Error deleting file: ${e.message}", e)
            Result.failure(e)
        }
    }

    /**
     * Plain English Summary: Gets R2 bucket information
     */
    suspend fun getBucketInfo(): Result<BucketInfo> = withContext(Dispatchers.IO) {
        try {
            val request = Request.Builder()
                .url("$localServerUrl/api/r2/beats")
                .get()
                .build()

            val response = client.newCall(request).execute()
            if (!response.isSuccessful) {
                return@withContext Result.failure(Exception("Failed to get bucket info: ${response.code}"))
            }

            val body = response.body?.string() ?: return@withContext Result.failure(Exception("Empty response"))
            val json = JSONObject(body)

            val bucketInfo = BucketInfo(
                bucketUrl = json.getString("bucketUrl"),
                beatsCatalogUrl = json.getString("beatsCatalogUrl"),
                uploadUrl = json.getString("uploadUrl")
            )

            Log.d(TAG, "Got bucket info: ${bucketInfo.bucketUrl}")
            Result.success(bucketInfo)
        } catch (e: Exception) {
            Log.e(TAG, "Error getting bucket info: ${e.message}", e)
            Result.failure(e)
        }
    }
}

/**
 * Plain English Summary: Data class representing a beat file in R2 storage
 */
data class BeatFile(
    val id: String,
    val name: String,
    val url: String,
    val duration: Int,
    val bpm: Int
)

/**
 * Plain English Summary: Data class representing R2 bucket information
 */
data class BucketInfo(
    val bucketUrl: String,
    val beatsCatalogUrl: String,
    val uploadUrl: String
)
