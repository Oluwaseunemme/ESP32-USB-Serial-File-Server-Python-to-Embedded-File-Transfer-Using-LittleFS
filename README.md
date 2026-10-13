# ESP32 USB Serial File Server — Python-to-Embedded File Transfer Using LittleFS

A USB serial-based file server built with **ESP32, Arduino C++, Python, and LittleFS**, designed to transfer files between a computer and an embedded device without relying on Wi-Fi or HTTP.

This project recreates the core functionality of my previous ESP32 Wi-Fi file server over a USB serial connection. It supports file uploads, downloads, and deletion, with a custom transfer protocol designed to handle limited serial buffers, constrained RAM, and differences in processing speed between Python and the ESP32.

What started as a simple serial communication exercise became a deeper exploration of binary data transfer, buffer management, file streaming, and synchronization between a host computer and a microcontroller.

## 🚀 Features

- **File Upload:** Transfer files from a computer to the ESP32 through USB serial.
- **File Download:** Retrieve files stored on the ESP32 and save them back to the computer.
- **File Deletion:** Delete stored files remotely using a serial command.
- **LittleFS Integration:** Read, write, and manage files in the ESP32's flash filesystem.
- **Custom Binary Transfer Protocol:** Exchange file-size headers, chunk-length information, file data, and acknowledgements.
- **Chunked Data Transmission:** Transfer files incrementally instead of loading entire files into ESP32 RAM.
- **Serial Buffer Management:** Handle limited receive buffers and differences between host transmission speed and device processing speed.
- **Python-Based Debugging:** Display diagnostic messages in Python while the serial port is dedicated to file transfers.

## ⚙️ How It Works

The Python application communicates with the ESP32 through a USB serial connection. Instead of sending the entire file as one large payload, both sides follow a defined protocol to coordinate file transfer.

1. **Command transmission:** Python sends a command such as `UPLOAD`, `DOWNLOAD`, or `DELETE`.
2. **Operation selection:** The ESP32 reads the command in its main `loop()` and calls the appropriate file-handling function.
3. **Transfer initialization:** The sender communicates the file size using a 4-byte header, allowing the receiver to determine how much data to expect.
4. **Chunked transfer:** File contents are transmitted in manageable portions, with chunk-length information and acknowledgements used to coordinate the transfer.
5. **Filesystem operations:** The ESP32 writes incoming data to LittleFS or reads stored data from LittleFS during downloads.
6. **Transfer completion:** Python and the ESP32 use the protocol's control information to manage the transfer and determine when the operation has finished.

## 🔄 Serial Commands

| Command | Function |
|---|---|
| `UPLOAD` | Receives a file from Python and stores it in LittleFS. |
| `DOWNLOAD` | Reads a stored file and transmits it to Python. |
| `DELETE` | Deletes a specified file from LittleFS. |

The ESP32 dispatches commands through its main loop:

```cpp
void loop() {
    if (Serial.available() > 0) {
        String action = Serial.readStringUntil('\n');
        action.trim();

        if (action == "UPLOAD") {
            Serial.println("Upload operation in progress");
            receiveFileOverSerial();
        }
        else if (action == "DOWNLOAD") {
            sendFileToPython();
        }
        else if (action == "DELETE") {
            deleteFile();
        }
    }
}
```

Each command invokes a dedicated function responsible for the corresponding file operation.

## 🧠 Engineering Challenges & Solutions

### 1. Serial Buffer Limitations

Unlike a file transfer over a network stack, direct serial communication requires careful handling of the available receive buffer. Python can push bytes faster than the ESP32 can consume and write them, potentially causing buffer overflow and data loss.

I addressed this by increasing the serial buffer capacity where necessary and implementing deliberate chunk-based data handling.

### 2. Binary Headers and Transfer Synchronization

The receiver needs to know how many bytes belong to a file before it can reliably reconstruct the transfer. The protocol therefore sends a 4-byte unsigned integer representing the total file size.

ESP32:
```cpp
uint32_t fileSize = file.size();
Serial.write((uint8_t*)&fileSize, 4);
```

Python:
```python
ser.write(total_size.to_bytes(4, byteorder="little"))
```

Both sides use a little-endian 4-byte representation. The receiver must decode the header using the same byte order.

### 3. Chunked File Reception and Transmission

An entire file may exceed the ESP32's available RAM. To avoid this, uploads are received and written incrementally, while downloads are read from LittleFS and sent in chunks.

The protocol also includes chunk lengths and acknowledgements to help coordinate the exchange between Python and the ESP32.

### 4. LittleFS File Management

LittleFS provides persistent file storage in the ESP32's flash memory. It allows the device to manage uploaded files without needing to keep the complete contents in RAM.

### 5. Serial Debugging During Transfers

The Arduino Serial Monitor cannot independently access a serial port while Python is using it. To work around this, I routed debugging output to Python for visualization, making it easier to inspect the device's behavior during uploads and downloads.

## 🛠️ Technologies Used

- **C++ / Arduino IDE** — Embedded firmware and file-transfer logic.
- **Python** — Host-side upload, download, deletion, and debugging.
- **PySerial** — Communication between Python and the ESP32.
- **LittleFS** — Flash-based file storage.
- **USB Serial** — Physical communication link between the computer and microcontroller.

## ▶️ Getting Started

### Requirements

- ESP32 development board
- USB data cable
- Arduino IDE and ESP32 board support
- Python 3
- PySerial
- LittleFS support configured for the ESP32 firmware

### Installation

1. Upload the ESP32 firmware using the Arduino IDE.
2. Install PySerial:

   ```bash
   pip install pyserial
   ```

3. Connect the ESP32 to your computer.
4. Configure the correct COM port and baud rate in the Python scripts.
5. Run the desired file operation from the Python application.

### Example Operations

```python
serialUpload("start_image.jpg")

serverDownload(
    "start_image.jpg",
    "downloaded_start_image.jpg"
)

deleteFile("star_hex.txt")
```

These examples demonstrate the intended upload, download, and deletion functions. Run the appropriate operation separately, and ensure the filenames match those expected by the ESP32 firmware.

## 📁 Project Structure

```text
usbServer/
├── ESP32 firmware
├── Python client scripts
└── screenshot/
    └── Project screenshots
```

The `screenshot/` folder contains images documenting the project and its operation. Add the actual source filenames to this structure when publishing the repository.

## 📌 Project Status

**Working prototype:** File upload, download, and deletion are functional using USB serial communication and LittleFS.

The current implementation works with relatively small files, but practical transfer size and throughput remain areas for further testing and optimization.

## 🔮 Future Improvements

- Optimize transfer throughput and buffer utilization.
- Expand testing with progressively larger files.
- Improve recovery from interrupted transfers and communication timeouts.
- Add file-integrity verification using checksums.
- Strengthen protocol validation and error reporting.

## 🎯 What This Project Taught Me

This project helped me understand that transferring files over serial is much more involved than sending simple strings or commands. It required me to manage binary headers, chunk lengths, acknowledgements, limited buffers, storage operations, and synchronization between a computer and a resource-constrained embedded device.

It was a valuable step toward building more robust communication systems between Python applications and embedded hardware.

**The goal:** Build a reliable foundation for transferring files between a computer and an ESP32 without depending on a network connection.
