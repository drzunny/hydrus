# -*- coding:utf8 -*-

from libcpp.string cimport string
from libcpp.vector cimport vector

from cython.operator cimport dereference as iter_decr
from cython.operator cimport preincrement as iter_inc

__VERSION__ = '0.1.0'

CONST_HYDRUS_ERROR_RESPONSE = {
    400: "HTTP/1.1 400 Bad Request\r\n\r\n",
    500: "HTTP/1.1 500 Internal Server Error\r\n\r\n",
    404: "HTTP/1.1 404 Not Found\r\n\r\n"
}

# ----------------------------------------------
#  C++ definitions
# ----------------------------------------------
cdef extern from "request.h" namespace "hydrus":
    cdef cppclass Headers:
        string name
        string value

    cdef cppclass Request:
        string url
        string method
        size_t status_code
        vector[Headers] headers
        char * body
        bint keepalive



cdef extern from "server.h" namespace "hydrus":
    cdef cppclass Client:
        void send(const char * buf, size_t n) nogil
        const char * remoteAddress() nogil

    ctypedef void (*WSGICallback)(const Client&, const Request&)

    cdef cppclass Server:
        Server(WSGICallback) nogil
        void listen(const char*, int) nogil
        void run()


# Global Instance
cdef Server* G_hy_server = NULL
G_hy_app = None

# ----------------------------------------------
#  Hydrus Server Wrapper
# ----------------------------------------------
cdef class _HydrusResponse:
    cdef const char* status
    cdef const Client* client
    cdef str response_header
    cdef list headers

    def __init__(self):
        self.status = NULL
        self.client = NULL
        self.response_header = ''
        self.headers = []

    cdef void _raise_error(self, const Client& client, int code):
        if code in CONST_HYDRUS_ERROR_RESPONSE:
            message = CONST_HYDRUS_ERROR_RESPONSE[code]
            client.send(message, len(message))
        else:
            client.send('\r\n', 2)

    cdef to_environ(self, const Client& client, const Request & request):
        environ = {'Server': 'hydrus %s' % __VERSION__}

        environ['Url'] = request.url.c_str()
        environ['Method'] = request.method.c_str()
        environ['Keepalive'] = not (not request.keepalive)
        environ['REMOTE_ADDR'] = client.remoteAddress()

        cdef int n = request.headers.size()
        for _i in range(n):
            environ[request.headers[_i].name.c_str()] = request.headers[_i].value.c_str()

    cdef write(self, data):
        if not self.response_header:
            for name, val in self.headers:
                self.response_header += '%s: %s\r\n' % (name, val)
            self.client.send(self.response_header, len(self.response_header))

        self.client.send(data, len(data))

    def start_response(self, const char * status, headers, exec_info=None):
        if exec_info:
            try:
                if self.response_header:
                    raise exec_info[0], exec_info[1], exec_info[2]
            except:
                exec_info = None

        self.status = status
        self.headers = headers
        return self.write


cdef void _hydrus_response_callback(const Client& client, const Request& req):
    response = _HydrusResponse()
    response.client = &client
    environ = response.to_environ(client, req)

    retval = G_hy_app(environ, response.start_response)
    if not response.response_header:
        response.raise_error(400)
    else:
        for rs in retval:
            client.send(rs, len(rs))


# ----------------------------------------------
#  Main API of hydrus
# ----------------------------------------------
def listen(app, const char* addr, int port):
    global G_hy_server, G_hy_app
    if G_hy_server is not NULL:
        return
    G_hy_server = new Server(_hydrus_response_callback)
    G_hy_app = app
    G_hy_server.listen(addr, port)


def run(app=None, host=None, port=None):
    if G_hy_server is NULL:
        assert host is not None and port is not None, 'Cannot initialize hydrus server'
        listen(app, host, port)
    G_hy_server.run()
