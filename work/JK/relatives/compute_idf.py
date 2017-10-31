from __future__ import division
# -*- coding: utf-8 -*-

import jieba
import jieba.analyse
import argparse
import codecs
import math
import time

def get_stop_words(f):
	try:
		words = codecs.open(f, "r", "utf-8").read().split("\r\n")
	except:
		pass
	return words

def cut_words(stop_words, input_file, output_file, max_docs):
        count = 0
	fpw = codecs.open(output_file, "w", "utf-8")
	with codecs.open(input_file, "r", "utf-8") as fr:
		for line in fr:
                        count += 1
                        print count
			segs = jieba.cut(line, cut_all=True)
			words=[]
			for seg in segs:
				if seg.strip()=='' or len(seg) <=1:
					continue
				if seg not in stop_words:
					words.append(seg)
			words = " ".join(words)
			fpw.write("%s\n" % (words))
                        if count == max_docs:
                            print "cut all done." 
                            break 


def compute_tfidf(input_file, tfidf_file):
	words = []
	with codecs.open(input_file, "r", "utf-8") as fr:
		for line in fr:
			words.append(line.split(" "))
	total_documents = len(words)
	print "total docuemnts %s" % (total_documents)
	idf = {}
        for line_words in words:
		for word in line_words:
			if word not in idf:
				times = 0
				for _line_words in words:
					if word in _line_words:
						times += 1
				idf[word]=math.log(round(total_documents / (times),4))
	
	idf = sorted(idf.items(), key=lambda d:d[1], reverse=True)
	with codecs.open(tfidf_file, "w", "utf-8") as fw:
		for _item in idf:
		    fw.write("%s %s\n" % (_item[0].strip().replace("\n",""), _item[1]))


if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	parser.add_argument("-i", "--input_file")
	parser.add_argument("-o", "--output_file")
	parser.add_argument("-s", "--stop_file")
	parser.add_argument("-t", "--tfidf_file")
	parser.add_argument("-m", "--max")
	args = parser.parse_args()
	stop_words = get_stop_words(args.stop_file)
        jieba.analyse.load_user_dict("./jk_dict.txt")
	cut_words(stop_words, args.input_file, args.output_file, int(args.max))
	compute_tfidf(args.output_file, args.tfidf_file)


