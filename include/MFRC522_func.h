/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

bool authenticate(byte trailerBlock, MFRC522::MIFARE_Key key) {
    MFRC522::StatusCode status;

    // Authenticate using key A
    Serial.println(F("Authenticating using key A..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return false;
    }
    return true;
}

bool read_block(byte blockAddr, byte buffer[], byte size) {
    MFRC522::StatusCode status;
    
    // Read data from the block
    Serial.print(F("Reading data from block ")); Serial.print(blockAddr);
    Serial.println(F(" ..."));
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return false;
    }
    Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
    dump_byte_array(buffer, 16); Serial.println();
    Serial.println();
    return true;
}

bool write_and_verify(byte blockAddr, byte dataBlock[], byte buffer[], byte size) {
    MFRC522::StatusCode status;

    // Authenticate using key A
    //Serial.println(F("Authenticating using key A..."));
    //status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    //if (status != MFRC522::STATUS_OK) {
    //    Serial.print(F("PCD_Authenticate() failed: "));
    //    Serial.println(mfrc522.GetStatusCodeName(status));
    //    return false;
    //}
    
    // Write data to the block
    Serial.print(F("Writing data into block ")); Serial.print(blockAddr);
    Serial.println(F(" ..."));
    dump_byte_array(dataBlock, 16); Serial.println();
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return false;
    }
    Serial.println();

    // Read data from the block (again, should now be what we have written)
    Serial.print(F("Reading data from block ")); Serial.print(blockAddr);
    Serial.println(F(" ..."));
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return false;
    }
    Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
    dump_byte_array(buffer, 16); Serial.println();

    // Check that data in block is what we have written
    // by counting the number of bytes that are equal
    Serial.println(F("Checking result..."));
    byte count = 0;
    for (byte i = 0; i < 16; i++) {
        // Compare buffer (= what we've read) with dataBlock (= what we've written)
        if (buffer[i] == dataBlock[i])
            count++;
    }
    Serial.print(F("Number of bytes that match = ")); Serial.println(count);
    if (count == 16) {
        Serial.println(F("Success :-)"));
        return true;
    } else {
        Serial.println(F("Failure, no match :-("));
        Serial.println(F("  perhaps the write didn't work properly..."));
        Serial.println();
        return false;
    }
}

/**
 * True when the reader still has the settings PCD_Init gave it. A dip on its 3.3 V
 * (e.g. WiFi starting) can reset it to power-on defaults: it still answers on SPI and
 * reports its version, but its antenna is off, so it never sees a chip.
 */
byte reader_version = 0; // last value read from VersionReg, reported on the monitor topic

bool reader_ok(bool verbose = false) {
    byte version = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
    reader_version = version;
    byte tx_ask = mfrc522.PCD_ReadRegister(MFRC522::TxASKReg);
    byte tx_control = mfrc522.PCD_ReadRegister(MFRC522::TxControlReg);
    if (verbose)
        Serial.printf("[RFID] version 0x%02X, TxASK 0x%02X, TxControl 0x%02X\n", version, tx_ask, tx_control);
    // MFRC522 v1/v2 and the common clones; anything else is garbage on SPI
    if (version != 0x91 && version != 0x92 && version != 0x88 && version != 0x12 && version != 0xB2)
        return false;
    // PCD_Init sets TxASKReg to 0x40 and the antenna bits; both are 0 after a reset
    return (tx_ask & 0x40) && (tx_control & 0x03) == 0x03;
}

/**
 * Hard reset through the RST pin, then init. PCD_Init alone only does a soft reset,
 * which is an SPI command and can't recover a reader whose SPI is stuck.
 */
void reader_reset() {
    pinMode(RST_PIN, OUTPUT);
    digitalWrite(RST_PIN, LOW);
    delay(10);
    digitalWrite(RST_PIN, HIGH);
    delay(50); // oscillator start-up (datasheet 8.8.2)
    mfrc522.PCD_Init();
}

bool UIDcompare (unsigned int prevUID[], unsigned int currUID[], int UIDLen) {
    for (int i = 0; i < UIDLen; i++) {
        if (prevUID[i] != currUID[i])
            return false;
    }
    return true;
}