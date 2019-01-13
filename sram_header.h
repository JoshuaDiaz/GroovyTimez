// === RAM byte write =====================================================
// address between 0 and 2^17-1
// data bytes
void ram_write_byte(int addr, char data){
    int junk;
    // set 32-bit transfer for read/write command ORed with
    // actual address
    Mode32();
    mPORTBClearBits(BIT_0);
    WriteSPI2(RAM_WRITE_CMD | addr); // addr not greater than 17 bits
    while (SPI2STATbits.SPIBUSY); // wait for it to end of transaction
    junk = ReadSPI2(); // must always read, even if nothing useful is returned
    // set 8-bit transfer for each byte
    Mode8();
    WriteSPI2(data); // data write
    while (SPI2STATbits.SPIBUSY); // wait for it to end of transaction
    junk = ReadSPI2();
    mPORTBSetBits(BIT_0);
    return ;
}

// === RAM array write =====================================================
// address, pointer to an array containing the data, number of BYTES to store
void ram_write_byte_array(int addr, char* data, int count){
    int junk, i;
    Mode32();
    mPORTBClearBits(BIT_0);
    WriteSPI2(RAM_WRITE_CMD | addr); // addr not greater than 17 bits
    while (SPI2STATbits.SPIBUSY); // wait for it to end of transaction
    junk = ReadSPI2();
    Mode8();
    for(i=0; i<count; i++){
        WriteSPI2(data[i]); // data write
        while (SPI2STATbits.SPIBUSY); // wait for it to end of transaction
        junk = ReadSPI2();
    }
    mPORTBSetBits(BIT_0);
    return ;
}

// === RAM byte read ======================================================
int ram_read_byte(int addr){
    int junk, data;
    Mode32();
    mPORTBClearBits(BIT_0);
    WriteSPI2(RAM_READ_CMD | addr); // addr not greater than 17 bits
    while (SPI2STATbits.SPIBUSY); // wait for it to end of transaction
    junk = ReadSPI2();
    Mode8();
    WriteSPI2(junk); // force the read
    while (SPI2STATbits.SPIBUSY); // wait for it to end of transaction
    data = ReadSPI2();
    mPORTBSetBits(BIT_0);
    return data;
}

// === RAM array read ======================================================
// address, pointer to an array receiving the data, number of BYTES to read
int ram_read_byte_array(int addr, char* data, int count){
    int junk, i;
    Mode32();
    mPORTBClearBits(BIT_0);
    WriteSPI2(RAM_READ_CMD | addr); // addr not greater than 17 bits
    while (SPI2STATbits.SPIBUSY); // wait for it to end of transaction
    junk = ReadSPI2();
    Mode8();
    for(i=0; i<count; i++){
        WriteSPI2(junk); // force the read
        while (SPI2STATbits.SPIBUSY); // wait for it to end of transaction
        data[i] = ReadSPI2();
    }
    mPORTBSetBits(BIT_0);
    return ;
}
