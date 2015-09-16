# -*- coding:utf8 -*-

from libcpp.string cimport string
from libcpp.vector cimport vector

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
        bool keepalive



cdef extern from "server.h" namespace "hydrus":
    cdef cppclass Client:
        void send(const char * buf, size_t n) nogil
        void end()

    ctypedef void (*WSGICallback)(const Client&, const Request&)

    cdef cppclass Server:
        Server(WSGICallback) nogil
        void listen(const char*, int)
        void run()


# ----------------------------------------------
#  Hydrus Server Wrapper
# ----------------------------------------------
cdef class _HydrusResponse:
    def __init__(self):
        pass

    def __cinit__(self):
        pass

    cdef void _raise_error(self, const Client& client, int code):
        if code in CONST_HYDRUS_ERROR_RESPONSE:
            client.send(CONST_HYDRUS_ERROR_RESPONSE[code])
        else:
            client.send('\r\n')

    cdef to_environ(self, const Request & request):
        environ = {'Server': 'hydrus %s' % __VERSION__}
        return environ

    cdef void send_header(self, const Client& client, const Request& req):
        pass

    cdef start_response(self, const char * status, dict headers, exec_info=None):
        pass


cdef void _hydrus_response_callback(const Client& client, const Request& req):
    response = _HydrusResponse()

    environ = response.to_environ(req)
    retval = G_hy_app(environ, response.start_response)

    sent_header = response.send_header(client, req)
    if not sent_header:
        response.raise_error(400)
    else:
        for rs in retval:
            client.send(rs, len(rs))


# ----------------------------------------------
#  Main API of hydrus
# ----------------------------------------------
def listen(app, addr, port):
    global G_hy_server, G_hy_app
    if G_hy_server:
        return
    G_hy_server = new HttpServer()
    G_hy_app = G_hy_app


def run(app, host=None, port=None):
    if G_hy_server is None:
        assert host is not None and port is not None, 'Cannot initialize hydrus server'
        listen(host, port)
    G_hy_server.run()
