import struct, hashlib

def fix_hash(data):
    # DXBC checksum: MD5 over everything from `version` (byte 20) onward.
    h = hashlib.md5()
    h.update(data[20:])
    data[4:20] = h.digest()
    return data

def patch_temps(src, dst, new_temps, fix_stat):
    data = bytearray(open(src,'rb').read())
    n = struct.unpack_from('<I', data, 28)[0]
    offs = [struct.unpack_from('<I', data, 32+4*i)[0] for i in range(n)]
    for off in offs:
        name = data[off:off+4].decode('ascii')
        size = struct.unpack_from('<I', data, off+4)[0]
        if name in ('SHEX','SHDR'):
            d = off+8
            pos = 8
            while pos + 4 <= size:
                token = struct.unpack_from('<I', data, d+pos)[0]
                op = token & 0x7FF
                ln = token >> 24
                if op == 104 and ln >= 2:
                    struct.pack_into('<I', data, d+pos+4, new_temps)
                    print(f"  patched dcl_temps -> {new_temps}")
                if ln == 0: break
                pos += ln*4
        if name == 'STAT' and fix_stat:
            struct.pack_into('<I', data, off+8+4, new_temps)
            print(f"  patched STAT temp -> {new_temps}")
    fix_hash(data)
    open(dst,'wb').write(data)

src = r"e:\RenoDX\renodx\tmp\senkiseki3\dump\0x0D5DABC6.cso"
patch_temps(src, r"e:\RenoDX\renodx\tmp\senkiseki3\dt17.cso", 17, True)
patch_temps(src, r"e:\RenoDX\renodx\tmp\senkiseki3\dt17_stat5.cso", 17, False)
