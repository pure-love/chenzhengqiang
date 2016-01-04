/*
*@filename:nana.cpp
*@author:chenzhengqiang
*@start date:2016/01/01 09:55:05
*@modified date:
*@desc: 
*/


#include "nana.h"
#include <pthread.h>
#include <sys/stat.h>
#include <stdarg.h>


namespace czq
{
	Life Nana::life_ = 0;
	int Nana::emotion_ = Nana::HAPPY;
	std::string Nana::cradle_ = "";
	std::string Nana::said_ = "\r";
	time_t Nana::tidyTime_ = 0;
	int Nana::breakTime_ = 0;
	static const char * NANA_EMOTION[]={"_COMPLAIN_", "_PEACE_", "_HAPPY_"};
	static pthread_mutex_t  GuardianAngel= PTHREAD_MUTEX_INITIALIZER;
	
	int Nana::born(std::string & cradle, int emotion, int breakTime)
	{
		life_ = fopen(cradle.c_str(), "w");
    		if (life_ == 0)
    		{
        		return -1;
    		}
    		
    		setbuf(life_, NULL);
    		cradle_ = cradle;
    		emotion_ = emotion;
 		breakTime_ = breakTime;
    		return 0;
	}


	void Nana::die()
	{
    		if (life_ != 0)
       	fclose(life_);
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
    		char curTime[24];/* "YYYY-MM-DD HH:MM:SS" */
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
    			int printBytes = snprintf(detailedSay, sizeof(detailedSay),"%s %-15s : %-8s : ", timeBuffer, toWho, NANA_EMOTION[emotion]);
    			vsnprintf(detailedSay+printBytes, sizeof(detailedSay)-printBytes, about, valist);
    			said_+=std::string().assign(detailedSay)+"\n\r";
    			tidy();
    			pthread_mutex_unlock(&GuardianAngel);
    			va_end(valist);
    		}
	}

	void Nana::tidy()
	{
		if( breakTime_ == 0 )
		{
			if (said_.length() > 1)
			{
				fprintf(life_, said_.c_str());
    				said_="\r";
			}
		}
		else
		{
			time_t now = time(NULL);
    			if ((said_.length() > (size_t)(50*KB)) || (now-tidyTime_) >=(time_t) breakTime_)
    			{
    				fprintf(life_, said_.c_str());
    				said_="\r";
    			}
    			tidyTime_ = now;
    		}
	}
};
