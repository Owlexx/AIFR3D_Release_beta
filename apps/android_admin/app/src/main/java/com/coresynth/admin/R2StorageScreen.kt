package com.coresynth.admin

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlinx.coroutines.launch

/**
 * Plain English Summary: This composable provides a UI for browsing and managing
 * files directly from Cloudflare R2 storage within the admin app.
 */
@Composable
fun R2StorageScreen(
    r2Manager: R2StorageManager = R2StorageManager(),
    modifier: Modifier = Modifier
) {
    var beats by remember { mutableStateOf<List<BeatFile>>(emptyList()) }
    var bucketInfo by remember { mutableStateOf<BucketInfo?>(null) }
    var isLoading by remember { mutableStateOf(false) }
    var errorMessage by remember { mutableStateOf("") }
    var selectedBeat by remember { mutableStateOf<BeatFile?>(null) }
    
    val scope = rememberCoroutineScope()
    val scrollState = rememberScrollState()

    /**
     * Plain English Summary: Loads the beat list from R2 storage
     */
    fun loadBeats() {
        isLoading = true
        errorMessage = ""
        scope.launch {
            val result = r2Manager.listBeats()
            result.onSuccess { loadedBeats ->
                beats = loadedBeats
                isLoading = false
            }
            result.onFailure { error ->
                errorMessage = error.message ?: "Unknown error"
                isLoading = false
            }
        }
    }

    /**
     * Plain English Summary: Loads bucket information
     */
    fun loadBucketInfo() {
        scope.launch {
            val result = r2Manager.getBucketInfo()
            result.onSuccess { info ->
                bucketInfo = info
            }
            result.onFailure { error ->
                errorMessage = error.message ?: "Unknown error"
            }
        }
    }

    /**
     * Plain English Summary: Downloads a beat file
     */
    fun downloadBeat(beat: BeatFile) {
        isLoading = true
        scope.launch {
            val result = r2Manager.downloadFile(beat.id, "/sdcard/Music/${beat.name}.mp3")
            result.onSuccess {
                errorMessage = "Downloaded: ${beat.name}"
                isLoading = false
            }
            result.onFailure { error ->
                errorMessage = "Download failed: ${error.message}"
                isLoading = false
            }
        }
    }

    // Load data on first composition
    LaunchedEffect(Unit) {
        loadBeats()
        loadBucketInfo()
    }

    Column(
        modifier = modifier
            .fillMaxSize()
            .background(Color(0xFF050B12))
            .verticalScroll(scrollState)
            .padding(12.dp)
    ) {
        // Header
        Text(
            "R2 Storage - Beat Catalog",
            color = Color(0xFF18D2E7),
            fontSize = 18.sp,
            fontFamily = FontFamily.Monospace,
            modifier = Modifier.padding(bottom = 12.dp)
        )

        // Bucket Info
        if (bucketInfo != null) {
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(Color(0xFF0F1B2E))
                    .padding(12.dp)
            ) {
                Column {
                    Text(
                        "Bucket URL:",
                        color = Color(0xFF7BD6C8),
                        fontSize = 12.sp,
                        fontFamily = FontFamily.Monospace
                    )
                    Text(
                        bucketInfo!!.bucketUrl,
                        color = Color(0xFFE9F6FF),
                        fontSize = 11.sp,
                        fontFamily = FontFamily.Monospace,
                        modifier = Modifier.padding(top = 4.dp)
                    )
                }
            }
            Spacer(modifier = Modifier.height(12.dp))
        }

        // Error Message
        if (errorMessage.isNotEmpty()) {
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(Color(0xFF8B0000))
                    .padding(12.dp)
            ) {
                Text(
                    errorMessage,
                    color = Color.White,
                    fontSize = 12.sp
                )
            }
            Spacer(modifier = Modifier.height(12.dp))
        }

        // Refresh Button
        Button(
            onClick = { loadBeats() },
            modifier = Modifier.fillMaxWidth(),
            colors = ButtonDefaults.buttonColors(
                containerColor = Color(0xFF18D2E7),
                contentColor = Color(0xFF001116)
            ),
            enabled = !isLoading
        ) {
            Text(if (isLoading) "Loading..." else "Refresh Beats")
        }

        Spacer(modifier = Modifier.height(12.dp))

        // Loading Indicator
        if (isLoading) {
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(24.dp),
                contentAlignment = Alignment.Center
            ) {
                CircularProgressIndicator(
                    color = Color(0xFF18D2E7)
                )
            }
        }

        // Beats List
        if (beats.isNotEmpty()) {
            Text(
                "Available Beats (${beats.size})",
                color = Color(0xFF7BD6C8),
                fontSize = 14.sp,
                fontFamily = FontFamily.Monospace,
                modifier = Modifier.padding(vertical = 8.dp)
            )

            LazyColumn {
                items(beats) { beat ->
                    BeatItemCard(
                        beat = beat,
                        isSelected = selectedBeat?.id == beat.id,
                        onSelect = { selectedBeat = beat },
                        onDownload = { downloadBeat(beat) }
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                }
            }
        } else if (!isLoading) {
            Text(
                "No beats found in R2 storage",
                color = Color(0xFFFF6B6B),
                fontSize = 12.sp,
                modifier = Modifier.padding(vertical = 16.dp)
            )
        }

        // Selected Beat Details
        if (selectedBeat != null) {
            Spacer(modifier = Modifier.height(16.dp))
            BeatDetailsPanel(
                beat = selectedBeat!!,
                onDownload = { downloadBeat(selectedBeat!!) }
            )
        }
    }
}

/**
 * Plain English Summary: Displays a single beat item in a card format
 */
@Composable
fun BeatItemCard(
    beat: BeatFile,
    isSelected: Boolean,
    onSelect: () -> Unit,
    onDownload: () -> Unit
) {
    Box(
        modifier = Modifier
            .fillMaxWidth()
            .background(
                if (isSelected) Color(0xFF1A4D5C) else Color(0xFF0F1B2E)
            )
            .clickable { onSelect() }
            .padding(12.dp)
    ) {
        Column {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Column(modifier = Modifier.weight(1f)) {
                    Text(
                        beat.name,
                        color = Color(0xFF18D2E7),
                        fontSize = 14.sp,
                        fontFamily = FontFamily.Monospace
                    )
                    Text(
                        "${beat.bpm} BPM • ${beat.duration}s",
                        color = Color(0xFF7BD6C8),
                        fontSize = 11.sp,
                        fontFamily = FontFamily.Monospace,
                        modifier = Modifier.padding(top = 4.dp)
                    )
                }
                
                if (isSelected) {
                    Button(
                        onClick = onDownload,
                        modifier = Modifier.size(width = 80.dp, height = 32.dp),
                        colors = ButtonDefaults.buttonColors(
                            containerColor = Color(0xFF00FF00),
                            contentColor = Color(0xFF001116)
                        )
                    ) {
                        Text("Download", fontSize = 10.sp)
                    }
                }
            }
        }
    }
}

/**
 * Plain English Summary: Displays detailed information about a selected beat
 */
@Composable
fun BeatDetailsPanel(
    beat: BeatFile,
    onDownload: () -> Unit
) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .background(Color(0xFF0F1B2E))
            .padding(12.dp)
    ) {
        Text(
            "Beat Details",
            color = Color(0xFF18D2E7),
            fontSize = 14.sp,
            fontFamily = FontFamily.Monospace,
            modifier = Modifier.padding(bottom = 8.dp)
        )

        DetailRow("Name", beat.name)
        DetailRow("ID", beat.id)
        DetailRow("BPM", beat.bpm.toString())
        DetailRow("Duration", "${beat.duration}s")
        DetailRow("URL", beat.url)

        Spacer(modifier = Modifier.height(12.dp))

        Button(
            onClick = onDownload,
            modifier = Modifier.fillMaxWidth(),
            colors = ButtonDefaults.buttonColors(
                containerColor = Color(0xFF18D2E7),
                contentColor = Color(0xFF001116)
            )
        ) {
            Text("Download Beat")
        }
    }
}

/**
 * Plain English Summary: Displays a detail row with label and value
 */
@Composable
fun DetailRow(label: String, value: String) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 4.dp)
    ) {
        Text(
            "$label:",
            color = Color(0xFF7BD6C8),
            fontSize = 11.sp,
            fontFamily = FontFamily.Monospace,
            modifier = Modifier.width(80.dp)
        )
        Text(
            value,
            color = Color(0xFFE9F6FF),
            fontSize = 11.sp,
            fontFamily = FontFamily.Monospace,
            modifier = Modifier.weight(1f)
        )
    }
}
