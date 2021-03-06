//========================================================================
//
// Network.cpp - the core classes for creating a multiplayer game
//
// Part of the GameEngine Application
//
// GameEngine is the sample application that encapsulates much of the source code
// discussed in "Game Coding Complete - 4th Edition" by Mike McShaffry and David
// "Rez" Graham, published by Charles River Media. 
// ISBN-10: 1133776574 | ISBN-13: 978-1133776574
//
// If this source code has found it's way to you, and you think it has helped you
// in any way, do the authors a favor and buy a new copy of the book - there are 
// detailed explanations in it that compliment this code well. Buy a copy at Amazon.com
// by clicking here: 
//    http://www.amazon.com/gp/product/1133776574/ref=olp_product_details?ie=UTF8&me=&seller=
//
// There's a companion web site at http://www.mcshaffry.com/GameCode/
// 
// The source code is managed and maintained through Google Code: 
//    http://code.google.com/p/GameEngine/
//
// (c) Copyright 2012 Michael L. McShaffry and David Graham
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser GPL v3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See 
// http://www.gnu.org/licenses/lgpl-3.0.txt for more details.
//
// You should have received a copy of the GNU Lesser GPL v3
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//========================================================================

 
#include <errno.h>
#include "Network.h"

#include "Core/OS/Os.h"
#include "Core/Event/Event.h"
#include "Core/Event/EventManager.h"

#pragma comment(lib, "Ws2_32")

const char *BinaryPacket::Type = "BinaryPacket";
const char *TextPacket::Type = "TextPacket";


BaseSocketManager* BaseSocketManager::SocketMngr = NULL;

void testCode()
{
	// Use of utility and conversion functions.
	unsigned long ipAddress = inet_addr("128.64.16.2");

	struct in_addr addr;
	addr.S_un.S_addr = htonl(0x88482818);

	char ipAddressString[16];
	strcpy(ipAddressString, inet_ntoa(addr));

	char buffer[256];
	sprintf(buffer, "0x%08x 0x%08x %s\n:", ipAddress, addr.S_un.S_addr, ipAddressString);
	OutputDebugStringA(buffer);


	// Use of DNS functions
	struct hostent *pHostEnt; 
	const char *host = "ftp.microsoft.com";
	pHostEnt = gethostbyname(host);

    if (pHostEnt == NULL)
    {
        fprintf(stderr, "No such host");
    }
	else
	{
	    struct sockaddr_in addr;
		memcpy(&addr.sin_addr,pHostEnt->h_addr,pHostEnt->h_length);

		char buffer[256];
		sprintf(buffer, "Address of %s is 0x%08x\n", host, ntohl(addr.sin_addr.s_addr));
		OutputDebugStringA(buffer);
	}

	unsigned int netip = inet_addr("207.46.133.140");
	pHostEnt = gethostbyaddr((const char *)&netip, 4, PF_INET);
}

//
// TextPacket::TextPacket			- not described in the book
//
TextPacket::TextPacket(char const * const text)
 :BinaryPacket(static_cast<u_long>(strlen(text) + 2))
{ 
	MemCpy(text, strlen(text), 0);
	MemCpy("\r\n", 2, 2);
	*(u_long *)mData = 0;
}

//-------------------------------------------------------------------
// NetSocket Implementation

//
// NetSocket::NetSocket							- Chapter 19, page 668
//
NetSocket::NetSocket() 
{ 
	mSock = INVALID_SOCKET;
	mDeleteFlag = 0;
	mSendOfs = 0;
	mTimeOut = 0;

	mRecvOfs = mRecvBegin = 0;
	mInternal = 0;
	mIsBinaryProtocol = 1;
}

//
// NetSocket::NetSocket							- Chapter 19, page 668
//
NetSocket::NetSocket(SOCKET new_sock, unsigned int hostIP)
{
	mDeleteFlag = 0;
	mSendOfs = 0;
	mTimeOut = 0;

	mIsBinaryProtocol = 1;

	mRecvOfs = mRecvBegin = 0;
	mInternal = 0;

	mTimeCreated = Timer::GetTime();

	mSock = new_sock;
	mIPAddr = hostIP;

	mInternal = BaseSocketManager::SocketMngr->IsInternal(mIPAddr);

	setsockopt (mSock, SOL_SOCKET, SO_DONTLINGER, NULL, 0);

	// Here's how to find the host address of the connection. It is very slow, however.
	if (mIPAddr)
	{
		char buffer[128];
		const char *ansiIpaddress = BaseSocketManager::SocketMngr->GetHostByAddr(mIPAddr);
		if (ansiIpaddress)
		{
			char genIpaddress[64];
			//AnsiToGenericCch(genIpaddress, ansiIpaddress, static_cast<int>(strlen(ansiIpaddress)+1));
			//_tcssprintf(buffer, _T("User connected: %s %s"), genIpaddress, (m_internal) ? _T("(internal)") : _T(""));
            sprintf(buffer, "User connected: %s %s", genIpaddress, (mInternal) ? "(internal)" : "");
			OutputDebugStringA(buffer);
		}
	}
}

//
// NetSocket::~NetSocket						- Chapter 19, page 668
//
NetSocket::~NetSocket() 
{
	if (mSock != INVALID_SOCKET) 
	{
		closesocket(mSock);
		mSock = INVALID_SOCKET;
	}
 }


//
// NetSocket::Connect							- Chapter 19, page 669
//
bool NetSocket::Connect(unsigned int ip, unsigned int port, bool forceCoalesce) 
{
	struct sockaddr_in sa;
	int x = 1;

	if ((mSock = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		return false;

	if (!forceCoalesce)
	{
		setsockopt(mSock, IPPROTO_TCP, TCP_NODELAY, (char *)&x, sizeof(x));
	}

	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(ip);
	sa.sin_port = htons(port);

	if (connect(mSock, (struct sockaddr *)&sa, sizeof(sa))) 
	{
		closesocket(mSock);
		mSock = INVALID_SOCKET;
		return false;
	}

	return true;
 }

//
// NetSocket::Send								- Chapter 19, page 670
//
void NetSocket::Send(eastl::shared_ptr<BasePacket> pkt, bool clearTimeOut)
{
	if (clearTimeOut)
		mTimeOut = 0;

	mOutList.push_back(pkt);
}

//
// NetSocket::SetBlocking						- Chapter 19, page 670
//
void NetSocket::SetBlocking(bool blocking) 
{
	#ifdef WIN32
		unsigned long val = blocking ? 0 : 1;
		ioctlsocket(mSock, FIONBIO, &val);
	#else
		int val = fcntl(m_sock, F_GETFL, 0);
		if (blocking)
			val &= ~(O_NONBLOCK);
		else
			val |= O_NONBLOCK;

		fcntl(m_sock, F_SETFL, val);
	#endif
}

//
// NetSocket::HandleOutput						- Chapter 19, page 670
//
void NetSocket::HandleOutput() 
{
	int fSent = 0;
	do 
	{
		LogAssert(!mOutList.empty(), "output is empty");
		PacketList::iterator i = mOutList.begin();

		eastl::shared_ptr<BasePacket> pkt = *i;
		const char *buf = pkt->GetData();
		int len = static_cast<int>(pkt->GetSize());

		int rc = send(mSock, buf+mSendOfs, len-mSendOfs, 0);
		if (rc > 0) 
		{
			BaseSocketManager::SocketMngr->AddToOutbound(rc);
			mSendOfs += rc;
			fSent = 1;
		}
		else if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			HandleException();
			fSent = 0;
		}
		else
		{
			fSent = 0;
		}

		if (mSendOfs == pkt->GetSize()) 
		{
			mOutList.pop_front();
			mSendOfs = 0;
		}

	} while ( fSent && !mOutList.empty() );
}

//
// NetSocket::HandleInput						- Chapter 19, page 671
//
void NetSocket::HandleInput() 
{
	bool bPktRecieved = false;
	u_long packetSize = 0;
	int rc = recv(mSock, mRecvBuf+mRecvBegin+mRecvOfs, RECV_BUFFER_SIZE-(mRecvBegin+mRecvOfs), 0);

	char metrics[1024];
	sprintf_s(metrics, 1024, "Incoming: %6d bytes. Begin %6d Offset %4d\n", rc, mRecvBegin, mRecvOfs);
    LogInformation(metrics);

	if (rc==0)
	{
		return;
	}

	if (rc==SOCKET_ERROR)
	{
		mDeleteFlag = 1;
		return;
	}

	const int hdrSize = sizeof(u_long);
	unsigned int newData = mRecvOfs + rc;
	int processedData = 0;

	while (newData > hdrSize)
	{
		// There are two types of packets at the lowest level of our design:
		// BinaryPacket - Sends the size as a positive 4 byte integer
		// TextPacket - Sends 0 for the size, the parser will search for a CR

		packetSize = *(reinterpret_cast<u_long*>(mRecvBuf+mRecvBegin));
		packetSize = ntohl(packetSize);

		if (mIsBinaryProtocol)
		{
			// we don't have enough new data to grab the next packet
			if (newData < packetSize)
				break;

			if (packetSize > MAX_PACKET_SIZE)
			{
				// prevent nasty buffer overruns!
				HandleException();
				return;
			}

			if (newData >= packetSize)
			{
				// we know how big the packet is...and we have the whole thing
				//
				//[mrmike] - a little code to aid debugging network packets here!
				//char test[1024];
				//memcpy(test, &m_recvBuf[m_recvBegin+hdrSize], packetSize);
				//test[packetSize+1]='\r';
				//test[packetSize+2]='\n';
				//test[packetSize+3]=0;
                //LogInformation(test);
				eastl::shared_ptr<BinaryPacket> pkt(
					new BinaryPacket(&mRecvBuf[mRecvBegin+hdrSize], packetSize-hdrSize));
				mInList.push_back(pkt);
				bPktRecieved = true;
				processedData += packetSize;
				newData -= packetSize;
				mRecvBegin += packetSize;
			}
		}
		else
		{
			// the text protocol waits for a carraige return and creates a string
			char *cr = static_cast<char *>(memchr(&mRecvBuf[mRecvBegin], 0x0a, rc));
			if (cr)
			{
				*(cr+1) = 0;
				eastl::shared_ptr<TextPacket> pkt(new TextPacket(&mRecvBuf[mRecvBegin]));
				mInList.push_back(pkt);
				packetSize = cr - &mRecvBuf[mRecvBegin];
				bPktRecieved = true;

				processedData += packetSize;
				newData -= packetSize;
				mRecvBegin += packetSize;
			}
		}
	}

	BaseSocketManager::SocketMngr->AddToInbound(rc);
	mRecvOfs = newData;

	if (bPktRecieved)
	{
		if (mRecvOfs == 0)
		{
			mRecvBegin = 0;
		}
		else if (mRecvBegin + mRecvOfs + MAX_PACKET_SIZE > RECV_BUFFER_SIZE)
		{
			// we don't want to overrun the buffer - so we copy the leftover bits 
			// to the beginning of the recieve buffer and start over
			int leftover = mRecvOfs;
			memcpy(mRecvBuf, &mRecvBuf[mRecvBegin], mRecvOfs);
			mRecvBegin = 0;
		}
	}	
}


//-------------------------------------------------------------------
// NetListenSocket::NetListenSocket				- Chapter 19, page 674
//
NetListenSocket::NetListenSocket(int portnum) 
{
	port = 0;
	Init(portnum);
}


//
// NetListenSocket::Init						- Chapter 19, page 674
//
void NetListenSocket::Init(int portnum)
{
	struct sockaddr_in sa;
	int value = 1;

	mSock = socket(PF_INET, SOCK_STREAM, 0);
	LogAssert(mSock == INVALID_SOCKET, "NetListenSocket Error: Init failed to create socket handle");

	if (setsockopt(mSock, SOL_SOCKET, SO_REUSEADDR, (char *)&value, sizeof(value))== SOCKET_ERROR) 
	{
		perror("NetListenSocket::Init: setsockopt");
		closesocket(mSock);
		mSock = INVALID_SOCKET;
		LogAssert(mSock == INVALID_SOCKET, "NetListenSocket Error: Init failed to set socket options");

	}
	
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = ADDR_ANY;
	sa.sin_port = htons(portnum);

	// bind to port
	if (bind(mSock, (struct sockaddr *)&sa, sizeof(sa)) == SOCKET_ERROR) 
	{
		perror("NetListenSocket::Init: bind");
		closesocket(mSock);
		mSock = INVALID_SOCKET;
		LogAssert(mSock == INVALID_SOCKET, "NetListenSocket Error: Init failed to bind");
	}

	// set nonblocking - accept() blocks under some odd circumstances otherwise
	SetBlocking(false);

	// start listening
	if (listen(mSock, 256) == SOCKET_ERROR) 
	{
		closesocket(mSock);
		mSock = INVALID_SOCKET;
		LogAssert(mSock == INVALID_SOCKET, "NetListenSocket Error: Init failed to listen");
	}

	port = portnum;
}

//
// NetListenSocket::InitScan			- Chapter X, page Y
//   Opens multiple ports to listen for connections.
//
void NetListenSocket::InitScan(int portnum_min, int portnum_max) 
{
	struct sockaddr_in sa;
	int portnum, x = 1;

	if ((mSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		LogAssert(mSock == INVALID_SOCKET, "Invalid socket");
		exit(1);
	}

	if (setsockopt(mSock, SOL_SOCKET, SO_REUSEADDR, (char *)&x, sizeof(x)) == SOCKET_ERROR) 
	{
		closesocket(mSock);
		mSock = INVALID_SOCKET;
		LogAssert(mSock == INVALID_SOCKET, "Invalid socket");
		exit(1);
	}

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	for (portnum = portnum_min; portnum < portnum_max; portnum++) 
	{
		sa.sin_port = htons(portnum);
		// bind to port
		if (bind(mSock, (struct sockaddr *)&sa, sizeof(sa)) != SOCKET_ERROR)
			break;
	}

	if (portnum == portnum_max) 
	{
		closesocket(mSock);
		mSock = INVALID_SOCKET;
		LogAssert(mSock == INVALID_SOCKET, "Invalid socket");
		exit(1);
	}

	// set nonblocking - accept() blocks under some odd circumstances otherwise
	SetBlocking(false);

	// start listening
	if (listen(mSock, 8) == SOCKET_ERROR) 
	{
		closesocket(mSock);
		mSock = INVALID_SOCKET;
		LogAssert(mSock == INVALID_SOCKET, "Invalid socket");
		exit(1);
	}

	port = portnum;
 }

//
// NetListenSocket::AcceptConnection				- Chapter 19, page 675
//
SOCKET NetListenSocket::AcceptConnection(unsigned int *pAddr)
{
	SOCKET new_sock;
	struct sockaddr_in sock;
	int size = sizeof(sock);

	if ((new_sock = accept(mSock, (struct sockaddr *)&sock, &size))== INVALID_SOCKET)
		return INVALID_SOCKET;

	if (getpeername(new_sock, (struct sockaddr *)&sock, &size) == SOCKET_ERROR)
	{
		closesocket(new_sock);
		return INVALID_SOCKET;
	}
	*pAddr = ntohl(sock.sin_addr.s_addr);
	return new_sock;
 }




//
// BaseSocketManager::BaseSocketManager				- Chapter 19, page 677
//
BaseSocketManager::BaseSocketManager() 
{ 
	mInbound = 0;
	mOutbound = 0;
	mMaxOpenSockets = 0;
	mSubnetMask = 0;
	mSubNet = 0xffffffff;
	mNextSocketId = 0;

	SocketMngr = this; 
	ZeroMemory(&mWsaData, sizeof(WSADATA)); 
}


//
// BaseSocketManager::Init							- Chapter 19, page 677
//
bool BaseSocketManager::Init()
{
	if (WSAStartup(0x0202, &mWsaData)==0)
		return true;
	else
	{
		LogError("WSAStartup failure!");
		return false;
	}
}


//
// BaseSocketManager::Shutdown						- Chapter 19, page 678
//
void BaseSocketManager::Shutdown()
{
	// Get rid of all those pesky kids...
	while (!mSockList.empty())
	{
		delete *mSockList.begin();
		mSockList.pop_front();
	}

	WSACleanup();
}

//
// BaseSocketManager::AddSocket					- Chapter 19, page 678
//
int BaseSocketManager::AddSocket(NetSocket *socket) 
{ 
	socket->mID = mNextSocketId;
	mSockMap[mNextSocketId] = socket;
	++mNextSocketId; 

	mSockList.push_front(socket); 	
	if (mSockList.size() > mMaxOpenSockets)
		++mMaxOpenSockets;

	return socket->mID; 
}

//
// BaseSocketManager::RemoveSocket					- not described in the book
//
//   Removes a sock from the socket list - done once it is not connected anymore
//
void BaseSocketManager::RemoveSocket(NetSocket *socket) 
{ 
	mSockList.remove(socket); 
	mSockMap.erase(socket->mID);
	delete socket;
}

//
// BaseSocketManager::FindSocket					- Chapter 19, page 679
//
NetSocket *BaseSocketManager::FindSocket(int sockId)
{
	SocketIdMap::iterator i = mSockMap.find(sockId);
	if (i==mSockMap.end())
	{
		return NULL;
	}
	return i->second;
}


//
// BaseSocketManager::GetIpAddress					- not described in the book
//
//   Given a sockId, return the IP Address.
//
int BaseSocketManager::GetIpAddress(int sockId)
{
	NetSocket *socket = FindSocket(sockId);
	if (socket)
	{
		return socket->GetIpAddress();
	}
	else
	{
		return 0;
	}
}



//
// BaseSocketManager::Send						- Chapter 19, page 679
//
bool BaseSocketManager::Send(int sockId, eastl::shared_ptr<BasePacket> packet)
{
	NetSocket *sock = FindSocket(sockId);
	if (!sock)
		return false;
	sock->Send(packet);
	return true;
}

//
// BaseSocketManager::DoSelect					- Chapter 19, page 679
//
void BaseSocketManager::DoSelect(int pauseMicroSecs, bool handleInput) 
{
	timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = pauseMicroSecs;    // 100 microseconds is 0.1 milliseconds or .0001 seconds

	fd_set inp_set, out_set, exc_set;
	int maxdesc;

	FD_ZERO(&inp_set);
	FD_ZERO(&out_set);
	FD_ZERO(&exc_set);

	maxdesc = 0;

	// set everything up for the select
	for (SocketList::iterator i = mSockList.begin(); i != mSockList.end(); ++i)
	{
		NetSocket *pSock = *i;
		if ((pSock->mDeleteFlag&1) || pSock->mSock == INVALID_SOCKET)
			continue;

		if (handleInput)
			FD_SET(pSock->mSock, &inp_set);

		FD_SET(pSock->mSock, &exc_set);

		if (pSock->HasOutput())
			FD_SET(pSock->mSock, &out_set);

		if ((int)pSock->mSock > maxdesc)
			maxdesc = (int)pSock->mSock;

	 }
  
	int selRet = 0;

	// do the select (duration passed in as tv, NULL to block until event)
	selRet = select(maxdesc+1, &inp_set, &out_set, &exc_set, &tv) ;
	if (selRet == SOCKET_ERROR)
	{
		PrintError();
		return;
	}

	// handle input, output, and exceptions

	if (selRet)
	{
		for (SocketList::iterator i = mSockList.begin(); i != mSockList.end(); ++i)
		{
			NetSocket *pSock = *i;

			if ((pSock->mDeleteFlag&1) || pSock->mSock == INVALID_SOCKET)
				continue;

			if (FD_ISSET(pSock->mSock, &exc_set))
			{
				pSock->HandleException();
			}

			if (!(pSock->mDeleteFlag&1) && FD_ISSET(pSock->mSock, &out_set))
			{
				pSock->HandleOutput();
			}

			if (   handleInput
				&& !(pSock->mDeleteFlag&1) && FD_ISSET(pSock->mSock, &inp_set))
			{
				pSock->HandleInput();
			}
		 }	
	}

	unsigned int timeNow = Timer::GetTime();

	// handle deleting any sockets
	SocketList::iterator i = mSockList.begin();
	while (i != mSockList.end())
	{
		NetSocket *pSock = *i;
		if (pSock->mTimeOut) 
		{
			if (pSock->mTimeOut < timeNow)
			{
				pSock->TimeOut();
			}
		}

		if (pSock->mDeleteFlag&1)
		{
			switch (pSock->mDeleteFlag) 
			{
			  case 1:
					SocketMngr->RemoveSocket(pSock);
					i = mSockList.begin();
					break;
				case 3:
					pSock->mDeleteFlag = 2;
					if (pSock->mSock != INVALID_SOCKET) 
					{
						closesocket(pSock->mSock);
						pSock->mSock = INVALID_SOCKET;
					}
					break;
			}
		}

		++i;

	 }
 }


//
// BaseSocketManager::IsInternal					- Chapter 19, page 682
//
bool BaseSocketManager::IsInternal(unsigned int ipaddr)
{
   if (!mSubnetMask)
      return false;

   if ((ipaddr & mSubnetMask) == mSubNet)
      return false;

   return true;
}


//
// BaseSocketManager::GetHostByName					- Chapter 19, page 683
//
unsigned int BaseSocketManager::GetHostByName(const eastl::string &hostName)
{
   //This will retrieve the ip details and put it into pHostEnt structure
	struct hostent *pHostEnt = gethostbyname(hostName.c_str());
    struct sockaddr_in tmpSockAddr; //placeholder for the ip address

    if(pHostEnt == NULL)
    {
        LogError("Error occured");
        return 0;
    }

    memcpy(&tmpSockAddr.sin_addr,pHostEnt->h_addr,pHostEnt->h_length);
	return ntohl(tmpSockAddr.sin_addr.s_addr);
}

//
// BaseSocketManager::GetHostByAddr					- Chapter 19, page 683
//
const char *BaseSocketManager::GetHostByAddr(unsigned int ip)
{
	static char host[256];

	int netip = htonl(ip);
	struct hostent *lpHostEnt = gethostbyaddr((const char *)&netip, 4, PF_INET);

	if (lpHostEnt)
	{
		strcpy_s(host, 256, lpHostEnt->h_name);
		return host;
	}

	return NULL;
}

//
// BaseSocketManager::PrintError					- not described in the book
//
//   A helper function to send a human readable message to the error log
//
void BaseSocketManager::PrintError()
{
	int realError = WSAGetLastError();
	char* reason;

	switch(realError)
	{
		case WSANOTINITIALISED: reason = "A successful WSAStartup must occur before using this API."; break;
		case WSAEFAULT: reason = "The Windows Sockets implementation was unable to allocated needed resources for its internal operations, or the readfds, writefds, exceptfds, or timeval parameters are not part of the user address space."; break;
		case WSAENETDOWN: reason = "The network subsystem has failed."; break;
		case WSAEINVAL: reason = "The timeout value is not valid, or all three descriptor parameters were NULL."; break;
		case WSAEINTR: reason = "The (blocking) call was canceled via WSACancelBlockingCall."; break;
		case WSAEINPROGRESS: reason = "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function."; break;
		case WSAENOTSOCK: reason = "One of the descriptor sets contains an entry which is not a socket."; break;
		default: reason = "Unknown."; 
	}

	char buffer[256];
	sprintf(buffer, "SOCKET error: %s", reason);
    LogInformation(buffer);
}


//
// ClientSocketManager::Connect					- Chapter 19, page 684
//
bool ClientSocketManager::Connect()
{
	if (!BaseSocketManager::Init())
		return false;

	RemoteEventSocket *pSocket = new RemoteEventSocket();
	
	if (!pSocket->Connect(GetHostByName(mHostName), mPort) )
	{
		delete pSocket;
		return false;
	}
	AddSocket(pSocket);
	return true;
}



//
// GameServerListenSocket::HandleInput			- Chapter 19, page 685
//
void GameServerListenSocket::HandleInput() 
{
	unsigned int theipaddr;
	SOCKET new_sock = AcceptConnection(&theipaddr);

	int value = 1;
	setsockopt(new_sock, SOL_SOCKET, SO_DONTLINGER, (char *)&value, sizeof(value));

	if (new_sock != INVALID_SOCKET)
	{
		RemoteEventSocket * sock = new RemoteEventSocket(new_sock, theipaddr);
		int sockId = BaseSocketManager::SocketMngr->AddSocket(sock);
		int ipAddress = BaseSocketManager::SocketMngr->GetIpAddress(sockId);
        eastl::shared_ptr<EventDataRemoteClient> pEvent(new EventDataRemoteClient(sockId, ipAddress));
        BaseEventManager::Get()->QueueEvent(pEvent);
	}
 }

//
// RemoteEventSocket::HandleInput				- Chapter 19, page 688
//
void RemoteEventSocket::HandleInput()
{
	NetSocket::HandleInput();

	// traverse the list of m_InList packets and do something useful with them
	while (!mInList.empty())
	{
		eastl::shared_ptr<BasePacket> packet = *mInList.begin();
		mInList.pop_front();
		if (!strcmp(packet->GetType(), BinaryPacket::Type))
		{
			const char *buf = packet->GetData();
			int size = static_cast<int>(packet->GetSize());

			std::istrstream in(buf+sizeof(u_long), (size-sizeof(u_long)));
			
			int type;
			in >> type;
			switch(type)
			{
				case NMS_EVENT:
					CreateEvent(in);
					break;

				case NMS_PLAYERLOGINOK:
				{
					int serverSockId, actorId;
					in >> serverSockId;
					in >> actorId;
					std::string level;
					in >> level;			// [mrmike] This was the best spot to set the level !
                    eastl::shared_ptr<EventDataNetworkPlayerActorAssignment> pEvent(
						new EventDataNetworkPlayerActorAssignment(actorId, serverSockId));
                    BaseEventManager::Get()->QueueEvent(pEvent);
					break;
				}

				default:
					LogError("Unknown message type.");
			}
		}
		else if (!strcmp(packet->GetType(), TextPacket::Type))
		{
            LogInformation(packet->GetData()+sizeof(u_long));
		}
	}
}


//
// RemoteEventSocket::CreateEvent				- Chapter 19, page 689
//
void RemoteEventSocket::CreateEvent(std::istrstream &in)
{
	// Note:  We can improve the efficiency of this by comparing the hash values instead of strings.
	// But strings are easier to debug!
	BaseEventType eventType;
	in >> eventType;

    BaseEventDataPtr pEvent(CREATE_EVENT(eventType));
    if (pEvent)
    {
        pEvent->Deserialize(in);
        BaseEventManager::Get()->QueueEvent(pEvent);
    }
    else
    {
		LogError("ERROR Unknown event type from remote: " + eastl::to_string(eventType));
    }
}



//
// NetworkEventForwarder::ForwardEvent			- Chapter 19, page 690
//
void NetworkEventForwarder::ForwardEvent(BaseEventDataPtr pEventData)
{
	std::ostrstream out;

	out << static_cast<int>(RemoteEventSocket::NMS_EVENT) << " ";
	out << pEventData->GetEventType() << " ";
	pEventData->Serialize(out);
	out << "\r\n";

	eastl::shared_ptr<BinaryPacket> eventMsg(
		new BinaryPacket(out.rdbuf()->str(), (u_long)out.pcount()));

	BaseSocketManager::SocketMngr->Send(mSockId, eventMsg);
}



//
// NetworkGameView::NetworkGameView				- Chapter 19, page 691
//
NetworkGameView::NetworkGameView()
{ 
	mSockId = INVALID_SOCKET_ID;
	mActorId = INVALID_ACTOR_ID;
	BaseEventManager::Get()->AddListener(
		MakeDelegate(this, &NetworkGameView::NewActorDelegate), EventDataNewActor::skEventType);
}

//
// NetworkGameView::AttachRemotePlayer			- Chapter 19, page 692
//
void NetworkGameView::AttachRemotePlayer(int sockID) 
{ 
	mSockId = sockID; 
	// this is the first thing that happens when the 
	// network view is attached. The socket id is sent, 
	// which is how each client can be uniquely identified from other
	// clients attached to the server.

	std::ostrstream out;

	out << static_cast<int>(RemoteEventSocket::NMS_PLAYERLOGINOK) << " ";
	out << mSockId << " ";
	out << mActorId << " ";
	out << "\r\n";

	eastl::shared_ptr<BinaryPacket> gvidMsg(
		new BinaryPacket(out.rdbuf()->str(), (u_long)out.pcount()));
	BaseSocketManager::SocketMngr->Send(mSockId, gvidMsg);
}

//
// NetworkGameView::OnAttach					- Chapter X, page 619
//
void NetworkGameView::OnAttach(GameViewId viewId, ActorId aid)
{
	mViewId = viewId; 
	mActorId = aid;
}


void NetworkGameView::OnUpdate(unsigned int timeMs, unsigned long deltaMs)
{ 
	if (mActorId != INVALID_ACTOR_ID)
	{
		BaseEventManager::Get()->RemoveListener(
			MakeDelegate(this, &NetworkGameView::NewActorDelegate), EventDataNewActor::skEventType);
	}
};

void NetworkGameView::NewActorDelegate(BaseEventDataPtr pEventData)
{
    eastl::shared_ptr<EventDataNewActor> pCastEventData = 
		eastl::static_pointer_cast<EventDataNewActor>(pEventData);
	ActorId actorId = pCastEventData->GetActorId();

	eastl::shared_ptr<Actor> pActor(GameLogic::Get()->GetActor(actorId).lock());

	// FUTURE WORK: This could be in a script.
    if (pActor && pActor->GetType() == "Player")
    {
        if (pCastEventData->GetViewId() == mViewId)
        {
            mActorId = actorId;
			eastl::shared_ptr<EventDataNetworkPlayerActorAssignment> pEvent(
				new EventDataNetworkPlayerActorAssignment(mActorId, mSockId));
			BaseEventManager::Get()->QueueEvent(pEvent);
        }
    }
}