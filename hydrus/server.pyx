# -*- coding:utf8 -*-
import sys

from cStringIO import StringIO

from libcpp.string cimport string
from libcpp.vector cimport vector

from cpython cimport PyBytes_FromStringAndSize

__VERSION__ = '0.1.0'
__HY_VERSION__ = tuple([int(x) for x in __VERSION__.split('.')])


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

        string SERVER_NAME
        string REQUEST_METHOD
        string REMOTE_ADDR
        int SERVER_PORT
        bint SERVER_CLOSED
        string URL
        vector[char] BODY
        string CONTENT_TYPE
        unsigned long long CONTENT_LENGTH
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


cdef class _HydrusResponse:
    cdef str status
    cdef WSGIApplication* wsgi
    cdef list headers
    cdef dict env
    cdef bint has_header_complete

    def __init__(self):
        self.wsgi = NULL
        self.status = None
        self.headers = None
        self.has_header_complete = 0
        self.env = {}

    cdef void raise_error(self, int code):
        self.wsgi.raiseUp(code)

    cdef dict environ_vars(self):
        cdef dict env = {}
        cdef bytes url = <bytes>self.wsgi.URL
        cdef bytes body = PyBytes_FromStringAndSize(self.wsgi.BODY.data(), self.wsgi.BODY.size())
        cdef bytes server_name = <bytes>self.wsgi.SERVER_NAME
        cdef int server_port = self.wsgi.SERVER_PORT
        cdef bytes request_method = <bytes>self.wsgi.REQUEST_METHOD
        cdef bytes remote_addr = <bytes>self.wsgi.REMOTE_ADDR
        cdef bytes content_type = <bytes>self.wsgi.CONTENT_TYPE
        cdef unsigned long long content_length = self.wsgi.CONTENT_LENGTH

        cdef int queryPos = url.find('?')
        cdef bytes qs = b'' if queryPos < 0 else url[queryPos+1:]
        cdef bytes path = url if queryPos < 0 else url[:queryPos]

        env['SERVER_SOFTWARE'] = 'hydrus %s' % __VERSION__
        env['SERVER_NAME'] = server_name
        env['SERVER_PORT'] = server_port
        env['SERVER_PROTOCOL'] = 'HTTP/1.1'
        env['REQUEST_METHOD'] = request_method
        env['REMOTE_ADDR'] = remote_addr
        env['PATH_INFO'] = path
        env['QUERY_STRING'] = qs
        env['CONTENT_LENGTH'] = content_length
        env['CONTENT_TYPE'] = content_type

        # WSGI parameters
        env['wsgi.errors'] = sys.stderr
        env['wsgi.input'] = StringIO(body)
        
        # env['wsgi.file_wrapper'] = None
        env['SCRIPT_NAME'] = ''
        env['wsgi.version'] = __HY_VERSION__
        env['wsgi.multithread'] = False
        env['wsgi.multiprocess'] = True
        env['wsgi.run_once'] = False
        env['wsgi.url_scheme'] = 'http'

        # Set request header
        cdef int n = self.wsgi.HEADERS.size()
        for i in range(n):
            name = self.wsgi.HEADERS[i].name.c_str()
            value = self.wsgi.HEADERS[i].value.c_str()           
            # set request header value to HTTP_variable
            env['_'.join(('HTTP', name.upper().replace('-', '_')))] = value

        self.env = env
        return env

    def send_http_header(self):
        cdef bint hasContentLength = 0
        cdef bint hasTransferEncoding = 0
        cdef bint hasConnectionClosed = 0
        cdef list headers = [self.status]
        cdef basestring header_buffer = None
        
        if self.has_header_complete:
            return
        
        for it in self.headers:
            name, value = it
            if name == 'Content-Length':
                hasContentLength = 1
            elif name == 'Transfer-Encoding':
                hasTransferEncoding = 1
            elif name == 'Connection' and value.lower() == 'close':
                self.wsgi.SERVER_CLOSED = 1
                hasConnectionClosed = 1
            
            headers.append(':'.join(it))
        
        # is keepalive connection ?
        if self.wsgi.keepalive():
            # if both `Transfer-Encoding` and `Content-Length` were not specified, close connection 
            if not hasContentLength and not hasTransferEncoding:
                self.wsgi.SERVER_CLOSED = 1
                headers.append('Connection:close')            
        else:
            # if `Connection:close` was not specified, but not keepalive (For HTTP 1.1)
            if not hasConnectionClosed:
                self.wsgi.SERVER_CLOSED = 1
                headers.append('Connection:close')            
        
        headers.append('\r\n')
        header_buffer = '\r\n'.join(headers)        
        self.wsgi.send(header_buffer, len(header_buffer))
        self.has_header_complete = True

    def start_response(self, const char * status, headers, exec_info=None):
        if exec_info:
            try:
                if self.has_header_complete:
                    raise exec_info[0], exec_info[1], exec_info[2]
            except:
                exec_info = None

        # Add special headers
        headers.append(('Server', self.env['SERVER_SOFTWARE']))

        self.status = 'HTTP/1.1 %s' % status
        self.headers = headers
        self.send_http_header()


cdef void _hydrus_response_callback(WSGIApplication & wsgi):
    cdef _HydrusResponse response = _HydrusResponse()
    response.wsgi = &wsgi
    
    cdef dict environ = response.environ_vars()

    try:
        retval = G_hy_app(environ, response.start_response)
        if not retval:
            wsgi.send(' ', 1)
        else:
            for rs in retval:
                if not rs:
                    continue
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
    if not callable(app):
        raise RuntimeError('WSGI application must be callable')
    if G_hy_app is not None:
        return
    G_hy_app = app
    hydrus_setup(_hydrus_response_callback)
    hydrus_listen(addr, port)


def run():
    try:
        hydrus_run()
    except KeyboardInterrupt:
        print('Keyboard Interrupt, Good Bye :-)')
