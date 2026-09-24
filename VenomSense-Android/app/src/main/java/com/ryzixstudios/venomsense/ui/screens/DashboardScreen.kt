package com.ryzixstudios.venomsense.ui.screens

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.ryzixstudios.venomsense.network.WebSocketManager
import com.ryzixstudios.venomsense.ui.components.GlassCard
import com.ryzixstudios.venomsense.ui.theme.*
import java.util.Locale

@Composable
fun DashboardScreen(wsManager: WebSocketManager) {
    val sensorData by wsManager.sensorData.collectAsState()

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(20.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        TopBar(connected = sensorData.connected)
        Spacer(modifier = Modifier.height(30.dp))
        
        GaugeSection(toxinLevel = sensorData.toxinLevel, connected = sensorData.connected)
        
        Spacer(modifier = Modifier.height(30.dp))
        Button(
            onClick = { /* TODO Deep Scan */ },
            modifier = Modifier
                .fillMaxWidth()
                .height(60.dp),
            colors = ButtonDefaults.buttonColors(
                containerColor = PrimaryColor.copy(alpha = 0.15f),
                contentColor = PrimaryColor
            ),
            shape = RoundedCornerShape(20.dp)
        ) {
            Text("Initiate Deep Scan", fontSize = 18.sp, fontWeight = FontWeight.Bold)
        }
        
        Spacer(modifier = Modifier.height(30.dp))
        InfoGrid(
            hue = sensorData.hue,
            sat = sensorData.saturation,
            r = sensorData.r, g = sensorData.g, b = sensorData.b, c = sensorData.c
        )
    }
}

@Composable
fun TopBar(connected: Boolean) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text(
                text = "Venom",
                color = PrimaryColor,
                fontSize = 24.sp,
                fontWeight = FontWeight.Black
            )
            Text(
                text = "Sense",
                color = TextPrimary,
                fontSize = 24.sp,
                fontWeight = FontWeight.Black
            )
        }
        
        val badgeColor = if (connected) SafeColor else DangerColor
        val badgeBg = if (connected) SafeDim else DangerDim
        
        Box(
            modifier = Modifier
                .background(badgeBg, RoundedCornerShape(20.dp))
                .padding(horizontal = 12.dp, vertical = 6.dp)
        ) {
            Text(
                text = if (connected) "Connected" else "Offline",
                color = badgeColor,
                fontSize = 12.sp,
                fontWeight = FontWeight.Bold
            )
        }
    }
}

@Composable
fun GaugeSection(toxinLevel: Int, connected: Boolean) {
    val animatedProgress by animateFloatAsState(
        targetValue = if (connected) toxinLevel / 100f else 0f,
        animationSpec = tween(1000),
        label = "GaugeAnimation"
    )

    Box(
        modifier = Modifier
            .size(240.dp),
        contentAlignment = Alignment.Center
    ) {
        Canvas(modifier = Modifier.fillMaxSize()) {
            val strokeWidth = 15.dp.toPx()
            // Background arc
            drawArc(
                color = SurfaceColor,
                startAngle = 135f,
                sweepAngle = 270f,
                useCenter = false,
                style = Stroke(strokeWidth, cap = StrokeCap.Round)
            )
            // Foreground arc
            val gradient = Brush.sweepGradient(
                colors = listOf(SafeColor, WarningColor, DangerColor),
                center = center
            )
            drawArc(
                brush = gradient,
                startAngle = 135f,
                sweepAngle = 270f * animatedProgress,
                useCenter = false,
                style = Stroke(strokeWidth, cap = StrokeCap.Round)
            )
        }
        
        Column(horizontalAlignment = Alignment.CenterHorizontally) {
            Text(
                text = if (connected) "$toxinLevel%" else "--%",
                fontSize = 48.sp,
                fontWeight = FontWeight.Black,
                color = TextPrimary
            )
            Text(
                text = "TOXIN LEVEL",
                fontSize = 14.sp,
                color = TextSecondary,
                letterSpacing = 2.sp
            )
        }
    }
}

val WarningColor = Color(0xFFFFB300)

@Composable
fun InfoGrid(hue: Double, sat: Double, r: Int, g: Int, b: Int, c: Int) {
    Column(verticalArrangement = Arrangement.spacedBy(15.dp)) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(15.dp)
        ) {
            InfoCard(modifier = Modifier.weight(1f), label = "Phase Angle", value = String.format(Locale.US, "%.1f°", hue))
            InfoCard(modifier = Modifier.weight(1f), label = "Toxin Sat.", value = String.format(Locale.US, "%.1f%%", sat * 100))
        }
        InfoCard(
            modifier = Modifier.fillMaxWidth(),
            label = "Spectral Signature",
            value = "$r / $g / $b"
        )
        InfoCard(
            modifier = Modifier.fillMaxWidth(),
            label = "Optical Density",
            value = "$c"
        )
    }
}

@Composable
fun InfoCard(modifier: Modifier = Modifier, label: String, value: String) {
    GlassCard(modifier = modifier) {
        Column(
            modifier = Modifier.padding(20.dp),
            horizontalAlignment = Alignment.Start
        ) {
            Text(text = label, color = TextSecondary, fontSize = 12.sp)
            Spacer(modifier = Modifier.height(8.dp))
            Text(text = value, color = TextPrimary, fontSize = 24.sp, fontWeight = FontWeight.Bold)
        }
    }
}
