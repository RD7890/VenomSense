package com.ryzixstudios.venomsense

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Scaffold
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import com.ryzixstudios.venomsense.network.WebSocketManager
import com.ryzixstudios.venomsense.ui.screens.DashboardScreen
import com.ryzixstudios.venomsense.ui.theme.BgColor
import com.ryzixstudios.venomsense.ui.theme.PrimaryGlow
import com.ryzixstudios.venomsense.ui.theme.VenomSenseTheme

class MainActivity : ComponentActivity() {
    private val wsManager = WebSocketManager()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            VenomSenseTheme {
                MainApp(wsManager)
            }
        }
    }

    override fun onResume() {
        super.onResume()
        wsManager.connect()
    }

    override fun onPause() {
        super.onPause()
        wsManager.disconnect()
    }
}

@Composable
fun MainApp(wsManager: WebSocketManager) {
    Scaffold(
        containerColor = BgColor
    ) { innerPadding ->
        Box(
            modifier = Modifier
                .fillMaxSize()
                .background(
                    Brush.radialGradient(
                        colors = listOf(PrimaryGlow, Color.Transparent),
                        center = androidx.compose.ui.geometry.Offset(500f, -200f),
                        radius = 1200f
                    )
                )
                .padding(innerPadding)
        ) {
            DashboardScreen(wsManager)
        }
    }
}
