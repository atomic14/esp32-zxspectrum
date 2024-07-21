import serial
import serial.tools.list_ports
import time

# Mapping from pynput keys to SpecKeys enumeration values
key_map = {
    '1': 1, '2': 2, '3': 3, '4': 4, '5': 5,
    '6': 6, '7': 7, '8': 8, '9': 9, '0': 10,
    'q': 11, 'w': 12, 'e': 13, 'r': 14, 't': 15,
    'y': 16, 'u': 17, 'i': 18, 'o': 19, 'p': 20,
    'a': 21, 's': 22, 'd': 23, 'f': 24, 'g': 25,
    'h': 26, 'j': 27, 'k': 28, 'l': 29, '\n': 30,
    'shift': 31, 'z': 32, 'x': 33, 'c': 34, 'v': 35,
    'b': 36, 'n': 37, 'm': 38, 'sym': 39, ' ': 40,
    # Add any additional keys you need to map
}

def get_key_value(key):
    try:
        return key_map[key]
    except KeyError:
        print(f"Key not found in key_map. Key: {key}")
        # If the key is not in our mapping, return None
        return None


def list_serial_ports():
    ports = serial.tools.list_ports.comports()
    return ports

def select_serial_port():
    ports = list_serial_ports()
    for index, port in enumerate(ports):
        print(f"{index}: {port.device} - {port.description}")
    choice = input("Select the port index (refresh with 'r'): ")
    if choice.lower() == 'r':
        return select_serial_port()
    elif choice.isdigit() and 0 <= int(choice) < len(ports):
        return ports[int(choice)].device
    else:
        print("Invalid selection.")
        return select_serial_port()

def on_press(key):
    key_value = get_key_value(key)
    if key_value is not None:
        msg = f"down:{key_value}\n"
        print(msg, end='')
        ser.write(msg.encode())

def on_release(key):
    key_value = get_key_value(key)
    if key_value is not None:
        msg = f"up:{key_value}\n"
        print(msg, end='')
        ser.write(msg.encode())

# text_to_send = '''10p"Patreon Shoutout"
# 15p"----------------"
# 20p"Justin Neuhard"
# 30p"don williams"
# 40p"Red Rider"
# 50p"Sim Dorsey"
# 60p"Jeremy Wilkerson"
# 70p"Timothee Demierre"
# 80p"LysisofPie"
# 90p"Guy Dupont"
# 100g10
# r
# '''

text_to_send = '''10p"Top Challenges"
15p"----------------"
20p"Keyboard Improvements"
30p"TFT Display Selection"
40p"Firmware"
50p" - Develop existing?"
60p" - Use ESPectrum?"
r
'''



key_delay = 0.1
key_press_duration = 0.1

if __name__ == "__main__":
    port = select_serial_port()
    if port:
        ser = serial.Serial(port, 115200, timeout=1)
        print(f"Connected to {port}. Listening for keyboard events...")
        time.sleep(2)

        try:
            # send each letter from the text_to_send using on_press/on_release - for the quote mark, surround a "p" with sym
            for letter in text_to_send:
                time.sleep(key_delay)
                print("Sending letter: ", letter)
                if letter == '"':
                    on_press("sym")
                    time.sleep(key_delay)
                    on_press("p")
                    time.sleep(key_press_duration)
                    on_release("p")
                    time.sleep(key_delay)
                    on_release("sym")
                elif letter == '-':
                    on_press("sym")
                    time.sleep(key_delay)
                    on_press("j")
                    time.sleep(key_press_duration)
                    on_release("j")
                    time.sleep(key_delay)
                    on_release("sym")
                elif letter.isupper():
                    on_press("shift")
                    time.sleep(key_delay)
                    on_press(letter.lower())
                    time.sleep(key_press_duration)
                    on_release(letter.lower())
                    time.sleep(key_delay)
                    on_release("shift")
                else:
                    time.sleep(key_delay)
                    on_press(letter)
                    time.sleep(key_press_duration)
                    on_release(letter)
                    time.sleep(key_delay)
                
        except KeyboardInterrupt:
            print("Program exited.")
        finally:
            ser.close()
