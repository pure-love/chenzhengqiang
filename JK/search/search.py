# -*- coding: utf-8 -*-
# __author__ = chenzhengqiang
# __date__ = 2017-11-23
# __desc__ = search


import jieba
import tornado.ioloop
import tornado.web
import json
import codecs
from tornado import gen
import tasks
import tcelery

tcelery.setup_nonblocking_producer()

class SearchHandler(tornado.web.RequestHandler):

    @tornado.web.asynchronous
    def post(self):
        try:
            param = json.loads(self.request.body)
            query = param["query"]
            from_ = param["from"]
            size_ = param["size"]
        except:
            query = None
        if query is not None: 
            tasks.search.apply_async(args=[query,from_,size_], callback=self.on_response)
        else:
            self.finish({})   

    def on_response(self, response):
        self.finish({"hits":response.result[0], "recommend":response.result[1]})
        

if __name__ == "__main__":

    application = tornado.web.Application([
        (r"/search", SearchHandler),
    ])

    application.listen(1986)
    tornado.ioloop.IOLoop.instance().start()
