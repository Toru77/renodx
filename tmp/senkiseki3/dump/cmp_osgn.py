import struct

def osgn_dump(path, label):
    data = open(path,'rb').read()
    n = struct.unpack_from('<I', data, 28)[0]
    offs = [struct.unpack_from('<I', data, 32+4*i)[0] for i in range(n)]
    osgn = None
    for off in offs:
        if data[off:off+4] == b'OSGN':
            osgn = off
            break
    cnt = struct.unpack_from('<I', data, osgn+8)[0]
    print(f"=== {label} OSGN count={cnt} ===")
    strs = data[osgn+16+cnt*24 : osgn+16+cnt*24+128]
    for e in range(cnt):
        off = osgn + 16 + e*24
        nameoff, semidx, sysv, comptype, reg, mask = struct.unpack_from('<6I', data, off)
        # name is relative to chunk data start (osgn+8)
        nameoff_abs = osgn + 8 + nameoff
        nm = data[nameoff_abs:nameoff_abs+40].split(b'\x00')[0].decode('ascii','replace')
        print(f"  e{e}: nameOff={nameoff:#x} semIdx={semidx} sysv={sysv} compType={comptype} reg={reg} mask={mask:#x} name='{nm}'")
    print()

for f, l in [("e:\\RenoDX\\renodx\\tmp\\senkiseki3\\fresh_face.cso","fresh_face (WORKS)"),
             ("e:\\RenoDX\\renodx\\tmp\\senkiseki3\\s2_full.cso","s2_full (TDR)"),
             ("e:\\RenoDX\\renodx\\tmp\\senkiseki3\\dt17_osgn.cso","dt17_osgn (WORKS)")]:
    osgn_dump(f, l)
