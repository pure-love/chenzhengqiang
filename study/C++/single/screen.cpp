/*
*@filename:screen.h
*@author:chenzhengqiang
*@start date:2015/09/29 11:29:08
*@modified date:
*@desc: 
*/



//write the function prototypes or the declaration of variables here
#include<iostream>
using namespace std;
class screen
{
   public:
        typedef void(screen::*action)();
        static action menus[];
        enum directions{HOME,END,LEFT,RIGHT};
        void move( const directions & direction )
        {
            (this->*menus[direction])();
        }
        
   private:
        void home(){ cout<<"move to home"<<endl;}
        void end(){ cout<<"move to end"<<endl;}
        void left(){ cout<<"move to end"<<endl;}
        void right(){ cout<<"move to right"<<endl;}
};

screen::action screen::menus[]={&screen::home,&screen::end,&screen::left,&screen::right};

int main( int argc, char ** argv )
{
    (void)argc;
    (void)argv;
    screen S;
    S.move(screen::END);
    S.move(screen::LEFT);
    S.move(screen::RIGHT);
    S.move(screen::HOME);
    return 0;
}
