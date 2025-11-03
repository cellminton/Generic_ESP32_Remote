# Example Python client for ESP32 Pin Controller

import socket
import json
import time
from typing import Dict, Any, Optional, List, Tuple
import struct
import os
from pathlib import Path


class ESP32Controller:
    """
    Python client for controlling ESP32 pins remotely.
    Supports both TCP and UDP protocols with automatic device discovery.
    """

    # Cache file for last known IP
    _CACHE_FILE = Path.home() / ".esp32_last_ip"

    def __init__(self, host: Optional[str] = None, tcp_port: int = 8888, udp_port: int = 8889,
                 auto_discover: bool = False, verbose: bool = True):
        """
        Initialize the ESP32 controller client.

        Args:
            host: IP address of the ESP32 device (None for auto-discovery)
            tcp_port: TCP server port (default: 8888)
            udp_port: UDP server port (default: 8889)
            auto_discover: Automatically discover device if host is None
            verbose: Print status messages (default: True)
        """
        self.tcp_port = tcp_port
        self.udp_port = udp_port
        self.tcp_socket = None
        self.tcp_file = None  # File-like interface for line reading
        self.verbose = verbose

        # Auto-discover if no host specified
        if host is None and auto_discover:
            host = self.find_esp32(udp_port=udp_port)
            if host is None:
                raise ConnectionError("Could not find ESP32 device on network")

        if host is None:
            raise ValueError(
                "Host must be specified or auto_discover must be True")

        self.host = host
        # Save the IP for future use
        ESP32Controller._save_ip(self.host)
        if self.verbose:
            print(f"\nUsing ESP32 at {self.host}")

    @staticmethod
    def _save_ip(ip: str):
        """Save the last successful IP address to cache file."""
        try:
            with open(ESP32Controller._CACHE_FILE, 'w') as f:
                f.write(ip)
        except Exception:
            pass  # Silently fail if we can't save

    @staticmethod
    def _load_cached_ip() -> Optional[str]:
        """Load the last successful IP address from cache file."""
        try:
            if ESP32Controller._CACHE_FILE.exists():
                with open(ESP32Controller._CACHE_FILE, 'r') as f:
                    ip = f.read().strip()
                    if ip:
                        return ip
        except Exception:
            pass
        return None

    # reset all pins to low
    def reset_pins(self):
        """Reset all pins to LOW (0)."""
        print("Resetting all pins to LOW...")
        if self.verbose:
            print("Resetting all pins to LOW...")
        command = {"cmd": "RESET_PINS"}
        response = self.send_tcp(command)
        success = response and response.get("success", False)
        if self.verbose:
            if success:
                print("  ✓ All pins reset to LOW")
            else:
                print("  ✗ Failed to reset pins")
        return success

    @staticmethod
    def _test_ip(ip: str, udp_port: int = 8889, timeout: float = 1.0) -> bool:
        """
        Test if an IP address responds to STATUS command.

        Args:
            ip: IP address to test
            udp_port: UDP port to test
            timeout: Timeout in seconds

        Returns:
            True if device responds, False otherwise
        """
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.settimeout(timeout)

            # Send STATUS command
            status_cmd = json.dumps({"cmd": "STATUS"})
            sock.sendto(status_cmd.encode(), (ip, udp_port))

            # Wait for response
            data, addr = sock.recvfrom(4096)
            sock.close()

            # Parse response
            response = json.loads(data.decode())
            return response.get("success", False)

        except Exception:
            return False

    @staticmethod
    def discover_devices(timeout: float = 3.0, udp_port: int = 8889, verbose: bool = True) -> List[Tuple[str, Dict[str, Any]]]:
        """
        Discover ESP32 devices on the local network using UDP broadcast.

        Args:
            timeout: Discovery timeout in seconds
            udp_port: UDP port to scan (default: 8889)
            verbose: Print discovery progress (default: True)

        Returns:
            List of tuples (ip_address, status_info)
        """
        devices = []

        try:
            # Create UDP socket for broadcast
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            sock.settimeout(0.5)

            # Broadcast STATUS command
            status_cmd = json.dumps({"cmd": "STATUS"})
            broadcast_addr = ('<broadcast>', udp_port)

            if verbose:
                print(f"Scanning for ESP32 devices on UDP port {udp_port}...")

            # Send multiple broadcasts to increase chance of discovery
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

                    # Avoid duplicates
                    if ip in discovered_ips:
                        continue

                    discovered_ips.add(ip)

                    # Parse response
                    response = json.loads(data.decode())
                    if response.get("success"):
                        devices.append((ip, response))
                        if verbose:
                            print(f"  Found ESP32 at {ip}")

                except socket.timeout:
                    continue
                except json.JSONDecodeError:
                    continue
                except Exception:
                    continue

            sock.close()

        except Exception as e:
            if verbose:
                print(f"Discovery error: {e}")

        return devices

    @staticmethod
    def discover_mdns(hostname: str = "esp32.local", timeout: float = 2.0, verbose: bool = True) -> str:
        """
        Try to resolve ESP32 using mDNS.

        Args:
            hostname: mDNS hostname (default: "esp32.local")
            timeout: Resolution timeout in seconds
            verbose: Print resolution status (default: True)

        Returns:
            IP address if found, None otherwise
        """
        try:
            # Try to resolve mDNS hostname
            socket.setdefaulttimeout(timeout)
            ip = socket.gethostbyname(hostname)
            if verbose:
                print(f"Resolved {hostname} to {ip}")
            return ip
        except socket.gaierror:
            return None
        except Exception as e:
            if verbose:
                print(f"mDNS resolution error: {e}")
            return None
        finally:
            socket.setdefaulttimeout(None)

    @staticmethod
    def find_esp32(mdns_hostname: str = "esp32-controller.local",
                   udp_port: int = 8889,
                   scan_timeout: float = 3.0,
                   use_cache: bool = True,
                   verbose: bool = True) -> Optional[str]:
        """
        Automatically find ESP32 device on the network.
        Tries cached IP first, then mDNS, then falls back to UDP broadcast scan.

        Args:
            mdns_hostname: mDNS hostname to try
            udp_port: UDP port for broadcast scan
            scan_timeout: Timeout for broadcast scan
            use_cache: Whether to try cached IP first (default: True)
            verbose: Print search progress (default: True)

        Returns:
            IP address of ESP32 or None if not found
        """
        if verbose:
            print("=== Searching for ESP32 device ===")

        # Try cached IP first (fastest)
        if use_cache:
            cached_ip = ESP32Controller._load_cached_ip()
            if cached_ip:
                if verbose:
                    print(f"\n1. Trying cached IP ({cached_ip})...")
                if ESP32Controller._test_ip(cached_ip, udp_port=udp_port):
                    if verbose:
                        print("   ✓ Cached IP still valid!")
                    return cached_ip
                if verbose:
                    print("   ✗ Cached IP no longer responds")

        # Try mDNS (faster if available)
        if verbose:
            print(f"\n2. Trying mDNS resolution ({mdns_hostname})...")
        ip = ESP32Controller.discover_mdns(mdns_hostname, verbose=verbose)
        if ip:
            ESP32Controller._save_ip(ip)
            return ip
        if verbose:
            print("   mDNS not available or device not found")

        # Fall back to UDP broadcast scan
        if verbose:
            print(f"\n3. Performing UDP broadcast scan...")
        devices = ESP32Controller.discover_devices(
            timeout=scan_timeout, udp_port=udp_port, verbose=verbose)

        if devices:
            if verbose:
                print(f"\nFound {len(devices)} device(s)")
            ip = devices[0][0]  # Use first device found
            ESP32Controller._save_ip(ip)
            return ip

        if verbose:
            print("\nNo ESP32 devices found on the network")
        return None

    def connect_tcp(self) -> bool:
        """Establish TCP connection to ESP32."""
        try:
            self.tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.tcp_socket.settimeout(5.0)
            self.tcp_socket.connect((self.host, self.tcp_port))

            # Create file-like interface for line reading
            self.tcp_file = self.tcp_socket.makefile('r')

            # Read and discard welcome messages (non-JSON lines)
            for _ in range(2):  # Expect 2 welcome lines
                line = self.tcp_file.readline()
                if line and self.verbose:
                    print(f"Connected: {line.strip()}")

            if self.verbose:
                print(
                    f"✓ TCP connection established to {self.host}:{self.tcp_port}")
            return True

        except Exception as e:
            if self.verbose:
                print(f"✗ TCP connection failed: {e}")
            return False

    def disconnect_tcp(self):
        """Close TCP connection."""
        if self.tcp_file:
            self.tcp_file.close()
            self.tcp_file = None
        if self.tcp_socket:
            self.tcp_socket.close()
            self.tcp_socket = None
        if self.verbose:
            print("TCP connection closed")

    def send_tcp(self, command: Dict[str, Any]) -> Optional[Dict[str, Any]]:
        """
        Send command via TCP and get response.

        Args:
            command: Command dictionary

        Returns:
            Response dictionary or None on error
        """
        if not self.tcp_socket:
            if not self.connect_tcp():
                return None

        try:
            # Send command
            cmd_str = json.dumps(command) + '\n'
            self.tcp_socket.send(cmd_str.encode())

            # Read response - accumulate lines until we have valid JSON
            json_buffer = ""
            brace_count = 0
            in_json = False

            while True:
                line = self.tcp_file.readline()
                if not line:
                    raise Exception("Connection closed")

                line = line.strip()

                # Skip empty lines before JSON starts
                if not line:
                    if not in_json:
                        continue
                    else:
                        # Empty line after JSON started might mean end
                        if brace_count == 0 and json_buffer:
                            break

                # Check if this line starts JSON
                if line.startswith('{'):
                    in_json = True

                # If we're in JSON, accumulate the line
                if in_json:
                    json_buffer += line
                    # Count braces to know when JSON is complete
                    brace_count += line.count('{') - line.count('}')

                    # When braces balance, we have complete JSON
                    if brace_count == 0 and json_buffer:
                        return json.loads(json_buffer)

        except json.JSONDecodeError as e:
            if self.verbose:
                print(f"✗ JSON parse error: {e}")
                print(f"Received: {json_buffer[:200]}")  # Show first 200 chars
            self.disconnect_tcp()
            return None
        except Exception as e:
            if self.verbose:
                print(f"✗ TCP send error: {e}")
            self.disconnect_tcp()
            return None

    def send_udp(self, command: Dict[str, Any]) -> Optional[Dict[str, Any]]:
        """
        Send command via UDP and get response.

        Args:
            command: Command dictionary

        Returns:
            Response dictionary or None on error
        """
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.settimeout(2.0)

            # Send command
            cmd_str = json.dumps(command)
            sock.sendto(cmd_str.encode(), (self.host, self.udp_port))

            # Receive response
            response, addr = sock.recvfrom(4096)
            sock.close()

            return json.loads(response.decode())

        except Exception as e:
            if self.verbose:
                print(f"✗ UDP send error: {e}")
            return None

    # Convenience methods

    def set_pin(self, pin: int, value: int, use_tcp: bool = True) -> bool:
        """Set pin to HIGH (1) or LOW (0)."""
        if self.verbose:
            print(f"Setting pin {pin} to {'HIGH' if value else 'LOW'}...")
        command = {"cmd": "SET", "pin": pin, "value": value}
        response = self.send_tcp(
            command) if use_tcp else self.send_udp(command)
        success = response and response.get("success", False)
        if self.verbose:
            if success:
                print(f"  ✓ Pin {pin} set to {value}")
            else:
                print(f"  ✗ Failed to set pin {pin}")
        return success

    def get_pin(self, pin: int, use_tcp: bool = True) -> Optional[int]:
        """Get current pin state."""
        if self.verbose:
            print(f"Reading pin {pin}...")
        command = {"cmd": "GET", "pin": pin}
        response = self.send_tcp(
            command) if use_tcp else self.send_udp(command)
        if response and response.get("success"):
            value = response.get("value")
            if self.verbose:
                print(f"  Pin {pin} = {value} ({'HIGH' if value else 'LOW'})")
            return value
        if self.verbose:
            print(f"  ✗ Failed to read pin {pin}")
        return None

    def toggle_pin(self, pin: int, use_tcp: bool = True) -> bool:
        """Toggle pin state."""
        if self.verbose:
            print(f"Toggling pin {pin}...")
        command = {"cmd": "TOGGLE", "pin": pin}
        response = self.send_tcp(
            command) if use_tcp else self.send_udp(command)
        success = response and response.get("success", False)
        if self.verbose:
            if success:
                # Read back the new state
                new_state = response.get("value", "unknown")
                print(f"  ✓ Pin {pin} toggled to {new_state}")
            else:
                print(f"  ✗ Failed to toggle pin {pin}")
        return success

    def set_pwm(self, pin: int, value: int, use_tcp: bool = True) -> bool:
        """Set PWM value (0-255)."""
        if self.verbose:
            print(f"Setting pin {pin} PWM to {value}...")
        command = {"cmd": "PWM", "pin": pin, "value": value}
        response = self.send_tcp(
            command) if use_tcp else self.send_udp(command)
        success = response and response.get("success", False)
        if self.verbose:
            if success:
                print(f"  ✓ PWM set to {value}")
            else:
                print(f"  ✗ Failed to set PWM")
        return success

    def get_status(self, use_tcp: bool = True) -> Optional[Dict[str, Any]]:
        """Get system status."""
        if self.verbose:
            print("Getting system status...")
        command = {"cmd": "STATUS"}
        response = self.send_tcp(
            command) if use_tcp else self.send_udp(command)
        if self.verbose:
            if response:
                print("  ✓ Status received")
                # Debug: show what we got
                if not response.get("success"):
                    print(f"  ⚠ Response marked as unsuccessful: {response}")
            else:
                print("  ✗ No response received")
        return response

    def reset(self, use_tcp: bool = True) -> bool:
        """Restart the ESP32."""
        if self.verbose:
            print("Sending restart command to ESP32...")
        command = {"cmd": "RESET"}
        response = self.send_tcp(
            command) if use_tcp else self.send_udp(command)
        success = response and response.get("success", False)
        if self.verbose and success:
            print("  ✓ Restart command sent")
        return success
