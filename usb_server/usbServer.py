import serial
import time

# Open serial port (adjust COM port/baud rate)
ser = serial.Serial('COM10', 115200, timeout=4)
time.sleep(2) # Wait for ESP32 reboot

def serialUpload(file_name):
    #this method helps upload file to esp32

    with open(file_name, "rb") as f:
        file_bytes = f.read()
        total_size = len(file_bytes)

        # 1. we send the name of the file and method
        ser.write("UPLOAD\n".encode("utf-8"))
        time.sleep(.1)
        ser.write(f"/{file_name}\n".encode("utf-8"))

        #confirm sent file name below
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        print(f"ESP32 received file name:{line}")
        # 2. First, tell the ESP32 how many total bytes to expect (4-byte integer)
        ser.write(total_size.to_bytes(4, byteorder='little'))
        time.sleep(0.1) # Brief pause for ESP32 to process

        # 2. Send the raw binary stream
        chunk_size = 1024
        for i in range(0, len(file_bytes), chunk_size):
            ser.write(file_bytes[i:i+chunk_size])
            time.sleep(0.0) # Gives ESP32 10ms to complete its LittleFS flash cycle
            print(f"Sent {total_size} bytes over Serial.")

    while True:
        #print("active.....")
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                print(f"[ESP32 LOG]: {line}")
        except KeyboardInterrupt:
            break # Exit on Ctrl+C
    ser.close()

def serverDownload(requested_file, save_as_name):#this method helps get data coming from esp32

    print(f"Requesting '{requested_file}' from ESP32...")
    # Send the download command terminated by a newline
    ser.write("DOWNLOAD\n".encode("utf-8"))
    ser.reset_input_buffer()  # Wipes away any 'ets ' or debug text strings
    time.sleep(.1)
    ser.write(f"/{requested_file}\n".encode('utf-8'))

    # 1. Read the first 4 bytes to determine the total file size
    size_bytes = ser.read(4)

    if len(size_bytes) < 4:
        print("Error: Did not receive the file size header from ESP32.")
        ser.close()
        exit()

    # Convert the 4 bytes back into an integer
    total_file_size = int.from_bytes(size_bytes, byteorder='little')
    print(f"File found! raw size {size_bytes} Total size to expect: {total_file_size} bytes.")

    # 2. Read the file stream in chunks safely
    downloaded_data = bytearray()
    bytes_received = 0
    chunk_size = 1024

    while bytes_received < total_file_size:
        # Calculate how many bytes are remaining to read
        remaining_bytes = total_file_size - bytes_received
        bytes_to_read = min(chunk_size, remaining_bytes)
        
        # Read the block from the serial port
        chunk = ser.read(bytes_to_read)
        if not chunk:
            print("Error: Serial timeout occurred during transfer!")
            break
            
        downloaded_data.extend(chunk)
        bytes_received += len(chunk)
        print(f"Progress: {bytes_received}/{total_file_size} bytes downloaded.")

    # 3. Save the binary array back to your local PC drive
    if len(downloaded_data) == total_file_size:
        with open(save_as_name, "wb") as f:
            f.write(downloaded_data)
        print(f"Success! File saved locally to PC as '{save_as_name}'")
    else:
        print("Download failed. File size mismatch or data corrupted.")

    ser.close()

def deleteFile(file_name):
    ser.write("DELETE\n".encode("utf-8"))
    line = ser.readline().decode('utf-8', errors='ignore').strip()
    time.sleep(.1)
    print(f"Recived comand:{line}")
    ser.write(f"/{file_name}\n".encode('utf-8'))
    time.sleep(1)
    line = ser.readline().decode('utf-8', errors='ignore').strip()
    print(f"Operation comand:{line}")
    ser.close()

#serialUpload("start_image.jpg")
#serverDownload("start_image.jpg", "downloaded_start_image.jpg")
deleteFile("star_hex.txt")