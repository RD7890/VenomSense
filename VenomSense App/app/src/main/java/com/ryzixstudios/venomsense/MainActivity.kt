package com.ryzixstudios.venomsense

import android.animation.ObjectAnimator
import android.animation.PropertyValuesHolder
import android.animation.ValueAnimator
import android.annotation.SuppressLint
import android.graphics.Color
import android.graphics.Typeface
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.text.Html
import android.view.Gravity
import android.view.ViewGroup
import android.webkit.WebResourceRequest
import android.webkit.WebResourceResponse
import android.webkit.WebSettings
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import androidx.webkit.WebViewAssetLoader

class MainActivity : AppCompatActivity() {
    private lateinit var webView: WebView

    @SuppressLint("SetJavaScriptEnabled")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // Force light theme globally
        AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_NO)

        val root = FrameLayout(this)
        root.layoutParams = ViewGroup.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.MATCH_PARENT
        )
        root.setBackgroundColor(Color.WHITE)

        webView = WebView(this)
        webView.setBackgroundColor(Color.TRANSPARENT)
        root.addView(webView, FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT,
            FrameLayout.LayoutParams.MATCH_PARENT
        ))

        val assetLoader = WebViewAssetLoader.Builder()
            .addPathHandler("/assets/", WebViewAssetLoader.AssetsPathHandler(this))
            .build()
        webView.webViewClient = object : WebViewClient() {
            override fun shouldInterceptRequest(
                view: WebView?,
                request: WebResourceRequest?
            ): WebResourceResponse? {
                return assetLoader.shouldInterceptRequest(request?.url!!)
            }
        }
        val settings = webView.settings
        settings.javaScriptEnabled = true
        settings.domStorageEnabled = true
        settings.allowFileAccess = true
        settings.allowContentAccess = true
        settings.mixedContentMode = WebSettings.MIXED_CONTENT_ALWAYS_ALLOW
        settings.textZoom = 100
        settings.useWideViewPort = true
        settings.loadWithOverviewMode = true
        webView.loadUrl("https://appassets.androidplatform.net/assets/index.html")

        // 1:1 Native Splash Screen Overlay
        val splash = LinearLayout(this)
        splash.orientation = LinearLayout.VERTICAL
        splash.gravity = Gravity.CENTER
        splash.setBackgroundColor(Color.WHITE)
        splash.layoutParams = FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT,
            FrameLayout.LayoutParams.MATCH_PARENT
        )

        // Microscope Icon
        val iconView = TextView(this)
        iconView.text = "\uf610" // fa-microscope
        iconView.textSize = 64f
        iconView.setTextColor(Color.parseColor("#00A996"))
        iconView.typeface = Typeface.createFromAsset(assets, "webfonts/fa-solid-900.ttf")
        iconView.gravity = Gravity.CENTER
        
        // Setup Icon Pulse Animation (same as pulseSplash CSS)
        val pulse = ObjectAnimator.ofPropertyValuesHolder(
            iconView,
            PropertyValuesHolder.ofFloat("scaleX", 1f, 1.1f, 1f),
            PropertyValuesHolder.ofFloat("scaleY", 1f, 1.1f, 1f)
        )
        pulse.duration = 2000
        pulse.repeatCount = ValueAnimator.INFINITE
        pulse.start()

        // Title
        val titleView = TextView(this)
        titleView.text = Html.fromHtml("<font color='#00A996'>Venom</font><font color='#0f172a'>Sense</font>", Html.FROM_HTML_MODE_LEGACY)
        titleView.textSize = 32f
        titleView.typeface = Typeface.create("sans-serif-black", Typeface.NORMAL)
        titleView.letterSpacing = 0.05f
        titleView.gravity = Gravity.CENTER
        
        val titleParams = LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.WRAP_CONTENT,
            LinearLayout.LayoutParams.WRAP_CONTENT
        )
        titleParams.topMargin = (20 * resources.displayMetrics.density).toInt()
        titleView.layoutParams = titleParams

        // Subtitle
        val subtitleView = TextView(this)
        subtitleView.text = "DIAGNOSTIC SYSTEM"
        subtitleView.textSize = 14f
        subtitleView.setTextColor(Color.parseColor("#64748b"))
        subtitleView.setTypeface(null, Typeface.BOLD)
        subtitleView.letterSpacing = 0.15f
        subtitleView.gravity = Gravity.CENTER
        
        val subParams = LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.WRAP_CONTENT,
            LinearLayout.LayoutParams.WRAP_CONTENT
        )
        subParams.topMargin = (12 * resources.displayMetrics.density).toInt()
        subtitleView.layoutParams = subParams

        splash.addView(iconView)
        splash.addView(titleView)
        splash.addView(subtitleView)

        root.addView(splash)
        setContentView(root)

        // Hide splash after 1800ms with 600ms fade (matches CSS transition)
        Handler(Looper.getMainLooper()).postDelayed({
            splash.animate()
                .alpha(0f)
                .setDuration(600)
                .withEndAction { root.removeView(splash) }
                .start()
        }, 1800)
    }

    override fun onBackPressed() {
        if (webView.canGoBack()) {
            webView.goBack()
        } else {
            super.onBackPressed()
        }
    }
}
