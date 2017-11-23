__author__ = 'chenzhengqiang'
__date__ = '2017/10/10'
# -*- coding: utf-8 -*-
import cv2,os,shutil,datetime,re,time
from threading import Thread
from hashlib import md5
PICHASH= {}
def md5_file(name):
    try:
        m = md5()
        a_file = open(name, 'rb')
        m.update(a_file.read())
        a_file.close()
        return m.hexdigest()
    except:
        return None

def do_watermark_remove(f):
    global  PICHASH
    for ppicdir in dirlist:
        if(os.path.isdir(dir+ppicdir)):
            sortfiles=os.listdir(dir+ppicdir)
            if '.DS_Store' in sortfiles:
                sortfiles.remove('.DS_Store')
            sortfiles.sort()
            for oldfile in sortfiles:
                filetype="."+oldfile.split(".")[len(oldfile.split("."))-1]
                picname_front=oldfile.split(filetype)[0]
                oldfile=dir+ppicdir+"/"+oldfile
                jpgname=picname_front+".jpg"
                jpgname=newdir+ppicdir+"/"+jpgname
                try:
                    oldfile_hash=md5_file(oldfile)
                    oldfile_tmphashvalue=PICHASH.get(oldfile_hash)
                    file_object = open('pichash.txt', 'a')
                    file_object.write(oldfile+":"+oldfile_hash+'\n')
                    file_object.close()
                    if(oldfile_tmphashvalue==None):#新文件,已经处理过的图片，就不会再次处理了
                        if not os.path.exists(newdir+ppicdir):
                            os.makedirs(newdir+ppicdir)

                        print datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")+","+oldfile+",ing\n"
                        img=cv2.imread(oldfile)
                        x,y,z=img.shape
                        if x < 10:#太小文件不处理
                            print datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")+","+jpgname+"文件太小，跳过"
                        elif x >8000:#太大的文件不处理
                            print datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")+","+jpgname+"文件太大，跳过"
                        elif not os.path.exists(jpgname):#这就是最关键的代码了
                            for i in xrange(x):
                                for j in xrange(y):
                                    varP=img[i,j]
                                    if sum(varP)>250 and sum(varP)<765 :#大于250，小于765（sum比白色的小）
                                        img[i,j]=[255,255,255]
                            #cv2.imwrite(jpgname,img,[int(cv2.IMWRITE_JPEG_QUALITY),70])#linux跑悲剧了
                            cv2.imwrite(jpgname,img)
                            print "jpgname:"+jpgname
                            PICHASH[oldfile_hash]=oldfile
                            print datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")+","+oldfile+",done\n"
                        else:
                            print datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")+","+jpgname+"文件已存在，跳过\n"
                    elif(oldfile_tmphashvalue!=None):
                        if(os.path.exists(jpgname)):
                            print datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")+","+jpgname+"文件已存在，跳过\n"
                        else:
                            shutil.copyfile(oldfile_tmphashvalue,oldfile)
                            shutil.copyfile(oldfile,jpgname)
                            print datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")+","+jpgname+"和老文件一样，拷贝旧文件，跳过"
                except Exception,e:
                    print "Exception:",e
                    continue

if __name__=='__main__':
    do_watermark_remove()
