./wbregs cfgscope         | tee    sdtest.txt
./wbregs sddata 1         | tee -a sdtest.txt
./wbregs sdcard 0x080ff   | tee -a sdtest.txt
./wbregs sdcard 0x080bf   | tee -a sdtest.txt
./wbregs sddata           | tee -a sdtest.txt

./wbregs sddata   0x00    | tee -a sdtest.txt
./wbregs cfgscope 0x01fc  | tee -a sdtest.txt
./wbregs sdcard   0x040   | tee -a sdtest.txt
./wbregs sdcard           | tee -a sdtest.txt
./sdcardscop              | tee -a sdtest.txt

./wbregs sddata 0x40000000 | tee    sdtest.txt
./wbregs cfgscope 0x01fc   | tee -a sdtest.txt
./wbregs sdcard 0xc041     | tee -a sdtest.txt
./wbregs sdcard            | tee -a sdtest.txt
./sdcardscop               | tee -a sdtest.txt

./wbregs sddata   0x01a7 | tee -a sdtest.txt
./wbregs cfgscope 0x01fc | tee -a sdtest.txt
./wbregs sdcard   0xc248 | tee -a sdtest.txt
./wbregs sdcard          | tee -a sdtest.txt
./wbregs sddata          | tee -a sdtest.txt
./sdcardscop             | tee -a sdtest.txt


# Repeat while device is busy
./wbregs sddata 0        | tee -a sdtest.txt
./wbregs cfgscope 0x01fc | tee -a sdtest.txt
./wbregs sdcard 0x077    | tee -a sdtest.txt
./wbregs sdcard          | tee -a sdtest.txt
./sdcardscop             | tee -a sdtest.txt

./wbregs sddata 0x40000000 | tee -a sdtest.txt
./wbregs cfgscope 0x01fc   | tee -a sdtest.txt
./wbregs sdcard 0x069      | tee -a sdtest.txt
./wbregs sdcard            | tee -a sdtest.txt
./sdcardscop               | tee -a sdtest.txt
./wbregs sdcard
#End Repeat

sleep 1
# Repeat while device is busy
./wbregs sddata 0        | tee -a sdtest.txt
./wbregs cfgscope 0x01fc | tee -a sdtest.txt
./wbregs sdcard 0x077    | tee -a sdtest.txt
./wbregs sdcard          | tee -a sdtest.txt
./sdcardscop             | tee -a sdtest.txt

./wbregs sddata 0x40000000 | tee -a sdtest.txt
./wbregs cfgscope 0x01fc   | tee -a sdtest.txt
./wbregs sdcard 0x069      | tee -a sdtest.txt
./wbregs sdcard            | tee -a sdtest.txt
./sdcardscop               | tee -a sdtest.txt
./wbregs sdcard
#End Repeat





# READ_OCR
./wbregs sddata   0x0    | tee -a sdtest.txt
./wbregs cfgscope 0x01fc | tee -a sdtest.txt
./wbregs sdcard   0xc27a | tee -a sdtest.txt
./wbregs sdcard          | tee -a sdtest.txt
./wbregs sddata          | tee -a sdtest.txt # c0ff8000
./sdcardscop             | tee -a sdtest.txt

# SEND_CSD_COND (Requires FIFO support)
./wbregs sddata   0x0200     | tee -a sdtest.txt
./wbregs sdcard   0xc0ff     | tee -a sdtest.txt
./wbregs sdcard   0x00bf     | tee -a sdtest.txt
./wbregs sddata              | tee -a sdtest.txt
./wbregs sdcard              | tee -a sdtest.txt
./wbregs sddata   0x40000000 | tee -a sdtest.txt
./wbregs cfgscope 0x01fc     | tee -a sdtest.txt
./wbregs sdcard   0x0849     | tee -a sdtest.txt
./wbregs sdcard              | tee -a sdtest.txt
./wbregs sddata              | tee -a sdtest.txt
./wbregs sddata   0x0700     | tee -a sdtest.txt
./wbregs sdcard   0xc0ff     | tee -a sdtest.txt
./wbregs sdcard   0x00bf     | tee -a sdtest.txt
./wbregs sddata              | tee -a sdtest.txt
./wbregs sdfif0              | tee -a sdtest.txt
./wbregs sdfif0              | tee -a sdtest.txt
./wbregs sdfif0              | tee -a sdtest.txt
./wbregs sdfif0              | tee -a sdtest.txt
./sdcardscop                 | tee -a sdtest.txt

# SEND_CSD_COND (Requires FIFO support)
# Let's test with the wrong length, and see if we get a CRC error
# --- BEAUTIFUL!!! TEST PASSES WITH CRC ERROR AS DESIRED!
./wbregs sddata   0x0300     | tee -a sdtest.txt
./wbregs sdcard   0x80ff     | tee -a sdtest.txt
./wbregs sdcard   0x00bf     | tee -a sdtest.txt
# Clear the FIFO first
./wbregs sdfif0 0xdeadbeef | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x0201 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x0301 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x0401 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x0501 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x0601 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x0701 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x0801 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x0901 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x0a01 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x0b01 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x0c01 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x0d01 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x0e01 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x0f01 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x010001 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x010101 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x010201 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x010301 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x010401 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x010501 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x010601 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x010701 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x010801 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x010901 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x010a01 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x010b01 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x010c01 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x010d01 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x010e01 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x010f01 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x020001 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x020101 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x020201 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x020301 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x020401 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x020501 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x020601 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x020701 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x020801 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x020901 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x020a01 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x020b01 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x020c01 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x020d01 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x020e01 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0x020f01 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0 | tee -a sdtest.txt
./wbregs sdfif0 0xf0f0f00f | tee -a sdtest.txt
./wbregs sdcard   0x00bf   | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt
./wbregs sdfif0 | tee -a sdtest.txt

./wbregs sddata              | tee -a sdtest.txt
./wbregs sdcard              | tee -a sdtest.txt
./wbregs sddata   0x40000000 | tee -a sdtest.txt
./wbregs cfgscope 0x01fc     | tee -a sdtest.txt
./wbregs sdcard   0x0849     | tee -a sdtest.txt
./wbregs sdcard              | tee -a sdtest.txt
./wbregs sddata              | tee -a sdtest.txt
./wbregs sddata   0x0700     | tee -a sdtest.txt
./wbregs sdcard   0x00ff     | tee -a sdtest.txt
./wbregs sdcard   0x00bf     | tee -a sdtest.txt
./wbregs sddata              | tee -a sdtest.txt
./wbregs sdfif0              | tee -a sdtest.txt
./wbregs sdfif0              | tee -a sdtest.txt
./wbregs sdfif0              | tee -a sdtest.txt
./wbregs sdfif0              | tee -a sdtest.txt
./wbregs sdfif0              | tee -a sdtest.txt
./wbregs sdfif0              | tee -a sdtest.txt
./wbregs sdfif0              | tee -a sdtest.txt
./wbregs sdfif0              | tee -a sdtest.txt
echo "End of valid data" | tee -a sdtest.txt
./wbregs sdfif0              | tee -a sdtest.txt
./wbregs sdfif0              | tee -a sdtest.txt
./wbregs sdfif0              | tee -a sdtest.txt
./wbregs sdfif0              | tee -a sdtest.txt
./sdcardscop                 | tee -a sdtest.txt

