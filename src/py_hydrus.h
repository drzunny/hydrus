#ifndef PY_HYDRUS_CORE_H
#define PY_HYDRUS_CORE_H

/* ------------------------------------------ *
 *  This is a Python API export module
 * ------------------------------------------ */

#ifdef __cplusplus
extern "C" {
#endif

typedef void(PyObject*) WSGICallback

void hydrus_listen(const char * addr, int port, int ip6_enabled);
void hydrus_run(WSGICallback app);

#ifdef __cplusplus
}
#endif

#endif
