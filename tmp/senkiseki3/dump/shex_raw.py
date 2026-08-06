import struct

def shex_raw(path, label):
    data = open(path,'rb').read()
    n = struct.unpack_from('<I', data, 28)[0]
    offs = [struct.unpack_from('<I', data, 32+4*i)[0] for i in range(n)]
    for off in offs:
        if data[off:off+4] in (b'SHEX', b'SHDR'):
            size = struct.unpack_from('<I', data, off+4)[0]
            d = off+8
            print(f"=== {label} chunk@{off:#x} size={size} ===")
            for i in range(0, 24, 4):
                print(f"  +{i:02d}: {struct.unpack_from('<I', data, d+i)[0]:08X}")
            # chunk size relative to header (8 bytes) + version(4) + count(4) + tokens
            cnt = struct.unpack_from('<I', data, d+4)[0]
            print(f"  version={data[d:d+4].hex()} dword_count={cnt} expected_size_if_cnt={8+cnt*4}")
            break

for f, l in [("e:\\RenoDX\\renodx\\tmp\\senkiseki3\\s1_bt.cso","s1_bt (TDR)"),
             ("e:\\RenoDX\\renodx\\tmp\\senkiseki3\\t_identity_osgn.cso","t_identity_osgn (WORKS)"),
             ("e:\\RenoDX\\renodx\\tmp\\senkiseki3\\dt17_osgn.cso","dt17_osgn (WORKS)")]:
    shex_raw(f, l)
