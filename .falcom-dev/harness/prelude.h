#pragma once
#define __declspec(x)
template <class T> struct UuidStub { static inline const unsigned char v[16] = {}; };
#define __uuidof(T) UuidStub<T>::v
// Shader bytecode used by the classifier tests (build.sh passes the absolute path).
#ifndef FALCOM_BYTECODE_DIR
#define FALCOM_BYTECODE_DIR "../bytecode/"
#endif
