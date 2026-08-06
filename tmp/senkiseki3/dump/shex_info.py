import struct

def shex_info(path):
    data = open(path,'rb').read()
    n = struct.unpack_from('<I', data, 28)[0]
    offs = [struct.unpack_from('<I', data, 32+4*i)[0] for i in range(n)]
    for off in offs:
        name = data[off:off+4].decode('ascii')
        if name in ('SHEX','SHDR'):
            size = struct.unpack_from('<I', data, off+4)[0]
            d = off+8
            ver = struct.unpack_from('<I', data, d)[0]
            cnt = struct.unpack_from('<I', data, d+4)[0]
            return name, size, hex(ver), cnt, len(data)
    return None

for f, l in [("e:\\RenoDX\\renodx\\tmp\\senkiseki3\\dt17.cso","dt17 (WORKS)"),
             ("e:\\RenoDX\\renodx\\tmp\\senkiseki3\\dt17_osgn.cso","dt17_osgn (WORKS)"),
             ("e:\\RenoDX\\renodx\\tmp\\senkiseki3\\t_identity_osgn.cso","t_identity_osgn (WORKS)"),
             ("e:\\RenoDX\\renodx\\tmp\\senkiseki3\\s1_bt.cso","s1_bt (TDR)"),
             ("e:\\RenoDX\\renodx\\tmp\\senkiseki3\\s2_full.cso","s2_full (TDR)"),
             ("e:\\RenoDX\\renodx\\tmp\\senkiseki3\\fresh_face.cso","fresh_face (WORKS)"),
             ("e:\\RenoDX\\renodx\\tmp\\senkiseki3\\dump\\0x0D5DABC6.cso","TRUE ORIGINAL")]:
    info = shex_info(f)
    print(f"{l:22s}: chunk={info[0]} size={info[1]} ver={info[2]} dword_count={info[3]} file={info[4]}")
