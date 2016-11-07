/* Progetto di Programmazione di Sistema, prof. Giovanni Malnati */
/* Developed by Riccardo Persiani and Davide Visentin */

#include "stdafx.h"

/* RapidXML Libraries */
#include "rapidxml-1.13/rapidxml.hpp"
#include "rapidxml-1.13/rapidxml_print.hpp"
#include "rapidxml-1.13/rapidxml_utils.hpp"
#include "rapidxml-1.13/rapidxml_iterators.hpp"

/* Standard Libraries */
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <comdef.h>  // for WCHAR* to char*

/* Need? */
#pragma comment(lib, "Ws2_32.lib")

/* Need? */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

/* Namespaces */
using namespace std;
using namespace rapidxml;

/* Global Variables */ 
HWND focus_handle = 0; //
int count_windows = 0; // Counting how many windows are open 
xml_document<> doc; // XML Document which containts the root of the tree

HBITMAP BitmapFromIcon(HICON hIcon)
{
	ICONINFO iconinfo;
	GetIconInfo(hIcon, &iconinfo);

	// Return colored bitmap of the icon?
	return iconinfo.hbmColor;
}

void OpenSocket() {
	WSADATA wsa;
	SOCKET s, new_socket;
	struct sockaddr_in server, client;
	int c;
	char *message;

	// Rapidxml - root node: application list with attribute server="130.192.0.10"
	xml_node<>* root = doc.allocate_node(node_element, "application_list");
	root->append_attribute(doc.allocate_attribute("server", "130.192.0.10"));
	doc.append_node(root);
	
	xml_node<>* window = doc.allocate_node(node_element, "window");
	window->append_attribute(doc.allocate_attribute("id", "1"));
	window->append_attribute(doc.allocate_attribute("focus", "0"));
	window->append_attribute(doc.allocate_attribute("operation", "add"));
	doc.first_node("application_list")->append_node(window);

	xml_node<>* title = doc.allocate_node(node_element, "title");
	doc.first_node("application_list")->first_node("window")->append_node(title);
	title->value("blocco note");

	xml_node<>* icon = doc.allocate_node(node_element, "icon");
	doc.first_node("application_list")->first_node("window")->append_node(icon);

	cout << "Printing XML document after root node declaration " << doc << endl;

	printf("Initialising Winsock... ");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
	}

	printf("Initialised.\n");

	//Create a socket
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		printf("Failed. Could not create socket : %d", WSAGetLastError());
	}

	printf("Socket created.\n");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(1993);

	//Bind
	if (bind(s, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
	{
		printf("Failed. Bind failed with error code : %d", WSAGetLastError());
	}

	puts("Binding done.\n");

	//Listen to incoming connections
	listen(s, 3);

	//Accept and incoming connection
	puts("Waiting for incoming connections... ");

	c = sizeof(struct sockaddr_in);
	new_socket = accept(s, (struct sockaddr *)&client, &c);
	if (new_socket == INVALID_SOCKET)
	{
		printf("accept() failed with error code : %d", WSAGetLastError());
	}

	puts("Connection accepted!");

	/* Setting info for reply to the client */
	char buffer[4096];                      // You are responsible for making the buffer large enough!
	char *end = print(buffer, doc, 0);      // end contains pointer to character after last printed character
	*end = '\r';
	*(end + 1) = '\n';

	/* Server reply to the client */
	send(new_socket, buffer, strlen(buffer), 0);

	getchar();

	/* Closing the socket + cleanup */
	closesocket(s);
	WSACleanup();

	//return 0;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
		if (IsWindowVisible(hwnd)) // Check if the window is visible
		{
			WCHAR wnd_title[256];
			GetWindowText(hwnd, wnd_title, sizeof(wnd_title));
			wcout << count_windows <<" - "<< wnd_title << endl;
			count_windows++;

			if (hwnd == focus_handle) {
				wcout << " <-- This window has FOCUS" << endl;
				// Prendo handle dell'icona
				HICON hicon = (HICON)GetClassLong(hwnd, GCL_HICON);
				// Converto handle icona in un handle di una bitmap perchè poi posso ottere i bit dell'icona
				HBITMAP hbit = BitmapFromIcon(hicon);
				BITMAPFILEHEADER   bmfHeader;
				BITMAPINFOHEADER   bi;
				BITMAP bitmap;

				// Ottengo una bitmap dell'icona perchè così conosco la dimensione 
				GetObject(hbit, sizeof(BITMAP), &bitmap);

				bi.biSize = sizeof(BITMAPINFOHEADER);
				bi.biWidth = bitmap.bmWidth;
				bi.biHeight = bitmap.bmHeight;
				bi.biPlanes = 1;
				bi.biBitCount = 24;
				bi.biCompression = BI_RGB;
				bi.biSizeImage = 0;
				bi.biXPelsPerMeter = 0x0ec4;
				bi.biYPelsPerMeter = 0x0ec4;
				bi.biClrUsed = 0;
				bi.biClrImportant = 0;

				// Dimensione della bitmap in byte
				DWORD dwBmpSize = ((bitmap.bmWidth * bi.biBitCount + 31) / 32) * 4 * bitmap.bmHeight;
				
				// Starting with 32-bit Windows, GlobalAlloc and LocalAlloc are implemented as wrapper functions that 
				// call HeapAlloc using a handle to the process's default heap. Therefore, GlobalAlloc and LocalAlloc 
				// have greater overhead than HeapAlloc.
				HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
				char *lpbitmap = (char *)GlobalLock(hDIB);

				//Prendo i bit dell'icona
				HDC hdc = GetDC(hwnd);
				GetDIBits(hdc, hbit, 0, (UINT)bitmap.bmHeight, lpbitmap, (BITMAPINFO*)&bi,DIB_RGB_COLORS);

				// A file is created, this is where we will save the screen capture.
				HANDLE hFile = CreateFile(L"captureqwsx.bmp",
					GENERIC_WRITE,
					0,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL, NULL);

				// Add the size of the headers to the size of the bitmap to get the total file size
				DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

				//Offset to where the actual bitmap bits start.
				bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

				//Size of the file
				bmfHeader.bfSize = dwSizeofDIB;

				//bfType must always be BM for Bitmaps
				bmfHeader.bfType = 0x4D42; //BM   

				DWORD dwBytesWritten = 0;
				WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
				WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
				WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);

				//Close the handle for the file that was created
				CloseHandle(hFile);

				
			}
			
			// rapidxml - XML declaration
			/*xml_node<>* decl = doc.allocate_node(node_declaration);
			 *decl->append_attribute(doc.allocate_attribute("version", "1.0"));
			 *decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
			 *doc.append_node(decl);
			*/
			
			//rapidxml - windown ode

			//d.SetObject(); // Instanzia oggetto a NULL

			//Value tmp; // Window attuale
			//tmp.SetObject(); // Instanzia oggetto a NULL

			//Set "id" value for each window
			/*Value window_id((int)hwnd);
			tmp.AddMember("id", window_id, d.GetAllocator());
			*/
			// Set "focus" value 1 for window with focus 
			/*
			if(hwnd == focus_handle){
				Value has_focus(true);
				tmp.AddMember("focus", has_focus, d.GetAllocator());

			}
			else {
				Value has_focus(false);
				tmp.AddMember("focus", has_focus, d.GetAllocator());

			}

			// Set "operation" value for operation to be performed
			Value window_operation("add");
			tmp.AddMember("operation", window_operation, d.GetAllocator());

			// Set "title" value for each window
			Value window_title("yo");
			char* char_wnd_title = new char[wcslen(wnd_title) + 1];
			char DefChar = ' ';
			WideCharToMultiByte(CP_ACP, 0, wnd_title, -1, char_wnd_title, 260, &DefChar, NULL);

			window_title = StringRef(char_wnd_title);
			tmp.AddMember("title", window_title, d.GetAllocator());

			window_list.PushBack(tmp, d.GetAllocator());			
			*/
		}

		return true; // function must return true if you want to continue enumeration
}

int main()
{
	/* Keyboard input */
	INPUT ip;
	ip.type = INPUT_KEYBOARD;
	ip.ki.wScan = 0;
	ip.ki.time = 0;
	ip.ki.dwExtraInfo = 0;

	/* Focus on window */
	focus_handle = GetForegroundWindow();
	wcout << "focus_handle = " << focus_handle << endl;
	EnumWindows(EnumWindowsProc, 0);

	// Apro il socket
	OpenSocket();
	
	/* Server + array di tutte le finestre + finestra con focus + tra
	 * i campi della parte di json va aggiunto un campo e devo aggiungere,
	 * togliere o mpdificare la ifnstra+icona con maschera e colore*/
	//d.AddMember("server", server, d.GetAllocator());

	//d.AddMember("allocations", window_list, d.GetAllocator());

	//StringBuffer buffer;
	//Writer<StringBuffer> writerUTF8(buffer);
	//d.Accept(writerUTF8);
	//cout << buffer.GetString() << endl;

	getchar();
}