# Example shell scripts for testing ESP32 Pin Controller

ESP32_IP="192.168.1.100"
TCP_PORT="8888"
UDP_PORT="8889"

# Test TCP connection
echo "=== Testing TCP Connection ==="
echo '{"cmd":"STATUS"}' | nc $ESP32_IP $TCP_PORT

# Set pin HIGH
echo ""
echo "=== Setting Pin 13 HIGH ==="
echo '{"cmd":"SET","pin":13,"value":1}' | nc $ESP32_IP $TCP_PORT

# Get pin state
echo ""
echo "=== Getting Pin 13 State ==="
echo '{"cmd":"GET","pin":13}' | nc $ESP32_IP $TCP_PORT

# Toggle pin
echo ""
echo "=== Toggling Pin 13 ==="
echo '{"cmd":"TOGGLE","pin":13}' | nc $ESP32_IP $TCP_PORT

# Set PWM
echo ""
echo "=== Setting PWM on Pin 25 ==="
echo '{"cmd":"PWM","pin":25,"value":128}' | nc $ESP32_IP $TCP_PORT

# Get help
echo ""
echo "=== Getting Help ==="
echo '{"cmd":"HELP"}' | nc $ESP32_IP $TCP_PORT

echo ""
echo "=== Tests Complete ==="
