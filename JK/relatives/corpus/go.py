 # -*- coding: utf-8 -*-

import sys 
reload(sys)
sys.setdefaultencoding('utf8')

import codecs
prefixes=["一","二","两","三","四","五","六","七","八","九","十","1","2","3","4","5","6","7","8","9","10"]
fr = codecs.open("./liangci.txt","r","utf-8")
fw = codecs.open("./newstop.txt","w","utf-8")
lines = fr.readlines()

for prefix in prefixes:
    for line in lines:
        words = line.split()
        for word in words:
            fw.write("%s%s\n" % (prefix, word)) 
