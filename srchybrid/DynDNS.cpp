// MORPH START - Added by Commander, DynDNS.org IP updater

//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "stdafx.h"
#include <share.h>
#include "emule.h"
#include "otherfunctions.h"
#include "Preferences.h"
#include "emuleDlg.h"
#include "sockets.h"
#include "HttpDownloadDlg.h"
#include <wininet.h>
#include "DynDNS.h"
#include "AmHttpSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void CDynDNS::UpdateDynDNSAccount(bool account[]){

// Needed vars
CString hostname;
CString username;
CString password;
CString currentip;
CString userpass;
CString buffer;
CString status;
CString type;

// Maybe another IP checking function pls!?
if(!theApp.IsConnected())
	theApp.serverconnect->ConnectToAnyServer();
	
currentip = ipstr(theApp.serverconnect->GetClientID());

/*
CString hostnames = _T("");

bool b_firsthost = true;

for(i=0;i<=accounts.GetLenght();i++){
    if(accounts.GetAt(i) != true)
       continue;
    else {
        if (b_firsthost){
           hostnames += thePrefs.GetDynDNSHostname( i );
           b_firsthost = false;
           }
        else 
           hostnames += _T(",") + thePrefs.GetDynDNSHostname( i );
           }
     break;
}
*/

for(int i=0;i<=accounts.GetLength();i++){
    if(!accounts.GetAt(i))
       continue;
    else {
        // Load values of to updating accounts	
	hostname = thePrefs.GetDynDNSHostname(i);
	username = thePrefs.GetDynDNSUsername(i); 
	password = thePrefs.GetDynDNSPassword(i);
        
        //Melt string together and prepare for Base64 encoding
	buffer.Format(_T("%s:%s"), username, password);
	userpass = buffer;

    	// Userpass has to be encoded in Base64 - God knows why... :)
    	CString str64;
    	int nDestLen = Base64EncodeGetRequiredLength(userpass.GetLength());
    	Base64Encode((const BYTE*)(LPCSTR)userpass, userpass.GetLength(),str64.GetBuffer(nDestLen), &nDestLen);
    	str64.ReleaseBuffer(nDestLen);
        
        // Generate the Update URL
	CString strURL;
	buffer.Format(_T("http://%s@members.dyndns.org/nic/update?system=dyndns&hostname=%s&myip=%s"), str64, hostname, currentip);
	strURL = buffer;

	// Advanced Http-Functions
	CAmHttpSocket request;
	CString answer;

	// Save the server answer in 'answer'
	answer = request.GetPage(strURL, false, NULL);

	// check on return codes from the server
	switch(answer){
	
		case 'badagent':
			status = _T("The user agent that was sent has been blocked for not following these specifications or no user agent was specified");
			type = _T("MB_ICONERROR");
			break;
		case 'badauth':
			status = _T("The username or password specified are incorrect");
			type = _T("MB_ICONERROR");
			break;
		case 'good':
			status = _T("The update was successful, and the hostname is now updated");
			type = _T("MB_ICONINFORMATION");
			break;
		case 'nochg':
			status = _T("The update changed no settings, and is considered abusive. Additional updates will cause the hostname to become blocked!");
			type = _T("MB_ICONERROR");
			break;
		case 'notfqdn':
			status = _T("The hostname specified is not a fully-qualified domain name (not in the form hostname.dyndns.org or domain.com).");
			type = _T("MB_ICONERROR");
			break;
		case 'nohost':
			status = _T("The hostname specified does not exist (or is not in the service specified in the system parameter)");
			type = _T("MB_ICONERROR");
			break;
		case '!yours':
			status = _T("The hostname specified exists, but not under the username specified");
			type = _T("MB_ICONERROR");
			break;
		case 'abuse':
			status = _T("The hostname specified is blocked for update abuse!");
			type = _T("MB_ICONERROR");
			break;
		case 'numhost':
			status = _T("Too many or too few hosts found");
			type = _T("MB_ICONERROR");
			break;
		case 'dnserr':
			status = _T("DNS error encountered");
			type = _T("MB_ICONERROR");
			break;
		case '911':
			status = _T("There is a serious problem on our side, such as a database or DNS server failure. You stop updating until notified via the status page (http://www.dyndns.org/news/status) that the service is back up.");
			type = _T("MB_ICONERROR");
			break;
			
		default:
			status = _T("No return code recieved from the server");
			type = _T("MB_ICONERROR");
			break;
			}

		// Show the server answer to the user
		AfxMessageBox(_T("status"), type);
	}
   break;
   }

}
/*
void CDynDNS::Reset(){


}
*/
/*
DynDNS::GetEnabledAccounts(bool accounts[]){
int number = 0;
for(i=0;i<=accounts.GetLenght();i++){
    if(accounts.GetAt(i) != true)
       continue; 
    else
    	number++;
        
     break;
     }
return number;
}
*/
// MORPH END - Added by Commander, DynDNS.org IP updater
