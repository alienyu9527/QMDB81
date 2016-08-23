#include <string>
#include <iostream>
#include "Tools/mdbCheckState.h"
#include "Helper/TThreadLog.h"


//using namespace QuickMDB;

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        printf( "-------\n"
        " Usage:\n"
        "   <DSN> \n"
        "-------\n");
        return -1;
    }
    int iRet = 0;
    char sInputNumb[64];
    //TADD_START(argv[1],0, "mdbStat", 0, false);
    TADD_START(argv[1],"mdbStat",  0, false,false);

    TMDBCheckState CheckState;
    if(CheckState.Init(argv[1]) < 0)
    {
        cout<<"Init TMDBCheckState Failed"<<endl;
        return 0;
    }
    printf("\n************* QuickMDB State Check ************\n");
    printf("[1] Check All Process State:\n");
	//mjx sql tool modify start
    printf("[2] Check DB(oracle or mysql) Rep State:\n");
	//mjx sql tool modify end
    printf("[3] Check Peer Rep State:\n");
    printf("[4] Check Error Information in QuickMDB Log:\n");
    printf("[5] Check Index State:\n");
    printf("[6] Quit:\n");
    while(true)
    {
        memset(sInputNumb,0,sizeof(sInputNumb));
        cout<<"\nPlease Choose Check Number: ";
        scanf("%9s",sInputNumb);
        cout<<endl;

        int iSelect= atoi(sInputNumb); 
        if(iSelect < 1 || iSelect > 6)
        {
            cout<<"Input number error. "<<endl;
            continue;
        }

        if(iSelect == 6)
        {
            break;
        }

        switch(iSelect)
        {
        case 1:
            {
                CheckState.CheckAllProcess();
                break;
            }
        case 2:
            {
                CheckState.CheckOraRep();
                break;
            }
        case 3:
            {
                CheckState.CheckPeerRep();
                break;
            }
        case 4:
            {
                CheckState.CheckLog();
                break;
            }
        case 5:
            {
                CheckState.CheckIndex();
                break;
            }
        default:
            {
                cout<<"Input number error. "<<endl;
                break;
            }
        }
    }
    return iRet;
}
