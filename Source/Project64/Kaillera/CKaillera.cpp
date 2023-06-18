#include "stdafx.h"
#include "CKaillera.h"

void(__stdcall* kailleraGetVersion) (char *version);
void(__stdcall* kailleraInit) ();
void(__stdcall* kailleraShutdown) ();
void(__stdcall* kailleraSetInfos) (kailleraInfos *infos);
void(__stdcall* kailleraSelectServerDialog) (HWND parent);
int(__stdcall* kailleraModifyPlayValues)  (void *values, int size);
void(__stdcall* kailleraChatSend)  (char *text);
void(__stdcall* kailleraEndGame) ();

CKaillera* ck;

void ShowError(LPCWSTR str)
{
	MessageBox(NULL, str, str, NULL);
}

int WINAPI kailleraGameCallback(char *game, int player, int numplayers)
{
	ck->playerNumber = player-1; // since our player #1 is in index zero
	ck->numberOfPlayers = numplayers;

	// find game in local list and run it based off of the full path
	char *temp = ck->szKailleraNamedRoms;

	for (int x = 0; x < ck->numberOfGames; x++)
	{
		if (strncmp(temp, game, strlen(temp)) == 0)
		{
			temp = ck->szFullFileNames[x];
			break;
		}
		temp += strlen(temp) + 1;
	}

	//g_Notify->BringToTop();

	ck->isPlayingKailleraGame = true;
	ck->clearCodes();
	g_BaseSystem->RunFileImage(temp);

	return 0;
}

void WINAPI kailleraChatReceivedCallback(char *nick, char *text)
{
	/* Do what you want with this :) */
	//ShowInfo("Kaillera : <%s> : %s", nick, text);
}

void WINAPI kailleraClientDroppedCallback(char *nick, int playernb)
{
	/* Do what you want with this :) */
	//ShowInfo("Kaillera : <%s> dropped (%d)", nick, playernb);
	if ((playernb - 1) == ck->playerNumber) // that means we dropped, so stop emulation!
	{
		ck->endGame();
		g_BaseSystem->CloseSystem();
		ck->isPlayingKailleraGame = false;
	}
}

void WINAPI kailleraMoreInfosCallback(char *gamename)
{
	/* Do what you want with this :) */
	//ShowInfo("Kaillera : MoreInfosCallback %s ", gamename);
}

CKaillera::CKaillera()
{
	kInfos.appName = sAppName;
	kInfos.gameList = szKailleraNamedRoms;
	kInfos.gameCallback = kailleraGameCallback;
	kInfos.chatReceivedCallback = kailleraChatReceivedCallback;
	kInfos.clientDroppedCallback = kailleraClientDroppedCallback;
	kInfos.moreInfosCallback = kailleraMoreInfosCallback;

	LoadKailleraFuncs();

	kailleraInit();

	isPlayingKailleraGame = false;
	numberOfGames = playerNumber = numberOfPlayers = 0;
	memset(values, 0, sizeof(values));
	playValuesLength = 0;
}

CKaillera::~CKaillera()
{
	kailleraShutdown();
}

void CKaillera::clearGameList()
{
	// clear the list
	pszKailleraNamedRoms = szKailleraNamedRoms;

	memset(szKailleraNamedRoms, 0, sizeof(szKailleraNamedRoms));
	memset(szFullFileNames, 0, sizeof(szFullFileNames));

	numberOfGames = 2;
}

void CKaillera::addGame(char *gameName, char *szFullFileName)
{
	strncpy(pszKailleraNamedRoms, gameName, strlen(gameName) + 1);
	pszKailleraNamedRoms += strlen(gameName) + 1;

	strncpy(szFullFileNames[numberOfGames], szFullFileName, strlen(szFullFileName) + 1);

	numberOfGames++;
}

void CKaillera::terminateGameList()
{
	*(++pszKailleraNamedRoms) = '\0';
}

void CKaillera::setInfos()
{
	kailleraSetInfos(&kInfos);
}

void CKaillera::selectServerDialog(HWND hWnd)
{
	kailleraSelectServerDialog(hWnd);
}

void CKaillera::modifyPlayValues(DWORD val)
{
	memset(&values, 0, sizeof(values)); // clear the input array

	CKailleraPacket ckp[4];
	memset(ckp, 0, sizeof(ckp));

	ckp[0].Type = PACKET_TYPE_INPUT;
	ckp[0].input = val;

	playValuesLength = kailleraModifyPlayValues(ckp, sizeof(CKailleraPacket));

	processResult(ckp);
}

void CKaillera::processResult(CKailleraPacket ckp[])
{
	for (int x = 0; x < 4; x++)
	{
		switch (ckp[x].Type)
		{
			case PACKET_TYPE_INPUT:
				values[x] = ckp[x].input;
				break;
			case PACKET_TYPE_CHEAT:
				if (strncmp(ckp[x].code, CONFIRM, CODE_LENGTH) == 0) // this is a response packet which requires no processing
					continue;

				if (playerNumber == 0) // if we're player 1 and we're receiving a cheat code from another player... 
				{
					if (x != 0)
					{
						// this should never happen.  this means another player is sending us, the host, a cheat.
						// print some sort of error?  player #1 (index 0) should never receive cheats from other players

						MessageBox(NULL, L"ERROR: Player #1 received a cheat code from another player. This should never happen. Desync likely.", L"Whoops", NULL);
					}
					else // its us seeing our own cheat!  ignore it
					{
						continue;
					}
				}
				else
				{
					if (strncmp(ckp[x].code, RESET, CODE_LENGTH) == 0) // this is a reset command (probably a cheat was disabled)
					{
						ck->clearCodes();
					}
					else if (strncmp(ckp[x].code, LOAD, CODE_LENGTH) == 0) // this is a load command command (probably just finished sending all cheats)
					{
						// reload the cheats
						//g_BaseSystem->m_Cheats.LoadCheats(false);
					}
					else // just a regular cheat
					{
						// store the cheat locally
						addCode(ckp[x].code);
					}

					// send a confirmation response
					sendConfirmCode();
				}
				break;
		}
	}
}

void CKaillera::sendResetCode()
{
	CKailleraPacket response[4];
	memset(response, 0, sizeof(response));
	response[0].Type = PACKET_TYPE_CHEAT;
	strncpy(response[0].code, RESET, CODE_LENGTH);

	playValuesLength = kailleraModifyPlayValues(response, sizeof(CKailleraPacket));

	processResult(response);
}
void CKaillera::sendLoadCode()
{
	CKailleraPacket response[4];
	memset(response, 0, sizeof(response));
	response[0].Type = PACKET_TYPE_CHEAT;
	strncpy(response[0].code, LOAD, CODE_LENGTH);

	playValuesLength = kailleraModifyPlayValues(response, sizeof(CKailleraPacket));

	processResult(response);
}

void CKaillera::sendConfirmCode()
{
	CKailleraPacket response[4];
	memset(response, 0, sizeof(response));
	response[0].Type = PACKET_TYPE_CHEAT;
	strncpy(response[0].code, CONFIRM, CODE_LENGTH);

	playValuesLength = kailleraModifyPlayValues(response, sizeof(CKailleraPacket));

	processResult(response);
}

void CKaillera::addCode(LPCSTR str)
{
	int length = strlen(str);
	char *c = new char[length + 1];
	c[length] = '\0';
	strncpy(c, str, length);
	const char delimiter[] = ",";
	char *token;
	char *context = NULL;

	token = strtok_s(c, delimiter, &context);

	while (token != NULL)
	{
		char * newCode = new char[CODE_LENGTH];
		strncpy(newCode, token, CODE_LENGTH);

		codes.push_back(newCode);

		token = strtok_s(NULL, delimiter, &context);
	}
}

void CKaillera::delCode(LPCSTR str)
{
	int length = numCodes();
	for (int x = 0; x < length; x++)
	{
		if (strncmp(codes.at(x), str, strlen(str)) == 0) // the code matches at location x
		{
			// deallocate the memory for the code
			char *c = codes.at(x);
			delete(c);

			// remove the code from the vector
			codes.erase(codes.begin() + x);

			// short circut out. our job is done
			return;
		}
	}
}

void CKaillera::clearCodes()
{
	while(codes.size() > 0)
	{
		char * temp = codes.front();
		delete(temp);
		codes.erase(codes.begin());
	}
}

LPCSTR CKaillera::getCode(int i)
{
	return codes.at(i);
}

void CKaillera::sendCodes()
{
	sendResetCode();

	CKailleraPacket ckp[4];

	for (int x = 0; x < codes.size(); x++)
	{
		memset(ckp, 0, sizeof(ckp));

		ckp[0].Type = PACKET_TYPE_CHEAT;
		strncpy(ckp[0].code, codes.at(x), strlen(codes.at(x)));

		playValuesLength = kailleraModifyPlayValues(ckp, sizeof(CKailleraPacket));

		processResult(ckp);
	}

	sendLoadCode();
}

int CKaillera::numCodes()
{
	return codes.size();
}

DWORD CKaillera::getValues(int player)
{
	return values[player];
}

void CKaillera::endGame()
{
	kailleraEndGame();
}

int CKaillera::LoadKailleraFuncs()
{
	KailleraHandle = LoadLibrary(L"Plugin/Netplay/KNet.dll");

	if (KailleraHandle) {
		//ShowInfo("Kaillera Library found");
		kailleraGetVersion = (void(__stdcall*) (char*)) GetProcAddress(KailleraHandle, "_kailleraGetVersion@4");
		if (kailleraGetVersion == NULL) {
			ShowError(L"kailleraGetVersion not found");
			return 0;
		}

		kailleraInit = (void(__stdcall *)(void)) GetProcAddress(KailleraHandle, "_kailleraInit@0");
		if (kailleraInit == NULL) {
			ShowError(L"kailleraInit not found");
			return 0;
		}

		kailleraShutdown = (void(__stdcall *) (void)) GetProcAddress(KailleraHandle, "_kailleraShutdown@0");
		if (kailleraShutdown == NULL) {
			ShowError(L"kailleraShutdown not found");
			return 0;
		}

		kailleraSetInfos = (void(__stdcall *) (kailleraInfos *)) GetProcAddress(KailleraHandle, "_kailleraSetInfos@4");
		if (kailleraSetInfos == NULL) {
			ShowError(L"kailleraSetInfos not found");
			return 0;
		}

		kailleraSelectServerDialog = (void(__stdcall*) (HWND parent)) GetProcAddress(KailleraHandle, "_kailleraSelectServerDialog@4");
		if (kailleraSelectServerDialog == NULL) {
			ShowError(L"kailleraSelectServerDialog not found");
			return 0;
		}

		kailleraModifyPlayValues = (int(__stdcall *) (void *values, int size)) GetProcAddress(KailleraHandle, "_kailleraModifyPlayValues@8");
		if (kailleraModifyPlayValues == NULL) {
			ShowError(L"kailleraModifyPlayValues not found");
			return 0;
		}

		kailleraChatSend = (void(__stdcall *) (char *)) GetProcAddress(KailleraHandle, "_kailleraChatSend@4");
		if (kailleraChatSend == NULL) {
			ShowError(L"kailleraChatSend not found");
			return 0;
		}

		kailleraEndGame = (void(__stdcall *) (void)) GetProcAddress(KailleraHandle, "_kailleraEndGame@0");
		if (kailleraEndGame == NULL) {
			ShowError(L"kailleraEndGame not found");
			return 0;
		}
	}
	else
	{
		MessageBox(NULL, L"Netplay plugin KNet.dll not found. Please place it in the Plugin/Netplay folder and run Project64-MPN again!", L"OOPS", NULL);
		PostQuitMessage(0);
	}

	return 1;
}