#encoding=utf-8
import requests
import json
import re

SOGO_VALIDATE_URL = "http://open.sogou.com/xmlview/validation/validate?templateID=70088400&type=3&xml="
with open('./sitemap_pc.txt','rb') as urls:
    for line in urls:
	    line = line.replace('\n','')
	    sogo_validate_url = ('%s%s')%(SOGO_VALIDATE_URL,line)
	    rep = requests.post(url=sogo_validate_url)
	    text = rep.text
	    rep = json.loads(text)
	    print "rep=",rep
	    if rep["code"] != 1:
		f1 = file("invalid_pc.txt","a+")
		f1.write(text+'\n')
	    else:
	        print text

