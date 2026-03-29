package com.coresynth.admin

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlinx.coroutines.launch

/**
 * Plain English Summary: This composable provides a remote terminal interface that connects
 * to the WebSocket terminal server on the user's local machine.
 */
@Composable
fun RemoteTerminalScreen(
    serverHost: String,
    serverPort: Int = 8765,
    authToken: String,
    modifier: Modifier = Modifier
) {
    var terminalOutput by remember { mutableStateOf("🔌 Connecting to terminal server...\n") }
    var commandInput by remember { mutableStateOf("") }
    var isConnected by remember { mutableStateOf(false) }
    var serverHostInput by remember { mutableStateOf(serverHost) }
    var serverPortInput by remember { mutableStateOf(serverPort.toString()) }
    var authTokenInput by remember { mutableStateOf(authToken) }
    var isConnecting by remember { mutableStateOf(false) }
    
    val scrollState = rememberScrollState()
    val scope = rememberCoroutineScope()
    
    var webSocketClient by remember { mutableStateOf<WebSocketTerminalClient?>(null) }

    /**
     * Plain English Summary: Automatically scroll to the bottom when new output arrives
     */
    LaunchedEffect(terminalOutput) {
        scope.launch {
            scrollState.animateScrollTo(scrollState.maxValue)
        }
    }

    Column(
        modifier = modifier
            .fillMaxSize()
            .background(Color(0xFF050B12))
            .padding(8.dp)
    ) {
        // Connection settings panel
        if (!isConnected) {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(Color(0xFF0F1B2E))
                    .padding(12.dp)
            ) {
                Text(
                    "Remote Terminal Connection",
                    color = Color(0xFF18D2E7),
                    fontSize = 14.sp
                )
                
                Spacer(modifier = Modifier.height(8.dp))
                
                OutlinedTextField(
                    value = serverHostInput,
                    onValueChange = { serverHostInput = it },
                    label = { Text("Server Host/IP") },
                    modifier = Modifier.fillMaxWidth(),
                    singleLine = true
                )
                
                Spacer(modifier = Modifier.height(8.dp))
                
                OutlinedTextField(
                    value = serverPortInput,
                    onValueChange = { serverPortInput = it },
                    label = { Text("Port") },
                    modifier = Modifier.fillMaxWidth(),
                    singleLine = true
                )
                
                Spacer(modifier = Modifier.height(8.dp))
                
                OutlinedTextField(
                    value = authTokenInput,
                    onValueChange = { authTokenInput = it },
                    label = { Text("Auth Token") },
                    modifier = Modifier.fillMaxWidth(),
                    singleLine = true
                )
                
                Spacer(modifier = Modifier.height(12.dp))
                
                Button(
                    onClick = {
                        isConnecting = true
                        terminalOutput = "🔌 Connecting to $serverHostInput:$serverPortInput...\n"
                        
                        val port = serverPortInput.toIntOrNull() ?: 8765
                        webSocketClient = WebSocketTerminalClient(
                            serverHost = serverHostInput,
                            serverPort = port,
                            authToken = authTokenInput,
                            onOutput = { output ->
                                terminalOutput += output
                            },
                            onError = { error ->
                                terminalOutput += "❌ Error: $error\n"
                            },
                            onConnected = {
                                isConnected = true
                                isConnecting = false
                                terminalOutput += "✅ Connected!\n"
                            },
                            onDisconnected = {
                                isConnected = false
                                isConnecting = false
                                terminalOutput += "❌ Disconnected\n"
                            }
                        )
                        webSocketClient?.connect()
                    },
                    modifier = Modifier.fillMaxWidth(),
                    enabled = !isConnecting,
                    colors = ButtonDefaults.buttonColors(
                        containerColor = Color(0xFF18D2E7),
                        contentColor = Color(0xFF001116)
                    )
                ) {
                    Text(if (isConnecting) "Connecting..." else "Connect")
                }
            }
            
            Spacer(modifier = Modifier.height(8.dp))
        }

        // Terminal output area
        Box(
            modifier = Modifier
                .weight(1f)
                .fillMaxWidth()
                .background(Color(0xFF050B12))
                .verticalScroll(scrollState)
                .padding(8.dp)
        ) {
            Text(
                text = terminalOutput,
                color = Color(0xFF18D2E7),
                fontFamily = FontFamily.Monospace,
                fontSize = 12.sp,
                lineHeight = 16.sp
            )
        }

        // Connection status
        if (isConnected) {
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(Color(0xFF0F1B2E))
                    .padding(8.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    "🟢 Connected to $serverHostInput:$serverPortInput",
                    color = Color(0xFF00FF00),
                    fontSize = 11.sp,
                    modifier = Modifier.weight(1f)
                )
                
                Button(
                    onClick = {
                        webSocketClient?.disconnect()
                        isConnected = false
                        terminalOutput += "\n🔌 Disconnected\n"
                    },
                    modifier = Modifier.size(width = 80.dp, height = 32.dp),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = Color(0xFF8B0000),
                        contentColor = Color.White
                    )
                ) {
                    Text("Disconnect", fontSize = 10.sp)
                }
            }
        }

        // Command input area
        if (isConnected) {
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(top = 8.dp)
            ) {
                Text(
                    text = "$ ",
                    color = Color(0xFF7BD6C8),
                    fontFamily = FontFamily.Monospace,
                    fontSize = 14.sp
                )
                
                BasicTextField(
                    value = commandInput,
                    onValueChange = { commandInput = it },
                    modifier = Modifier
                        .weight(1f)
                        .background(Color(0xFF0F1B2E))
                        .padding(4.dp),
                    textStyle = TextStyle(
                        color = Color(0xFFE9F6FF),
                        fontFamily = FontFamily.Monospace,
                        fontSize = 14.sp
                    ),
                    cursorBrush = SolidColor(Color(0xFF18D2E7)),
                    keyboardOptions = KeyboardOptions(
                        imeAction = ImeAction.Done
                    ),
                    keyboardActions = KeyboardActions(
                        onDone = {
                            if (commandInput.isNotBlank()) {
                                webSocketClient?.executeCommand(commandInput)
                                terminalOutput += "$ $commandInput\n"
                                commandInput = ""
                            }
                        }
                    ),
                    singleLine = true
                )
                
                Button(
                    onClick = {
                        if (commandInput.isNotBlank()) {
                            webSocketClient?.executeCommand(commandInput)
                            terminalOutput += "$ $commandInput\n"
                            commandInput = ""
                        }
                    },
                    modifier = Modifier
                        .size(width = 60.dp, height = 40.dp)
                        .padding(start = 4.dp),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = Color(0xFF18D2E7),
                        contentColor = Color(0xFF001116)
                    )
                ) {
                    Text("Send", fontSize = 10.sp)
                }
            }
        }
    }
}
