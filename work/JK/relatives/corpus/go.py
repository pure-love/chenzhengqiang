import codecs
fpw = codecs.open("./jk_lexicon.txt", "a+")
fpr = codecs.open("./k_symptom.txt", "r")
for line in fpr:
    keyword = line.split()[1].strip().replace("\r\n", "")
    fpw.write("%s\r\n" % (keyword))

