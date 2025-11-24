import serial
import time
import os
from serial.serialutil import SerialException

PORT = "COM5"      # adjust if needed
BAUD = 115200

SESSION_DURATION_SEC = 120  # 2 minutes of actual logging


def wait_for_port(port, baud):
    """Wait until the given serial port becomes available."""
    while True:
        try:
            ser = serial.Serial(port, baud, timeout=1)
            print(f"Connected to {port}")
            return ser
        except SerialException:
            print(f"Waiting for ESP32 on {port} ... (plug it in)")
            time.sleep(2)  # wait and retry


def make_output_filename():
    """Generate an automatic CSV filename with timestamp."""
    timestamp = time.strftime("%Y%m%d_%H%M%S")
    return f"session_{timestamp}.csv"


def main():
    ser = wait_for_port(PORT, BAUD)

    output_file = make_output_filename()
    print(f"Will save to: {output_file} (only if full 2 min recorded)")

    header_written = False
    log_start_time = None

    with ser, open(output_file, "w") as f:
        print("Looking for CSV header starting with 't_ms'...")

        try:
            while True:
                line = ser.readline().decode("utf-8", errors="ignore").strip()
                if not line:
                    continue

                # Wait until we see a valid header
                if not header_written:
                    if line.startswith("t_ms"):
                        f.write(line + "\n")
                        print("Header:", line)
                        header_written = True
                        log_start_time = time.time()
                        print(f"Started timed logging ({SESSION_DURATION_SEC} s)...")
                    else:
                        print("Ignoring:", line)
                    continue

                # After header: check if session time is done
                elapsed = time.time() - log_start_time
                if elapsed >= SESSION_DURATION_SEC:
                    print(f"\nFinished {SESSION_DURATION_SEC} s session.")
                    print(f"Saved: {output_file}")
                    return  # normal exit, keep file

                # Normal logging
                f.write(line + "\n")
                print(line)

        except KeyboardInterrupt:
            print("\nStopped manually before timer ended.")

    # Outside the 'with' block: file is closed here.
    # Decide whether to keep or discard it.
    # If we never saw the header or didn't reach full duration, delete file.

    if not header_written or log_start_time is None:
        print("Header never received. Discarding file.")
        if os.path.exists(output_file):
            os.remove(output_file)
    else:
        elapsed_total = time.time() - log_start_time
        if elapsed_total < SESSION_DURATION_SEC:
            print(f"Session incomplete ({elapsed_total:.1f}s < {SESSION_DURATION_SEC}s). Discarding file.")
            if os.path.exists(output_file):
                os.remove(output_file)
        else:
            # This case normally won't happen here, because we return() earlier,
            # but we keep it for safety.
            print(f"Session complete. File kept: {output_file}")


if __name__ == "__main__":
    main()
