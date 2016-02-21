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
            
        public: 
            //using the singleton pattern 
            //callNana return the global instance of nana
            static Nana *born(const std::string & cradle, int emotion, int breakTime)
            {
                //don't use the double-check and mutex lock
                if ( ! nana_ )
                {
                    nana_ = new Nana(cradle, emotion, breakTime);
                }
                return nana_;
            }
           
            void say(int emotion, const char *toWho, const char *about,...);
            bool is(int emotion){ return emotion <= emotion_;}
            void shutup();
            void die();
        private:
            Nana(const std::string & cradle, int emotion, int breakTime);
            Nana( const Nana & ){}
            Nana & operator=( const Nana & ){ return *this;}
            ~Nana(){die();}
            int reborn();
            unsigned long lifeLength();
        private:
            static Nana * nana_;
            std::string cradle_; 
            int emotion_;
            Life life_;
            int breakTime_;
            time_t shutupTime_;
            char said_[4*KB*9];//9 pages
            char *nowQ_;
            char *endQ_;
    };
};
#endif
