#pragma once

#ifndef __GPCOMMONTEMP_H__
#define __GPCOMMONTEMP_H__

enum GPErrorCode;
enum GPEnum {
	GP_OFFLINE,
	GP_ONLINE,
	GP_RECV_GAME_INVITE
};
enum GPResult; 
enum RoomType {
	StagingRoom,
	GroupRoom
};

#endif // __GPCOMMONTEMP_H__