import struct
def count_instr(path, incl_decl):
    data = open(path,'rb').read()
    n = struct.unpack_from('<I', data, 28)[0]
    offs = [struct.unpack_from('<I', data, 32+4*i)[0] for i in range(n)]
    for off in offs:
        name = data[off:off+4].decode('ascii')
        size = struct.unpack_from('<I', data, off+4)[0]
        if name in ('SHEX','SHDR'):
            d = off+8
            pos = 8
            total = 0
            body = 0
            while pos + 4 <= size:
                token = struct.unpack_from('<I', data, d+pos)[0]
                op = token & 0x7FF
                ln = token >> 24
                if ln == 0: break
                total += 1
                if not (88 <= op <= 106) and op != 162 and op != 107 and op != 163:
                    body += 1
                pos += ln*4
            return total, body
    return None, None
for label, path in [("ORIGINAL", r"e:\RenoDX\renodx\tmp\senkiseki3\dump\0x0D5DABC6.cso"),
                    ("OUR PATCH", r"e:\RenoDX\renodx\tmp\senkiseki3\0x0D5DABC6.minimal.cso")]:
    total, body = count_instr(path, True)
    print(f"{label}: total_instr={total} non_decl_instr={body}")
