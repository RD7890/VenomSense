package com.ryzixstudios.venomsense.network

import android.util.Log
import com.google.gson.Gson
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.Response
import okhttp3.WebSocket
import okhttp3.WebSocketListener
import java.util.concurrent.TimeUnit

data class SensorData(
    val connected: Boolean = false,
    val distance: Int = 0,
    val r: Int = 0,
    val g: Int = 0,
    val b: Int = 0,
    val c: Int = 0,
    val toxinLevel: Int = 0,
    val hue: Double = 0.0,
    val saturation: Double = 0.0
)

class WebSocketManager {
    private val client = OkHttpClient.Builder()
        .readTimeout(3, TimeUnit.SECONDS)
        .retryOnConnectionFailure(true)
        .build()

    private var webSocket: WebSocket? = null
    private val _sensorData = MutableStateFlow(SensorData())
    val sensorData: StateFlow<SensorData> = _sensorData
    private val gson = Gson()

    fun connect() {
        val request = Request.Builder()
            .url("ws://192.168.4.1:81")
            .build()

        webSocket = client.newWebSocket(request, object : WebSocketListener() {
            override fun onOpen(webSocket: WebSocket, response: Response) {
                _sensorData.value = _sensorData.value.copy(connected = true)
                Log.d("WebSocket", "Connected")
            }

            override fun onMessage(webSocket: WebSocket, text: String) {
                try {
                    // Expecting JSON: {"distance":25,"r":120,"g":50,"b":40,"c":210}
                    val rawData = gson.fromJson(text, Map::class.java) as Map<String, Double>
                    
                    val distance = rawData["distance"]?.toInt() ?: 0
                    val r = rawData["r"]?.toInt() ?: 0
                    val g = rawData["g"]?.toInt() ?: 0
                    val b = rawData["b"]?.toInt() ?: 0
                    val c = rawData["c"]?.toInt() ?: 0

                    // Replicate Toxin Level Calculation Logic
                    val hueSat = rgbToHueSat(r, g, b)
                    var toxin = 0
                    if (distance > 0 && distance < 100) {
                        toxin = ((hueSat.second * 100) + (c % 50)).toInt().coerceIn(0, 100)
                    }

                    _sensorData.value = SensorData(
                        connected = true,
                        distance = distance,
                        r = r, g = g, b = b, c = c,
                        toxinLevel = toxin,
                        hue = hueSat.first,
                        saturation = hueSat.second
                    )
                } catch (e: Exception) {
                    Log.e("WebSocket", "Parse error", e)
                }
            }

            override fun onClosed(webSocket: WebSocket, code: Int, reason: String) {
                _sensorData.value = SensorData(connected = false)
            }

            override fun onFailure(webSocket: WebSocket, t: Throwable, response: Response?) {
                _sensorData.value = SensorData(connected = false)
            }
        })
    }

    fun disconnect() {
        webSocket?.close(1000, "App closed")
        webSocket = null
        _sensorData.value = SensorData(connected = false)
    }

    private fun rgbToHueSat(r: Int, g: Int, b: Int): Pair<Double, Double> {
        val rNorm = r / 255.0
        val gNorm = g / 255.0
        val bNorm = b / 255.0
        val max = maxOf(rNorm, gNorm, bNorm)
        val min = minOf(rNorm, gNorm, bNorm)
        val delta = max - min

        var h = 0.0
        if (delta != 0.0) {
            h = when (max) {
                rNorm -> 60 * (((gNorm - bNorm) / delta) % 6)
                gNorm -> 60 * (((bNorm - rNorm) / delta) + 2)
                else -> 60 * (((rNorm - gNorm) / delta) + 4)
            }
        }
        if (h < 0) h += 360

        val s = if (max == 0.0) 0.0 else delta / max
        return Pair(h, s)
    }
}
