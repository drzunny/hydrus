# -*- coding:utf8 -*-
import sys

from cStringIO import StringIO

from libcpp.string cimport string
from libcpp.vector cimport vector

from cython.operator cimport dereference as iter_decr
from cython.operator cimport preincrement as iter_inc
from cpython cimport PyString_FromStringAndSize, Py_DECREF

__VERSION__ = '0.1.0'

# ----------------------------------------------
#  C++ definitions
# ----------------------------------------------
cdef extern from "../src/base/types.hpp" namespace "hydrus":
    cdef struct RefBuf:
        const char * buf
        size_t       n


cdef extern from "../src/wsgi.h" namespace "hydrus":
    cdef struct WSGIHeader:
        RefBuf name
        RefBuf value


    cdef cppclass WSGIApplication:
        void send(const char * buffer, size_t n)
        void raiseUp(int code)

        const char * SERVER_SOFTWARE
        const char * SERVER_NAME
        const char * SERVER_PROTOCOL
        const char * REQUEST_METHOD
        const char * REMOTE_ADDR
        size_t CONTENT_LENGTH
        int SERVER_PORT
        RefBuf URL
        RefBuf BODY
        vector[WSGIHeader] HEADERS


    ctypedef void(*WSGICallback)(WSGIApplication&)


cdef extern from "../src/hy_core.h":
    void hydrus_setup(WSGICallback callback)
    int hydrus_listen(const char * address, int port)
    void hydrus_run()


# ----------------------------------------------
#  Hydrus Server Wrapper
# ----------------------------------------------
G_hy_app = None

cdef class HydrusFileWrapper:
    pass


cdef class _HydrusResponse:
    cdef const char* status
    cdef WSGIApplication* wsgi
    cdef basestring response_header
    cdef basestring data
    cdef list headers
    cdef dict env

    def __init__(self):
        self.wsgi = NULL
        self.status = NULL
        self.response_header = ''
        self.data = ''
        self.headers = []
        self.env = {}

    cdef void raise_error(self, int code):
        self.wsgi.raiseUp(code)

    cdef dict environ(self):
        env = {'SERVER_SOFTWARE': 'hydrus %s' % __VERSION__}

        cdef bytes url = PyString_FromStringAndSize(self.wsgi.URL.buf, self.wsgi.URL.n)
        cdef bytes body = PyString_FromStringAndSize(self.wsgi.URL.buf, self.wsgi.URL.n)

        cdef int queryPos = url.find('?')
        cdef bytes qs = b'' if queryPos < 0 else url[queryPos+1:]
        cdef bytes path = url if queryPos < 0 else url[:queryPos]

        env['SERVER_NAME'] = self.wsgi.SERVER_NAME
        env['SERVER_PORT'] = self.wsgi.SERVER_PORT
        env['SERVER_PROTOCOL'] = 'HTTP/1.1'
        env['REQUEST_METHOD'] = self.wsgi.REQUEST_METHOD
        env['REMOTE_ADDR'] = self.wsgi.REMOTE_ADDR
        env['PATH_INFO'] = url
        env['QUERY_STRING'] = qs

        # WSGI parameters
        env['wsgi.errors'] = sys.stderr
        env['wsgi.input'] = StringIO(body)
        env['wsgi.file_wrapper'] = None
        env['SCRIPT_NAME'] = ''
        env['wsgi.version'] = (0, 1, 0)
        env['wsgi.multithread'] = False
        env['wsgi.multiprocess'] = True
        env['wsgi.run_once'] = False
        env['wsgi.url_scheme'] = 'http'

        Py_DECREF(url)
        Py_DECREF(body)

        cdef int n = self.wsgi.HEADERS.size()
        for i in range(n):
            name = PyString_FromStringAndSize(self.wsgi.HEADERS[i].name.buf, self.wsgi.HEADERS[i].name.n)
            value = PyString_FromStringAndSize(self.wsgi.HEADERS[i].value.buf, self.wsgi.HEADERS[i].value.n)
            env[name] = value
            Py_DECREF(name)
            Py_DECREF(value)
        self.env = env
        print(env)
        return env

    def write(self, bytes data):
        if not self.response_header:
            for name, val in self.headers:
                self.response_header += '%s:%s\r\n' % (name, val)
            self.wsgi.send(self.response_header, len(self.response_header))
        if data:
            self.wsgi.send(data, len(data))


    def start_response(self, const char * status, headers, exec_info=None):
        if exec_info:
            try:
                if self.response_header:
                    raise exec_info[0], exec_info[1], exec_info[2]
            except:
                exec_info = None
        self.status = status
        self.data = 'HTTP/1.1 %s\r\n' % status
        self.headers = headers
        self.write('\r\n')


cdef void _hydrus_response_callback(WSGIApplication & wsgi):
    print('callback here')
    response = _HydrusResponse()
    print('callback here2')
    response.wsgi = &wsgi
    environ = response.environ()
    print('callback here3')

    try:
        retval = G_hy_app(environ, response.start_response)
        print('callback here4')
        if not response.response_header:
            print('callback here5.0')
            response.raise_error(400)
            print('callback here5.1')
        else:
            print('callback here6.0')
            for rs in retval:
                wsgi.send(rs, len(rs))
            print('callback here6.1')
    except:
        import traceback
        traceback.print_exc()
        response.raise_error(500)


# ----------------------------------------------
#  Main API of hydrus
# ----------------------------------------------
def listen(app, const char* addr, int port):
    global G_hy_app
    if G_hy_app is not None:
        return
    G_hy_app = app
    hydrus_setup(_hydrus_response_callback)
    hydrus_listen(addr, port)


def run():
    hydrus_run()
