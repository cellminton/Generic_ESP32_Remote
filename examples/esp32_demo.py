#!/usr/bin/env python3
"""
ESP32 Controller Demo Script
Demonstrates various commands and features of the ESP32 controller.
"""

import time
import sys
from python_client import ESP32Controller


def demo_basic_commands(esp: ESP32Controller):
    """Demonstrate basic pin control commands."""
    print("=" * 60)
    print(" Basic Pin Control Demo")
    print("=" * 60)

    # Get system status
    # print("\n1. Getting system status...")
    # status = esp.get_status()
    # if status:
    #     # Support both camelCase and snake_case for compatibility
    #     system = status.get('system', {})
    #     wifi = status.get('wifi', {})

    #     uptime = system.get('uptime', 0)
    #     free_heap = system.get('freeHeap', system.get('free_heap', 0))
    #     chip_model = system.get(
    #         'chipModel', system.get('chip_model', 'Unknown'))

    #     print(f"   Uptime:        {uptime} seconds")
    #     print(f"   Free Heap:     {free_heap} bytes")
    #     print(f"   Chip Model:    {chip_model}")
    #     print(f"   WiFi SSID:     {wifi.get('ssid', 'N/A')}")
    #     print(f"   WiFi IP:       {wifi.get('ip', 'N/A')}")
    #     print(f"   WiFi RSSI:     {wifi.get('rssi', 0)} dBm")
    # else:
    #     print("   ✗ Failed to get status")
    #     return False

    # Set pin HIGH
    print("\n2. Setting pin 13 HIGH...")
    if esp.set_pin(13, 1):
        print("   ✓ Success")
    else:
        print("   ✗ Failed")

    time.sleep(1)

    # Read pin state
    print("\n3. Reading pin 13 state...")
    state = esp.get_pin(13)
    if state is not None:
        print(f"   Pin 13 = {state} ({'HIGH' if state else 'LOW'})")
    else:
        print("   ✗ Failed to read pin")

    time.sleep(1)

    # Toggle pin
    print("\n4. Toggling pin 13...")
    if esp.toggle_pin(13):
        state = esp.get_pin(13)
        print(f"   ✓ Toggled to {state} ({'HIGH' if state else 'LOW'})")
    else:
        print("   ✗ Failed to toggle")

    time.sleep(1)

    # Set pin LOW
    print("\n5. Setting pin 13 LOW...")
    if esp.set_pin(13, 0):
        print("   ✓ Success")
    else:
        print("   ✗ Failed")

    return True


def demo_pwm_control(esp: ESP32Controller):
    """Demonstrate PWM control."""
    print("\n" + "=" * 60)
    print(" PWM Control Demo")
    print("=" * 60)

    pin = 14

    print(f"\n1. Setting pin {pin} to PWM value 0 (off)...")
    if esp.set_pwm(pin, 0):
        print("   ✓ Success")
    else:
        print("   ✗ Failed")

    time.sleep(0.5)

    print(f"\n2. Fading pin {pin} up (0 -> 255)...")
    for value in range(0, 256, 32):
        if esp.set_pwm(pin, value):
            print(f"   PWM = {value:3d}")
        else:
            print(f"   ✗ Failed at {value}")
            break
        time.sleep(0.1)

    time.sleep(0.5)

    print(f"\n3. Fading pin {pin} down (255 -> 0)...")
    for value in range(255, -1, -32):
        if esp.set_pwm(pin, value):
            print(f"   PWM = {value:3d}")
        else:
            print(f"   ✗ Failed at {value}")
            break
        time.sleep(0.1)

    return True


def demo_multiple_pins(esp: ESP32Controller):
    """Demonstrate controlling multiple pins."""
    print("\n" + "=" * 60)
    print(" Multiple Pin Control Demo")
    print("=" * 60)

    pins = [12, 13, 14, 15]

    print(f"\n1. Setting pins {pins} all HIGH...")
    for pin in pins:
        if esp.set_pin(pin, 1):
            print(f"   Pin {pin}: ✓")
        else:
            print(f"   Pin {pin}: ✗")
        time.sleep(0.2)

    time.sleep(1)

    print(f"\n2. Setting pins {pins} all LOW...")
    for pin in pins:
        if esp.set_pin(pin, 0):
            print(f"   Pin {pin}: ✓")
        else:
            print(f"   Pin {pin}: ✗")
        time.sleep(0.2)

    time.sleep(1)

    print(f"\n3. Knight Rider effect on pins {pins}...")
    for _ in range(3):
        # Forward
        for pin in pins:
            esp.set_pin(pin, 1)
            time.sleep(0.1)
            esp.set_pin(pin, 0)

        # Backward
        for pin in reversed(pins):
            esp.set_pin(pin, 1)
            time.sleep(0.1)
            esp.set_pin(pin, 0)

    return True


def demo_udp_vs_tcp(esp: ESP32Controller):
    """Compare UDP vs TCP performance."""
    print("\n" + "=" * 60)
    print(" UDP vs TCP Performance Comparison")
    print("=" * 60)

    pin = 13
    iterations = 10

    # Test TCP
    print(f"\n1. Testing TCP: {iterations} commands...")
    start_time = time.time()
    for _ in range(iterations):
        esp.set_pin(pin, 1, use_tcp=True)
        esp.set_pin(pin, 0, use_tcp=True)
    tcp_time = time.time() - start_time
    print(f"   TCP Time: {tcp_time:.3f} seconds")
    print(f"   Rate: {(iterations * 2) / tcp_time:.1f} commands/sec")

    time.sleep(0.5)

    # Test UDP
    print(f"\n2. Testing UDP: {iterations} commands...")
    start_time = time.time()
    for _ in range(iterations):
        esp.set_pin(pin, 1, use_tcp=False)
        esp.set_pin(pin, 0, use_tcp=False)
    udp_time = time.time() - start_time
    print(f"   UDP Time: {udp_time:.3f} seconds")
    print(f"   Rate: {(iterations * 2) / udp_time:.1f} commands/sec")

    print(f"\n3. Performance comparison:")
    print(f"   UDP is {tcp_time / udp_time:.1f}x faster than TCP")

    return True


def run_interactive_mode(esp: ESP32Controller):
    """Interactive command mode."""
    print("\n" + "=" * 60)
    print(" Interactive Mode")
    print("=" * 60)
    print("\nCommands:")
    print("  set <pin> <0|1>    - Set pin HIGH or LOW")
    print("  get <pin>          - Get pin state")
    print("  toggle <pin>       - Toggle pin")
    print("  pwm <pin> <0-255>  - Set PWM value")
    print("  status             - Get system status")
    print("  quit               - Exit interactive mode")
    print()

    while True:
        try:
            cmd = input("ESP32> ").strip().lower()

            if not cmd:
                continue

            if cmd == "quit" or cmd == "exit":
                break

            parts = cmd.split()

            if parts[0] == "set" and len(parts) == 3:
                pin = int(parts[1])
                value = int(parts[2])
                if esp.set_pin(pin, value):
                    print(f"✓ Pin {pin} set to {value}")
                else:
                    print(f"✗ Failed to set pin {pin}")

            elif parts[0] == "get" and len(parts) == 2:
                pin = int(parts[1])
                value = esp.get_pin(pin)
                if value is not None:
                    print(
                        f"Pin {pin} = {value} ({'HIGH' if value else 'LOW'})")
                else:
                    print(f"✗ Failed to get pin {pin}")

            elif parts[0] == "toggle" and len(parts) == 2:
                pin = int(parts[1])
                if esp.toggle_pin(pin):
                    value = esp.get_pin(pin)
                    print(f"✓ Pin {pin} toggled to {value}")
                else:
                    print(f"✗ Failed to toggle pin {pin}")

            elif parts[0] == "pwm" and len(parts) == 3:
                pin = int(parts[1])
                value = int(parts[2])
                if esp.set_pwm(pin, value):
                    print(f"✓ Pin {pin} PWM set to {value}")
                else:
                    print(f"✗ Failed to set PWM on pin {pin}")

            elif parts[0] == "status":
                status = esp.get_status()
                if status:
                    print(json.dumps(status, indent=2))
                else:
                    print("✗ Failed to get status")

            else:
                print("Invalid command. Type 'quit' to exit.")

        except ValueError:
            print("Invalid pin or value. Use numbers only.")
        except KeyboardInterrupt:
            print("\nExiting...")
            break
        except Exception as e:
            print(f"Error: {e}")


def main():
    """Main demo program."""
    print("=" * 60)
    print(" ESP32 Pin Controller - Demo Script")
    print("=" * 60)

    # Auto-discover and connect to ESP32
    try:
        print("\nSearching for ESP32 device...")
        esp = ESP32Controller(auto_discover=True, verbose=False)
    except (ConnectionError, ValueError) as e:
        print(f"\n✗ Error: {e}")
        print("\nTroubleshooting:")
        print("  • Make sure the ESP32 is powered on")
        print("  • Verify the ESP32 is connected to WiFi")
        print("  • Check that your computer is on the same network")
        print("\nAlternatively, specify the IP address manually:")
        print('  esp = ESP32Controller("192.168.1.100", verbose=False)')
        return 1

    print(f"✓ Found ESP32 at {esp.host}")

    # Connect via TCP
    print("\nConnecting to ESP32...")
    if not esp.connect_tcp():
        print("✗ Failed to connect to ESP32")
        return 1

    print("✓ Connected to ESP32")

    try:
        # Run demos
        if not demo_basic_commands(esp):
            return 1

        input("\nPress Enter to continue to PWM demo...")
        if not demo_pwm_control(esp):
            return 1

        input("\nPress Enter to continue to multiple pin demo...")
        if not demo_multiple_pins(esp):
            return 1

        input("\nPress Enter to continue to performance comparison...")
        if not demo_udp_vs_tcp(esp):
            return 1

        input("\nPress Enter to start interactive mode (or Ctrl+C to quit)...")
        run_interactive_mode(esp)

    except KeyboardInterrupt:
        print("\n\nDemo interrupted by user")

    finally:
        # Cleanup
        print("\nCleaning up...")
        esp.disconnect_tcp()
        print("Disconnected from ESP32")

    return 0


if __name__ == "__main__":
    import json  # For status display
    sys.exit(main())
