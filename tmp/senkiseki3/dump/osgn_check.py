import struct
path = r"e:\RenoDX\renodx\tmp\senkiseki3\dump\0x0D5DABC6.cso"
data = open(path,'rb').read()
# header: magic(4) hash(16) version(4) filesize(4) chunkcount(4) = 32, then offsets
n = struct.unpack_from('<I', data, 28)[0]
offs = [struct.unpack_from('<I', data, 32+4*i)[0] for i in range(n)]
for off in offs:
    name = data[off:off+4].decode('ascii')
    size = struct.unpack_from('<I', data, off+4)[0]
    if name in ('ISGN','OSGN'):
        d = off+8
        cnt = struct.unpack_from('<I', data, d)[0]
        print(f"=== {name} count={cnt} size={size}")
        for j in range(cnt):
            e = d+8+j*24
            no, si, sv, ct, reg = struct.unpack_from('<IIIII', data, e)
            mask, rw = data[e+20], data[e+21]
            s = data[d+no:d+no+40].split(b'\0')[0].decode('ascii','replace')
            print(f"  [{j}] name={s!r} idx={si} sysval={sv} comptype={ct} reg={reg} mask=0x{mask:02X} rw=0x{rw:02X}")
