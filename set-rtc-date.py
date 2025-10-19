Import("env")

import time
from datetime import datetime, timezone

# Ensure pyserial is installed
try:
    import serial
except ImportError:
    env.Execute("$PYTHONEXE -m pip install pyserial")

def set_device_date(*args, **kwargs):
    """
    Opens serial connection to device and sets the RTC date/time
    """
    # Skip if this is an integration dump
    if env.IsIntegrationDump():
        return

    # Get the serial port from environment
    upload_port = env.get("UPLOAD_PORT", None)

    if not upload_port:
        print("Warning: No upload port specified, skipping RTC date set")
        return

    # Get monitor speed from environment, default to 115200
    monitor_speed = env.get("MONITOR_SPEED", "115200")

    print(f"\n{'='*60}")
    print("Setting RTC date on device...")
    print(f"{'='*60}")

    try:
        # Open serial connection
        ser = serial.Serial(
            port=upload_port,
            baudrate=int(monitor_speed),
            timeout=2
        )

        # Wait for device to be ready
        print("Waiting for device to be ready...")
        time.sleep(2)

        # Clear any existing data in the buffer
        ser.reset_input_buffer()
        ser.reset_output_buffer()

        # Get current UTC time in ISO format
        current_time = datetime.now(timezone.utc)
        iso_date = current_time.strftime("%Y-%m-%dT%H:%M:%SZ")

        # Send date command
        command = f"date {iso_date}\n"
        print(f"Sending command: {command}")
        ser.write(command.encode('ascii'))
        ser.flush()

        # Wait for response
        time.sleep(0.5)

        # Read response
        response_lines = []
        while ser.in_waiting > 0:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                response_lines.append(line)

        # Display response
        if response_lines:
            print("\nDevice response:")
            for line in response_lines:
                print(f"  {line}")

        # Verify by reading the date back
        time.sleep(0.2)
        ser.reset_input_buffer()
        ser.write(b"date\n")
        ser.flush()
        time.sleep(0.5)

        verify_lines = []
        while ser.in_waiting > 0:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                verify_lines.append(line)

        if verify_lines:
            print("\nVerification - Current RTC time:")
            for line in verify_lines:
                print(f"  {line}")

        # Close serial connection
        ser.close()

        print(f"\n{'='*60}")
        print("RTC date set successfully!")
        print(f"{'='*60}\n")

    except serial.SerialException as e:
        print(f"\nWarning: Could not open serial port {upload_port}")
        print(f"Error: {e}")
        print("Skipping RTC date set. You can manually set it using: date YYYY-MM-DDTHH:MM:SSZ")
        print(f"{'='*60}\n")
    except Exception as e:
        print(f"\nWarning: Error setting RTC date: {e}")
        print("Skipping RTC date set. You can manually set it using: date YYYY-MM-DDTHH:MM:SSZ")
        print(f"{'='*60}\n")

# Add the post-upload action
env.AddPostAction("upload", set_device_date)
