package com.coresynth.admin

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlinx.coroutines.launch

// Plain English Summary: This component creates a professional terminal-style interface for the Android app.
// It allows users to type commands and see output in a classic "green-on-black" console look.
@Composable
fun TerminalComponent(
    output: String,
    onExecute: (String) -> Unit,
    modifier: Modifier = Modifier
) {
    var input by remember { mutableStateOf("") }
    val scrollState = rememberScrollState()
    val scope = rememberCoroutineScope()

    // Plain English Summary: Automatically scroll to the bottom of the terminal when new output arrives.
    LaunchedEffect(output) {
        scope.launch {
            scrollState.animateScrollTo(scrollState.maxValue)
        }
    }

    Column(
        modifier = modifier
            .fillMaxSize()
            .background(Color(0xFF050B12)) // Deep dark background
            .padding(8.dp)
    ) {
        // Plain English Summary: The scrollable area showing command history and results.
        Box(
            modifier = Modifier
                .weight(1f)
                .fillMaxWidth()
                .verticalScroll(scrollState)
        ) {
            Text(
                text = output,
                color = Color(0xFF18D2E7), // Cyan/Teal terminal text
                fontFamily = FontFamily.Monospace,
                fontSize = 13.sp,
                lineHeight = 18.sp
            )
        }

        // Plain English Summary: The interactive command line at the bottom.
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(top = 8.dp)
        ) {
            Text(
                text = "$ ",
                color = Color(0xFF7BD6C8), // Different color for the prompt
                fontFamily = FontFamily.Monospace,
                fontSize = 14.sp
            )
            
            BasicTextField(
                value = input,
                onValueChange = { input = it },
                modifier = Modifier.fillMaxWidth(),
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
                        if (input.isNotBlank()) {
                            onExecute(input)
                            input = ""
                        }
                    }
                ),
                singleLine = true
            )
        }
    }
}
