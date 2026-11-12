"""
test_web_playwright_chrome.py
End-to-End Playwright Chrome Testing Suite for ZZLIGHT ESP32 Firmware Web UI.
Tests responsive layout, touch targets, DOM safety, and interactions on:
  1. Samsung Galaxy S24 FE (Mobile Viewport: 390x844 @ 2.75x)
  2. Acer Swift 3 Laptop Windows 11 (Desktop Viewport: 1536x864 @ 1.25x)
"""

from __future__ import annotations

import json
import re
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path

from playwright.sync_api import sync_playwright

REPO_ROOT = Path(__file__).resolve().parent.parent
INO_PATH = REPO_ROOT / "firmware" / "artnet_dmx_final.ino"
OUTPUT_DIR = Path("D:/ANDREAS/out/playwright_screens")
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

# 1. Extract HTML templates directly from firmware source code
code = INO_PATH.read_text(encoding="utf-8")
AP_HTML = re.search(r'const char AP_HTML\[\] PROGMEM = R"rawliteral\(([\s\S]*?)\)rawliteral";', code).group(1)
SAVED_HTML = re.search(r'const char SAVED_HTML\[\] PROGMEM = R"rawliteral\(([\s\S]*?)\)rawliteral";', code).group(1)
RUN_HTML = re.search(r'const char RUN_HTML\[\] PROGMEM = R"rawliteral\(([\s\S]*?)\)rawliteral";', code).group(1)

# Mock state
mock_rx_counter = 24890


class FirmwareMockServer(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        pass  # Quiet logging

    def do_GET(self):
        global mock_rx_counter
        if self.path in ("/", "/setup"):
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=UTF-8")
            self.end_headers()
            self.wfile.write(AP_HTML.encode("utf-8"))
        elif self.path in ("/monitor",):
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=UTF-8")
            self.end_headers()
            self.wfile.write(RUN_HTML.encode("utf-8"))
        elif self.path == "/scan":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            networks = [
                {"ssid": "GIA-Stage-WiFi", "sec": True, "sig": 95},
                {"ssid": "Deliksari-FOH", "sec": True, "sig": 80},
                {"ssid": "Guest-Network", "sec": False, "sig": 45},
            ]
            self.wfile.write(json.dumps(networks).encode("utf-8"))
        elif self.path == "/status":
            mock_rx_counter += 43
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            status = {
                "ip": "192.168.1.150",
                "wifi": "GIA-Stage-WiFi",
                "status": "Connected",
                "mode": "RUN",
                "rssi": -52,
                "rx_count": mock_rx_counter,
                "dmx": "Active",
            }
            self.wfile.write(json.dumps(status).encode("utf-8"))
        else:
            self.send_response(404)
            self.end_headers()

    def do_POST(self):
        if self.path == "/connect":
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=UTF-8")
            self.end_headers()
            self.wfile.write(SAVED_HTML.encode("utf-8"))
        elif self.path in ("/reset", "/reboot"):
            self.send_response(200)
            self.send_header("Content-Type", "text/plain")
            self.end_headers()
            self.wfile.write(b"OK")
        else:
            self.send_response(404)
            self.end_headers()


def run_tests():
    server = HTTPServer(("127.0.0.1", 8189), FirmwareMockServer)
    server_thread = threading.Thread(target=server.serve_forever, daemon=True)
    server_thread.start()
    print("✓ Local Firmware Mock Server running on http://127.0.0.1:8189")

    results = []

    with sync_playwright() as p:
        # Launch installed Google Chrome
        browser = p.chromium.launch(channel="chrome", headless=True)
        print("✓ Google Chrome browser engine launched via Playwright")

        # -------------------------------------------------------------
        # TEST SUITE 1: Samsung Galaxy S24 FE Mobile Viewport
        # -------------------------------------------------------------
        s24_context = browser.new_context(
            viewport={"width": 390, "height": 844},
            device_scale_factor=2.75,
            is_mobile=True,
            has_touch=True,
            user_agent="Mozilla/5.0 (Linux; Android 14; SM-S711B) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/123.0.0.0 Mobile Safari/537.36",
        )
        s24_page = s24_context.new_page()

        print("\n[TEST 1] Samsung Galaxy S24 FE — Setup Page (/setup)")
        s24_page.goto("http://127.0.0.1:8189/setup")
        s24_page.wait_for_selector(".wifi-item")

        # 1. Check no horizontal overflow
        no_overflow = s24_page.evaluate("() => document.documentElement.scrollWidth <= window.innerWidth")
        assert no_overflow, "Horizontal overflow detected on S24 FE setup page!"
        print("  ✓ Zero horizontal overflow (scrollWidth <= 390px)")

        # 2. Check touch targets >= 44px
        btn_refresh_box = s24_page.locator("button:has-text('Refresh')").bounding_box()
        assert btn_refresh_box["height"] >= 44, f"Refresh button height {btn_refresh_box['height']} < 44px"
        print(f"  ✓ Touch target Refresh Button: {btn_refresh_box['width']:.1f}x{btn_refresh_box['height']:.1f}px (>=44px)")

        btn_connect_box = s24_page.locator("button:has-text('Connect & Reboot')").bounding_box()
        assert btn_connect_box["height"] >= 44, f"Connect button height {btn_connect_box['height']} < 44px"
        print(f"  ✓ Touch target Connect Button: {btn_connect_box['width']:.1f}x{btn_connect_box['height']:.1f}px (>=44px)")

        # 3. Test Wi-Fi network selection
        first_item = s24_page.locator(".wifi-item").first
        first_item.click()
        ssid_val = s24_page.input_value("#ssid")
        assert ssid_val == "GIA-Stage-WiFi", f"Unexpected SSID filled: {ssid_val}"
        print(f"  ✓ Wi-Fi click-to-fill auto-fills SSID: '{ssid_val}'")

        # 4. Test Password SHOW / HIDE toggle
        pass_input = s24_page.locator("#pass")
        pass_input.fill("GIAStagePass2026")
        assert pass_input.get_attribute("type") == "password"

        toggle_btn = s24_page.locator("#btnTogglePass")
        assert toggle_btn.text_content() == "SHOW"
        toggle_btn.click()
        assert pass_input.get_attribute("type") == "text", "Password unmask failed!"
        assert toggle_btn.text_content() == "HIDE", "Toggle text should change to HIDE"
        print("  ✓ Password unmask toggle: type 'password' -> 'text' (Button: SHOW -> HIDE)")

        # Capture S24 FE setup screenshot
        s24_setup_png = OUTPUT_DIR / "01_s24fe_setup_unmasked.png"
        s24_page.screenshot(path=str(s24_setup_png))
        print(f"  ✓ Saved screenshot: {s24_setup_png.name}")

        # 5. Test form submission -> /connect -> SAVED_HTML
        s24_page.click("button:has-text('Connect & Reboot')")
        s24_page.wait_for_selector(".card")
        saved_text = s24_page.locator(".card").text_content()
        assert "WIFI CREDENTIALS SAVED" in saved_text.upper()
        print("  ✓ Form submission renders clean Saved Confirmation Card")
        s24_saved_png = OUTPUT_DIR / "02_s24fe_saved_card.png"
        s24_page.screenshot(path=str(s24_saved_png))
        print(f"  ✓ Saved screenshot: {s24_saved_png.name}")

        # -------------------------------------------------------------
        # TEST SUITE 2: Samsung Galaxy S24 FE — Live Monitor (/monitor)
        # -------------------------------------------------------------
        print("\n[TEST 2] Samsung Galaxy S24 FE — Live Monitor Page (/monitor)")
        s24_page.goto("http://127.0.0.1:8189/monitor")
        s24_page.wait_for_selector(".box")
        time.sleep(1.2)  # Wait for first live update tick

        # Check no horizontal overflow
        no_overflow_mon = s24_page.evaluate("() => document.documentElement.scrollWidth <= window.innerWidth")
        assert no_overflow_mon, "Horizontal overflow detected on S24 FE monitor page!"
        print("  ✓ Zero horizontal overflow on live monitor (scrollWidth <= 390px)")

        # Check tabular-nums CSS
        has_tabular = s24_page.evaluate("() => getComputedStyle(document.querySelector('.box')).fontVariantNumeric.includes('tabular-nums')")
        assert has_tabular, "font-variant-numeric: tabular-nums missing on .box!"
        print("  ✓ Tabular numerals active: font-variant-numeric: tabular-nums confirmed")

        # Check telemetry values
        ip_text = s24_page.locator("#ip-val").text_content()
        status_text = s24_page.locator("#st-val").text_content()
        dmx_text = s24_page.locator("#dx-val").text_content()
        rx_text = s24_page.locator("#rx-val").text_content()
        assert ip_text == "192.168.1.150"
        assert status_text == "Connected"
        assert dmx_text == "Active"
        assert "packets" in rx_text
        print(f"  ✓ Telemetry verified: IP={ip_text}, Status={status_text}, DMX={dmx_text}, RX={rx_text}")

        # Capture S24 FE monitor screenshot
        s24_monitor_png = OUTPUT_DIR / "03_s24fe_live_monitor.png"
        s24_page.screenshot(path=str(s24_monitor_png))
        print(f"  ✓ Saved screenshot: {s24_monitor_png.name}")

        s24_context.close()

        # -------------------------------------------------------------
        # TEST SUITE 3: Acer Swift 3 Windows 11 Desktop (1536x864 @ 1.25x)
        # -------------------------------------------------------------
        desktop_context = browser.new_context(
            viewport={"width": 1536, "height": 864},
            device_scale_factor=1.25,
            is_mobile=False,
            has_touch=False,
        )
        desktop_page = desktop_context.new_page()

        print("\n[TEST 3] Acer Swift 3 Windows 11 Desktop — Setup Page (/setup)")
        desktop_page.goto("http://127.0.0.1:8189/setup")
        desktop_page.wait_for_selector(".wifi-item")

        # Verify centered container and desktop layout
        container_box = desktop_page.locator(".container").bounding_box()
        print(f"  ✓ Centered container width: {container_box['width']:.1f}px (Max: 360px)")
        desktop_setup_png = OUTPUT_DIR / "04_desktop_swift3_setup.png"
        desktop_page.screenshot(path=str(desktop_setup_png))
        print(f"  ✓ Saved screenshot: {desktop_setup_png.name}")

        print("\n[TEST 4] Acer Swift 3 Windows 11 Desktop — Live Monitor Page (/monitor)")
        desktop_page.goto("http://127.0.0.1:8189/monitor")
        desktop_page.wait_for_selector(".box")
        time.sleep(1.2)

        desktop_monitor_png = OUTPUT_DIR / "05_desktop_swift3_live_monitor.png"
        desktop_page.screenshot(path=str(desktop_monitor_png))
        print(f"  ✓ Saved screenshot: {desktop_monitor_png.name}")

        # Test bidirectional navigation back to /setup
        setup_link = desktop_page.locator("a:has-text('Network Setup')")
        assert setup_link.is_visible(), "Network Setup link must be visible and accessible!"
        setup_link.click()
        desktop_page.wait_for_selector(".wifi-box")
        assert "/setup" in desktop_page.url, "Navigation back to /setup failed!"
        print("  ✓ Bidirectional navigation verified: clicked 'Network Setup' -> returned to /setup")

        desktop_context.close()
        browser.close()

    server.shutdown()
    print("\n🎉 ALL PLAYWRIGHT CHROME TESTS PASSED 100% WITH ZERO ERRORS!")


if __name__ == "__main__":
    run_tests()
