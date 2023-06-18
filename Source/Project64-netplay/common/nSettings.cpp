/******************************************************************************
***  n02 v0.3 winnt                                                         ***
**   Open Kaillera Client Core                                               **
***  For latest sources, visit http://sf.net/projects/okai                  ***
******************************************************************************/
#include "nSettings.h"

#include <windows.h>

char file[5000];
char * subm;


void nSettings::Initialize(char * submo, bool global){
	file[0] = 0;
	strcat(file, "Config/Netplay.ini");
	subm = submo;
}

void nSettings::Terminate(){

}
int nSettings::get_int(char * key, int def_){
	return GetPrivateProfileInt(subm, key, def_, file);
}
char* nSettings::get_str(char * key, char * buf, char * def_){
	GetPrivateProfileString(subm, key, def_, buf, 128, file);
	return buf;
}
void nSettings::set_int(char * key, int val){
	char bft [128];
	wsprintf(bft, "%i", val);
	WritePrivateProfileString(subm, key, bft, file);
}
void nSettings::set_str(char * key, char * val){
	WritePrivateProfileString(subm, key, val, file);
}
