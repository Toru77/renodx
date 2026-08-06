import struct

def chunks(path, label):
    data = open(path,'rb').read()
    n = struct.unpack_from('<I', data, 28)[0]
    offs = [struct.unpack_from('<I', data, 32+4*i)[0] for i in range(n)]
    print(f"=== {label}: {len(data)} bytes, {n} chunks ===")
    prev_end = 32 + 4*n
    for i, off in enumerate(offs):
        name = data[off:off+4].decode('ascii','replace')
        size = struct.unpack_from('<I', data, off+4)[0]
        gap = off - prev_end
        print(f"  [{i}] {name} @{off:#x} size={size} end={off+8+size:#x} gap_from_prev={gap}")
        prev_end = off + 8 + size
    print(f"  file_end = {len(data):#x}, last_chunk_end = {prev_end:#x}, trailing = {len(data)-prev_end}")

for f, l in [("e:\\RenoDX\\renodx\\tmp\\senkiseki3\\dump\\0x0D5DABC6.cso","TRUE ORIGINAL"),
             ("e:\\RenoDX\\renodx\\tmp\\senkiseki3\\s1_bt.cso","s1_bt (TDR)"),
             ("e:\\RenoDX\\renodx\\tmp\\senkiseki3\\t_identity_osgn.cso","t_identity_osgn (WORKS)")]:
    chunks(f, l)
