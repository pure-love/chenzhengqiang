/*
*@filename:nana.h
*@author:chenzhengqiang
*@start date:2016/01/01 09:55:05
*@modified date:
*@desc: 
*/



#ifndef _CZQ_NANA_H_
#define _CZQ_NANA_H_
//write the function prototypes or the declaration of variables here
#include <cstdio>
#include <string>
#include <time.h>

namespace czq
{
    //make everything personate
    typedef FILE * Life;
    //what nana means?
    //find the answer if you really want to know about the "nana"
    class Nana
    {
        public:
            enum NanaEmotion
            {
                COMPLAIN=0,
                PEACE=1,
                HAPPY=2
            };
            
            enum NanaLife
            {
                KB = 1024,
                MB = ((KB)*(KB)),
                LIFE_LENGTH =100*(MB)
            };
            
            static int born(std::string &cradle, int emotion, int breakTime);
            static void die();
            static void say(int emotion, const char *toWho, const char *about,...);
            static inline bool is(int emotion){ return emotion <= emotion_;}
            static void tidy();
        private:
            static int reborn();
            static unsigned long lifeLength();
        public:
            static int emotion_;
            static Life life_;
            static std::string cradle_; 
            static std::string said_;
            static time_t tidyTime_;
            static int breakTime_;
    };
};
#endif
