import streamlit as st
import requests
import pandas as pd
import plotly.graph_objects as go
import time
from datetime import datetime

# --- Change this to YOUR ESP32 IP address ---
ESP32_IP  = "10.27.135.21"
ESP32_URL = f"http://10.27.135.21/data"

st.set_page_config(page_title="Medical Telemetry", page_icon="🏥", layout="wide")

if "history" not in st.session_state:
    st.session_state.history = pd.DataFrame(
        columns=["time","hr","spo2","temp","systolic","diastolic","rr"]
    )

def fetch():
    try:
        r = requests.get(ESP32_URL, timeout=3)
        if r.status_code == 200:
            return r.json(), True
    except:
        pass
    return None, False

data, connected = fetch()

st.title("🏥 Medical Device Telemetry Platform")
st.caption("Arduino UNO  →  ESP32 DevKit V1  →  Streamlit Dashboard")

tab1, tab2, tab3, tab4, tab5 = st.tabs([
    "📡 System Status",
    "💓 Current Vitals",
    "📊 Analytics",
    "🚨 Alerts",
    "📈 Graphs"
])

with tab1:
    st.subheader("Connection Status")
    c1, c2, c3 = st.columns(3)
    with c1:
        if connected:
            st.success("✅ ESP32: CONNECTED")
        else:
            st.error("❌ ESP32: NOT REACHABLE")
    with c2:
        st.info(f"🌐 ESP32 IP: {ESP32_IP}")
    with c3:
        st.info(f"🕐 Last checked: {datetime.now().strftime('%H:%M:%S')}")
    if data:
        st.subheader("Communication Statistics")
        c1, c2, c3, c4 = st.columns(4)
        c1.metric("Packets Received", data.get("packets_received", 0))
        c2.metric("Packets Rejected", data.get("packets_rejected", 0))
        c3.metric("Packet Loss",      data.get("packet_loss", 0))
        c4.metric("Total Alerts",     data.get("alert_count", 0))

with tab2:
    st.subheader("Live Patient Vitals")
    if data:
        new_row = {
            "time":      datetime.now().strftime("%H:%M:%S"),
            "hr":        data.get("hr", 0),
            "spo2":      data.get("spo2", 0),
            "temp":      data.get("temp", 0),
            "systolic":  data.get("systolic", 0),
            "diastolic": data.get("diastolic", 0),
            "rr":        data.get("rr", 0)
        }
        st.session_state.history = pd.concat([
            st.session_state.history,
            pd.DataFrame([new_row])
        ], ignore_index=True).tail(200)

        hr   = data.get("hr", 0)
        spo2 = data.get("spo2", 0)
        temp = data.get("temp", 0)
        sys  = data.get("systolic", 0)
        dia  = data.get("diastolic", 0)
        rr   = data.get("rr", 0)

        c1, c2, c3 = st.columns(3)
        with c1:
            if 60 <= hr <= 100:
                st.metric("❤️ Heart Rate", f"{hr} BPM", "Normal")
            elif hr > 100:
                st.metric("❤️ Heart Rate", f"{hr} BPM", "⚠️ Too Fast")
            else:
                st.metric("❤️ Heart Rate", f"{hr} BPM", "⚠️ Too Slow")
        with c2:
            if spo2 >= 95:
                st.metric("🫁 SpO2", f"{spo2}%", "Normal")
            else:
                st.metric("🫁 SpO2", f"{spo2}%", "🚨 LOW OXYGEN")
        with c3:
            if temp <= 37.5:
                st.metric("🌡️ Temperature", f"{temp:.1f}°C", "Normal")
            else:
                st.metric("🌡️ Temperature", f"{temp:.1f}°C", "🚨 FEVER")

        c4, c5, c6 = st.columns(3)
        c4.metric("💊 Blood Pressure", f"{sys}/{dia} mmHg")
        c5.metric("🌬️ Breathing Rate", f"{rr} br/min")
        with c6:
            if data.get("status") == "NORM":
                st.success("✅ Status: ALL NORMAL")
            else:
                st.error("🚨 Status: ALERT ACTIVE")
    else:
        st.warning("Cannot reach ESP32. Check IP address and WiFi.")

with tab3:
    st.subheader("Statistical Summary")
    if data:
        c1, c2, c3 = st.columns(3)
        c1.metric("Avg Heart Rate", f"{data.get('avg_hr',0):.1f} BPM")
        c2.metric("Max Heart Rate", f"{data.get('max_hr',0):.0f} BPM")
        c3.metric("Min Heart Rate", f"{data.get('min_hr',0):.0f} BPM")
        c4, c5 = st.columns(2)
        c4.metric("Avg SpO2",        f"{data.get('avg_spo2',0):.1f}%")
        c5.metric("Avg Temperature", f"{data.get('avg_temp',0):.2f}°C")
    else:
        st.warning("No data available.")

with tab4:
    st.subheader("Medical Alert Log")
    if data:
        alerts = data.get("alerts", [])
        if alerts:
            for a in reversed(alerts):
                st.error(f"🚨 {a}")
        else:
            st.success("No alerts recorded yet.")
    else:
        st.warning("No data available.")

with tab5:
    st.subheader("Vital Signs Over Time")
    df = st.session_state.history
    if len(df) >= 2:
        fig1 = go.Figure()
        fig1.add_trace(go.Scatter(x=df["time"], y=df["hr"],
            mode="lines+markers", name="HR", line=dict(color="red", width=2)))
        fig1.add_hline(y=100, line_dash="dash", line_color="orange",
                       annotation_text="Tachycardia limit")
        fig1.add_hline(y=60, line_dash="dash", line_color="blue",
                       annotation_text="Bradycardia limit")
        fig1.update_layout(title="Heart Rate", height=280,
                           xaxis_title="Time", yaxis_title="BPM")
        st.plotly_chart(fig1, use_container_width=True)

        fig2 = go.Figure()
        fig2.add_trace(go.Scatter(x=df["time"], y=df["spo2"],
            mode="lines+markers", name="SpO2", line=dict(color="blue", width=2)))
        fig2.add_hline(y=92, line_dash="dash", line_color="red",
                       annotation_text="Hypoxia limit")
        fig2.update_layout(title="SpO2", height=280,
                           xaxis_title="Time", yaxis_title="%")
        st.plotly_chart(fig2, use_container_width=True)

        fig3 = go.Figure()
        fig3.add_trace(go.Scatter(x=df["time"], y=df["temp"],
            mode="lines+markers", name="Temp", line=dict(color="green", width=2)))
        fig3.add_hline(y=38.0, line_dash="dash", line_color="red",
                       annotation_text="Fever limit")
        fig3.update_layout(title="Temperature", height=280,
                           xaxis_title="Time", yaxis_title="°C")
        st.plotly_chart(fig3, use_container_width=True)

        st.download_button(
            label="⬇️ Download Data as CSV",
            data=df.to_csv(index=False),
            file_name="telemetry_data.csv",
            mime="text/csv"
        )
    else:
        st.info("Waiting for data... need at least 2 readings.")

time.sleep(2)
st.rerun()