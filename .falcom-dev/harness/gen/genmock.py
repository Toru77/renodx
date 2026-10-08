import re, sys
src = open(sys.argv[1] if len(sys.argv) > 1 else '../../../external/reshade/include/reshade_api_device.hpp').read()
def body(name):
    m = re.search(r'struct __declspec\(novtable\) ' + name + r'\b[^{]*\{', src)
    i = m.end(); depth = 1
    while depth:
        c = src[i]
        if c == '{': depth += 1
        elif c == '}': depth -= 1
        i += 1
    return src[m.end():i-1]
def decls(name):
    b = re.sub(r'//[^\n]*', '', body(name))
    out = []
    for m in re.finditer(r'virtual\s+(.*?)=\s*0\s*;', b, re.S):
        d = ' '.join(m.group(1).split())
        ret = d.split('(')[0].rsplit(' ', 1)[0].strip()
        ret = re.sub(r'\s+', ' ', ret)
        if ret == 'void':
            out.append('  ' + d + ' override {}')
        else:
            out.append('  ' + d + ' override { return {}; }')
    return out
def emit(cls, bases):
    lines = ['struct %s : reshade::api::%s {' % (cls, bases[-1])]
    for b in bases: lines += decls(b)
    lines.append('};')
    return '\n'.join(lines)
print('#pragma once\n#include <include/reshade.hpp>\nnamespace mock {\nusing namespace reshade::api;')
print(emit('DeviceBase', ['api_object', 'device']))
print(emit('CommandListBase', ['api_object', 'device_object', 'command_list']))
print(emit('QueueBase', ['api_object', 'device_object', 'command_queue']))
print('}')
