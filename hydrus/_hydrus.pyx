# -*- coding:utf8 -*-
import cython

G_hy_server = None


# ----------------------------------------------
#  definitions
# ----------------------------------------------
cdef extern from "server.h" namespace "hydrus":
    cdef cppclass HttpServer:
        HttpServer(const char * addr, int port) nogil
        void run()
        const char * address()
        int port()


# ----------------------------------------------
#  helpers
# ----------------------------------------------
cdef void wsgi_callback():
    pass



# ----------------------------------------------
#  Main API of hydrus
# ----------------------------------------------
def listen(const char * addr, int port):
    global G_hy_server
    if G_hy_server:
        return
    G_hy_server = new HttpServer()


def run(app):
    pass
