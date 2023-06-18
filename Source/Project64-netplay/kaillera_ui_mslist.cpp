#include "kaillera_ui.h"

#include <windows.h>
#include "uihlp.h"

#include "resource.h"
#include "kailleraclient.h"
#include "common/nSTL.h"
#include "common/k_socket.h"
#include "common/nThread.h"
#include "common/nSettings.h"




void ConnectToServer(char * ip, int port, HWND pDlg, char*name);








HWND kaillera_mslref;
#define KAILLERA_PING_LIMIT 1000
#define KAILLERA_PING_SIMULT 10

class PingSocket: public k_socket {
	DWORD start_time;
	int state;
public:
	unsigned int limit;
	char * ptr;
	char * temp;
	PingSocket(){
		k_socket();
		state = 0;
		start_time = 0;
		limit = 0;
		initialize(0, -1);
	}
	~PingSocket(){
		close();
	}
	void ping_host(char * host, int p_port){
		set_address(host, p_port);
		start_time = GetTickCount();
		send("PING", 5);
		limit = KAILLERA_PING_LIMIT;
	}
	bool is_done(DWORD tiNow){
		if (tiNow - start_time > KAILLERA_PING_LIMIT){
			start_time = -1;
			return true;
		} else {
			if (has_data()){
				start_time = tiNow - start_time;
				return true;
			}
			return false;
		}
	}
	int return_ping(){
		if (start_time != -1){
			return start_time;
		}
		start_time = 0;
		return -1;
	}
	static bool step(int ms){
		return check_sockets(0, ms);
	}
};


int DownloadListToBuffer(char * buffer, int size, char * url){
	
	char urx[2000];
	strcpy(urx, url);
	
	
	char * urlbuf = urx;
	char * host;
	char * port;
	char addr[500]; 
	unsigned long ul = 5;
	
	sockaddr_in server;
	
	
	char * RequestBuffer = (char*)malloc(1024);
	
	host = strstr(urlbuf, "http://")+7;
	
	if(strstr(host, ":")!=NULL){
		port = strstr(urlbuf, ":")+1;
		*(port-1) = 0x00;
		port = (char*)atoi(port);
		server.sin_port = htons((int)port);
		strcpy(addr, strstr(port, "/"));
		*(strstr(port, "/"))=0x00;
	} else{
		server.sin_port = htons(80);
		strcpy(addr, strstr(host, "/"));
		*(strstr(host, "/"))=0x00;
	}
	
	wsprintf(RequestBuffer,"GET %s HTTP/1.0\r\nHost:%s\r\n\r\n", addr, host);
	
	
	//socket
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if(s==SOCKET_ERROR)
		return 0;
	
	ioctlsocket(s, FIONBIO, &ul);
	
	
	if(*(host)> 0x30 && *(host)<0x3A){//ip address
		server.sin_addr.s_addr = inet_addr(host);
	} else {
		server.sin_addr = *(struct in_addr*)gethostbyname(host)->h_addr_list[0];
	}
	server.sin_family = AF_INET;
	
	if(connect(s, (struct sockaddr *)&server, sizeof(server))!=0){
		if(WSAGetLastError()!=WSAEWOULDBLOCK) return 0;
	}
	
	Sleep(1000);
	sendto(s, RequestBuffer, strlen(RequestBuffer), 0, (sockaddr*)&server, sizeof(server));
	
	char * bf = buffer;
	int il = size;
	
	DWORD t = GetTickCount();
	while((GetTickCount()-t) < 60000){
		int rec = recv(s, bf, il, 0);
		if(rec==0)
			break;
		if(rec==SOCKET_ERROR)
			continue;
		bf += rec;
		il -= rec;
	}
	
	closesocket(s);
	return size-il;
}



static nLVw kaillera_mlv;



int kaillera_mslistColumn;
int kaillera_mslistColumnTypes[7] = {1, 1, 0, 0, 0, 1, 1};
int kaillera_mslistColumnOrder[7];

int CALLBACK kaillera_mslistCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort){

	int ind1 = kaillera_mlv.Find(lParam1);
	int ind2 = kaillera_mlv.Find(lParam2);
	if (ind1 == -1 || ind2 == -1)
		return 0;

	char ItemText1[128];
	char ItemText2[128];

	
	kaillera_mlv.CheckRow(ItemText1, 128, lParamSort, ind1);
	kaillera_mlv.CheckRow(ItemText2, 128, lParamSort, ind2);

	if (kaillera_mslistColumnTypes[lParamSort]) {
		if (kaillera_mslistColumnOrder[lParamSort])
			return strcmp(ItemText1, ItemText2);
		else
			return -1* strcmp(ItemText1, ItemText2);
	} else {
		ind1 = atoi(ItemText1);
		ind2 = atoi(ItemText2);

		if (kaillera_mslistColumnOrder[lParamSort])
			return (ind1==ind2? 0 : (ind1>ind2? 1 : -1));
		else
			return (ind1==ind2? 0 : (ind1>ind2? -1 : 1));

	}
}

void kaillera_mslistReSort() {
	ListView_SortItems(kaillera_mlv.handle, kaillera_mslistCompareFunc, kaillera_mslistColumn);
}

void kaillera_mslistSort(int column) {
	kaillera_mslistColumn = column;

	if (kaillera_mslistColumnOrder[column])
		kaillera_mslistColumnOrder[column] = 0;
	else
		kaillera_mslistColumnOrder[column] = 5;

	kaillera_mslistReSort();
}



class ListRefresher: public nThread {
public:
	
	bool running;
	bool runn;
	char buffer[0x8000];
	
	slist<PingSocket*, KAILLERA_PING_SIMULT> plist;
	
	
	
	void run(){
		
		runn = running = true;

		SetWindowText(kaillera_mslref,"Downloading list...");

		SetDlgItemText(kaillera_mslref, BTN_REFRESH, "Stop");
		kaillera_mlv.DeleteAllRows();
		memset(buffer, 0, 0x7FFF);
		int l = DownloadListToBuffer(buffer, 0x8000, "http://kaillera.com/raw_server_list2.php?version=0.9");

		
		
		char * ptr = strstr(buffer, "\r\n\r\n") + 4;
		
		int total=0;
		
		if(ptr==NULL){
			return;
		}
		
		int len = strlen(ptr);
		
		char * end = ptr + len;


		union {
			char xxz[201];
			struct {
				char stt;
				char statusb[200];
			}xx;
		}xxx;
		
		struct {
			unsigned z :2;
		} xx;

		xx.z = 50;
		

		while (runn && (ptr + 30 < end || plist.size() > 0)) { 
			
			char * name;
			
			if (ptr + 30 < end && plist.size() < KAILLERA_PING_SIMULT && plist.size() < total + 1){
				
				name = ptr;
				
				while(*++ptr!='\n'){}*ptr++ = 0;
				
				char * host = ptr;
				
				while(*++ptr!=':'){}*ptr++ = 0;
				
				int port = atoi(ptr);
				while(*++ptr!='\n'){}*ptr++ = 0;
				
				//	10.0.0.0/8
				if (strncmp("10.", host, 3)==0)
					continue;
				//	192.168.0.0/16
				if (strncmp("192.168.", host, 8)==0)
					continue;

				//  172.16.0.0/12
				if (strncmp("172.", host, 4)==0){
					int secoct = atoi(host+4);
					if (secoct >= 16 && secoct <=31)
						continue;
				}

				if (strncmp("127.", host, 4)==0)
					continue;
					

				PingSocket * ps = new PingSocket;
				ps->ping_host(host, port);
				ps->ptr = name;
				
				plist.add(ps);
				continue;
			}
			
			//xxx.xx.stt = st[xx.z++];
			
			wsprintf(xxx.xxz, "Refreshing List... [C: %i, T: %i, P: %i, F: %i]", k_socket::list.size(), total, plist.size(), k_socket::sockets.fd_count);
			SetWindowText(kaillera_mslref, xxx.xxz);
			
			PingSocket::step(10);
			//Sleep(0);

			DWORD ti = GetTickCount();
			
			
			for (int _x = 0; _x < plist.size(); _x++) {
				
				PingSocket * ps = plist.get(_x);
				
				if (ps != 0 && ps->limit != 0 && ps->is_done(ti)){
					
					int ping = ps->return_ping();
					
					char * loca = ps->ptr;
					
					plist.remove(ps);

					delete ps;
					
					_x--;
					
					name = loca;
					
					int x = kaillera_mlv.Find((int)name);
					while(*++loca!= 0){}*loca++ = 0;
					kaillera_mlv.AddRow(name, (int)name);
					
					
					char * host = loca;
					while(*++loca!=0){}*loca++ = ':';
					while(*++loca!=';'){}*loca++ = 0;
					
					x = kaillera_mlv.Find((int)name); kaillera_mlv.FillRow(host, 6, x);
					
					
					char * users = loca;
					while(*++loca!=';'){}*loca++ = 0;
					x = kaillera_mlv.Find((int)name); kaillera_mlv.FillRow(users, 3, x);
					
					
					char * games = loca;
					while(*++loca!=';'){}*loca++ = 0;
					x = kaillera_mlv.Find((int)name); kaillera_mlv.FillRow(games, 4, x);
					
					char * version = loca;
					while(*++loca!=';'){}*loca++ = 0;
					x = kaillera_mlv.Find((int)name); kaillera_mlv.FillRow(version, 5, x);
					
					
					char * location = loca;
					while(*++loca!=0){}*loca++ = 0;
					x = kaillera_mlv.Find((int)name); kaillera_mlv.FillRow(location, 1, x);
					
					
					char xxxj[20] = "1000ms+";
					
					if (ping != -1)
						wsprintf(xxxj, "%ims", ping);
					
					x = kaillera_mlv.Find((int)name); kaillera_mlv.FillRow(xxxj, 2, x);
					
					total++;

				}
				
			}
			
		}
		
		SetWindowText(kaillera_mslref,"Cleaning up...");
		
		while (plist.size() > 0){
			
			PingSocket::step(10);
			DWORD ti = GetTickCount();
			
			for (int _x = 0; _x < plist.size(); _x++) {
				
				PingSocket * ps = plist.get(_x);
				
				if (ps != 0 && ps->limit != 0 && ps->is_done(ti)){
					
					int ping = ps->return_ping();
					
					char * loca = ps->ptr;
					
					plist.remove(ps);

					delete ps;
					
					_x--;
					
					char * name = loca;
					
					int x = kaillera_mlv.Find((int)name);
					while(*++loca!= 0){}*loca++ = 0;
					kaillera_mlv.AddRow(name, (int)name);
					
					
					
					char * host = loca;
					while(*++loca!=0){}*loca++ = ':';
					while(*++loca!=';'){}*loca++ = 0;
					
					x = kaillera_mlv.Find((int)name); kaillera_mlv.FillRow(host, 6, x);
					
					
					char * users = loca;
					while(*++loca!=';'){}*loca++ = 0;
					x = kaillera_mlv.Find((int)name); kaillera_mlv.FillRow(users, 3, x);
					
					
					char * games = loca;
					while(*++loca!=';'){}*loca++ = 0;
					x = kaillera_mlv.Find((int)name); kaillera_mlv.FillRow(games, 4, x);
					
					char * version = loca;
					while(*++loca!=';'){}*loca++ = 0;
					x = kaillera_mlv.Find((int)name); kaillera_mlv.FillRow(version, 5, x);
					
					
					char * location = loca;
					while(*++loca!=0){}*loca++ = 0;
					x = kaillera_mlv.Find((int)name); kaillera_mlv.FillRow(location, 1, x);
					
					
					char xxxj[20] = "1000ms+";
					
					if (ping != -1)
						wsprintf(xxxj, "%ims", ping);
					
					x = kaillera_mlv.Find((int)name); kaillera_mlv.FillRow(xxxj, 2, x);
					
					total++;
					
				}
				
			}

		}

		wsprintf(xxx.xxz, "%i servers found", total);
		SetWindowText(kaillera_mslref,xxx.xxz);


		SetDlgItemText(kaillera_mslref, BTN_REFRESH, "Refresh");
		

		running = false;
		
	}
	
	
	
	
	void start(){
		yield();yield();yield();yield();yield();yield();yield();yield();

		if (!running)
			create();
		else runn = false;
	}
	
	
} refresher_thread;

//////////////////////////////////////////////////
void KLSListAdd(char * name, char * host);


void kaillera_mldlg_connect_selected(HWND hDlg){
	refresher_thread.runn = false;
	int selected = kaillera_mlv.SelectedRow();
	if (selected >= 0) {
		char * buf = (char*)kaillera_mlv.RowNo(selected);
		char * name = buf;
		while(*++buf!= 0){}*buf++ = 0;
		char * host = buf;
		while(*++buf!= 0){}*buf++ = 0;
		char * users = buf;
		while(*++buf!= 0){}*buf++ = 0;
		char * games = buf;
		while(*++buf!=0){}*buf++ = 0;
		char * version = buf;
		while(*++buf!=0){}*buf++ = 0;
		char * location = buf;
		while(*++buf!=0){}*buf++ = 0;
		char * tx = host;
		int port = 27888;
		while (*++host != ':' && *host != 0);
		if (*host == ':') {
			*host++ = 0x00;
			port = atoi(host);
			port = port==0?27888:port;
		}
		host = tx;
		ConnectToServer(host, port, hDlg, name);
	}
}

LRESULT CALLBACK MasterSLDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_INITDIALOG:

			kaillera_mslref = hDlg;

			refresher_thread.running = false;

			SetWindowText(hDlg, "Master servers list...");

			kaillera_mlv.handle = GetDlgItem(hDlg, LV_SLIST);
			kaillera_mlv.AddColumn("Name", 140);
			kaillera_mlv.AddColumn("Location", 140);
			kaillera_mlv.AddColumn("ping", 60);
			kaillera_mlv.AddColumn("users", 50);
			kaillera_mlv.AddColumn("games", 50);
			kaillera_mlv.AddColumn("ver", 40);
			kaillera_mlv.AddColumn("IP", 140);
			kaillera_mlv.FullRowSelect();

			break;
		case WM_CLOSE:
			EndDialog(hDlg, 0);
			break;
		case WM_NOTIFY:
			switch (((NMHDR*)lParam)->code) {
				case NM_DBLCLK:
				if(((NMHDR*)lParam)->hwndFrom == kaillera_mlv.handle){
					kaillera_mldlg_connect_selected(hDlg);
					return TRUE;					
				}
				break;
			}
			if(((LPNMHDR)lParam)->code==LVN_COLUMNCLICK && ((LPNMHDR)lParam)->hwndFrom==kaillera_mlv.handle){
				kaillera_mslistSort(((NMLISTVIEW*)lParam)->iSubItem);
			}

			break;
		case WM_COMMAND:
			if (LOWORD(wParam)==BTN_CONNECT){
				kaillera_mldlg_connect_selected(hDlg);
			} else if (LOWORD(wParam)==BTN_REFRESH) {
				refresher_thread.start();
			} else if (LOWORD(wParam)==BTN_CLOSE) {
				EndDialog(hDlg, 0);
			} else if (LOWORD(wParam)==BTN_ADD) {

				int selected = kaillera_mlv.SelectedRow();
				if (selected >= 0) {

					char * buf = (char*)kaillera_mlv.RowNo(selected);

					char * name = buf;
					while(*++buf!= 0){}*buf++ = 0;
					
					char * host = buf;
					while(*++buf!= 0){}*buf++ = 0;
					
					char * users = buf;
					while(*++buf!= 0){}*buf++ = 0;
					
					char * games = buf;
					while(*++buf!=0){}*buf++ = 0;
					
					char * version = buf;
					while(*++buf!=0){}*buf++ = 0;
					
					char * location = buf;
					while(*++buf!=0){}*buf++ = 0;
					
					KLSListAdd(name, host);

				}
				
			}

			break;
	};
	return 0;
}
