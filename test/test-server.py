# -*-coding:utf8 -*-
import bottle
import os
from bottle import route, request, default_app
from hydrus import server


# Simple HTTP GET
#
@route('/simple/:name')
def simple_http_get(name):
	d = {}
	d.update(request.params)
	for c in xrange(ord('A'), ord('Z') + 1):
		ch = chr(c)
		if ch not in d:
			bottle.abort(400, 'Query not found')
		elif d[ch] != 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789':
			bottle.abort(500, 'Fail')			
	return d

# file GET
#
@route('/file/:name')
def file_http_get(name):
	hello = os.path.abspath(os.path.dirname(__file__) + '/test-server.py')
	return open(hello, 'rb')


# post a short query
#
@route('/:what', 'POST')
def short_post_request(what):
	return what


# Post a long query string
#
@route('/long/:what', 'POST')
def long_post_request(what):
	return what


# Post with file
#
@route('/file/:what', 'POST')
def file_post_request(what):
	return what


# Post with file and query string
#
@route('/mix/:what')
def mix_post_request(what):
	return what


# JSON post
#
@route('/json')
def json_http_post():
	return ''


server.listen(default_app, '127.0.0.1', 9876)
server.run()