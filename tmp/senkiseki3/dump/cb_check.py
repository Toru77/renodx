import struct
path = r"e:\RenoDX\renodx\tmp\senkiseki3\dump\0x0D5DABC6.cso"
data = open(path,'rb').read()
n = struct.unpack_from('<I', data, 28)[0]
offs = [struct.unpack_from('<I', data, 32+4*i)[0] for i in range(n)]
for off in offs:
    name = data[off:off+4].decode('ascii')
    size = struct.unpack_from('<I', data, off+4)[0]
    if name == 'SHEX':
        d = off+8
        pos = 8
        end = size
        print("=== SHEX dcl_constantbuffer declarations ===")
        while pos + 4 <= end:
            token = struct.unpack_from('<I', data, d+pos)[0]
            op = token & 0x7FF
            ln = token >> 24
            if op == 89 and ln >= 4:  # DCL_CONSTANT_BUFFER
                slot = struct.unpack_from('<I', data, d+pos+8)[0]
                cnt = struct.unpack_from('<I', data, d+pos+12)[0]
                print(f"  cb{slot} count={cnt} (regs {slot}..{slot+cnt-1})  @dword {pos//4}")
            if ln == 0: break
            pos += ln*4
    if name == 'RDEF':
        d = off+8
        cb_count = struct.unpack_from('<I', data, d+0)[0]
        cb_off = struct.unpack_from('<I', data, d+4)[0]
        print("=== RDEF cbuffers ===")
        for i in range(cb_count):
            e = d + cb_off + i*24
            no, varcount, varoff, size, flags, ctype = struct.unpack_from('<IIIIII', data, e)
            nm = data[d+no:d+no+48].split(b'\0')[0].decode('ascii','replace')
            print(f"  {nm}: size={size} ({size//16} regs) vars={varcount}")
