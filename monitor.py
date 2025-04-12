import serial
import sys
import time

def main():
    if len(sys.argv) < 2:
        print("Usage: python serial_reader.py <PORT>")
        sys.exit(1)

    port = sys.argv[1]
    baudrate = 115200  # Change this if your ESP32 uses a different baud rate

    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"Connected to {port} at {baudrate} baud.")
        time.sleep(2)  # Allow time for the ESP32 to reset on connect

        while True:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(line)

    except serial.SerialException as e:
        print(f"Serial error: {e}")
    except KeyboardInterrupt:
        print("\nExiting.")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

if __name__ == "__main__":
    main()