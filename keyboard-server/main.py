from pynput import keyboard
import socket

# Static hostname of the ESP32 on the local network
hostname = "espectrum.local"
# Port to send the UDP packets to
esp32_port = 4210

# Resolve the ESP32's IP address from the hostname
try:
    esp32_address = socket.gethostbyname(hostname)
    print(f"Resolved IP for {hostname}: {esp32_address}")
except socket.gaierror:
    print(f"Failed to resolve hostname {hostname}. Make sure the device is connected to the network.")
    exit(1)

# UDP socket setup
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Mapping from pynput keys to SpecKeys enumeration values
key_map = {
    '1': 1, '2': 2, '3': 3, '4': 4, '5': 5,
    '6': 6, '7': 7, '8': 8, '9': 9, '0': 10,
    'q': 11, 'w': 12, 'e': 13, 'r': 14, 't': 15,
    'y': 16, 'u': 17, 'i': 18, 'o': 19, 'p': 20,
    'a': 21, 's': 22, 'd': 23, 'f': 24, 'g': 25,
    'h': 26, 'j': 27, 'k': 28, 'l': 29, 'enter': 30,
    'shift': 31, 'z': 32, 'x': 33, 'c': 34, 'v': 35,
    'b': 36, 'n': 37, 'm': 38, 'space': 40
    # Add any additional keys you need to map
}

def get_key_value(key):
    try:
        return key_map[key.char]
    except AttributeError:
        # Handle special keys
        if key == keyboard.Key.enter:
            return key_map['enter']
        elif key == keyboard.Key.space:
            return key_map['space']
        elif key == keyboard.Key.shift:
            return key_map['shift']
        # Add additional special key handling here
    except KeyError:
        # If the key is not in our mapping, return None
        return None

def on_press(key):
    key_value = get_key_value(key)
    if key_value is not None:
        msg = f"down:{key_value}"
        sock.sendto(msg.encode(), (esp32_address, esp32_port))

def on_release(key):
    key_value = get_key_value(key)
    if key_value is not None:
        msg = f"up:{key_value}"
        sock.sendto(msg.encode(), (esp32_address, esp32_port))

    if key == keyboard.Key.esc:
        return False

# Set up the listener for keyboard events
with keyboard.Listener(on_press=on_press, on_release=on_release) as listener:
    listener.join()

sock.close()
