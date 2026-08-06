import struct

def tokens(path):
    data = open(path,'rb').read()
    n = struct.unpack_from('<I', data, 28)[0]
    offs = [struct.unpack_from('<I', data, 32+4*i)[0] for i in range(n)]
    shex = None
    for off in offs:
        if data[off:off+4] in (b'SHEX', b'SHDR'):
            shex = off
            break
    size = struct.unpack_from('<I', data, shex+4)[0]
    d = shex+8
    cnt = struct.unpack_from('<I', data, d+4)[0]
    return [struct.unpack_from('<I', data, d+8+4*i)[0] for i in range(cnt)]

a = tokens(r"e:\RenoDX\renodx\tmp\senkiseki3\t_identity_osgn.cso")
b = tokens(r"e:\RenoDX\renodx\tmp\senkiseki3\s1_bt.cso")
print(f"t_identity_osgn tokens={len(a)}, s1_bt tokens={len(b)}")
# find first difference
i = 0
while i < min(len(a), len(b)) and a[i] == b[i]:
    i += 1
print(f"first diff at token idx {i} (0x{i*4:x} bytes into stream)")
# print context around the difference
lo = max(0, i-8)
hi = min(max(len(a), len(b)), i+12)
for j in range(lo, hi):
    va = a[j] if j < len(a) else None
    vb = b[j] if j < len(b) else None
    mark = " <--" if va != vb else ""
    print(f"  [{j:4d}] a={va:08X} b={vb:08X}{mark}")
