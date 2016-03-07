/*
*@filename:nana.cpp
*@author:chenzhengqiang
*@start date:2016/01/01 09:55:05
*@modified date:
*@desc: 
*/


#include "nana.h"
#include <cstring>
#include <pthread.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdexcept>

using std::runtime_error;

namespace czq
{
	static const char * NANA_EMOTION[]={"_COMPLAIN_", "_PEACE_", "_HAPPY_"};
	static pthread_mutex_t  GuardianAngel= PTHREAD_MUTEX_INITIALIZER;

	
	Nana * Nana::nana_ = 0;
	
	Nana::Nana( const std::string & cradle, int emotion, int breakTime)
	:cradle_(cradle), emotion_(emotion), breakTime_(breakTime),shutupTime_(0)
	{
		life_ = fopen(cradle_.c_str(), "w");
    		if (life_ == 0)
    		{
        		throw runtime_error("failed to open cradle");
    		}
    		setbuf(life_, NULL);
 		memset(said_, 0, sizeof(said_));
 		nowQ_ = said_;
 		endQ_ = said_+sizeof(said_);
	}
	

	void Nana::die()
	{
    		if ( life_ != 0 )
    		{
       		fclose(life_);
       	}
       	
       	if ( Nana::nana_ != 0 )
       	{
       		delete Nana::nana_;
       		Nana::nana_ = 0;
       	}
	}

	unsigned long Nana::lifeLength()
	{
		if (cradle_.empty())
		return 0;
		
		struct stat lifeStat; 
    		if (stat(cradle_.c_str(), &lifeStat) < 0)
    		{    
        		return -1;    
    		}
    		return lifeStat.st_size;
	}


	int Nana::reborn( void )
	{
    		char curTime[24];//the time format:"YYYY-MM-DD HH:MM:SS" 
    		time_t now;
    		struct tm *localTime;
    		now = time( NULL );
    		localTime = localtime(&now);
    		strftime(curTime, sizeof(curTime), "%Y%m%d%H%M%S", localTime);
    		std::string oldCradle=cradle_+"."+curTime;
    		rename(cradle_.c_str(), oldCradle.c_str());
    		fclose(life_);
    		life_ = fopen(cradle_.c_str(), "w");
    		if (life_ == 0)
    		{
        		return -1;
    		}
    		setbuf(life_, NULL);
    		return 0;
	}



	void Nana::say(int emotion, const char * toWho, const char *about, ...)
	{
    		if (emotion <= emotion_)
    		{
    			va_list valist;
    			va_start(valist, about);
        		char timeBuffer[50];
    			time_t now;
    			struct tm *loctime;
    			pthread_mutex_lock(&GuardianAngel);
    			if ( lifeLength() >= LIFE_LENGTH )
    			{
         			if ( reborn() == -1 )
         			{
             				pthread_mutex_unlock(&GuardianAngel);
             				return;
         			}
    			}
    			
    			now = time (NULL);
    			loctime = localtime(&now);
    			char detailedSay[1024];
    			strftime (timeBuffer, sizeof(timeBuffer), "%F %T : ", loctime);
    			int saidQ = snprintf(detailedSay, sizeof(detailedSay),"%s %-15s : %-8s : ", timeBuffer, toWho, NANA_EMOTION[emotion]);
    			saidQ += vsnprintf(detailedSay+saidQ, sizeof(detailedSay)-saidQ, about, valist);
    			char *prevQ = nowQ_;
    			
    			if ( (nowQ_+saidQ+1) < (endQ_) )
    			{
    				memcpy(nowQ_, detailedSay, saidQ);
    				nowQ_ += saidQ;
    				*nowQ_ = '\n';
    				nowQ_ += 1;
    			}

    			//just shutup nana's talking
    			shutup();

    			if ((prevQ+saidQ+1) >= (endQ_))
    			{
    				memcpy(nowQ_, detailedSay, saidQ);
    				nowQ_ += saidQ;
    				*nowQ_ = '\n';
    				nowQ_ += 1;
    			}
    			
    			pthread_mutex_unlock(&GuardianAngel);
    			va_end(valist);
    		}
	}

	void Nana::shutup()
	{
		if( breakTime_ <= 0 )
		{
			if ( nowQ_ > said_ )
			{
				fprintf(life_, said_);
				nowQ_ = said_;
				memset(said_, 0, 4*KB*9);
			}
		}
		else
		{
			
			time_t now = time(NULL);
    			if (((nowQ_ - said_) >= (8*KB)) || (now-shutupTime_) >=(time_t) breakTime_)
    			{
    				fprintf(life_, said_);
    				nowQ_ = said_;
    				memset(said_, 0, 4*KB*9);
    			}
    			shutupTime_ = now;
    		}
	}
};
