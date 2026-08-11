# Disassemble a DXBC shader blob (.cso) via D3DDisassemble (d3dcompiler_47.dll).
# Usage: python disasm_dxbc.py <file.cso> [out.asm]
import ctypes
import ctypes.wintypes as wt
import sys
import os

d3dcompiler = ctypes.WinDLL("d3dcompiler_47.dll")

# HRESULT D3DDisassemble(LPCVOID pSrcData, SIZE_T SrcDataSize, UINT Flags,
#                        LPCSTR szComments, ID3DBlob **ppDisassembly)
D3DDisassemble = d3dcompiler.D3DDisassemble
D3DDisassemble.restype = ctypes.c_long
D3DDisassemble.argtypes = [
    ctypes.c_void_p,
    ctypes.c_size_t,
    ctypes.c_uint,
    ctypes.c_char_p,
    ctypes.POINTER(ctypes.c_void_p),
]


class Blob:
    """Thin wrapper over an ID3DBlob* to read its buffer."""
    def __init__(self, ptr):
        self.ptr = ptr
        self.obj = ctypes.c_void_p(ptr)

    @property
    def buffer_pointer(self):
        # ID3DBlob vtable: 0=QI,1=AddRef,2=Release,3=GetBufferPointer,4=GetBufferSize
        vtbl = ctypes.cast(self.ptr, ctypes.POINTER(ctypes.c_void_p)).contents.value
        fn = ctypes.cast(
            ctypes.cast(vtbl + 3 * ctypes.sizeof(ctypes.c_void_p),
                        ctypes.POINTER(ctypes.c_void_p)).contents.value,
            ctypes.CFUNCTYPE(ctypes.c_void_p, ctypes.c_void_p))
        return fn(self.ptr)

    @property
    def buffer_size(self):
        vtbl = ctypes.cast(self.ptr, ctypes.POINTER(ctypes.c_void_p)).contents.value
        fn = ctypes.cast(
            ctypes.cast(vtbl + 4 * ctypes.sizeof(ctypes.c_void_p),
                        ctypes.POINTER(ctypes.c_void_p)).contents.value,
            ctypes.CFUNCTYPE(ctypes.c_size_t, ctypes.c_void_p))
        return fn(self.ptr)

    def read(self):
        n = self.buffer_size
        if n == 0:
            return b""
        return ctypes.string_at(self.buffer_pointer, n)

    def release(self):
        if self.ptr:
            ctypes.cast(self.ptr, ctypes.POINTER(ctypes.c_void_p)).contents  # noqa
            # Call Release (vtable index 2)
            vtbl = ctypes.cast(self.ptr, ctypes.POINTER(ctypes.c_void_p)).contents.value
            fn = ctypes.cast(
                ctypes.cast(vtbl + 2 * ctypes.sizeof(ctypes.c_void_p),
                            ctypes.POINTER(ctypes.c_void_p)).contents.value,
                ctypes.CFUNCTYPE(ctypes.c_ulong, ctypes.c_void_p))
            fn(self.ptr)
            self.ptr = 0


def disassemble(data: bytes) -> bytes:
    src = ctypes.create_string_buffer(data, len(data))
    blob_ptr = ctypes.c_void_p(0)
    hr = D3DDisassemble(ctypes.cast(src, ctypes.c_void_p), len(data), 0, None,
                        ctypes.byref(blob_ptr))
    if hr < 0 or not blob_ptr.value:
        raise RuntimeError(f"D3DDisassemble failed hr=0x{hr & 0xFFFFFFFF:08X}")
    b = Blob(blob_ptr.value)
    out = b.read()
    b.release()
    return out


def main():
    src = sys.argv[1]
    with open(src, "rb") as f:
        data = f.read()
    asm = disassemble(data)
    text = asm.decode("utf-8", errors="replace")
    if len(sys.argv) > 2:
        with open(sys.argv[2], "w", encoding="utf-8") as f:
            f.write(text)
        print(f"wrote {sys.argv[2]} ({len(asm)} bytes)")
    else:
        print(text)


if __name__ == "__main__":
    main()
