package com.coresynth.admin

import android.util.Log
import kotlinx.coroutines.*
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.WebSocket
import okhttp3.WebSocketListener
import okio.ByteString
import org.json.JSONObject
import java.util.concurrent.TimeUnit

/**
 * Plain English Summary: This class manages WebSocket connections to the remote terminal server.
 * It handles authentication, command execution, and message parsing for the Android terminal UI.
 */
class WebSocketTerminalClient(
    private val serverHost: String,
    private val serverPort: Int = 8765,
    private val authToken: String,
    private val onOutput: (String) -> Unit,
    private val onError: (String) -> Unit,
    private val onConnected: () -> Unit,
    private val onDisconnected: () -> Unit
) {
    private var webSocket: WebSocket? = null
    private var isConnected = false
    private var sessionId: String? = null
    private val client = OkHttpClient.Builder()
        .connectTimeout(10, TimeUnit.SECONDS)
        .readTimeout(30, TimeUnit.SECONDS)
        .writeTimeout(30, TimeUnit.SECONDS)
        .build()

    companion object {
        private const val TAG = "WebSocketTerminalClient"
    }

    /**
     * Plain English Summary: Establishes a WebSocket connection to the remote terminal server
     */
    fun connect() {
        try {
            val url = "ws://$serverHost:$serverPort"
            Log.d(TAG, "Connecting to $url")
            
            val request = Request.Builder()
                .url(url)
                .header("User-Agent", "AIFR3D-Admin-App/1.0")
                .build()

            webSocket = client.newWebSocket(request, object : WebSocketListener() {
                override fun onOpen(webSocket: WebSocket, response: okhttp3.Response) {
                    Log.d(TAG, "WebSocket connected")
                    isConnected = true
                    onConnected()
                    
                    // Send authentication message
                    authenticate()
                }

                override fun onMessage(webSocket: WebSocket, text: String) {
                    handleMessage(text)
                }

                override fun onMessage(webSocket: WebSocket, bytes: ByteString) {
                    handleMessage(bytes.utf8())
                }

                override fun onClosing(webSocket: WebSocket, code: Int, reason: String) {
                    Log.d(TAG, "WebSocket closing: $code $reason")
                    isConnected = false
                    onDisconnected()
                }

                override fun onClosed(webSocket: WebSocket, code: Int, reason: String) {
                    Log.d(TAG, "WebSocket closed: $code $reason")
                    isConnected = false
                    onDisconnected()
                }

                override fun onFailure(webSocket: WebSocket, t: Throwable, response: okhttp3.Response?) {
                    Log.e(TAG, "WebSocket failure: ${t.message}", t)
                    isConnected = false
                    onError("Connection failed: ${t.message}")
                    onDisconnected()
                }
            })
        } catch (e: Exception) {
            Log.e(TAG, "Failed to connect: ${e.message}", e)
            onError("Failed to connect: ${e.message}")
        }
    }

    /**
     * Plain English Summary: Sends authentication credentials to the server
     */
    private fun authenticate() {
        val authMessage = JSONObject().apply {
            put("type", "auth")
            put("token", authToken)
            put("shell", "/bin/bash")
        }
        webSocket?.send(authMessage.toString())
    }

    /**
     * Plain English Summary: Processes incoming messages from the WebSocket server
     */
    private fun handleMessage(text: String) {
        try {
            val message = JSONObject(text)
            val type = message.optString("type", "")

            when (type) {
                "auth_success" -> {
                    sessionId = message.optString("sessionId")
                    val hostname = message.optString("hostname", "unknown")
                    val shell = message.optString("shell", "/bin/bash")
                    onOutput("✅ Connected to $hostname\n🐚 Shell: $shell\n\n")
                    Log.d(TAG, "Authentication successful: $sessionId")
                }
                
                "auth_failed" -> {
                    val errorMsg = message.optString("message", "Authentication failed")
                    onError(errorMsg)
                    disconnect()
                }
                
                "output" -> {
                    val output = message.optString("output", "")
                    val error = message.optBoolean("error", false)
                    if (error) {
                        onError(output)
                    } else {
                        onOutput(output)
                    }
                }
                
                "error" -> {
                    val errorMsg = message.optString("message", "Unknown error")
                    onError(errorMsg)
                }
                
                "pong" -> {
                    Log.d(TAG, "Received pong")
                }
                
                "session_info" -> {
                    val uptime = message.optLong("uptime", 0)
                    val lastActivity = message.optLong("lastActivity", 0)
                    Log.d(TAG, "Session info - Uptime: $uptime, Last activity: $lastActivity")
                }
                
                else -> {
                    Log.w(TAG, "Unknown message type: $type")
                }
            }
        } catch (e: Exception) {
            Log.e(TAG, "Error parsing message: ${e.message}", e)
            onError("Error parsing response: ${e.message}")
        }
    }

    /**
     * Plain English Summary: Sends a command to the remote terminal server for execution
     */
    fun executeCommand(command: String) {
        if (!isConnected || sessionId == null) {
            onError("Not connected to terminal server")
            return
        }

        try {
            val commandMessage = JSONObject().apply {
                put("type", "execute")
                put("command", command)
                put("sessionId", sessionId)
            }
            webSocket?.send(commandMessage.toString())
            Log.d(TAG, "Command sent: ${command.take(50)}")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to send command: ${e.message}", e)
            onError("Failed to send command: ${e.message}")
        }
    }

    /**
     * Plain English Summary: Sends a keep-alive ping to the server
     */
    fun sendPing() {
        if (isConnected) {
            try {
                val pingMessage = JSONObject().apply {
                    put("type", "ping")
                }
                webSocket?.send(pingMessage.toString())
            } catch (e: Exception) {
                Log.e(TAG, "Failed to send ping: ${e.message}", e)
            }
        }
    }

    /**
     * Plain English Summary: Requests session information from the server
     */
    fun getSessionInfo() {
        if (isConnected) {
            try {
                val infoMessage = JSONObject().apply {
                    put("type", "info")
                }
                webSocket?.send(infoMessage.toString())
            } catch (e: Exception) {
                Log.e(TAG, "Failed to request session info: ${e.message}", e)
            }
        }
    }

    /**
     * Plain English Summary: Disconnects from the WebSocket server
     */
    fun disconnect() {
        try {
            webSocket?.close(1000, "Client disconnect")
            webSocket = null
            isConnected = false
            sessionId = null
        } catch (e: Exception) {
            Log.e(TAG, "Error disconnecting: ${e.message}", e)
        }
    }

    /**
     * Plain English Summary: Returns whether the client is currently connected
     */
    fun getIsConnected(): Boolean = isConnected

    /**
     * Plain English Summary: Returns the current session ID
     */
    fun getSessionId(): String? = sessionId
}
