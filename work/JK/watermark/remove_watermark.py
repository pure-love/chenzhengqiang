#-*-coding:utf-8-*-
import numpy as np
import cv2
import os
import argparse


def remove_watermark(img,x1,x2,y1,y2, mask):
#water addr:[x1,y1]~[x2,y2] [mx1,my1]~[mx2,my2]
    endmask = np.zeros(img.shape[:2], np.uint8)
    for i in range(endmask.shape[0]):
        for j in range(endmask.shape[1]):
            if (i >= y1 and i <= y2 and j >= x1 and j <= x2):
                endmask[i,j,] = 255 # if mask[my1+i-y1,mx1+j-x1,]!=255 else 255
    dst=cv2.inpaint(img, endmask, 10.0, cv2.INPAINT_TELEA)
    return dst


def relogo(img, pic_mask):
    img = cv2.imread(img)
    mask = cv2.imread(pic_mask, cv2.IMREAD_GRAYSCALE)
    y = img.shape[0]
    x = img.shape[1]

    if x<=100 and y<=100:
        return img
    else:
        dst = remove_watermark(img,x-52,x-2,y-58,y-4, mask)
        return dst


def do_watermark_remove(src_dir, save_dir, pic_mask):
    '''

    :param dir_entry:
    :param pic_mask_path:
    :return:
    '''
    for root, dirs, files in os.walk(src_dir):
        for file in files:
            img = os.path.join(root, file)
            dst = relogo(img, pic_mask)
            cv2.imwrite(os.path.join(save_dir,file), dst)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-d", "--src_dir")
    parser.add_argument("-D", "--save_dir")
    parser.add_argument("-m", "--pic_mask")
    args = parser.parse_args()
    do_watermark_remove(args.src_dir, args.save_dir, args.pic_mask)
