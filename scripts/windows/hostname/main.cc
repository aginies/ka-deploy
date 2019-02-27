/********************************************************/
/*Author: Arnaud Defrenne								*/
/*Date: 13/09/2001										*/
/*Lab: INRIA Rhone-Alpes (France) Projet Apache			*/
/*File Name: main.cpp									*/
/*Project:	HostnameService.mcp (CodeWarrior)			*/
/*Subject:												*/
/*		This program has to be launch whenever we need	*/
/*		to change the names of the computers on the		*/
/*		iCluster project.								*/
/*		1. 	We find out the IP adress of the local 		*/
/*			computer.									*/
/*		2.	We find out the DNS name of the local 		*/	
/*			computer from the DNS server (lookup on the */
/*			IP).We use a system call to a script copying*/
/*			stdout from nslookup to a file				*/
/*		3.	We name (NETBIOS) the computer after the 	*/
/*			DNS name we have just found out.		 	*/
/*		4.	We modify the windows register in order to 	*/
/*			change the local name (= dns name).			*/
/********************************************************/

#ifndef _SETMACHINENAME
#define _SETMACHINENAME

#include <windows.h>
#include <iostream.h>
#include <string>
#include <IPHlpApi.h>
#include <stdio.h>
#include "srchfile.h"


/**
	get the local IP adress from the system
*/
in_addr getLocalIp(void) {
	
	PMIB_IPADDRTABLE myaddrtable= new _MIB_IPADDRTABLE;
	BOOL order(0);
	PULONG size_buffer = new unsigned long(100);
	in_addr myip;

	if (GetIpAddrTable(myaddrtable, size_buffer, order)) 
		throw "problem getting IP adress Table ";
		
	myip.s_addr = myaddrtable->table[0].dwAddr; 

	delete size_buffer;	
	return myip;
}


/**
	if we can't find the real name, let's invent a new one
*/
bool inventNewName(char* buffer) {
	char* store = new char[20];

	strcpy(buffer, "iCluster");
	strcat(buffer, _itoa(GetTickCount(),store,10));
	delete store;
	return true;
}

/**
	get the dns name from the local IP, by asking the default dns server.
	through a system call to nslookup
	NB: need c:\getDNSName.cmd
*/
bool getDNSNameByServer(in_addr localIp, char* ComputerName) {
	//parameters of the System call trough ShellExecute
	HWND WindowState = GetActiveWindow();
	LPCTSTR operation = "open";
	LPCTSTR command = "c:\\install_cluster\\getDNSName.cmd";
	char* filename = "c:\\install_cluster\\iClusterTemp.txt";
	FILE* f;
	LPCTSTR parameter = inet_ntoa(localIp);
	LPCTSTR directory = "c:\\";
	HINSTANCE herror;
	char* separator1f = "Nom";
	char* separator1e = "Name";
	char* separator2 = ".";
	char* fileContent = new char[100];
	int i=0;	
	
	herror = ShellExecute(WindowState, operation, command, parameter, NULL, SW_HIDE);
	sleep(3); //wait for the file to be ready
	
	//open the file generated with nslookup	
	if ((f = fopen(filename,"r+"))==NULL) {
		char* err = "Problem opening file:";
		delete fileContent;
		throw strcat(err, filename);
	} 
		
	rewind(f);
	//copy the content of the file to a string	
	while ( i<100) {			
		*(fileContent + i++) = fgetc(f);
	}
	
	//did nslookup found the right info?
	//if not, invent a name
	char* ret = strstr(fileContent,separator1f);
	if (ret==NULL) ret = strstr(fileContent,separator1e); 
	if (ret==NULL) {
		delete fileContent;
		return false;
	}
	
	//get the ComputerName out of this  file
	ret = ret + 9;
	strcpy(ComputerName, strtok(ret,separator2));
	
	//close the temp file
	fclose(f);
	if (remove(filename) != 0) {
		char* err = "problem deleting file: ";
		delete fileContent;
		throw strcat(err, filename);
	}
	return true;
}

/**
	set the new DNS like name of the machine
*/
bool setDNSLocalName(const char* newname) {
 	/*first, we open the right HKEYs 										*/
 	/*	HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Services\Tcpip\Parameters	*/
 	/*and HKEY_LOCAL_MACHINE\SYSTEM\ControlSet002\Services\Tcpip\Parameters	*/
 	HKEY hkey = HKEY_LOCAL_MACHINE; 
 	LPCTSTR path1 = "SYSTEM\\ControlSet001\\Services\\Tcpip\\Parameters";
 	LPCTSTR path2 = "SYSTEM\\ControlSet002\\Services\\Tcpip\\Parameters";
 	REGSAM secu = KEY_SET_VALUE;
 	PHKEY pkey1 = new HKEY;
 	PHKEY pkey2 = new HKEY;
 		
	if (!((RegOpenKeyEx(hkey, path1, 0, secu, pkey1)==ERROR_SUCCESS) &&
		(RegOpenKeyEx(hkey, path2, 0, secu, pkey2)==ERROR_SUCCESS))) {
			delete pkey1;
			delete pkey2;
			throw "problem opening HKEYs, can't set the new DNS name";
	}
 
  	/*then, we set the new HKEYs value*/
	LPCSTR subkey = "NV Hostname";
	RegSetValueEx(*pkey1, subkey, 0UL, static_cast<unsigned long>(REG_SZ), 
	 		reinterpret_cast<const unsigned char *>(newname), static_cast<unsigned long>(strlen(newname))); 		
 	RegSetValueEx(*pkey2, subkey, 0UL, static_cast<unsigned long>(REG_SZ), 
	 		reinterpret_cast<const unsigned char *>(newname), static_cast<unsigned long>(strlen(newname))); 	
	
	delete pkey1;
	delete pkey2;
	return true;
}


int main() {
	
	
 	unsigned long size_buf = 50; 
	char* DNSname = new char[size_buf];
	char* hostname = new char[size_buf];
      
 	try {
 	
 		/*get info about the system, for the log*/
		unsigned long* nSize = new unsigned long(50);
		char* username = new char[50];
		bool echec1 = true;
		bool echec2 = false;
		int i=0;
		
		GetUserName(username, nSize);
		LPSYSTEMTIME lpSystemTime = new SYSTEMTIME;
		GetLocalTime(lpSystemTime);
		
 	
 		/*open the log file*/
 		FILE *f;

		// re-direct output from the console to a new file
		if (( f = freopen("c:\\install_cluster\\mylog.TXT", "a", stdout)) == NULL) {
			throw "Can't create new stdout file";
		}
 			
 		
 		/*append to the log file*/
 		cout << lpSystemTime->wDay << "/" << lpSystemTime->wMonth << "/" << lpSystemTime->wYear << " ";
		cout << lpSystemTime->wHour << "." << lpSystemTime->wMinute << "." << lpSystemTime->wSecond << " : ";
 		cout << " username=" << username << endl;
 		
 		/*we find the name of the computer (try several time if the dns server is overloaded)*/
	 	echec1 = !getDNSNameByServer(getLocalIp(), DNSname);
	 	cout << "try info form nslookup...";
	 	while (echec1 && (i<=5)){
	 		echec1 = !getDNSNameByServer(getLocalIp(), DNSname);
	 		i++;
	 		cout << "problem getting info from dns. Try nb:" << i << endl;
	 		sleep(2);
	 	}
	 	
	 	/*if nslookup doesn't give anything, then first try to get it from c:\winnt\system32\drivers\etc\hosts*/
	 	if (echec1) {
	 		FILE *hosts;
			long int success = 0L;
			char* sep = "\t .";
			cout << "try info from hosts file...";
			if (( hosts = fopen("c:\\winnt\\system32\\drivers\\etc\\hosts", "r")) == NULL) 
				echec2 = true;
			else { //file open ok
				char* ip = inet_ntoa(getLocalIp());
				success = ffsearch(hosts, ip, strlen(ip), 1);

				if (success >= 0) { //ok we found it
					fseek(hosts, strlen(ip)+success, SEEK_SET);
					if (fgets(DNSname, 50, hosts)!=NULL){
						DNSname = strtok(DNSname, sep);
					} else echec2 = true;
				} else echec2 = true;	 	
	 		}
	 	
	 	/*if the info is not in the file the we concatenate icluster + last elem from ip*/
	 	} else if (echec2) {
	 		char* sep = ".";
	 		char *token, *next;
	 		char* ip = inet_ntoa(getLocalIp());
	 		char * DNSname = "";
			cout << "set dnsname by hand...";
	 		strcat(DNSname, "icluster");
	 		
	 		next = strtok(ip, sep);
	 		
	 		while (next != NULL) {
	 			token = next;
	 			next = strtok(NULL, sep);
	 		}
	 		strcat(DNSname, token);	
	 	}
	 	
	 	
	 	
	 	/*if the current hostname is the same than the one*/
	 	/*extracted from the dns, then don't change anything*/
		WORD wVersionRequested = MAKEWORD( 2, 0 );
		WSADATA wsaData;
 		if (WSAStartup( wVersionRequested, &wsaData )) 
 			throw "problem opening socket";
 		sleep(3);
		gethostname(hostname, size_buf);
		
		/*closing socket*/
		WSACleanup();
		//if the name we found from the DNS is different to the current local name, then change it
		if (memcmp(DNSname, hostname, size_buf)!=0) {
			cout << "we rename the computer" << endl;
			
			/*we modify the NETBIOS name of the machine	*/
			/*PS: need to reboot 						*/
			
			SetComputerName(DNSname);
		
			/*we modify the DNS like local name of the machine	*/
			/*NB: need to modify the registers and reboot		*/
			setDNSLocalName(DNSname);
	 	
	 		
		} else cout << "don't rename the computer" << endl;
		
		
		//fclose(f);
	
 	} catch (const char* e) {
 		MessageBox(0, e, NULL, MB_APPLMODAL | MB_ICONSTOP);
 		cout << "Exception caught: " << e << endl;
 
 	} catch (...) {
 		char* e = "My Unhandled Exception";
		MessageBox(0, e, NULL, MB_APPLMODAL | MB_ICONSTOP);
		cout << "Exception caught: " << endl;
	
 	}
}	



	 	

#endif //_SETMACHINENAME