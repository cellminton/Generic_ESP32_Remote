#!/usr/bin/env python3
"""
Simple ESP32 discovery tool
Scans the network for ESP32 devices and displays their information.
"""

import socket
import json
import time
from typing import List, Tuple, Dict, Any, Optional
from pathlib import Path


# Cache file for last known IP
CACHE_FILE = Path.home() / ".esp32_last_ip"


def save_ip(ip: str):
    """Save the last successful IP address to cache file."""
    try:
        with open(CACHE_FILE, 'w') as f:
            f.write(ip)
    except Exception:
        pass


def load_cached_ip() -> Optional[str]:
    """Load the last successful IP address from cache file."""
    try:
        if CACHE_FILE.exists():
            with open(CACHE_FILE, 'r') as f:
                ip = f.read().strip()
                if ip:
                    return ip
    except Exception:
        pass
    return None


def test_ip(ip: str, udp_port: int = 8889, timeout: float = 1.0) -> bool:
    """Test if an IP address responds to STATUS command."""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(timeout)

        status_cmd = json.dumps({"cmd": "STATUS"})
        sock.sendto(status_cmd.encode(), (ip, udp_port))

        data, addr = sock.recvfrom(4096)
        sock.close()

        response = json.loads(data.decode())
        return response.get("success", False)

    except Exception:
        return False


def discover_mdns(hostname: str = "esp32-controller.local", timeout: float = 2.0) -> str:
    """Try to resolve ESP32 using mDNS."""
    try:
        socket.setdefaulttimeout(timeout)
        ip = socket.gethostbyname(hostname)
        return ip
    except:
        return None
    finally:
        socket.setdefaulttimeout(None)


def discover_udp_broadcast(udp_port: int = 8889, timeout: float = 3.0) -> List[Tuple[str, Dict[str, Any]]]:
    """Discover ESP32 devices using UDP broadcast."""
    devices = []

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        sock.settimeout(0.5)

        # Broadcast STATUS command
        status_cmd = json.dumps({"cmd": "STATUS"})
        broadcast_addr = ('<broadcast>', udp_port)

        # Send multiple broadcasts
        for _ in range(3):
            sock.sendto(status_cmd.encode(), broadcast_addr)
            time.sleep(0.1)

        # Collect responses
        start_time = time.time()
        discovered_ips = set()

        while time.time() - start_time < timeout:
            try:
                data, addr = sock.recvfrom(4096)
                ip = addr[0]

                if ip in discovered_ips:
                    continue

                discovered_ips.add(ip)
                response = json.loads(data.decode())

                if response.get("success"):
                    devices.append((ip, response))

            except socket.timeout:
                continue
            except:
                continue

        sock.close()

    except Exception as e:
        print(f"Discovery error: {e}")

    return devices


def main():
    print("=" * 60)
    print(" ESP32 Device Discovery Tool")
    print("=" * 60)

    # Try cached IP first
    cached_ip = load_cached_ip()
    if cached_ip:
        print(f"\n[1] Trying cached IP ({cached_ip})...")
        if test_ip(cached_ip):
            print(f"    ✓ Cached IP still valid!")
            print(f"\n    Device available at: {cached_ip}")
            print("=" * 60)
            return
        else:
            print("    ✗ Cached IP no longer responds")

    # Try mDNS
    print("\n[2] Trying mDNS resolution (esp32-controller.local)...")
    mdns_ip = discover_mdns()

    if mdns_ip:
        print(f"    ✓ Found via mDNS: {mdns_ip}")
        save_ip(mdns_ip)
    else:
        print("    ✗ mDNS resolution failed")

    # Try UDP broadcast
    print("\n[3] Performing UDP broadcast scan...")
    print("    Listening for responses (3 seconds)...")
    devices = discover_udp_broadcast()

    if devices:
        print(f"\n    ✓ Found {len(devices)} device(s):\n")
        # Save the first device found
        if devices:
            save_ip(devices[0][0])

        for i, (ip, info) in enumerate(devices, 1):
            print(f"    Device #{i}: {ip}")
            print(f"    {'─' * 50}")

            if 'system' in info:
                sys_info = info['system']
                print(
                    f"      Uptime:        {sys_info.get('uptime', 'N/A')} seconds")
                print(
                    f"      Free Heap:     {sys_info.get('freeHeap', 'N/A')} bytes")
                print(
                    f"      Chip Model:    {sys_info.get('chipModel', 'N/A')}")

            if 'wifi' in info:
                wifi_info = info['wifi']
                print(f"      WiFi SSID:     {wifi_info.get('ssid', 'N/A')}")
                print(
                    f"      WiFi RSSI:     {wifi_info.get('rssi', 'N/A')} dBm")
                print(f"      MAC Address:   {wifi_info.get('mac', 'N/A')}")

            print()
    else:
        print("    ✗ No devices found via UDP broadcast")

    # Summary
    print("\n" + "=" * 60)
    if mdns_ip or devices:
        print("Discovery complete! Use one of the following IPs:")
        if mdns_ip:
            print(f"  - {mdns_ip} (via mDNS)")
        for ip, _ in devices:
            print(f"  - {ip} (via UDP)")
    else:
        print("No ESP32 devices found on the network.")
        print("\nTroubleshooting:")
        print("  • Make sure the ESP32 is powered on")
        print("  • Verify the ESP32 is connected to WiFi")
        print("  • Check that your computer is on the same network")
        print("  • Ensure firewall allows UDP broadcast on port 8889")
    print("=" * 60)


if __name__ == "__main__":
    main()
