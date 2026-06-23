import sys
import os
import time
import json
import websocket
import config

print("=== Starting WebSocket Diagnostic (ZCloudWebTools Specs) ===")
print("WS URL:", config.CLOUD_WS_URL)
print("UID:", config.CLOUD_DEVICE_ID)
print("KEY:", config.CLOUD_API_KEY)

def on_message(ws, message):
    try:
        clean_msg = message.replace('\ufffd', '?')
        print("[MSG] Received:", clean_msg)
    except Exception:
        print("[MSG] Received (Encoding Error in print)")

def on_error(ws, error):
    print("[ERR] Error occurred:", error)

def on_close(ws, close_code, close_msg):
    print(f"[CLOSE] Closed: code={close_code}, msg={close_msg}")

def on_open(ws):
    print("[CONN] WebSocket Connected successfully!")
    reg_payload = {
        "method": "authenticate",
        "uid": config.CLOUD_DEVICE_ID,
        "key": config.CLOUD_API_KEY
    }
    print("[SEND] Sending authentication payload:", reg_payload)
    ws.send(json.dumps(reg_payload))
    
    # Wait 2 seconds, then send a control command in CUSTOM string format
    time.sleep(2)
    ctrl_payload = {
        "method": "control",
        "addr": "00:12:4B:00:1C:45:BB:54",
        "data": "{unlock=1,rgb=[0,255,0]}" # Match actual custom string format used in ZCloudWebTools
    }
    print("[SEND] Sending test control payload:", ctrl_payload)
    ws.send(json.dumps(ctrl_payload))

ws = websocket.WebSocketApp(
    config.CLOUD_WS_URL,
    on_open=on_open,
    on_message=on_message,
    on_error=on_error,
    on_close=on_close
)

try:
    ws.run_forever(ping_interval=10)
except Exception as e:
    print("[EXCEPTION] ws.run_forever threw:", e)
print("=== Diagnostic Finished ===")
