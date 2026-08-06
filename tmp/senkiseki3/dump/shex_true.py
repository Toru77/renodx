import struct

data = open(r"e:\RenoDX\renodx\tmp\senkiseki3\dump\0x0D5DABC6.cso",'rb').read()
n = struct.unpack_from('<I', data, 28)[0]
offs = [struct.unpack_from('<I', data, 32+4*i)[0] for i in range(n)]
for off in offs:
    if data[off:off+4] in (b'SHEX', b'SHDR'):
        size = struct.unpack_from('<I', data, off+4)[0]
        d = off+8
        print(f"SHEX @{off:#x} size={size}")
        print(f"  bytes +0..+31: {data[d:d+32].hex(' ')}")
        # print first 8 dwords
        for i in range(8):
            print(f"  dword[{i}] @{d+4*i:#x} = {struct.unpack_from('<I', data, d+4*i)[0]:08X}")
        # SHEX data region is [d, d+size)
        print(f"  data region [{d:#x}, {d+size:#x}) = {size} bytes = {size//4} dwords")
        break
