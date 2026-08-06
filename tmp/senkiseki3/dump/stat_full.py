import struct
def dump_stat(path, label):
    data = open(path,'rb').read()
    n = struct.unpack_from('<I', data, 28)[0]
    offs = [struct.unpack_from('<I', data, 32+4*i)[0] for i in range(n)]
    for off in offs:
        name = data[off:off+4].decode('ascii')
        size = struct.unpack_from('<I', data, off+4)[0]
        if name == 'STAT':
            d = off+8
            words = struct.unpack_from('<29I', data, d)
            print(f"=== {label} STAT size={size} ({len(words)} dwords) ===")
            names = ["InstructionCount","TempRegisterCount","TempArrayCount","DefCount","DclCount",
                     "TexNormal","TexLoad","TexComp","TexBias","TexGradient",
                     "FloatInstr","IntInstr","UintInstr","StaticFlow","DynamicFlow",
                     "MacroInstr","ArrayInstr","CutInstr","EmitInstr","GSOutputTopo",
                     "GSMaxOutVtx","InputPrim","PatchConst","GSInstance","ControlPoints",
                     "HSOutPrim","HSPartition","TessDomain","Barrier"]
            for i,w in enumerate(words):
                print(f"  [{i:2d}] {names[i] if i < len(names) else '?'} = {w} (0x{w:x})")
dump_stat(r"e:\RenoDX\renodx\tmp\senkiseki3\dump\0x0D5DABC6.cso", "ORIGINAL VS")
dump_stat(r"e:\RenoDX\renodx\tmp\senkiseki3\fresh_face.cso", "FRESH-HLSL VS (WORKS)")
