data = bytearray(open("SendSelfToClient-in-route.bin", "rb").read())

payload_len = len(data) - 5  # subtract opcode + length prefix
data[1] = payload_len & 0xFF
data[2] = (payload_len >> 8) & 0xFF
data[3] = (payload_len >> 16) & 0xFF
data[4] = (payload_len >> 24) & 0xFF

our_guid = bytes([0xa7, 0x2f, 0x00, 0x00, 0x97, 0x18, 0x00, 0x00])

offset = 5
data[offset:offset+8] = our_guid

open("sendself_patched.bin", "wb").write(data)
print(f"payload_len={payload_len}, GUID patched at offset {offset}")