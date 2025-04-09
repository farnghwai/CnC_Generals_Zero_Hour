/*
**	Command & Conquer Generals(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

// FILE: BuddyThread.h //////////////////////////////////////////////////////
// Generals GameSpy BuddyList thread class interface
// Author: Matthew D. Campbell, June 2002

#pragma once

#ifndef __BUDDYTHREAD_H__
#define __BUDDYTHREAD_H__

//#include "GameSpy/GP/GP.h"
#include "GPCommonTemp.h"

#define MAX_BUDDY_CHAT_LEN 128
#define MAX_COUNTRYCODE_LEN 240
#define MAX_EMAIL_LEN 240
#define MAX_LOCATION_STRING_LEN 240
#define MAX_NICK_LEN 240
#define MAX_PASSWORD_LEN 240
#define MAX_REASON_LEN 240
#define MAX_STATUS_STRING_LEN 240





// this class encapsulates a request for the buddy thread
class BuddyRequest
{
public:
	enum
	{
		BUDDYREQUEST_LOGIN,				// attempt to login
		BUDDYREQUEST_RELOGIN,			// log in after being disconnected
		BUDDYREQUEST_LOGOUT,			// log out if connected
		BUDDYREQUEST_MESSAGE,
		BUDDYREQUEST_LOGINNEW,		// attempt to create a new nick and login
		//BUDDYREQUEST_DELETELOGIN,
		BUDDYREQUEST_ADDBUDDY,		// add someone to your buddy list
		BUDDYREQUEST_DELBUDDY,		// delete someone from your buddy list
		BUDDYREQUEST_OKADD,				// allow someone to add you to their buddy list
		BUDDYREQUEST_DENYADD,			// don't allow someone to add you to their buddy list
		BUDDYREQUEST_SETSTATUS,		// Set our status
		BUDDYREQUEST_DELETEACCT,	// Delete our account
		BUDDYREQUEST_MAX
	} buddyRequestType;

	union
	{
		struct
		{
			INT recipient;
			WideChar text[MAX_BUDDY_CHAT_LEN];
		} message;

		struct
		{
			char nick[MAX_NICK_LEN];
			char email[MAX_EMAIL_LEN];
			char password[MAX_PASSWORD_LEN];
			Bool hasFirewall;
		} login;

		struct
		{
			INT id;
			WideChar text[MAX_BUDDY_CHAT_LEN];
		} addbuddy;

		struct
		{
			INT id;
		} profile;

		struct
		{
			GPEnum status;
			char statusString[MAX_STATUS_STRING_LEN];
			char locationString[MAX_LOCATION_STRING_LEN];
		} status;

	} arg;
};

//-------------------------------------------------------------------------

// this class encapsulates an action the buddy thread wants from the UI
class BuddyResponse
{
public:
	enum
	{
		BUDDYRESPONSE_LOGIN,
		BUDDYRESPONSE_DISCONNECT,
		BUDDYRESPONSE_MESSAGE,
		BUDDYRESPONSE_REQUEST,
		BUDDYRESPONSE_STATUS,
		BUDDYRESPONSE_MAX
	} buddyResponseType;

	INT profile;
	GPResult result;

	union
	{
		struct
		{
			UnsignedInt date;
			char nick[MAX_NICK_LEN];
			WideChar text[MAX_BUDDY_CHAT_LEN];
		} message;

		struct
		{
			char nick[MAX_NICK_LEN];
			char email[MAX_EMAIL_LEN];
			char countrycode[MAX_COUNTRYCODE_LEN];
			WideChar text[MAX_REASON_LEN];
		} request;

		struct
		{
			//GPResult result;
			GPErrorCode errorCode;
			char errorString[MAX_BUDDY_CHAT_LEN];
			GPEnum fatal;
		} error;

		struct
		{
			char nick[MAX_NICK_LEN];
			char email[MAX_EMAIL_LEN];
			char countrycode[MAX_COUNTRYCODE_LEN];
			char location[MAX_LOCATION_STRING_LEN];
			GPEnum status;
			char statusString[MAX_STATUS_STRING_LEN];
		} status;
	} arg;
};

//-------------------------------------------------------------------------

// this is the actual message queue used to pass messages between threads
class GameSpyBuddyMessageQueueInterface
{
public:
	virtual ~GameSpyBuddyMessageQueueInterface() {}
	virtual void startThread( void ) = 0;
	virtual void endThread( void ) = 0;
	virtual Bool isThreadRunning( void ) = 0;
	virtual Bool isConnected( void ) = 0;
	virtual Bool isConnecting( void ) = 0;

	virtual void addRequest( const BuddyRequest& req ) = 0;
	virtual Bool getRequest( BuddyRequest& req ) = 0;

	virtual void addResponse( const BuddyResponse& resp ) = 0;
	virtual Bool getResponse( BuddyResponse& resp ) = 0;

	virtual INT getLocalProfileID( void ) = 0;

	static GameSpyBuddyMessageQueueInterface* createNewMessageQueue( void );
};

extern GameSpyBuddyMessageQueueInterface *TheGameSpyBuddyMessageQueue;


#endif // __BUDDYTHREAD_H__
