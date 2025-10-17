// Example Node.js client for ESP32 Pin Controller

const net = require('net');
const dgram = require('dgram');

class ESP32Controller {
  constructor(host, tcpPort = 8888, udpPort = 8889) {
    this.host = host;
    this.tcpPort = tcpPort;
    this.udpPort = udpPort;
    this.tcpClient = null;
  }

  // TCP Connection
  connectTCP() {
    return new Promise((resolve, reject) => {
      this.tcpClient = new net.Socket();

      this.tcpClient.connect(this.tcpPort, this.host, () => {
        console.log('Connected to ESP32 via TCP');
        resolve(true);
      });

      this.tcpClient.on('error', (err) => {
        reject(err);
      });
    });
  }

  disconnectTCP() {
    if (this.tcpClient) {
      this.tcpClient.destroy();
      this.tcpClient = null;
    }
  }

  // Send command via TCP
  sendTCP(command) {
    return new Promise((resolve, reject) => {
      if (!this.tcpClient || this.tcpClient.destroyed) {
        reject(new Error('Not connected'));
        return;
      }

      let responseData = '';

      const dataHandler = (data) => {
        responseData += data.toString();

        // Try to parse JSON response
        try {
          const response = JSON.parse(responseData);
          this.tcpClient.removeListener('data', dataHandler);
          resolve(response);
        } catch (e) {
          // Not complete JSON yet, wait for more data
        }
      };

      this.tcpClient.on('data', dataHandler);

      // Send command
      const cmdStr = JSON.stringify(command) + '\n';
      this.tcpClient.write(cmdStr);
    });
  }

  // Send command via UDP
  sendUDP(command) {
    return new Promise((resolve, reject) => {
      const client = dgram.createSocket('udp4');
      const cmdStr = JSON.stringify(command);

      client.send(cmdStr, this.udpPort, this.host, (err) => {
        if (err) {
          client.close();
          reject(err);
        }
      });

      client.on('message', (msg, rinfo) => {
        try {
          const response = JSON.parse(msg.toString());
          client.close();
          resolve(response);
        } catch (e) {
          client.close();
          reject(e);
        }
      });

      // Timeout after 2 seconds
      setTimeout(() => {
        client.close();
        reject(new Error('UDP timeout'));
      }, 2000);
    });
  }

  // Convenience methods
  async setPin(pin, value, useTCP = true) {
    const command = { cmd: 'SET', pin, value };
    const response = useTCP
      ? await this.sendTCP(command)
      : await this.sendUDP(command);
    return response.success;
  }

  async getPin(pin, useTCP = true) {
    const command = { cmd: 'GET', pin };
    const response = useTCP
      ? await this.sendTCP(command)
      : await this.sendUDP(command);
    return response.success ? response.value : null;
  }

  async togglePin(pin, useTCP = true) {
    const command = { cmd: 'TOGGLE', pin };
    const response = useTCP
      ? await this.sendTCP(command)
      : await this.sendUDP(command);
    return response.success;
  }

  async setPWM(pin, value, useTCP = true) {
    const command = { cmd: 'PWM', pin, value };
    const response = useTCP
      ? await this.sendTCP(command)
      : await this.sendUDP(command);
    return response.success;
  }

  async getStatus(useTCP = true) {
    const command = { cmd: 'STATUS' };
    return useTCP ? await this.sendTCP(command) : await this.sendUDP(command);
  }

  async reset(useTCP = true) {
    const command = { cmd: 'RESET' };
    const response = useTCP
      ? await this.sendTCP(command)
      : await this.sendUDP(command);
    return response.success;
  }
}

// Example usage
async function main() {
  const esp = new ESP32Controller('192.168.1.100');

  console.log('=== ESP32 Pin Controller Demo ===\n');

  try {
    // Connect via TCP
    await esp.connectTCP();
    console.log('✓ Connected to ESP32\n');

    // Get system status
    console.log('1. Getting system status...');
    const status = await esp.getStatus();
    console.log(`   Uptime: ${status.system.uptime} seconds`);
    console.log(`   WiFi: ${status.wifi.ssid}`);
    console.log(`   IP: ${status.wifi.ip}\n`);

    // Set pin HIGH
    console.log('2. Setting pin 13 HIGH...');
    await esp.setPin(13, 1);
    console.log('   ✓ Success\n');

    await sleep(1000);

    // Get pin state
    console.log('3. Reading pin 13 state...');
    const state = await esp.getPin(13);
    console.log(`   Pin 13 = ${state}\n`);

    await sleep(1000);

    // Toggle pin
    console.log('4. Toggling pin 13...');
    await esp.togglePin(13);
    console.log('   ✓ Success\n');

    await sleep(1000);

    // PWM example
    console.log('5. Setting PWM on pin 25...');
    await esp.setPWM(25, 128);
    console.log('   ✓ PWM set to 128 (50%)\n');

    // Cleanup
    esp.disconnectTCP();
    console.log('Disconnected');
  } catch (error) {
    console.error('Error:', error.message);
    esp.disconnectTCP();
  }
}

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

// Run if executed directly
if (require.main === module) {
  main();
}

module.exports = ESP32Controller;
