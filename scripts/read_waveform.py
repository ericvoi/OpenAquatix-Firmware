import serial
import struct
import time
import os
from datetime import datetime

# Open the serial port
ser = serial.Serial('COM5', 3686400, timeout=0.1)  
ser.set_buffer_size(rx_size=1024*256)

# Constants for binary data detection
DATA_MARKER = b'DATA'
TERM_MARKER = b'TERM'
END_MARKER = b'\xAA\xBB\xCC\xDD'
DATA_SIZE = 1024  # Fixed size of 1024 bytes always

OUTPUT_DIR = "acoustic_data"
os.makedirs(OUTPUT_DIR, exist_ok=True)

# Buffer for data processing
buffer = bytearray()
output_samples = []
packet_count = 0

def save_samples_to_file():
    """Save all accumulated samples to a human-readable text file"""
    if not output_samples:
        print("No samples to save")
        return
        
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = os.path.join(OUTPUT_DIR, f"acoustic_data_{timestamp}.txt")
    
    with open(filename, 'w') as f:
        # Write header info
        f.write(f"# STM32H723 Acoustic Modem Data\n")
        f.write(f"# Timestamp: {datetime.now().isoformat()}\n")
        f.write(f"# Total packets: {packet_count}\n")
        f.write(f"# Total samples: {len(output_samples)}\n")
        f.write(f"# Format: One 16-bit sample value per line\n")
        f.write(f"# --------------------------\n")
        
        # Write one sample per line
        for sample in output_samples:
            f.write(f"{sample}\n")
            
    print(f"\nSaved {len(output_samples)} samples from {packet_count} packets to {filename}")

def process_buffer():
    """Process the buffer to extract binary data packets and text"""
    global buffer, output_samples, packet_count
    
    # Keep processing while we might have a complete packet
    while True:
        # Look for DATA or TERM markers
        data_idx = buffer.find(DATA_MARKER)
        term_idx = buffer.find(TERM_MARKER)
        
        if data_idx == -1 and term_idx == -1:
            # No markers found, check for text output
            try:
                # Try to decode part of the buffer as text
                text_end = min(len(buffer), 100)  # Limit how much we try to decode
                text = buffer[:text_end].decode('ascii', errors='replace')
                
                # If there's a newline, print up to that
                newline_pos = text.find('\n')
                if newline_pos != -1:
                    print(text[:newline_pos+1], end='')
                    buffer = buffer[newline_pos+1:]
                elif len(buffer) > 1000:  # If buffer is getting large without markers
                    # Just print what we have and clear
                    print(text, end='')
                    buffer = buffer[text_end:]
                else:
                    # Not enough data yet, wait for more
                    break
            except:
                # If decoding fails, just wait for more data
                break
        else:
            # Determine which marker appears first (if one doesn't exist, use the other)
            start_idx = data_idx if data_idx != -1 and (term_idx == -1 or data_idx < term_idx) else term_idx
            is_termination = (start_idx == term_idx)
            marker_len = len(TERM_MARKER if is_termination else DATA_MARKER)
            
            # If there's text before the marker, print it
            if start_idx > 0:
                try:
                    text = buffer[:start_idx].decode('ascii', errors='replace')
                    print(text, end='')
                except:
                    pass  # Ignore errors in text processing
            
            # Check if we have a complete packet
            packet_end = start_idx + marker_len + DATA_SIZE + len(END_MARKER)
            if len(buffer) < packet_end:
                # Not enough data for a full packet
                break
                
            # Verify end marker
            expected_end = start_idx + marker_len + DATA_SIZE
            if buffer[expected_end:expected_end + len(END_MARKER)] != END_MARKER:
                # This doesn't seem to be a valid packet - could be text that contains "DATA"
                # Move past this false marker and continue
                buffer = buffer[start_idx + 1:]
                continue
                
            # Extract and process the binary data
            binary_data = buffer[start_idx + marker_len:expected_end]
            
            # Parse samples (16-bit integers)
            for i in range(0, len(binary_data), 2):
                if i + 1 < len(binary_data):  # Ensure we have 2 bytes
                    sample = struct.unpack('<h', binary_data[i:i+2])[0]
                    output_samples.append(sample)
            
            packet_count += 1
            if packet_count % 10 == 0:
                print(f"\rProcessed {packet_count} packets...", end='')
                
            # If this was a termination packet, save the file and reset
            if is_termination:
                save_samples_to_file()
                output_samples = []
                packet_count = 0
                
            # Remove the processed packet from the buffer
            buffer = buffer[packet_end:]

def go_to_main_menu():
    for i in range(5):
        ser.write(b"\x1b")  # ESC key
        time.sleep(0.1)
        
def toggle_waveform_print():
    go_to_main_menu()
    ser.write(b"2\r\n")
    time.sleep(0.1)
    ser.write(b"3\r\n")
    time.sleep(0.1)
    
def feedback_message():
    go_to_main_menu()
    ser.write(b"4\r\n")
    time.sleep(0.1)
    ser.write(b"4\r\n")
    time.sleep(0.1)
    ser.write(b"test\r\n")

def main():
    global buffer
    
    print(f"Starting STM32H723 Acoustic Modem Receiver")
    print(f"All data will be saved to a single text file")
    print(f"Press Ctrl+C to exit\n")

    try:
        toggle_waveform_print()
        feedback_message() # optionally disable to get received waveform
        
        while True:
            # Read as much data as available
            if ser.in_waiting:
                new_data = ser.read(ser.in_waiting)
                buffer.extend(new_data)
                
                # Process the buffer
                process_buffer()
            else:
                # Small delay when no data is available
                time.sleep(0.001)
            
    except KeyboardInterrupt:
        # Save any remaining data before exiting
        if output_samples:
            save_samples_to_file()
        ser.close()
        print("\nSerial port closed")

if __name__ == "__main__":
    main()