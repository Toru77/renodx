import struct
def dump_stat(path, label):
    data = open(path,'rb').read()
    n = struct.unpack_from('<I', data, 28)[0]
    offs = [struct.unpack_from('<I', data, 32+4*i)[0] for i in range(n)]
    for off in offs:
        name = data[off:off+4].decode('ascii')
        size = struct.unpack_from('<I', data, off+4)[0]
        if name == 'STAT':
            d = off+8
            raw = data[d:d+size]
            words = struct.unpack_from('<16I', raw, 0)
            print(f"=== {label} STAT size={size} ===")
            print("  words:", [hex(w) for w in words])
            print("  as bytes:", raw[:64].hex())
dump_stat(r"e:\RenoDX\renodx\tmp\senkiseki3\dump\0x0D5DABC6.cso", "ORIGINAL VS")
dump_stat(r"e:\RenoDX\renodx\tmp\senkiseki3\0xFEA2B509.ps_4_1.hlsl.cso", "PS") if False else None
