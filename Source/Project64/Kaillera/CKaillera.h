#pragma once

#include <Project64-core/N64System/Enhancement/Enhancement.h>

#define KAILLERA_CLIENT_API_VERSION "0.8"

#define MAX_NUMBER_OF_GAMES 1024

struct kailleraInfos
{
	char *appName;
	char *gameList;

	int (WINAPI *gameCallback)(char *game, int player, int numplayers);

	void (WINAPI *chatReceivedCallback)(char *nick, char *text);
	void (WINAPI *clientDroppedCallback)(char *nick, int playernb);

	void (WINAPI *moreInfosCallback)(char *gamename);
};

#define PACKET_TYPE_INPUT 1
#define PACKET_TYPE_CHEAT 2

#define CODE_LENGTH 14 //8 for code, 1 for a space, 4 for value, 1 for trailing null

struct CKailleraPacket
{
	BYTE Type;
	union
	{
		char code[CODE_LENGTH];
		DWORD input;
	};
};

class CKaillera
{
public:
	CKaillera();
	~CKaillera();

	void clearGameList();
	void addGame(char *gameName, char *szFullFileName);
	void terminateGameList();
	HANDLE startDialogThread(HWND hWnd);
	void modifyPlayValues(DWORD values);
	//void modifyPlayValues(CODES c);
	void setInfos();

	int numberOfGames;
	char szKailleraNamedRoms[MAX_NUMBER_OF_GAMES * 2000];
	char szFullFileNames[MAX_NUMBER_OF_GAMES][2000];
	bool isPlayingKailleraGame;

	int playerNumber;
	int numberOfPlayers;

	DWORD getValues(int player);
	void endGame();

	void addCode(LPCSTR str);
	void delCode(LPCSTR str);
	LPCSTR getCode(int i);
	void clearCodes();
	void sendCodes();
	int numCodes();

private:

	const char* RESET = "00000000 0001";
	const char* LOAD = "00000000 0002";
	const char* CONFIRM = "00000000 0000";

	void sendResetCode();
	void sendLoadCode();
	void sendConfirmCode();
	static DWORD WINAPI selectServerDialog(LPVOID hWnd);

	kailleraInfos   kInfos;
	HMODULE KailleraHandle;
	int LoadKailleraFuncs();
	void processResult(CKailleraPacket ckp[]);
	char *pszKailleraNamedRoms;
	char *sAppName = "MPN64";
	DWORD values[4]; // for a maximum of 4 players
	std::vector<char*> codes;
	int playValuesLength;
};

extern CKaillera* ck;
