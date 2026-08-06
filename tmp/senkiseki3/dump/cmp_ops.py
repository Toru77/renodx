import struct, sys

def shex_tokens(path):
    data = open(path,'rb').read()
    n = struct.unpack_from('<I', data, 28)[0]
    offs = [struct.unpack_from('<I', data, 32+4*i)[0] for i in range(n)]
    shex = None
    for off in offs:
        name = data[off:off+4].decode('ascii')
        if name in ('SHEX','SHDR'):
            shex = off
            break
    size = struct.unpack_from('<I', data, shex+4)[0]
    d = shex+8  # chunk data start (version/type + count)
    toks = []
    pos = 8
    while pos + 4 <= size:
        token = struct.unpack_from('<I', data, d+pos)[0]
        op = token & 0x7FF
        ln = token >> 24
        if ln == 0:
            print(f"  !! zero-length instruction at dword {pos//4} op={op}")
            break
        toks.append((pos, op, ln))
        if op == 62:  # ret
            break
        pos += ln*4
    return data, d, toks

def dump_ops(path, label):
    data, d, toks = shex_tokens(path)
    print(f"=== {label} ===")
    for (pos, op, ln) in toks:
        if op in (104,101,103,102,89,162,106,95,88) or op==0:
            reg = ""
            if op in (101,103,102):
                reg = struct.unpack_from('<I', data, d+pos+8)[0]
            dwords = ' '.join(f"{struct.unpack_from('<I', data, d+pos+i*4)[0]:08X}" for i in range(min(ln,4)))
            print(f"  @{pos//4:5d}: op={op:3d} len={ln} {dwords}{' reg='+str(reg) if reg!='' else ''}")

for f, l in [("e:\\RenoDX\\renodx\\tmp\\senkiseki3\\fresh_face.cso","fresh_face (WORKS)"),
             ("e:\\RenoDX\\renodx\\tmp\\senkiseki3\\s1_bt.cso","s1_bt (TDR)"),
             ("e:\\RenoDX\\renodx\\tmp\\senkiseki3\\t_identity_osgn.cso","t_identity_osgn (WORKS)")]:
    dump_ops(f, l)
