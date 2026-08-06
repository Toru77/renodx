import struct

def dump_tail(path, label):
    data = open(path,'rb').read()
    n = struct.unpack_from('<I', data, 28)[0]
    offs = [struct.unpack_from('<I', data, 32+4*i)[0] for i in range(n)]
    for off in offs:
        if data[off:off+4] in (b'SHEX', b'SHDR'):
            size = struct.unpack_from('<I', data, off+4)[0]
            d = off+8
            length = struct.unpack_from('<I', data, d+4)[0]   # field at +4
            print(f"=== {label}: SHEX size={size}, length_field={length} (0x{length:x}) ===")
            print(f"    size==length*4? {size == length*4}  size==(length)*4 = {length*4}")
            # tokens start at d+8
            # walk instructions from d+8
            pos = 8
            instr = []
            while pos + 4 <= size:
                token = struct.unpack_from('<I', data, d+pos)[0]
                op = token & 0x7FF
                ln = token >> 24
                instr.append((pos, op, ln))
                if ln == 0:
                    print(f"  !! ZERO length at dword {pos//4}")
                    break
                pos += ln*4
                if op == 62:  # ret
                    break
                if len(instr) > 2000:
                    print("  !! runaway")
                    break
            print(f"  walked {len(instr)} instructions, ended at dword {pos//4} (pos={pos})")
            print(f"  size in dwords = {size//4}")
            if instr:
                print(f"  last 3 instructions:")
                for (p, o, l) in instr[-3:]:
                    toks = ' '.join(f"{struct.unpack_from('<I', data, d+p+i*4)[0]:08X}" for i in range(min(l,6)))
                    print(f"    @dword{p//4}: op={o} len={l} tokens={toks}")
            break

for f, l in [("e:\\RenoDX\\renodx\\tmp\\senkiseki3\\t_identity_osgn.cso","t_identity_osgn (WORKS)"),
             ("e:\\RenoDX\\renodx\\tmp\\senkiseki3\\s1_bt.cso","s1_bt (TDR)"),
             ("e:\\RenoDX\\renodx\\tmp\\senkiseki3\\s2_full.cso","s2_full (TDR)")]:
    dump_tail(f, l)
