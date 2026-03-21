data = bytearray(open("ClientUpdateBase-in-route.bin", "rb").read())
# Patch char GUID at offset 3 with our GUID 0x189700002FA7
our_guid = bytes([0xa7, 0x2f, 0x00, 0x00, 0x97, 0x18, 0x00, 0x00])
data[3:11] = our_guid
open("deploy.bin", "wb").write(data)
print("done")