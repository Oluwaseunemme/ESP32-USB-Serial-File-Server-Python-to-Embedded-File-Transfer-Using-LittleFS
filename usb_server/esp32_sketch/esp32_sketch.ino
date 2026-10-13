#include <LittleFS.h>

const int CHUNK_SIZE = 1024;
uint8_t chunkBuff[CHUNK_SIZE];
File uploadFile;


//USB data transmmision and processing below
void receiveFileOverSerial() {
  String incomingFileName = Serial.readStringUntil('\n');
  incomingFileName.trim();

  // Ensure leading slash for LittleFS
  if (!incomingFileName.startsWith("/")) {
    incomingFileName = "/" + incomingFileName;
  }
  
  while (Serial.available() < 4) {
    delay(1); 
  }
  
  uint32_t totalFileSize = 0;
  Serial.readBytes((uint8_t*)&totalFileSize, 4);
  
  Serial.print("Expecting total file size: ");
  Serial.println(totalFileSize);

  uint32_t bytesReceivedTotal = 0;
  int chunkIndex = 0;

  // --- TIMEOUT CONFIGURATION ---
  uint32_t lastByteTime = millis();
  const uint32_t SERIAL_TIMEOUT_MS = 3000; // Stop waiting if no data arrives for 3 seconds

  // Keep reading until we get every byte OR we hit a timeout
  while (bytesReceivedTotal < totalFileSize) {
    
    if (Serial.available() > 0) {
      uint8_t incomingByte = Serial.read(); 
      
      chunkBuff[chunkIndex] = incomingByte;
      chunkIndex++;
      bytesReceivedTotal++;

      // Reset the timeout timer because we successfully grabbed a byte
      lastByteTime = millis();

      if (chunkIndex == CHUNK_SIZE || bytesReceivedTotal == totalFileSize) {
        
        // Ensure file handle is reset/closed cleanly before check
        File uploadFile;
        if (bytesReceivedTotal <= CHUNK_SIZE) {
          uploadFile = LittleFS.open(incomingFileName, "w");
        } else {
          uploadFile = LittleFS.open(incomingFileName, "a");
        }

        if (uploadFile) {
          size_t bytesWritten = uploadFile.write(chunkBuff, chunkIndex);
          if (bytesWritten != chunkIndex) {
            Serial.println("ERROR: Chunk write mismatch!"); 
          }
          uploadFile.close(); 
        } else {
          Serial.println("ERROR: Failed to open file for writing!");
        }

        chunkIndex = 0; 
      }
    } else {
      // If the buffer is empty, check if we've been waiting too long
      if (millis() - lastByteTime > SERIAL_TIMEOUT_MS) {
        Serial.println("\nERROR: Serial read timeout! Loop broken to prevent infinite freeze.");
        break; // Force-break out of the while loop safely
      }
    }
  }

  Serial.println("File processing stopped at: " + String(bytesReceivedTotal));
  
  if (bytesReceivedTotal == totalFileSize) {
    Serial.printf("File %s transfer successfully completed! Total bytes: %d\n", incomingFileName.c_str(), totalFileSize);
  } else {
    Serial.printf("File %s transfer failed! Expected: %d, Received: %d\n", incomingFileName.c_str(), totalFileSize, bytesReceivedTotal);
  }
  bytesReceivedTotal = 0;
}

//below method help get back saved file to python device
void sendFileToPython() {
  // 1. Ensure the filename starts with a slash
  
  String outgoingFileName= Serial.readStringUntil('\n');
  outgoingFileName.trim();
  // 2. Check if the file exists
  if (!LittleFS.exists(outgoingFileName)) {
    Serial.print("ERROR: FILE_NOT_FOUND:");
    Serial.println(outgoingFileName);
    return;
  }

  File file = LittleFS.open(outgoingFileName, "r");
  if (!file) {
    Serial.println("ERROR: CANNOT_OPEN_FILE");
    return;
  }

  uint32_t fileSize = file.size(); // Sending the file size header first (4 bytes) so Python knows how much to read

  Serial.write((uint8_t*)&fileSize, 4);
  delay(50); // Small delay to let Python prepare its buffer

  // 4. Stream the file in 1024-byte chunks
  uint8_t buffer[1024];
  while (file.available()) {
    int bytesRead = file.read(buffer, sizeof(buffer));
    Serial.write(buffer, bytesRead);
  }

  file.close();
}

//below function helps delete existing file
void deleteFile(){
  String deleteFileName= Serial.readStringUntil('\n');
  deleteFileName.trim();
  if(LittleFS.exists(deleteFileName)){
    LittleFS.remove(deleteFileName);
    Serial.print(deleteFileName);
    Serial.println(" succesfully deleted!!!");
    }
  else{
    Serial.println("File inexistence....try again with a different name");
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200, SERIAL_8N1, -1, -1, false, 2048); 
  if (!LittleFS.begin(true)) { // true enables auto-formating if it fails to mount
    Serial.println("LittleFS Mount Failed");
    return;
  }
}

void loop() {

  if(Serial.available()>0){
    String action =Serial.readStringUntil('\n');
    action.trim();
    //Serial.print("Action is:");
    //Serial.println(action);
    if(action=="UPLOAD"){
      Serial.println("Upload operation in progress");
      receiveFileOverSerial();
    }
    else if(action=="DOWNLOAD"){
      //Serial.println("Download operation in progress");
      sendFileToPython();
    }
    else if(action=="DELETE"){
      //Serial.println("Delete operation in progress");
      deleteFile();
    }
   }
}
