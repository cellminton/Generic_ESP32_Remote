# Example Python client for ESP32 Pin Controller

import socket
import json
import time
from typing import Dict, Any, Optional


class ESP32Controller:
    """
    Python client for controlling ESP32 pins remotely.
    Supports both TCP and UDP protocols.
    """

    def __init__(self, host: str, tcp_port: int = 8888, udp_port: int = 8889):
        """
        Initialize the ESP32 controller client.

        Args:
            host: IP address of the ESP32 device
            tcp_port: TCP server port (default: 8888)
            udp_port: UDP server port (default: 8889)
        """
        self.host = host
        self.tcp_port = tcp_port
        self.udp_port = udp_port
        self.tcp_socket = None
        self.tcp_file = None  # File-like interface for line reading

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
                if line:
                    print(f"Connected: {line.strip()}")

            return True

        except Exception as e:
            print(f"TCP connection failed: {e}")
            return False

    def disconnect_tcp(self):
        """Close TCP connection."""
        if self.tcp_file:
            self.tcp_file.close()
            self.tcp_file = None
        if self.tcp_socket:
            self.tcp_socket.close()
            self.tcp_socket = None

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

            # Read response line by line until we get a JSON response
            while True:
                line = self.tcp_file.readline()
                if not line:
                    raise Exception("Connection closed")

                line = line.strip()

                # Skip empty lines and non-JSON lines
                if line and line.startswith('{'):
                    return json.loads(line)

        except json.JSONDecodeError as e:
            print(f"JSON parse error: {e}")
            print(f"Received: {line}")
            self.disconnect_tcp()
            return None
        except Exception as e:
            print(f"TCP send error: {e}")
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
            print(f"UDP send error: {e}")
            return None

    # Convenience methods

    def set_pin(self, pin: int, value: int, use_tcp: bool = True) -> bool:
        """Set pin to HIGH (1) or LOW (0)."""
        command = {"cmd": "SET", "pin": pin, "value": value}
        response = self.send_tcp(
            command) if use_tcp else self.send_udp(command)
        return response and response.get("success", False)

    def get_pin(self, pin: int, use_tcp: bool = True) -> Optional[int]:
        """Get current pin state."""
        command = {"cmd": "GET", "pin": pin}
        response = self.send_tcp(
            command) if use_tcp else self.send_udp(command)
        if response and response.get("success"):
            return response.get("value")
        return None

    def toggle_pin(self, pin: int, use_tcp: bool = True) -> bool:
        """Toggle pin state."""
        command = {"cmd": "TOGGLE", "pin": pin}
        response = self.send_tcp(
            command) if use_tcp else self.send_udp(command)
        return response and response.get("success", False)

    def set_pwm(self, pin: int, value: int, use_tcp: bool = True) -> bool:
        """Set PWM value (0-255)."""
        command = {"cmd": "PWM", "pin": pin, "value": value}
        response = self.send_tcp(
            command) if use_tcp else self.send_udp(command)
        return response and response.get("success", False)

    def get_status(self, use_tcp: bool = True) -> Optional[Dict[str, Any]]:
        """Get system status."""
        command = {"cmd": "STATUS"}
        return self.send_tcp(command) if use_tcp else self.send_udp(command)

    def reset(self, use_tcp: bool = True) -> bool:
        """Restart the ESP32."""
        command = {"cmd": "RESET"}
        response = self.send_tcp(
            command) if use_tcp else self.send_udp(command)
        return response and response.get("success", False)


# Example usage
if __name__ == "__main__":
    # Create controller instance
    esp = ESP32Controller("192.168.137.154")

    print("=== ESP32 Pin Controller Demo ===\n")

    # Connect via TCP
    if esp.connect_tcp():
        print("✓ Connected to ESP32\n")

        # Get system status
        print("1. Getting system status...")
        status = esp.get_status()
        if status:
            print(f"   Uptime: {status['system']['uptime']} seconds")
            print(f"   WiFi: {status['wifi']['ssid']}")
            print(f"   IP: {status['wifi']['ip']}\n")

        # Set pin HIGH
        print("2. Setting pin 13 HIGH...")
        if esp.toggle_pin(13):
            print("   ✓ Success\n")

        time.sleep(1)

        # Get pin state
        print("3. Reading pin 13 state...")
        state = esp.get_pin(13)
        if state is not None:
            print(f"   Pin 13 = {state}\n")

        time.sleep(1)

        # PWM example
        print("5. Setting PWM on pin 25...")
        if esp.set_pwm(25, 128):
            print("   ✓ PWM set to 128 (50%)\n")

        # Cleanup
        esp.disconnect_tcp()
        print("Disconnected")
    else:
        print("✗ Failed to connect to ESP32")
        print("Make sure the device is powered on and connected to WiFi")
