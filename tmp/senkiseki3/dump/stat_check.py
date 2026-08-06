import struct
def dump(path, label):
    data = open(path,'rb').read()
    n = struct.unpack_from('<I', data, 28)[0]
    offs = [struct.unpack_from('<I', data, 32+4*i)[0] for i in range(n)]
    print(f"=== {label} ({len(data)}B) ===")
    for off in offs:
        name = data[off:off+4].decode('ascii')
        size = struct.unpack_from('<I', data, off+4)[0]
        if name == 'STAT':
            d = off+8
            vals = struct.unpack_from('<10I', data, d)
            print(f"  STAT: {[hex(v) for v in vals]}")
        if name == 'SFI0':
            d = off+8
            print(f"  SFI0: {data[d:d+size].hex()}")
dump(r"e:\RenoDX\renodx\tmp\senkiseki3\fresh_face.cso", "FRESH-HLSL (WORKS)")
dump(r"e:\RenoDX\renodx\tmp\senkiseki3\0x0D5DABC6.minimal.cso", "OUR PATCH (CRASHES)")
