/*

   Copyright [2008] [Trevor Hogan]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   CODE PORTED FROM THE ORIGINAL GHOST PROJECT: http://ghost.pwner.org/

*/

#include "ghost.h"
#include "util.h"
#include "crc32.h"
#include "gameplayer.h"
#include "gameprotocol.h"
#include "game_base.h"

//
// CGameProtocol
//

CGameProtocol :: CGameProtocol( CGHost *nGHost ) : m_GHost( nGHost )
{

}

CGameProtocol :: ~CGameProtocol( )
{

}

///////////////////////
// RECEIVE FUNCTIONS //
///////////////////////

CIncomingJoinPlayer *CGameProtocol :: RECEIVE_W3GS_REQJOIN( BYTEARRAY data )
{
	// DEBUG_Print( "RECEIVED W3GS_REQJOIN" );
	// DEBUG_Print( data );

	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> Host Counter (Game ID)
	// 4 bytes					-> Entry Key (used in LAN)
	// 1 byte					-> ???
	// 2 bytes					-> Listen Port
	// 4 bytes					-> Peer Key
	// null terminated string	-> Name
	// 4 bytes					-> ???
	// 2 bytes					-> InternalPort (???)
	// 4 bytes					-> InternalIP

	if( ValidateLength( data ) && data.size( ) >= 20 )
	{
		uint32_t HostCounter = UTIL_ByteArrayToUInt32( data, false, 4 );
		uint32_t EntryKey = UTIL_ByteArrayToUInt32( data, false, 8 );
		BYTEARRAY Name = UTIL_ExtractCString( data, 19 );

		if( !Name.empty( ) && data.size( ) >= Name.size( ) + 30 )
		{
			BYTEARRAY InternalIP = BYTEARRAY( data.begin( ) + Name.size( ) + 26, data.begin( ) + Name.size( ) + 30 );
			return new CIncomingJoinPlayer( HostCounter, EntryKey, string( Name.begin( ), Name.end( ) ), InternalIP );
		}
	}

	return NULL;
}

uint32_t CGameProtocol :: RECEIVE_W3GS_LEAVEGAME( BYTEARRAY data )
{
	// DEBUG_Print( "RECEIVED W3GS_LEAVEGAME" );
	// DEBUG_Print( data );

	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> Reason

	if( ValidateLength( data ) && data.size( ) >= 8 )
		return UTIL_ByteArrayToUInt32( data, false, 4 );

	return 0;
}

bool CGameProtocol :: RECEIVE_W3GS_GAMELOADED_SELF( BYTEARRAY data )
{
	// DEBUG_Print( "RECEIVED W3GS_GAMELOADED_SELF" );
	// DEBUG_Print( data );

	// 2 bytes					-> Header
	// 2 bytes					-> Length

	if( ValidateLength( data ) )
		return true;

	return false;
}

CIncomingAction *CGameProtocol :: RECEIVE_W3GS_OUTGOING_ACTION( BYTEARRAY data, unsigned char PID )
{
	// DEBUG_Print( "RECEIVED W3GS_OUTGOING_ACTION" );
	// DEBUG_Print( data );

	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> CRC
	// remainder of packet		-> Action

	if( PID != 255 && ValidateLength( data ) && data.size( ) >= 8 )
	{
		BYTEARRAY CRC = BYTEARRAY( data.begin( ) + 4, data.begin( ) + 8 );
		BYTEARRAY Action = BYTEARRAY( data.begin( ) + 8, data.end( ) );
		return new CIncomingAction( PID, CRC, Action );
	}

	return NULL;
}

uint32_t CGameProtocol :: RECEIVE_W3GS_OUTGOING_KEEPALIVE( BYTEARRAY data )
{
	// DEBUG_Print( "RECEIVED W3GS_OUTGOING_KEEPALIVE" );
	// DEBUG_Print( data );

	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 1 byte					-> ???
	// 4 bytes					-> CheckSum??? (used in replays)

	if( ValidateLength( data ) && data.size( ) == 9 )
		return UTIL_ByteArrayToUInt32( data, false, 5 );

	return 0;
}

CIncomingChatPlayer *CGameProtocol :: RECEIVE_W3GS_CHAT_TO_HOST( BYTEARRAY data )
{
	// DEBUG_Print( "RECEIVED W3GS_CHAT_TO_HOST" );
	// DEBUG_Print( data );

	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 1 byte					-> Total
	// for( 1 .. Total )
	//		1 byte				-> ToPID
	// 1 byte					-> FromPID
	// 1 byte					-> Flag
	// if( Flag == 16 )
	//		null term string	-> Message
	// elseif( Flag == 17 )
	//		1 byte				-> Team
	// elseif( Flag == 18 )
	//		1 byte				-> Colour
	// elseif( Flag == 19 )
	//		1 byte				-> Race
	// elseif( Flag == 20 )
	//		1 byte				-> Handicap
	// elseif( Flag == 32 )
	//		4 bytes				-> ExtraFlags
	//		null term string	-> Message

	if( ValidateLength( data ) )
	{
		unsigned int i = 5;
		unsigned char Total = data[4];

		if( Total > 0 && Total <= MAX_SLOTS && data.size( ) >= i + Total )
		{
			BYTEARRAY ToPIDs = BYTEARRAY( data.begin( ) + i, data.begin( ) + i + Total );
			i += Total;
			unsigned char FromPID = data[i];
			unsigned char Flag = data[i + 1];
			i += 2;

			if( Flag == 16 && data.size( ) >= i + 1 )
			{
				// chat message

				BYTEARRAY Message = UTIL_ExtractCString( data, i );
				return new CIncomingChatPlayer( FromPID, ToPIDs, Flag, string( Message.begin( ), Message.end( ) ) );
			}
			else if( ( Flag >= 17 && Flag <= 20 ) && data.size( ) >= i + 1 )
			{
				// team/colour/race/handicap change request

				unsigned char Byte = data[i];
				return new CIncomingChatPlayer( FromPID, ToPIDs, Flag, Byte );
			}
			else if( Flag == 32 && data.size( ) >= i + 5 )
			{
				// chat message with extra flags

				BYTEARRAY ExtraFlags = BYTEARRAY( data.begin( ) + i, data.begin( ) + i + 4 );
				BYTEARRAY Message = UTIL_ExtractCString( data, i + 4 );
				return new CIncomingChatPlayer( FromPID, ToPIDs, Flag, string( Message.begin( ), Message.end( ) ), ExtraFlags );
			}
		}
	}

	return NULL;
}

bool CGameProtocol :: RECEIVE_W3GS_SEARCHGAME( BYTEARRAY data, unsigned char war3Version )
{
	uint32_t ProductID	= 1462982736;	// "W3XP"
	uint32_t Version	= war3Version;

	// DEBUG_Print( "RECEIVED W3GS_SEARCHGAME" );
	// DEBUG_Print( data );

	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> ProductID
	// 4 bytes					-> Version
	// 4 bytes					-> ??? (Zero)

	if( ValidateLength( data ) && data.size( ) >= 16 )
	{
		if( UTIL_ByteArrayToUInt32( data, false, 4 ) == ProductID )
		{
			if( UTIL_ByteArrayToUInt32( data, false, 8 ) == Version )
			{
				if( UTIL_ByteArrayToUInt32( data, false, 12 ) == 0 )
					return true;
			}
		}
	}

	return false;
}

CIncomingMapSize *CGameProtocol :: RECEIVE_W3GS_MAPSIZE( BYTEARRAY data, BYTEARRAY mapSize )
{
	// DEBUG_Print( "RECEIVED W3GS_MAPSIZE" );
	// DEBUG_Print( data );

	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> ???
	// 1 byte					-> SizeFlag (1 = have map, 3 = continue download)
	// 4 bytes					-> MapSize

	if( ValidateLength( data ) && data.size( ) >= 13 )
		return new CIncomingMapSize( data[8], UTIL_ByteArrayToUInt32( data, false, 9 ) );

	return NULL;
}

uint32_t CGameProtocol :: RECEIVE_W3GS_MAPPARTOK( BYTEARRAY data )
{
	// DEBUG_Print( "RECEIVED W3GS_MAPPARTOK" );
	// DEBUG_Print( data );

	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 1 byte					-> SenderPID
	// 1 byte					-> ReceiverPID
	// 4 bytes					-> ???
	// 4 bytes					-> MapSize

	if( ValidateLength( data ) && data.size( ) >= 14 )
		return UTIL_ByteArrayToUInt32( data, false, 10 );

	return 0;
}

uint32_t CGameProtocol :: RECEIVE_W3GS_PONG_TO_HOST( BYTEARRAY data )
{
	// DEBUG_Print( "RECEIVED W3GS_PONG_TO_HOST" );
	// DEBUG_Print( data );

	// 2 bytes					-> Header
	// 2 bytes					-> Length
	// 4 bytes					-> Pong

	// the pong value is just a copy of whatever was sent in SEND_W3GS_PING_FROM_HOST which was GetTicks( ) at the time of sending
	// so as long as we trust that the client isn't trying to fake us out and mess with the pong value we can find the round trip time by simple subtraction
	// (the subtraction is done elsewhere because the very first pong value seems to be 1 and we want to discard that one)

	if( ValidateLength( data ) && data.size( ) >= 8 )
		return UTIL_ByteArrayToUInt32( data, false, 4 );

	return 1;
}

////////////////////
// SEND FUNCTIONS //
////////////////////

BYTEARRAY CGameProtocol :: SEND_W3GS_PING_FROM_HOST( )
{
	BYTEARRAY packet;
	packet.push_back( W3GS_HEADER_CONSTANT );				// W3GS header constant
	packet.push_back( W3GS_PING_FROM_HOST );				// W3GS_PING_FROM_HOST
	packet.push_back( 0 );									// packet length will be assigned later
	packet.push_back( 0 );									// packet length will be assigned later
	UTIL_AppendByteArray( packet, GetTicks( ), false );		// ping value
	AssignLength( packet );
	// DEBUG_Print( "SENT W3GS_PING_FROM_HOST" );
	// DEBUG_Print( packet );
	return packet;
}

BYTEARRAY CGameProtocol :: SEND_W3GS_SLOTINFOJOIN( unsigned char PID, BYTEARRAY port, BYTEARRAY externalIP, vector<CGameSlot> &slots, uint32_t randomSeed, unsigned char layoutStyle, unsigned char playerSlots )
{
	unsigned char Zeros[] = { 0, 0, 0, 0 };

	BYTEARRAY SlotInfo = EncodeSlotInfo( slots, randomSeed, layoutStyle, playerSlots );
	BYTEARRAY packet;

	if( port.size( ) == 2 && externalIP.size( ) == 4 )
	{
		packet.push_back( W3GS_HEADER_CONSTANT );									// W3GS header constant
		packet.push_back( W3GS_SLOTINFOJOIN );										// W3GS_SLOTINFOJOIN
		packet.push_back( 0 );														// packet length will be assigned later
		packet.push_back( 0 );														// packet length will be assigned later
		UTIL_AppendByteArray( packet, (uint16_t)SlotInfo.size( ), false );			// SlotInfo length
		UTIL_AppendByteArrayFast( packet, SlotInfo );								// SlotInfo
		packet.push_back( PID );													// PID
		packet.push_back( 2 );														// AF_INET
		packet.push_back( 0 );														// AF_INET continued...
		UTIL_AppendByteArray( packet, port );										// port
		UTIL_AppendByteArrayFast( packet, externalIP );								// external IP
		UTIL_AppendByteArray( packet, Zeros, 4 );									// ???
		UTIL_AppendByteArray( packet, Zeros, 4 );									// ???
		AssignLength( packet );
	}
	else
		CONSOLE_Print( "[GAMEPROTO] invalid parameters passed to SEND_W3GS_SLOTINFOJOIN" );

	// DEBUG_Print( "SENT W3GS_SLOTINFOJOIN" );
	// DEBUG_Print( packet );
	return packet;
}

BYTEARRAY CGameProtocol :: SEND_W3GS_REJECTJOIN( uint32_t reason )
{
	BYTEARRAY packet;
	packet.push_back( W3GS_HEADER_CONSTANT );				// W3GS header constant
	packet.push_back( W3GS_REJECTJOIN );					// W3GS_REJECTJOIN
	packet.push_back( 0 );									// packet length will be assigned later
	packet.push_back( 0 );									// packet length will be assigned later
	UTIL_AppendByteArray( packet, reason, false );			// reason
	AssignLength( packet );
	// DEBUG_Print( "SENT W3GS_REJECTJOIN" );
	// DEBUG_Print( packet );
	return packet;
}

BYTEARRAY CGameProtocol :: SEND_W3GS_PLAYERINFO( unsigned char PID, string name, BYTEARRAY externalIP, BYTEARRAY internalIP )
{
	unsigned char PlayerJoinCounter[]	= { 2, 0, 0, 0 };
	unsigned char Zeros[]				= { 0, 0, 0, 0 };

	BYTEARRAY packet;

	if( !name.empty( ) && name.size( ) <= 15 && externalIP.size( ) == 4 && internalIP.size( ) == 4 )
	{
		packet.push_back( W3GS_HEADER_CONSTANT );							// W3GS header constant
		packet.push_back( W3GS_PLAYERINFO );								// W3GS_PLAYERINFO
		packet.push_back( 0 );												// packet length will be assigned later
		packet.push_back( 0 );												// packet length will be assigned later
		UTIL_AppendByteArray( packet, PlayerJoinCounter, 4 );				// player join counter
		packet.push_back( PID );											// PID
		UTIL_AppendByteArrayFast( packet, name );							// player name
		packet.push_back( 1 );												// ???
		packet.push_back( 0 );												// ???
		packet.push_back( 2 );												// AF_INET
		packet.push_back( 0 );												// AF_INET continued...
		packet.push_back( 0 );												// port
		packet.push_back( 0 );												// port continued...
		UTIL_AppendByteArrayFast( packet, externalIP );						// external IP
		UTIL_AppendByteArray( packet, Zeros, 4 );							// ???
		UTIL_AppendByteArray( packet, Zeros, 4 );							// ???
		packet.push_back( 2 );												// AF_INET
		packet.push_back( 0 );												// AF_INET continued...
		packet.push_back( 0 );												// port
		packet.push_back( 0 );												// port continued...
		UTIL_AppendByteArrayFast( packet, internalIP );						// internal IP
		UTIL_AppendByteArray( packet, Zeros, 4 );							// ???
		UTIL_AppendByteArray( packet, Zeros, 4 );							// ???
		AssignLength( packet );
	}
	else
		CONSOLE_Print( "[GAMEPROTO] invalid parameters passed to SEND_W3GS_PLAYERINFO" );

	// DEBUG_Print( "SENT W3GS_PLAYERINFO" );
	// DEBUG_Print( packet );
	return packet;
}

BYTEARRAY CGameProtocol :: SEND_W3GS_PLAYERLEAVE_OTHERS( unsigned char PID, uint32_t leftCode )
{
	BYTEARRAY packet;

	if( PID != 255 )
	{
		packet.push_back( W3GS_HEADER_CONSTANT );			// W3GS header constant
		packet.push_back( W3GS_PLAYERLEAVE_OTHERS );		// W3GS_PLAYERLEAVE_OTHERS
		packet.push_back( 0 );								// packet length will be assigned later
		packet.push_back( 0 );								// packet length will be assigned later
		packet.push_back( PID );							// PID
		UTIL_AppendByteArray( packet, leftCode, false );	// left code (see PLAYERLEAVE_ constants in gameprotocol.h)
		AssignLength( packet );
	}
	else
		CONSOLE_Print( "[GAMEPROTO] invalid parameters passed to SEND_W3GS_PLAYERLEAVE_OTHERS" );

	// DEBUG_Print( "SENT W3GS_PLAYERLEAVE_OTHERS" );
	// DEBUG_Print( packet );
	return packet;
}

BYTEARRAY CGameProtocol :: SEND_W3GS_GAMELOADED_OTHERS( unsigned char PID )
{
	BYTEARRAY packet;

	if( PID != 255 )
	{
		packet.push_back( W3GS_HEADER_CONSTANT );		// W3GS header constant
		packet.push_back( W3GS_GAMELOADED_OTHERS );		// W3GS_GAMELOADED_OTHERS
		packet.push_back( 0 );							// packet length will be assigned later
		packet.push_back( 0 );							// packet length will be assigned later
		packet.push_back( PID );						// PID
		AssignLength( packet );
	}
	else
		CONSOLE_Print( "[GAMEPROTO] invalid parameters passed to SEND_W3GS_GAMELOADED_OTHERS" );

	// DEBUG_Print( "SENT W3GS_GAMELOADED_OTHERS" );
	// DEBUG_Print( packet );
	return packet;
}

BYTEARRAY CGameProtocol :: SEND_W3GS_SLOTINFO( vector<CGameSlot> &slots, uint32_t randomSeed, unsigned char layoutStyle, unsigned char playerSlots )
{
	BYTEARRAY SlotInfo = EncodeSlotInfo( slots, randomSeed, layoutStyle, playerSlots );
	BYTEARRAY packet;
	packet.push_back( W3GS_HEADER_CONSTANT );									// W3GS header constant
	packet.push_back( W3GS_SLOTINFO );											// W3GS_SLOTINFO
	packet.push_back( 0 );														// packet length will be assigned later
	packet.push_back( 0 );														// packet length will be assigned later
	UTIL_AppendByteArray( packet, (uint16_t)SlotInfo.size( ), false );			// SlotInfo length
	UTIL_AppendByteArrayFast( packet, SlotInfo );								// SlotInfo
	AssignLength( packet );
	// DEBUG_Print( "SENT W3GS_SLOTINFO" );
	// DEBUG_Print( packet );
	return packet;
}

BYTEARRAY CGameProtocol :: SEND_W3GS_COUNTDOWN_START( )
{
	BYTEARRAY packet;
	packet.push_back( W3GS_HEADER_CONSTANT );		// W3GS header constant
	packet.push_back( W3GS_COUNTDOWN_START );		// W3GS_COUNTDOWN_START
	packet.push_back( 0 );							// packet length will be assigned later
	packet.push_back( 0 );							// packet length will be assigned later
	AssignLength( packet );
	// DEBUG_Print( "SENT W3GS_COUNTDOWN_START" );
	// DEBUG_Print( packet );
	return packet;
}

BYTEARRAY CGameProtocol :: SEND_W3GS_COUNTDOWN_END( )
{
	BYTEARRAY packet;
	packet.push_back( W3GS_HEADER_CONSTANT );		// W3GS header constant
	packet.push_back( W3GS_COUNTDOWN_END );			// W3GS_COUNTDOWN_END
	packet.push_back( 0 );							// packet length will be assigned later
	packet.push_back( 0 );							// packet length will be assigned later
	AssignLength( packet );
	// DEBUG_Print( "SENT W3GS_COUNTDOWN_END" );
	// DEBUG_Print( packet );
	return packet;
}

BYTEARRAY CGameProtocol :: SEND_W3GS_INCOMING_ACTION( queue<CIncomingAction *> actions, uint16_t sendInterval )
{
	BYTEARRAY packet;
	packet.push_back( W3GS_HEADER_CONSTANT );				// W3GS header constant
	packet.push_back( W3GS_INCOMING_ACTION );				// W3GS_INCOMING_ACTION
	packet.push_back( 0 );									// packet length will be assigned later
	packet.push_back( 0 );									// packet length will be assigned later
	UTIL_AppendByteArray( packet, sendInterval, false );	// send interval

	// create subpacket

	if( !actions.empty( ) )
	{
		BYTEARRAY subpacket;

		while( !actions.empty( ) )
		{
			CIncomingAction *Action = actions.front( );
			actions.pop( );
			subpacket.push_back( Action->GetPID( ) );
			UTIL_AppendByteArray( subpacket, (uint16_t)Action->GetAction( )->size( ), false );
			UTIL_AppendByteArrayFast( subpacket, *Action->GetAction( ) );
		}

		// calculate crc (we only care about the first 2 bytes though)

		BYTEARRAY crc32 = UTIL_CreateByteArray( m_GHost->m_CRC->FullCRC( (unsigned char *)string( subpacket.begin( ), subpacket.end( ) ).c_str( ), subpacket.size( ) ), false );
		crc32.resize( 2 );

		// finish subpacket

		UTIL_AppendByteArrayFast( packet, crc32 );			// crc
		UTIL_AppendByteArrayFast( packet, subpacket );		// subpacket
	}

	AssignLength( packet );
	// DEBUG_Print( "SENT W3GS_INCOMING_ACTION" );
	// DEBUG_Print( packet );
	return packet;
}

BYTEARRAY CGameProtocol :: SEND_W3GS_CHAT_FROM_HOST( unsigned char fromPID, BYTEARRAY toPIDs, unsigned char flag, BYTEARRAY flagExtra, string message )
{
	BYTEARRAY packet;

	if( !toPIDs.empty( ) && !message.empty( ) && message.size( ) < 255 )
	{
		packet.push_back( W3GS_HEADER_CONSTANT );		// W3GS header constant
		packet.push_back( W3GS_CHAT_FROM_HOST );		// W3GS_CHAT_FROM_HOST
		packet.push_back( 0 );							// packet length will be assigned later
		packet.push_back( 0 );							// packet length will be assigned later
		packet.push_back( toPIDs.size( ) );				// number of receivers
		UTIL_AppendByteArrayFast( packet, toPIDs );		// receivers
		packet.push_back( fromPID );					// sender
		packet.push_back( flag );						// flag
		UTIL_AppendByteArrayFast( packet, flagExtra );	// extra flag
		UTIL_AppendByteArrayFast( packet, message );	// message
		AssignLength( packet );
	}
	else
		CONSOLE_Print( "[GAMEPROTO] invalid parameters passed to SEND_W3GS_CHAT_FROM_HOST" );

	// DEBUG_Print( "SENT W3GS_CHAT_FROM_HOST" );
	// DEBUG_Print( packet );
	return packet;
}

BYTEARRAY CGameProtocol :: SEND_W3GS_START_LAG( vector<CGamePlayer *> players, bool loadInGame )
{
	BYTEARRAY packet;

	unsigned char NumLaggers = 0;

	for( vector<CGamePlayer *> :: iterator i = players.begin( ); i != players.end( ); i++ )
	{
		if( loadInGame )
		{
			if( !(*i)->GetFinishedLoading( ) )
				++NumLaggers;
		}
		else
		{
			if( (*i)->GetLagging( ) )
				++NumLaggers;
		}
	}

	if( NumLaggers > 0 )
	{
		packet.push_back( W3GS_HEADER_CONSTANT );	// W3GS header constant
		packet.push_back( W3GS_START_LAG );			// W3GS_START_LAG
		packet.push_back( 0 );						// packet length will be assigned later
		packet.push_back( 0 );						// packet length will be assigned later
		packet.push_back( NumLaggers );

		for( vector<CGamePlayer *> :: iterator i = players.begin( ); i != players.end( ); i++ )
		{
			if( loadInGame )
			{
				if( !(*i)->GetFinishedLoading( ) )
				{
					packet.push_back( (*i)->GetPID( ) );
					UTIL_AppendByteArray( packet, (uint32_t)0, false );
				}
			}
			else
			{
				if( (*i)->GetLagging( ) )
				{
					packet.push_back( (*i)->GetPID( ) );
					UTIL_AppendByteArray( packet, GetTicks( ) - (*i)->GetStartedLaggingTicks( ), false );
				}
			}
		}

		AssignLength( packet );
	}
	else
		CONSOLE_Print( "[GAMEPROTO] no laggers passed to SEND_W3GS_START_LAG" );

	// DEBUG_Print( "SENT W3GS_START_LAG" );
	// DEBUG_Print( packet );
	return packet;
}

BYTEARRAY CGameProtocol :: SEND_W3GS_STOP_LAG( CGamePlayer *player, bool loadInGame )
{
	BYTEARRAY packet;
	packet.push_back( W3GS_HEADER_CONSTANT );	// W3GS header constant
	packet.push_back( W3GS_STOP_LAG );			// W3GS_STOP_LAG
	packet.push_back( 0 );						// packet length will be assigned later
	packet.push_back( 0 );						// packet length will be assigned later
	packet.push_back( player->GetPID( ) );

	if( loadInGame )
		UTIL_AppendByteArray( packet, (uint32_t)0, false );
	else
		UTIL_AppendByteArray( packet, GetTicks( ) - player->GetStartedLaggingTicks( ), false );

	AssignLength( packet );
	// DEBUG_Print( "SENT W3GS_STOP_LAG" );
	// DEBUG_Print( packet );
	return packet;
}

BYTEARRAY CGameProtocol :: SEND_W3GS_SEARCHGAME( bool TFT, unsigned char war3Version )
{
	unsigned char ProductID_ROC[]	= {          51, 82, 65, 87 };	// "WAR3"
	unsigned char ProductID_TFT[]	= {          80, 88, 51, 87 };	// "W3XP"
	unsigned char Version[]			= { war3Version,  0,  0,  0 };
	unsigned char Unknown[]			= {           0,  0,  0,  0 };

	BYTEARRAY packet;
	packet.push_back( W3GS_HEADER_CONSTANT );				// W3GS header constant
	packet.push_back( W3GS_SEARCHGAME );					// W3GS_SEARCHGAME
	packet.push_back( 0 );									// packet length will be assigned later
	packet.push_back( 0 );									// packet length will be assigned later

	if( TFT )
		UTIL_AppendByteArray( packet, ProductID_TFT, 4 );	// Product ID (TFT)
	else
		UTIL_AppendByteArray( packet, ProductID_ROC, 4 );	// Product ID (ROC)

	UTIL_AppendByteArray( packet, Version, 4 );				// Version
	UTIL_AppendByteArray( packet, Unknown, 4 );				// ???
	AssignLength( packet );
	// DEBUG_Print( "SENT W3GS_SEARCHGAME" );
	// DEBUG_Print( packet );
	return packet;
}

BYTEARRAY CGameProtocol :: SEND_W3GS_GAMEINFO( bool TFT, unsigned char war3Version, BYTEARRAY mapGameType, BYTEARRAY mapFlags, BYTEARRAY mapWidth, BYTEARRAY mapHeight, string gameName, string hostName, uint32_t upTime, string mapPath, BYTEARRAY mapCRC, uint32_t slotsTotal, uint32_t slotsOpen, uint16_t port, uint32_t hostCounter, uint32_t entryKey )
{
	unsigned char ProductID_ROC[]	= {          51, 82, 65, 87 };	// "WAR3"
	unsigned char ProductID_TFT[]	= {          80, 88, 51, 87 };	// "W3XP"
	unsigned char Version[]			= { war3Version,  0,  0,  0 };
	unsigned char Unknown2[]		= {           1,  0,  0,  0 };

	BYTEARRAY packet;

	if( mapGameType.size( ) == 4 && mapFlags.size( ) == 4 && mapWidth.size( ) == 2 && mapHeight.size( ) == 2 && !gameName.empty( ) && !hostName.empty( ) && !mapPath.empty( ) && mapCRC.size( ) == 4 )
	{
		// make the stat string

		BYTEARRAY StatString;
		UTIL_AppendByteArrayFast( StatString, mapFlags );
		StatString.push_back( 0 );
		UTIL_AppendByteArrayFast( StatString, mapWidth );
		UTIL_AppendByteArrayFast( StatString, mapHeight );
		UTIL_AppendByteArrayFast( StatString, mapCRC );
		UTIL_AppendByteArrayFast( StatString, mapPath );
		UTIL_AppendByteArrayFast( StatString, hostName );
		StatString.push_back( 0 );
		StatString = UTIL_EncodeStatString( StatString );

		// make the rest of the packet

		packet.push_back( W3GS_HEADER_CONSTANT );						// W3GS header constant
		packet.push_back( W3GS_GAMEINFO );								// W3GS_GAMEINFO
		packet.push_back( 0 );											// packet length will be assigned later
		packet.push_back( 0 );											// packet length will be assigned later

		if( TFT )
			UTIL_AppendByteArray( packet, ProductID_TFT, 4 );			// Product ID (TFT)
		else
			UTIL_AppendByteArray( packet, ProductID_ROC, 4 );			// Product ID (ROC)

		UTIL_AppendByteArray( packet, Version, 4 );						// Version
		UTIL_AppendByteArray( packet, hostCounter, false );				// Host Counter
		UTIL_AppendByteArray( packet, entryKey, false );				// Entry Key
		UTIL_AppendByteArrayFast( packet, gameName );					// Game Name
		packet.push_back( 0 );											// ??? (maybe game password)
		UTIL_AppendByteArrayFast( packet, StatString );					// Stat String
		packet.push_back( 0 );											// Stat String null terminator (the stat string is encoded to remove all even numbers i.e. zeros)
		UTIL_AppendByteArray( packet, slotsTotal, false );				// Slots Total
		UTIL_AppendByteArrayFast( packet, mapGameType );				// Game Type
		UTIL_AppendByteArray( packet, Unknown2, 4 );					// ???
		UTIL_AppendByteArray( packet, slotsOpen, false );				// Slots Open
		UTIL_AppendByteArray( packet, upTime, false );					// time since creation
		UTIL_AppendByteArray( packet, port, false );					// port
		AssignLength( packet );
	}
	else
		CONSOLE_Print( "[GAMEPROTO] invalid parameters passed to SEND_W3GS_GAMEINFO" );

	// DEBUG_Print( "SENT W3GS_GAMEINFO" );
	// DEBUG_Print( packet );
	return packet;
}

BYTEARRAY CGameProtocol::SEND_MDNS_GAMEINFO(bool TFT, unsigned char war3Version, BYTEARRAY mapGameType, BYTEARRAY mapFlags, BYTEARRAY mapWidth, BYTEARRAY mapHeight, string gameName, string hostName, uint32_t upTime, string mapPath, BYTEARRAY mapCRC, uint32_t slotsTotal, uint32_t slotsOpen, uint16_t port, uint32_t hostCounter, uint32_t entryKey)
{

	BYTEARRAY packet;

	if (mapGameType.size() == 4 && mapFlags.size() == 4 && mapWidth.size() == 2 && mapHeight.size() == 2 && !gameName.empty() && !hostName.empty() && !mapPath.empty() && mapCRC.size() == 4)
	{
		// make the stat string

		/*BYTEARRAY StatString;
		UTIL_AppendByteArrayFast(StatString, mapFlags);
		StatString.push_back(0);
		UTIL_AppendByteArrayFast(StatString, mapWidth);
		UTIL_AppendByteArrayFast(StatString, mapHeight);
		UTIL_AppendByteArrayFast(StatString, mapCRC);
		UTIL_AppendByteArrayFast(StatString, mapPath);
		UTIL_AppendByteArrayFast(StatString, hostName);
		StatString.push_back(0);
		StatString = UTIL_EncodeStatString(StatString);*/

		// make the rest of the packet

		packet.push_back(0);	//mDNS header
		packet.push_back(0);
		packet.push_back(0x84);
		packet.push_back(0);
		packet.push_back(0);
		packet.push_back(0);
		packet.push_back(0);
		packet.push_back(6);
		packet.push_back(0);
		packet.push_back(0);
		packet.push_back(0);
		packet.push_back(5);			//do we need all additional RRs? can we just send the games' ones? or none?


		packet.push_back(18); //todo: detect
		UTIL_AppendByteArray(packet, (unsigned char*)gameName.c_str(), 18);				// Game Name
		packet.push_back(9);
		UTIL_AppendByteArray(packet, (unsigned char*)"_blizzard", 9);
		packet.push_back(4);
		UTIL_AppendByteArray(packet, (unsigned char*)"_udp", 4);
		packet.push_back(5);
		UTIL_AppendByteArray(packet, (unsigned char*)"local", 5);
		packet.push_back(0);
		packet.push_back(0); packet.push_back(0x10);	//Type: TXT
		packet.push_back(0x80);
		packet.push_back(0x01);
		packet.push_back(0);											//TTL: 4500
		packet.push_back(0);
		packet.push_back(0x11);
		packet.push_back(0x94);
		packet.push_back(0);											//Data length: 1
		packet.push_back(1);
		packet.push_back(0);											//TXT length: 0
		UTIL_AppendByteArray(packet, (unsigned char*)"\x09\x5f\x73\x65\x72\x76\x69\x63\x65\x73\x07\x5f\x64\x6e\x73\x2d\x73\x64\xc0\x2a\x00\x0c\x00\x01\x00\x00\x11\x94\x00\x02\xc0\x20", 32); // no changeable data, so meh
		packet.push_back(9);
		if (TFT)
			UTIL_AppendByteArray(packet, (unsigned char*)"_w3xp", 5);
		else
			UTIL_AppendByteArray(packet, (unsigned char*)"_war3", 5);
		packet.push_back('2');
		packet.push_back('7');
		switch (war3Version)
		{
		case 0x1e:
		{
			packet.push_back('2');
			packet.push_back('e');
			break;
		}
		case 0x1f:
		{
			packet.push_back('2');
			packet.push_back('f');
			break;
		}
		case 0x20:
		{
			packet.push_back('3');
			packet.push_back('0');
			break;
		}
		};
		packet.push_back(4);
		UTIL_AppendByteArray(packet, (unsigned char*)"\x5f\x73\x75\x62\xc0\x20\x00\x0c\x00\x01\x00\x00\x11\x94\x00\x02\xc0\x0c", 18);
		UTIL_AppendByteArray(packet, (unsigned char*)"\xc0\x20\x00\x0c\x00\x01\x00\x00\x11\x94\x00\x02\xc0\x0c", 14);
		UTIL_AppendByteArray(packet, (unsigned char*)"\xc0\x0c\x00\x42\x80\x01\x00\x00\x11\x94", 10);
		packet.push_back(1); packet.push_back(0x91);		//Data length		 (the lan game example in the comment below has 401 here)
		

		//Trying :)

		packet.push_back(0x0a); //10?
		packet.push_back(18);
		UTIL_AppendByteArray(packet, (unsigned char*)gameName.c_str(), 18);				// Game Name

		packet.push_back(0x1a); //26???
		packet.push_back(0x10); //16???
		packet.push_back(0x0a); //10 again?
		packet.push_back(11);
		UTIL_AppendByteArray(packet, (unsigned char*)"players_num", 11);
		packet.push_back(0x12);
		packet.push_back(1);
		packet.push_back('1');  //current players, shouldn't be fixed to one
		packet.push_back(0x1a);
		packet.push_back(0x1c);
		packet.push_back(0x0a);
		packet.push_back(5);
		UTIL_AppendByteArray(packet, (unsigned char*)"_name", 5);
		packet.push_back(0x12);
		packet.push_back(18);
		UTIL_AppendByteArray(packet, (unsigned char*)gameName.c_str(), 18);				// Game Name
		packet.push_back(0x1a);
		packet.push_back(0x10);
		packet.push_back(0x0a);
		packet.push_back(11);
		UTIL_AppendByteArray(packet, (unsigned char*)"players_max", 11);
		packet.push_back(0x12);
		if (slotsTotal > 9)
		{
			packet.push_back(2);
			packet.push_back((slotsTotal / 10) + '0');
			packet.push_back((slotsTotal % 10) + '0');
		}
		else
		{
			packet.push_back(1);
			packet.push_back(slotsTotal + '0');
		}
		packet.push_back(0x1a);
		packet.push_back(0x1e);
		packet.push_back(0x0a);
		packet.push_back(16);
		UTIL_AppendByteArray(packet, (unsigned char*)"game_create_time", 16);
		packet.push_back(0x12);
		packet.push_back(10); // unix time size
		uint32_t time = GetTime() - upTime;
		for (int i = 0; i < 10; i++)
		{
			packet.push_back((int)((time % (int)std::pow(10,10-i))/ (int)std::pow(10, 9-i)) + '0');
		}
		packet.push_back(0x1a);
		packet.push_back(0x0a);
		packet.push_back(0x0a);
		packet.push_back(5);
		UTIL_AppendByteArray(packet, (unsigned char*)"_type", 5);
		packet.push_back(0x12);
		packet.push_back(1);
		packet.push_back('1');	//No idea what type is
		packet.push_back(0x1a);
		packet.push_back(0x0d);
		packet.push_back(0x0a);
		packet.push_back(8);
		UTIL_AppendByteArray(packet, (unsigned char*)"_subtype", 8);
		packet.push_back(0x12);
		packet.push_back(1);
		packet.push_back('0');	//No idea what subtype is either
		packet.push_back(0x1a);
		packet.push_back(0x17);
		packet.push_back(0x0a);
		packet.push_back(11);
		UTIL_AppendByteArray(packet, (unsigned char*)"game_secret", 11);
		packet.push_back(0x12);
		packet.push_back(8);
		packet.push_back('7');	//literraly no clue
		packet.push_back('4');
		packet.push_back('9');
		packet.push_back('2');
		packet.push_back('5');
		packet.push_back('1');
		packet.push_back('4');
		packet.push_back('2');
		packet.push_back(0x1a);
		packet.push_back(0xc6);
		packet.push_back(0x01);
		packet.push_back(0x0a);
		packet.push_back(9);
		UTIL_AppendByteArray(packet, (unsigned char*)"game_data", 9);
		packet.push_back(0x12);
		packet.push_back(0xb8);
		packet.push_back(0x01);
		UTIL_AppendByteArray(packet, (unsigned char*)"\x43\x51\x41\x41\x41\x41\x45\x44\x53\x51\x63\x42\x41\x58\x30\x42\x69\x57\x38\x42\x39\x59\x74\x33\x45\x55\x32\x62\x59\x58\x46\x7a\x4c\x30\x64\x7a\x62\x34\x56\x37\x5a\x57\x39\x56\x61\x58\x4e\x76\x2f\x57\x39\x6c\x4c\x32\x4e\x76\x62\x57\x31\x72\x64\x57\x39\x70\x64\x58\x6b\x76\x4b\x56\x30\x31\x4b\x58\x4e\x35\x62\x32\x56\x7a\x74\x32\x64\x35\x59\x32\x6c\x6e\x63\x57\x46\x6c\x64\x32\x56\x6c\x4c\x33\x63\x7a\x65\x56\x6b\x42\x51\x32\x39\x6e\x5a\x57\x46\x76\x53\x51\x45\x42\x44\x66\x2b\x4a\x77\x54\x75\x42\x69\x79\x31\x35\x33\x34\x64\x68\x37\x77\x38\x33\x61\x54\x2b\x52\x42\x5a\x63\x6c\x41\x78\x6b\x41\x42\x41\x41\x41\x41\x45\x78\x76\x59\x32\x46\x73\x49\x45\x64\x68\x62\x57\x55\x67\x4b\x45\x4a\x76\x5a\x32\x52\x68\x62\x69\x6b\x41\x41\x4f\x51\x58"
			, 184);  //game data, base64, has map name, map path, a part is obfuscated like the old statsrring

		packet.push_back(0x1a);
		packet.push_back(0x0c);
		packet.push_back(0x0a);
		packet.push_back(7);
		UTIL_AppendByteArray(packet, (unsigned char*)"game_id", 7);
		packet.push_back(0x12);
		packet.push_back(1);
		packet.push_back('2');	//No idea what id is
		packet.push_back(0x1a);
		packet.push_back(0x0b);
		packet.push_back(0x0a);
		packet.push_back(6);
		UTIL_AppendByteArray(packet, (unsigned char*)"_flags", 6);
		packet.push_back(0x12);
		packet.push_back(1);
		packet.push_back('0');

		// Special mDNS packet TYPE:66
	/*	0000   c0 0c 00 42 80 01 00 00 11 94 01 91 0a 13 4c 6f   ...B..........Lo		// for local game with username Bogdan
		0010   63 61 6c 20 47 61 6d 65 20 28 42 6f 67 64 61 6e   cal Game(Bogdan		// and mappath maps\frozenthrone\community\(4)synergybigpaved.w3x
		0020   29 10 00 1a 10 0a 0b 70 6c 61 79 65 72 73 5f 6e   )......players_n
		0030   75 6d 12 01 31 1a 1c 0a 05 5f 6e 61 6d 65 12 13   um..1...._name..
		0040   4c 6f 63 61 6c 20 47 61 6d 65 20 28 42 6f 67 64   Local Game(Bogd
		0050   61 6e 29 1a 10 0a 0b 70 6c 61 79 65 72 73 5f 6d   an)....players_m
		0060   61 78 12 01 34 1a 1e 0a 10 67 61 6d 65 5f 63 72   ax..4....game_cr
		0070   65 61 74 65 5f 74 69 6d 65 12 0a 31 35 37 38 38   eate_time..15788
		0080   35 36 34 30 39 1a 0a 0a 05 5f 74 79 70 65 12 01   56409...._type..
		0090   31 1a 0d 0a 08 5f 73 75 62 74 79 70 65 12 01 30   1...._subtype..0
		00a0   1a 17 0a 0b 67 61 6d 65 5f 73 65 63 72 65 74 12   ....game_secret.
		00b0   08 37 34 39 32 35 31 34 32 1a c6 01 0a 09 67 61   .74925142.....ga
		00c0   6d 65 5f 64 61 74 61 12 b8 01 43 51 41 41 41 41   me_data...CQAAAA
		00d0   45 44 53 51 63 42 41 58 30 42 69 57 38 42 39 59   EDSQcBAX0BiW8B9Y
		00e0   74 33 45 55 32 62 59 58 46 7a 4c 30 64 7a 62 34   t3EU2bYXFzL0dzb4
		00f0   56 37 5a 57 39 56 61 58 4e 76 2f 57 39 6c 4c 32   V7ZW9VaXNv / W9lL2
		0100   4e 76 62 57 31 72 64 57 39 70 64 58 6b 76 4b 56   NvbW1rdW9pdXkvKV
		0110   30 31 4b 58 4e 35 62 32 56 7a 74 32 64 35 59 32   01KXN5b2Vzt2d5Y2
		0120   6c 6e 63 57 46 6c 64 32 56 6c 4c 33 63 7a 65 56   lncWFld2VlL3czeV
		0130   6b 42 51 32 39 6e 5a 57 46 76 53 51 45 42 44 66   kBQ29nZWFvSQEBDf
		0140   2b 4a 77 54 75 42 69 79 31 35 33 34 64 68 37 77 + JwTuBiy1534dh7w
		0150   38 33 61 54 2b 52 42 5a 63 6c 41 78 6b 41 42 41   83aT + RBZclAxkABA
		0160   41 41 41 45 78 76 59 32 46 73 49 45 64 68 62 57   AAAExvY2FsIEdhbW
		0170   55 67 4b 45 4a 76 5a 32 52 68 62 69 6b 41 41 4f   UgKEJvZ2RhbikAAO
		0180   51 58 1a 0c 0a 07 67 61 6d 65 5f 69 64 12 01 32   QX....game_id..2
		0190   1a 0b 0a 06 5f 66 6c 61 67 73 12 01 30            ...._flags..0*/


		UTIL_AppendByteArray(packet, (unsigned char*)"\xc0\x0c\x00\x21\x80\x01\x00\x00\x00\x78", 10);  //TYPE: SRV
		packet.push_back(0); //Data length
		packet.push_back(0x12); //Data length
		packet.push_back(0); //Priority
		packet.push_back(0);
		packet.push_back(0); //Weight
		packet.push_back(0);
		packet.push_back(0x17); //Port?  6112
		packet.push_back(0xe0);

		packet.push_back(9);  //local name size, gethostbyname?
		UTIL_AppendByteArray(packet, (unsigned char*)"NEWCOMP10", 9); //local name
		packet.push_back(0xc0);
		packet.push_back(0x2f);

		//Additional records


		UTIL_AppendByteArray(packet, (unsigned char*)"\xc2\x3b\x00\x01\x80\x01\x00\x00\x00\x78\x00\x04", 12);
		packet.push_back(0x19); //ipv4 address, matches source ip, 4 bytes;"\x19\x2a\x4e\x83"
		packet.push_back(0x2a);
		packet.push_back(0x4e);
		packet.push_back(0x83);
		UTIL_AppendByteArray(packet, (unsigned char*)"\xc2\x3c\x00\x1c\x80\x01\x00\x00\x00\x78\x00\x10\x26\x20\x00\x9b\x00\x00\x00\x00\x00\x00\x00\x00\x19\x2a\x4e\x83", 28);
		UTIL_AppendByteArray(packet, (unsigned char*)"\xc2\x3c\x00\x1c\x80\x01\x00\x00\x00\x78\x00\x10\xfe\x80\x00\x00\x00\x00\x00\x00\x1c\x95\x1a\x84\x83\xd7\x9a\xed", 28);

		UTIL_AppendByteArray(packet, (unsigned char*)"\xc2\x3b\x00\x2f\x80\x01\x00\x00\x00\x78\x00\x08\xc2\x3b\x00\x04\x40\x00\x00\x00", 20); //NSEC for ipv4
		UTIL_AppendByteArray(packet, (unsigned char*)"\xc0\x0c\x00\x2f\x80\x01\x00\x00\x11\x94\x00\x0d\xc0\x0c\x00\x09\x00\x00\x80\x00\x40\x00\x00\x00\x20", 25); //NSEC for game

	}
	return packet;
}

BYTEARRAY CGameProtocol :: SEND_W3GS_CREATEGAME( bool TFT, unsigned char war3Version )
{
	unsigned char ProductID_ROC[]	= {          51, 82, 65, 87 };	// "WAR3"
	unsigned char ProductID_TFT[]	= {          80, 88, 51, 87 };	// "W3XP"
	unsigned char Version[]			= { war3Version,  0,  0,  0 };
	unsigned char HostCounter[]		= {           1,  0,  0,  0 };

	BYTEARRAY packet;
	packet.push_back( W3GS_HEADER_CONSTANT );				// W3GS header constant
	packet.push_back( W3GS_CREATEGAME );					// W3GS_CREATEGAME
	packet.push_back( 0 );									// packet length will be assigned later
	packet.push_back( 0 );									// packet length will be assigned later

	if( TFT )
		UTIL_AppendByteArray( packet, ProductID_TFT, 4 );	// Product ID (TFT)
	else
		UTIL_AppendByteArray( packet, ProductID_ROC, 4 );	// Product ID (ROC)

	UTIL_AppendByteArray( packet, Version, 4 );				// Version
	UTIL_AppendByteArray( packet, HostCounter, 4 );			// Host Counter
	AssignLength( packet );
	// DEBUG_Print( "SENT W3GS_CREATEGAME" );
	// DEBUG_Print( packet );
	return packet;
}

BYTEARRAY CGameProtocol :: SEND_W3GS_REFRESHGAME( uint32_t players, uint32_t playerSlots )
{
	unsigned char HostCounter[]	= { 1, 0, 0, 0 };

	BYTEARRAY packet;
	packet.push_back( W3GS_HEADER_CONSTANT );			// W3GS header constant
	packet.push_back( W3GS_REFRESHGAME );				// W3GS_REFRESHGAME
	packet.push_back( 0 );								// packet length will be assigned later
	packet.push_back( 0 );								// packet length will be assigned later
	UTIL_AppendByteArray( packet, HostCounter, 4 );		// Host Counter
	UTIL_AppendByteArray( packet, players, false );		// Players
	UTIL_AppendByteArray( packet, playerSlots, false );	// Player Slots
	AssignLength( packet );
	// DEBUG_Print( "SENT W3GS_REFRESHGAME" );
	// DEBUG_Print( packet );
	return packet;
}

BYTEARRAY CGameProtocol :: SEND_W3GS_DECREATEGAME( )
{
	unsigned char HostCounter[]	= { 1, 0, 0, 0 };

	BYTEARRAY packet;
	packet.push_back( W3GS_HEADER_CONSTANT );			// W3GS header constant
	packet.push_back( W3GS_DECREATEGAME );				// W3GS_DECREATEGAME
	packet.push_back( 0 );								// packet length will be assigned later
	packet.push_back( 0 );								// packet length will be assigned later
	UTIL_AppendByteArray( packet, HostCounter, 4 );		// Host Counter
	AssignLength( packet );
	// DEBUG_Print( "SENT W3GS_DECREATEGAME" );
	// DEBUG_Print( packet );
	return packet;
}

BYTEARRAY CGameProtocol :: SEND_W3GS_MAPCHECK( string mapPath, BYTEARRAY mapSize, BYTEARRAY mapInfo, BYTEARRAY mapCRC, BYTEARRAY mapSHA1 )
{
	unsigned char Unknown[] = { 1, 0, 0, 0 };

	BYTEARRAY packet;

	if( !mapPath.empty( ) && mapSize.size( ) == 4 && mapInfo.size( ) == 4 && mapCRC.size( ) == 4 && mapSHA1.size( ) == 20 )
	{
		packet.push_back( W3GS_HEADER_CONSTANT );		// W3GS header constant
		packet.push_back( W3GS_MAPCHECK );				// W3GS_MAPCHECK
		packet.push_back( 0 );							// packet length will be assigned later
		packet.push_back( 0 );							// packet length will be assigned later
		UTIL_AppendByteArray( packet, Unknown, 4 );		// ???
		UTIL_AppendByteArrayFast( packet, mapPath );	// map path
		UTIL_AppendByteArrayFast( packet, mapSize );	// map size
		UTIL_AppendByteArrayFast( packet, mapInfo );	// map info
		UTIL_AppendByteArrayFast( packet, mapCRC );		// map crc
		UTIL_AppendByteArrayFast( packet, mapSHA1 );	// map sha1
		AssignLength( packet );
	}
	else
		CONSOLE_Print( "[GAMEPROTO] invalid parameters passed to SEND_W3GS_MAPCHECK" );

	// DEBUG_Print( "SENT W3GS_MAPCHECK" );
	// DEBUG_Print( packet );
	return packet;
}

BYTEARRAY CGameProtocol :: SEND_W3GS_STARTDOWNLOAD( unsigned char fromPID )
{
	unsigned char Unknown[] = { 1, 0, 0, 0 };

	BYTEARRAY packet;
	packet.push_back( W3GS_HEADER_CONSTANT );				// W3GS header constant
	packet.push_back( W3GS_STARTDOWNLOAD );					// W3GS_STARTDOWNLOAD
	packet.push_back( 0 );									// packet length will be assigned later
	packet.push_back( 0 );									// packet length will be assigned later
	UTIL_AppendByteArray( packet, Unknown, 4 );				// ???
	packet.push_back( fromPID );							// from PID
	AssignLength( packet );
	// DEBUG_Print( "SENT W3GS_STARTDOWNLOAD" );
	// DEBUG_Print( packet );
	return packet;
}

BYTEARRAY CGameProtocol :: SEND_W3GS_MAPPART( unsigned char fromPID, unsigned char toPID, uint32_t start, string *mapData )
{
	unsigned char Unknown[] = { 1, 0, 0, 0 };

	BYTEARRAY packet;

	if( start < mapData->size( ) )
	{
		packet.push_back( W3GS_HEADER_CONSTANT );				// W3GS header constant
		packet.push_back( W3GS_MAPPART );						// W3GS_MAPPART
		packet.push_back( 0 );									// packet length will be assigned later
		packet.push_back( 0 );									// packet length will be assigned later
		packet.push_back( toPID );								// to PID
		packet.push_back( fromPID );							// from PID
		UTIL_AppendByteArray( packet, Unknown, 4 );				// ???
		UTIL_AppendByteArray( packet, start, false );			// start position

		// calculate end position (don't send more than 1442 map bytes in one packet)

		uint32_t End = start + 1442;

		if( End > mapData->size( ) )
			End = mapData->size( );

		// calculate crc

		BYTEARRAY crc32 = UTIL_CreateByteArray( m_GHost->m_CRC->FullCRC( (unsigned char *)mapData->c_str( ) + start, End - start ), false );
		UTIL_AppendByteArrayFast( packet, crc32 );

		// map data

		BYTEARRAY Data = UTIL_CreateByteArray( (unsigned char *)mapData->c_str( ) + start, End - start );
		UTIL_AppendByteArrayFast( packet, Data );
		AssignLength( packet );
	}
	else
		CONSOLE_Print( "[GAMEPROTO] invalid parameters passed to SEND_W3GS_MAPPART" );

	// DEBUG_Print( "SENT W3GS_MAPPART" );
	// DEBUG_Print( packet );
	return packet;
}

BYTEARRAY CGameProtocol :: SEND_W3GS_INCOMING_ACTION2( queue<CIncomingAction *> actions )
{
	BYTEARRAY packet;
	packet.push_back( W3GS_HEADER_CONSTANT );				// W3GS header constant
	packet.push_back( W3GS_INCOMING_ACTION2 );				// W3GS_INCOMING_ACTION2
	packet.push_back( 0 );									// packet length will be assigned later
	packet.push_back( 0 );									// packet length will be assigned later
	packet.push_back( 0 );									// ??? (send interval?)
	packet.push_back( 0 );									// ??? (send interval?)

	// create subpacket

	if( !actions.empty( ) )
	{
		BYTEARRAY subpacket;

		while( !actions.empty( ) )
		{
			CIncomingAction *Action = actions.front( );
			actions.pop( );
			subpacket.push_back( Action->GetPID( ) );
			UTIL_AppendByteArray( subpacket, (uint16_t)Action->GetAction( )->size( ), false );
			UTIL_AppendByteArrayFast( subpacket, *Action->GetAction( ) );
		}

		// calculate crc (we only care about the first 2 bytes though)

		BYTEARRAY crc32 = UTIL_CreateByteArray( m_GHost->m_CRC->FullCRC( (unsigned char *)string( subpacket.begin( ), subpacket.end( ) ).c_str( ), subpacket.size( ) ), false );
		crc32.resize( 2 );

		// finish subpacket

		UTIL_AppendByteArrayFast( packet, crc32 );			// crc
		UTIL_AppendByteArrayFast( packet, subpacket );		// subpacket
	}

	AssignLength( packet );
	// DEBUG_Print( "SENT W3GS_INCOMING_ACTION2" );
	// DEBUG_Print( packet );
	return packet;
}

/////////////////////
// OTHER FUNCTIONS //
/////////////////////

bool CGameProtocol :: AssignLength( BYTEARRAY &content )
{
	// insert the actual length of the content array into bytes 3 and 4 (indices 2 and 3)

	BYTEARRAY LengthBytes;

	if( content.size( ) >= 4 && content.size( ) <= 65535 )
	{
		LengthBytes = UTIL_CreateByteArray( (uint16_t)content.size( ), false );
		content[2] = LengthBytes[0];
		content[3] = LengthBytes[1];
		return true;
	}

	return false;
}

bool CGameProtocol :: ValidateLength( BYTEARRAY &content )
{
	// verify that bytes 3 and 4 (indices 2 and 3) of the content array describe the length

	uint16_t Length;
	BYTEARRAY LengthBytes;

	if( content.size( ) >= 4 && content.size( ) <= 65535 )
	{
		LengthBytes.push_back( content[2] );
		LengthBytes.push_back( content[3] );
		Length = UTIL_ByteArrayToUInt16( LengthBytes, false );

		if( Length == content.size( ) )
			return true;
	}

	return false;
}

BYTEARRAY CGameProtocol :: EncodeSlotInfo( vector<CGameSlot> &slots, uint32_t randomSeed, unsigned char layoutStyle, unsigned char playerSlots )
{
	BYTEARRAY SlotInfo;
	SlotInfo.push_back( (unsigned char)slots.size( ) );		// number of slots

	for( unsigned int i = 0; i < slots.size( ); ++i )
		UTIL_AppendByteArray( SlotInfo, slots[i].GetByteArray( ) );

	UTIL_AppendByteArray( SlotInfo, randomSeed, false );	// random seed
	SlotInfo.push_back( layoutStyle );						// LayoutStyle (0 = melee, 1 = custom forces, 3 = custom forces + fixed player settings)
	SlotInfo.push_back( playerSlots );						// number of player slots (non observer)
	return SlotInfo;
}

//
// CIncomingJoinPlayer
//

CIncomingJoinPlayer :: CIncomingJoinPlayer( uint32_t nHostCounter, uint32_t nEntryKey, string nName, BYTEARRAY &nInternalIP ) : m_HostCounter( nHostCounter ), m_EntryKey( nEntryKey ), m_Name( nName ), m_InternalIP( nInternalIP )
{

}

CIncomingJoinPlayer :: ~CIncomingJoinPlayer( )
{

}

//
// CIncomingAction
//

CIncomingAction :: CIncomingAction( unsigned char nPID, BYTEARRAY &nCRC, BYTEARRAY &nAction ) : m_PID( nPID ), m_CRC( nCRC ), m_Action( nAction )
{

}

CIncomingAction :: ~CIncomingAction( )
{

}

//
// CIncomingChatPlayer
//

CIncomingChatPlayer :: CIncomingChatPlayer( unsigned char nFromPID, BYTEARRAY &nToPIDs, unsigned char nFlag, string nMessage ) : m_Type( CTH_MESSAGE ), m_FromPID( nFromPID ), m_ToPIDs( nToPIDs ), m_Flag( nFlag ), m_Message( nMessage )
{

}

CIncomingChatPlayer :: CIncomingChatPlayer( unsigned char nFromPID, BYTEARRAY &nToPIDs, unsigned char nFlag, string nMessage, BYTEARRAY &nExtraFlags ) : m_Type( CTH_MESSAGEEXTRA ), m_FromPID( nFromPID ), m_ToPIDs( nToPIDs ), m_Flag( nFlag ), m_Message( nMessage ), m_ExtraFlags( nExtraFlags )
{

}

CIncomingChatPlayer :: CIncomingChatPlayer( unsigned char nFromPID, BYTEARRAY &nToPIDs, unsigned char nFlag, unsigned char nByte ) : m_FromPID( nFromPID ), m_ToPIDs( nToPIDs ), m_Flag( nFlag ), m_Byte( nByte )
{
	if( nFlag == 17 )
		m_Type = CTH_TEAMCHANGE;
	else if( nFlag == 18 )
		m_Type = CTH_COLOURCHANGE;
	else if( nFlag == 19 )
		m_Type = CTH_RACECHANGE;
	else if( nFlag == 20 )
		m_Type = CTH_HANDICAPCHANGE;
}

CIncomingChatPlayer :: ~CIncomingChatPlayer( )
{

}

//
// CIncomingMapSize
//

CIncomingMapSize :: CIncomingMapSize( unsigned char nSizeFlag, uint32_t nMapSize ) : m_SizeFlag( nSizeFlag ), m_MapSize( nMapSize )
{

}

CIncomingMapSize :: ~CIncomingMapSize( )
{

}
