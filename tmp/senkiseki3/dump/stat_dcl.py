import struct
def get_dcl_temps(path):
    data = open(path,'rb').read()
    n = struct.unpack_from('<I', data, 28)[0]
    offs = [struct.unpack_from('<I', data, 32+4*i)[0] for i in range(n)]
    for off in offs:
        name = data[off:off+4].decode('ascii')
        size = struct.unpack_from('<I', data, off+4)[0]
        if name in ('SHEX','SHDR'):
            d = off+8
            pos = 8
            end = size
            temps = None
            while pos + 4 <= end:
                token = struct.unpack_from('<I', data, d+pos)[0]
                op = token & 0x7FF
                ln = token >> 24
                if op == 104 and ln >= 2:  # DCL_TEMPS
                    temps = struct.unpack_from('<I', data, d+pos+4)[0]
                if ln == 0: break
                pos += ln*4
            return temps, size
    return None, None
def get_stat(path):
    data = open(path,'rb').read()
    n = struct.unpack_from('<I', data, 28)[0]
    offs = [struct.unpack_from('<I', data, 32+4*i)[0] for i in range(n)]
    for off in offs:
        name = data[off:off+4].decode('ascii')
        size = struct.unpack_from('<I', data, off+4)[0]
        if name == 'STAT':
            d = off+8
            # STAT layout: InstructionCount, TempReg, TempArray, CBufCount, ResCount, SamplerCount, InterfaceSlots, UAVs, ...
            return struct.unpack_from('<16I', data, d)
    return None
for label, path in [("FRESH (works)", r"e:\RenoDX\renodx\tmp\senkiseki3\fresh_face.cso"),
                    ("ORIGINAL", r"e:\RenoDX\renodx\tmp\senkiseki3\dump\0x0D5DABC6.cso"),
                    ("OUR PATCH (crashes)", r"e:\RenoDX\renodx\tmp\senkiseki3\0x0D5DABC6.minimal.cso")]:
    temps, shexsize = get_dcl_temps(path)
    stat = get_stat(path)
    print(f"{label}: dcl_temps={temps} SHEX={shexsize}B")
    if stat:
        print(f"   STAT: instr={stat[0]} temp={stat[1]} cbuffers={stat[3]} resources={stat[4]} samplers={stat[5]} iface={stat[6]} uav={stat[7]}")
