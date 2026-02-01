import React, { useEffect, useState } from "react";
import {
  Button,
  Platform,
  SafeAreaView,
  StatusBar,
  StyleSheet,
  Text,
  View,
} from "react-native";
import { BleManager, Device } from "react-native-ble-plx";
import { PermissionsAndroid } from "react-native";
import base64 from "base-64";

const SERVICE_UUID = "0000FFFF-0000-1000-8000-00805F9B34FB";
const CMD_CHAR_UUID = "0000FF01-0000-1000-8000-00805F9B34FB";

const ble = new BleManager();

async function requestBlePermissions() {
  if (Platform.OS !== "android") return;

  const apiLevel = Number(Platform.Version);
  const perms: string[] = [];

  // Android 12+ needs new BT permissions.
  if (apiLevel >= 31) {
    perms.push(
      "android.permission.BLUETOOTH_SCAN",
      "android.permission.BLUETOOTH_CONNECT",
      PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION
    );
  } else {
    perms.push(
      PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
      "android.permission.BLUETOOTH"
    );
  }

  await PermissionsAndroid.requestMultiple(perms);
}

export default function App() {
  const [device, setDevice] = useState<Device | null>(null);
  const [connecting, setConnecting] = useState(false);
  const [connected, setConnected] = useState(false);
  const [status, setStatus] = useState("Idle");

  useEffect(() => {
    return () => {
      ble.destroy();
    };
  }, []);

  const connectToRobot = async () => {
    setStatus("Requesting permissions…");
    await requestBlePermissions();

    setConnecting(true);
    setStatus("Scanning for PingPongRobot…");

    ble.startDeviceScan([SERVICE_UUID], null, async (error, scanned) => {
      if (error) {
        console.warn("Scan error:", error);
        setStatus("Scan error");
        setConnecting(false);
        ble.stopDeviceScan();
        return;
      }

      if (!scanned) return;

      // Match by advertised name or service UUID
      if (
        scanned.name === "PingPongRobot" ||
        scanned.serviceUUIDs?.includes(SERVICE_UUID)
      ) {
        ble.stopDeviceScan();
        try {
          setStatus(`Connecting to ${scanned.name ?? scanned.id}…`);
          const conn = await scanned.connect();
          const dev = await conn.discoverAllServicesAndCharacteristics();
          setDevice(dev);
          setConnected(true);
          setStatus("Connected");
        } catch (e) {
          console.warn("Connect failed:", e);
          setStatus("Connect failed");
        } finally {
          setConnecting(false);
        }
      }
    });
  };

  const sendCommand = async (cmd: any) => {
    if (!device) {
      setStatus("Not connected");
      return;
    }
    try {
      const json = JSON.stringify(cmd);
      const value = base64.encode(json);
      await ble.writeCharacteristicWithResponseForDevice(
        device.id,
        SERVICE_UUID,
        CMD_CHAR_UUID,
        value
      );
      setStatus(`Sent: ${json}`);
    } catch (e) {
      console.warn("Write failed:", e);
      setStatus("Write failed");
    }
  };

  return (
    <SafeAreaView style={styles.safe}>
      <StatusBar barStyle="dark-content" />
      <View style={styles.container}>
        <Text style={styles.title}>Ping Pong Robot Controller</Text>
        <Text style={styles.status}>Status: {status}</Text>

        {!connected ? (
          <Button
            title={connecting ? "Connecting…" : "Connect to robot"}
            onPress={connectToRobot}
            disabled={connecting}
          />
        ) : (
          <View style={styles.buttonGroup}>
            <Button
              title="Serve one ball"
              onPress={() => sendCommand({ type: "SERVE_ONE" })}
            />
            <View style={styles.spacer} />
            <Button
              title="Start continuous"
              onPress={() =>
                sendCommand({ type: "START_CONTINUOUS", intervalMs: 400 })
              }
            />
            <View style={styles.spacer} />
            <Button
              title="Stop continuous"
              onPress={() => sendCommand({ type: "STOP_CONTINUOUS" })}
            />
            <View style={styles.spacer} />
            <Button
              title="Toggle wheels"
              onPress={() => sendCommand({ type: "TOGGLE_DCS" })}
            />
            <View style={styles.spacer} />
            <Button
              title="Toggle stepper"
              onPress={() => sendCommand({ type: "TOGGLE_STEPPER" })}
            />
          </View>
        )}
      </View>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  safe: {
    flex: 1,
    backgroundColor: "#fff",
  },
  container: {
    flex: 1,
    padding: 24,
    alignItems: "center",
    justifyContent: "center",
    backgroundColor: "#fff",
  },
  title: {
    fontSize: 22,
    fontWeight: "600",
    marginBottom: 16,
  },
  status: {
    marginBottom: 16,
  },
  buttonGroup: {
    width: "100%",
    gap: 8,
    marginTop: 8,
  },
  spacer: {
    height: 8,
  },
});
