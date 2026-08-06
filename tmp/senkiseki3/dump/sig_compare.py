import struct
def dump_sig(path, label):
    data = open(path,'rb').read()
    n = struct.unpack_from('<I', data, 28)[0]
    offs = [struct.unpack_from('<I', data, 32+4*i)[0] for i in range(n)]
    print(f"--- {label} ({len(data)} bytes) ---")
    for off in offs:
        name = data[off:off+4].decode('ascii')
        size = struct.unpack_from('<I', data, off+4)[0]
        if name in ('OSGN','ISGN'):
            d = off+8
            cnt = struct.unpack_from('<I', data, d)[0]
            for j in range(cnt):
                e = d+8+j*24
                no, si, sv, ct, reg = struct.unpack_from('<IIIII', data, e)
                mask, rw = data[e+20], data[e+21]
                s = data[d+no:d+no+40].split(b'\0')[0].decode('ascii','replace')
                print(f"  {name}[{j}] {s} idx={si} type={ct} reg={reg} mask=0x{mask:02X} rw=0x{rw:02X}")
dump_sig(r"e:\RenoDX\renodx\tmp\senkiseki3\dump\0x0D5DABC6.cso", "ORIGINAL face VS")
dump_sig(r"e:\RenoDX\renodx\tmp\senkiseki3\0x0D5DABC6.minimal.cso", "OUR minimal patch")
