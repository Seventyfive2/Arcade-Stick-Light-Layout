package com.example.arcadestickeditor

import android.annotation.SuppressLint
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.net.ConnectivityManager
import android.net.ConnectivityManager.NetworkCallback
import android.net.Network
import android.net.NetworkCapabilities
import android.net.NetworkInfo
import android.net.NetworkRequest
import android.net.NetworkSpecifier
import android.net.wifi.WifiInfo
import android.net.wifi.WifiManager
import android.net.wifi.WifiNetworkSpecifier.Builder
import android.os.Bundle
import android.view.View
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {
    private val arduinoSSID = "SSID"
    private val arduinoPass = "PASSWORD"
    private val webServer = "http://192.168.4.1"

    private val wifiManager: WifiManager by lazy {
        getApplicationContext().getSystemService(WIFI_SERVICE) as WifiManager
    }

    private val connectivityManager: ConnectivityManager by lazy {
        getApplicationContext().getSystemService(CONNECTIVITY_SERVICE) as ConnectivityManager
    }

    private var networkCallback: NetworkCallback = object : NetworkCallback() {
        override fun onAvailable(network: Network) {
            super.onAvailable(network)
            // To make sure that requests don't go over mobile data
            connectivityManager.bindProcessToNetwork(network)
        }

        override fun onUnavailable() {
            // This is to stop the looping request for OnePlus & Xiaomi models
            connectivityManager.bindProcessToNetwork(null)

            // Here you can have a fallback option to show a 'Please connect manually' page with an Intent to the Wifi settings
        }
    }

    private val wifiStateBC = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val action = intent.action

            when (action) {
                WifiManager.WIFI_STATE_CHANGED_ACTION -> {
                    // WiFi state changed, check the new state
                    val wifiState = intent.getIntExtra(WifiManager.EXTRA_WIFI_STATE, -1)
                    when (wifiState) {
                        WifiManager.WIFI_STATE_ENABLED -> {
                            // WiFi is enabled
                            //Toast.makeText(context, "WiFi enabled", Toast.LENGTH_SHORT).show()
                        }

                        WifiManager.WIFI_STATE_DISABLED -> {
                            // WiFi is disabled
                            //Toast.makeText(context, "WiFi disabled", Toast.LENGTH_SHORT).show()
                        }
                    }
                }

                WifiManager.NETWORK_STATE_CHANGED_ACTION -> {
                    // Network connectivity state changed
                    val networkInfo =
                        intent.getParcelableExtra<NetworkInfo>(WifiManager.EXTRA_NETWORK_INFO)
                    if (networkInfo != null && networkInfo.isConnected) {
                        // WiFi is connected
                        if(isConnectedToWiFi(arduinoSSID))
                        {
                            val textView: TextView = findViewById(R.id.text_view_id)
                            textView.text = "Loading webpage"
                            openWebpage()
                        }
                    } else {
                        // WiFi is disconnected
                        //Toast.makeText(context, "WiFi disconnected", Toast.LENGTH_SHORT).show()
                    }
                }
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setContentView(R.layout.activity_main)

        connectToWiFi(arduinoSSID, arduinoPass)
    }

    @SuppressLint("SetJavaScriptEnabled")
    fun openWebpage() {
        val textView: TextView = findViewById(R.id.text_view_id)
        val webView: WebView = findViewById(R.id.web)

        // loading http://www.google.com/ url in the WebView.
        webView.loadUrl(webServer)

        textView.text = "Loading Layout UI"

        // this will enable the javascript.
        webView.settings.javaScriptEnabled = true

        // WebViewClient allows you to handle
        // onPageFinished and override Url loading.
        webView.webViewClient = WebViewClient()

        textView.visibility = View.GONE
    }

    fun connectToWiFi(ssid: String, password: String) {
        val specifier: NetworkSpecifier = Builder()
            .setSsid(ssid)
            .setWpa2Passphrase(password)
            .build()

        val request: NetworkRequest = NetworkRequest.Builder()
            .addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
            .setNetworkSpecifier(specifier)
            .build()

        connectivityManager.requestNetwork(request, networkCallback)
    }

    fun disconnectFromWIFI()
    {
        connectivityManager.unregisterNetworkCallback(networkCallback)
    }

    @Suppress("DEPRECATION")
    fun isConnectedToWiFi(ssid: String): Boolean {
        val wifiInfo: WifiInfo? = wifiManager.connectionInfo

        return wifiInfo != null && wifiInfo.ssid == "\"$ssid\""
    }

    override fun onResume() {
        super.onResume()
        registerReceiver(
            wifiStateBC,
            IntentFilter(WifiManager.WIFI_STATE_CHANGED_ACTION)
        )
        registerReceiver(
            wifiStateBC,
            IntentFilter(WifiManager.NETWORK_STATE_CHANGED_ACTION)
        )

        connectToWiFi(arduinoSSID, arduinoPass)
    }

    override fun onPause() {
        super.onPause()
        unregisterReceiver(wifiStateBC)
    }

    override fun onStop()
    {
        super.onStop()
        //Disconnect from Wifi
        disconnectFromWIFI()
    }
}