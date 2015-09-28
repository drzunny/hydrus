# -*- coding:utf8 -*-
import sys

from cStringIO import StringIO

from libcpp.string cimport string
from libcpp.vector cimport vector

from cython.operator cimport dereference as iter_decr
from cython.operator cimport preincrement as iter_inc

__VERSION__ = '0.1.0'


# ----------------------------------------------
#  Helper
# ----------------------------------------------
cdef _file_size(f):
    f.seek(0, 2)
    n = f.tell()
    f.seek(0)
    return n


# ----------------------------------------------
#  C++ definitions
# ----------------------------------------------
cdef extern from "../src/wsgi.h" namespace "hydrus":
    cdef struct WSGIHeader:
        string name
        string value


    cdef cppclass WSGIApplication:
        void send(const char * buffer, size_t n)
        void sendFile(int fileFd, size_t n)
        void raiseUp(int code)
        bint keepalive()

        string SERVER_SOFTWARE
        string SERVER_NAME
        string SERVER_PROTOCOL
        string REQUEST_METHOD
        string REMOTE_ADDR
        size_t CONTENT_LENGTH
        int SERVER_PORT
        string URL
        string BODY
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
    cdef str status
    cdef WSGIApplication* wsgi
    cdef basestring response_content
    cdef list headers
    cdef dict env

    def __init__(self):
        self.wsgi = NULL
        self.status = None
        self.response_content = ''
        self.headers = []
        self.env = {}

    cdef void raise_error(self, int code):
        self.wsgi.raiseUp(code)

    cdef dict environ(self):
        env = {'SERVER_SOFTWARE': 'hydrus %s' % __VERSION__}

        cdef bytes url = <bytes>self.wsgi.URL
        cdef bytes body = <bytes>self.wsgi.BODY
        cdef bytes server_name = <bytes>self.wsgi.SERVER_NAME
        cdef int server_port = self.wsgi.SERVER_PORT
        cdef bytes request_method = <bytes>self.wsgi.REQUEST_METHOD
        cdef bytes remote_addr = <bytes>self.wsgi.REMOTE_ADDR

        cdef int queryPos = url.find('?')
        cdef bytes qs = b'' if queryPos < 0 else url[queryPos+1:]
        cdef bytes path = url if queryPos < 0 else url[:queryPos]

        env['SERVER_NAME'] = server_name
        env['SERVER_PORT'] = str(server_port)
        env['SERVER_PROTOCOL'] = 'HTTP/1.1'
        env['REQUEST_METHOD'] = request_method
        env['REMOTE_ADDR'] = remote_addr
        env['PATH_INFO'] = path
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

        cdef int n = self.wsgi.HEADERS.size()
        for i in range(n):
            name = self.wsgi.HEADERS[i].name.c_str()
            value = self.wsgi.HEADERS[i].value.c_str()
            env[name] = value

        self.env = env
        return env

    def write(self, bytes data):
        if not self.response_content:
            if not self.wsgi.keepalive():
                self.headers.append(('Connection', 'Close'))
            for name, val in self.headers:
                self.response_content += '%s:%s\r\n' % (name, val)
            header = '%s%s' % (self.status, self.response_content)
            self.wsgi.send(header, len(header))
        if data:
             self.wsgi.send(data, len(data))


    def start_response(self, const char * status, headers, exec_info=None):
        if exec_info:
            try:
                if self.response_content:
                    raise exec_info[0], exec_info[1], exec_info[2]
            except:
                exec_info = None
        self.status = 'HTTP/1.1 %s\r\n' % status
        self.headers = headers
        self.write(b'\r\n')


cdef void _hydrus_response_callback(WSGIApplication & wsgi):
    response = _HydrusResponse()
    response.wsgi = &wsgi
    environ = response.environ()

    try:
        retval = G_hy_app(environ, response.start_response)
        if not response.response_content:
            response.raise_error(400)
        else:
            for rs in retval:
                if isinstance(rs, file):
                    nread = _file_size(rs)
                    wsgi.sendFile(rs.fileno(), nread)
                else:
                    wsgi.send(rs, len(rs))
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
    try:
        hydrus_run()
    except KeyboardInterrupt:
        print('Bye :-)')
