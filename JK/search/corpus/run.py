# __author__ = chenzhengqiang
# __date__ = 2017-11-14


import codecs
import json

lines = codecs.open("jk.big", "r", "utf-8").readlines()
fpw = codecs.open("jk.lexicon.json", "w", "utf-8")
lexicon = {}
for line in lines:
    data = line.split()
    if len(data) !=2:
        continue
    if data[0].strip() == "":
        continue
    lexicon[data[0]] = int(data[1])

fpw.write("%s" % (json.dumps(lexicon, ensure_ascii = False)))
fpw.close()
   
