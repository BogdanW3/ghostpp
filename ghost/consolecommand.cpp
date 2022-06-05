#include "consolecommand.h"

void ProcessConsole(std::string Command, std::string Payload, CGHost *m_GHost)
{
	if (Command == "autohost")
	{
		if (Payload.empty() || Payload == "off")
		{
			CONSOLE_Print(m_GHost->m_Language->AutoHostDisabled());
			m_GHost->m_AutoHostGameName.clear();
			m_GHost->m_AutoHostOwner.clear();
			m_GHost->m_AutoHostServer.clear();
			m_GHost->m_AutoHostMaximumGames = 0;
			m_GHost->m_AutoHostAutoStartPlayers = 0;
			m_GHost->m_LastAutoHostTime = GetTime();
			m_GHost->m_AutoHostMatchMaking = false;
			m_GHost->m_AutoHostMinimumScore = 0.0;
			m_GHost->m_AutoHostMaximumScore = 0.0;
		}
		else
		{
			// extract the maximum games, auto start players, and the game name
			// e.g. "5 10 BattleShips Pro" -> maximum games: "5", auto start players: "10", game name: "BattleShips Pro"

			std::string Owner;
			uint32_t MaximumGames;
			uint32_t AutoStartPlayers;
			std::string GameName;
			std::stringstream SS;
			SS << Payload;
			SS >> Owner;

			if (SS.fail() || Owner.length() < 3)
			{
				CONSOLE_Print("[CONSOLE] bad input #1 to autohost command (host name, 3 letters or longer)");
				return;
			}
			SS >> MaximumGames;

			if (SS.fail() || MaximumGames == 0)
			{
				CONSOLE_Print("[CONSOLE] bad input #2 to autohost command");
				return;
			}
			SS >> AutoStartPlayers;

			if (SS.fail() || AutoStartPlayers == 0)
			{
				CONSOLE_Print("[CONSOLE] bad input #3 to autohost command");
				return;
			}
			if (SS.eof())
			{
				CONSOLE_Print("[CONSOLE] missing input #4 to autohost command");
				return;
			}
			getline(SS, GameName);
			std::string::size_type Start = GameName.find_first_not_of(" ");

			if (Start != std::string::npos)
				GameName = GameName.substr(Start);

			CONSOLE_Print(m_GHost->m_Language->AutoHostEnabled());
			delete m_GHost->m_AutoHostMap;
			m_GHost->m_AutoHostMap = new CMap(*m_GHost->m_Map);
			m_GHost->m_AutoHostGameName = GameName;
			m_GHost->m_AutoHostOwner = Owner;
			m_GHost->m_AutoHostServer.clear();
			m_GHost->m_AutoHostMaximumGames = MaximumGames;
			m_GHost->m_AutoHostAutoStartPlayers = AutoStartPlayers;
			m_GHost->m_LastAutoHostTime = GetTime();
			m_GHost->m_AutoHostMatchMaking = false;
			m_GHost->m_AutoHostMinimumScore = 0.0;
			m_GHost->m_AutoHostMaximumScore = 0.0;
		}
	}

	//
	// !AUTOHOSTMM
	//

	else if (Command == "autohostmm")
	{
		if (Payload.empty() || Payload == "off")
		{
			CONSOLE_Print(m_GHost->m_Language->AutoHostDisabled());
			m_GHost->m_AutoHostGameName.clear();
			m_GHost->m_AutoHostOwner.clear();
			m_GHost->m_AutoHostServer.clear();
			m_GHost->m_AutoHostMaximumGames = 0;
			m_GHost->m_AutoHostAutoStartPlayers = 0;
			m_GHost->m_LastAutoHostTime = GetTime();
			m_GHost->m_AutoHostMatchMaking = false;
			m_GHost->m_AutoHostMinimumScore = 0.0;
			m_GHost->m_AutoHostMaximumScore = 0.0;
		}
		else
		{
			// extract the maximum games, auto start players, and the game name
			// e.g. "5 10 800 1200 BattleShips Pro" -> maximum games: "5", auto start players: "10", minimum score: "800", maximum score: "1200", game name: "BattleShips Pro"

			std::string Owner;
			uint32_t MaximumGames;
			uint32_t AutoStartPlayers;
			double MinimumScore;
			double MaximumScore;
			std::string GameName;
			std::stringstream SS;
			SS << Payload;
			SS >> Owner;

			if (SS.fail() || Owner.length() < 3)
			{
				CONSOLE_Print("[CONSOLE] bad input #1 to autohost command (host name, 3 letters or longer)");
				return;
			}
			SS >> MaximumGames;

			if (SS.fail() || MaximumGames == 0)
			{
				CONSOLE_Print("[CONSOLE] bad input #2 to autohostmm command");
				return;
			}
			SS >> AutoStartPlayers;

			if (SS.fail() || AutoStartPlayers == 0)
			{
				CONSOLE_Print("[CONSOLE] bad input #3 to autohostmm command");
				return;
			}
			SS >> MinimumScore;

			if (SS.fail())
			{
				CONSOLE_Print("[CONSOLE] bad input #4 to autohostmm command");
				return;
			}
			SS >> MaximumScore;

			if (SS.fail())
			{
				CONSOLE_Print("[CONSOLE] bad input #5 to autohostmm command");
				return;
			}
			if (SS.eof())
			{
				CONSOLE_Print("[CONSOLE] missing input #6 to autohostmm command");
				return;
			}
			
			getline(SS, GameName);
			std::string::size_type Start = GameName.find_first_not_of(" ");

			if (Start != std::string::npos)
				GameName = GameName.substr(Start);

			CONSOLE_Print(m_GHost->m_Language->AutoHostEnabled());
			delete m_GHost->m_AutoHostMap;
			m_GHost->m_AutoHostMap = new CMap(*m_GHost->m_Map);
			m_GHost->m_AutoHostGameName = GameName;
			m_GHost->m_AutoHostOwner = Owner;
			m_GHost->m_AutoHostServer.clear();
			m_GHost->m_AutoHostMaximumGames = MaximumGames;
			m_GHost->m_AutoHostAutoStartPlayers = AutoStartPlayers;
			m_GHost->m_LastAutoHostTime = GetTime();
			m_GHost->m_AutoHostMatchMaking = true;
			m_GHost->m_AutoHostMinimumScore = MinimumScore;
			m_GHost->m_AutoHostMaximumScore = MaximumScore;
		}
	}

	//
	// !CHECKADMIN
	//

	else if (Command == "checkadmin" && !Payload.empty())
	{
		// extract the name and the server
		// e.g. "Varlock useast.battle.net" -> name: "Varlock", server: "useast.battle.net"

		std::string Name;
		std::string Server;
		std::stringstream SS;
		SS << Payload;
		SS >> Name;

		if (SS.eof())
		{
			if (m_GHost->m_BNETs.size() == 1)
				Server = m_GHost->m_BNETs[0]->GetServer();
			else
				CONSOLE_Print("[ADMINGAME] missing input #2 to checkadmin command");
		}
		else
			SS >> Server;

		if (!Server.empty())
		{
			std::string Servers;
			bool FoundServer = false;

			for (std::vector<CBNET*> ::iterator i = m_GHost->m_BNETs.begin(); i != m_GHost->m_BNETs.end(); ++i)
			{
				if (Servers.empty())
					Servers = (*i)->GetServer();
				else
					Servers += ", " + (*i)->GetServer();

				if ((*i)->GetServer() == Server)
				{
					FoundServer = true;

					if ((*i)->IsAdmin(Name))
						CONSOLE_Print(m_GHost->m_Language->UserIsAnAdmin(Server, Name));
					else
						CONSOLE_Print(m_GHost->m_Language->UserIsNotAnAdmin(Server, Name));

					break;
				}
			}

			if (!FoundServer)
				CONSOLE_Print(m_GHost->m_Language->ValidServers(Servers));
		}
	}

	//
	// !CHECKBAN
	//

	else if (Command == "checkban" && !Payload.empty())
	{
		// extract the name and the server
		// e.g. "Varlock useast.battle.net" -> name: "Varlock", server: "useast.battle.net"

		std::string Name;
		std::string Server;
		std::stringstream SS;
		SS << Payload;
		SS >> Name;

		if (SS.eof())
		{
			if (m_GHost->m_BNETs.size() == 1)
				Server = m_GHost->m_BNETs[0]->GetServer();
			else
				CONSOLE_Print("[ADMINGAME] missing input #2 to checkban command");
		}
		else
			SS >> Server;

		if (!Server.empty())
		{
			std::string Servers;
			bool FoundServer = false;

			for (std::vector<CBNET*> ::iterator i = m_GHost->m_BNETs.begin(); i != m_GHost->m_BNETs.end(); ++i)
			{
				if (Servers.empty())
					Servers = (*i)->GetServer();
				else
					Servers += ", " + (*i)->GetServer();

				if ((*i)->GetServer() == Server)
				{
					FoundServer = true;
					CDBBan* Ban = (*i)->IsBannedName(Name);

					if (Ban)
						CONSOLE_Print(m_GHost->m_Language->UserWasBannedOnByBecause(Server, Name, Ban->GetDate(), Ban->GetAdmin(), Ban->GetReason()));
					else
						CONSOLE_Print(m_GHost->m_Language->UserIsNotBanned(Server, Name));

					break;
				}
			}

			if (!FoundServer)
				CONSOLE_Print(m_GHost->m_Language->ValidServers(Servers));
		}
	}

	//
	// !DISABLE
	//

	else if (Command == "disable")
	{
		CONSOLE_Print(m_GHost->m_Language->BotDisabled());
		m_GHost->m_Enabled = false;
	}

	//
	// !DOWNLOADS
	//

	else if (Command == "downloads" && !Payload.empty())
	{
		uint32_t Downloads = UTIL_ToUInt32(Payload);

		if (Downloads == 0)
		{
			CONSOLE_Print(m_GHost->m_Language->MapDownloadsDisabled());
			m_GHost->m_AllowDownloads = 0;
		}
		else if (Downloads == 1)
		{
			CONSOLE_Print(m_GHost->m_Language->MapDownloadsEnabled());
			m_GHost->m_AllowDownloads = 1;
		}
		else if (Downloads == 2)
		{
			CONSOLE_Print(m_GHost->m_Language->MapDownloadsConditional());
			m_GHost->m_AllowDownloads = 2;
		}
	}

	//
	// !ENABLE
	//

	else if (Command == "enable")
	{
		CONSOLE_Print(m_GHost->m_Language->BotEnabled());
		m_GHost->m_Enabled = true;
	}

	//
	// !END
	//

	else if (Command == "end" && !Payload.empty())
	{
		// todotodo: what if a game ends just as you're typing this command and the numbering changes?

		uint32_t GameNumber = UTIL_ToUInt32(Payload) - 1;

		if (GameNumber < m_GHost->m_Games.size())
		{
			CONSOLE_Print(m_GHost->m_Language->EndingGame(m_GHost->m_Games[GameNumber]->GetDescription()));
			CONSOLE_Print("[GAME: " + m_GHost->m_Games[GameNumber]->GetGameName() + "] is over (admin ended game)");
			m_GHost->m_Games[GameNumber]->StopPlayers("was disconnected (admin ended game)");
		}
		else
			CONSOLE_Print(m_GHost->m_Language->GameNumberDoesntExist(Payload));
	}

	//
	// !ENFORCESG
	//

	else if (Command == "enforcesg" && !Payload.empty())
	{
		// only load files in the current directory just to be safe

		if (Payload.find("/") != std::string::npos || Payload.find("\\") != std::string::npos)
			CONSOLE_Print(m_GHost->m_Language->UnableToLoadReplaysOutside());
		else
		{
			std::string File = m_GHost->m_ReplayPath + Payload + ".w3g";

			if (UTIL_FileExists(File))
			{
				CONSOLE_Print(m_GHost->m_Language->LoadingReplay(File));
				CReplay* Replay = new CReplay();
				Replay->Load(File, false);
				Replay->ParseReplay(false);
				m_GHost->m_EnforcePlayers = Replay->GetPlayers();
				delete Replay;
			}
			else
				CONSOLE_Print(m_GHost->m_Language->UnableToLoadReplayDoesntExist(File));
		}
	}

	//
	// !GETGAME
	//

	else if (Command == "getgame" && !Payload.empty())
	{
		uint32_t GameNumber = UTIL_ToUInt32(Payload) - 1;

		if (GameNumber < m_GHost->m_Games.size())
			CONSOLE_Print(m_GHost->m_Language->GameNumberIs(Payload, m_GHost->m_Games[GameNumber]->GetDescription()));
		else
			CONSOLE_Print(m_GHost->m_Language->GameNumberDoesntExist(Payload));
	}

	//
	// !GETGAMES
	//

	else if (Command == "getgames")
	{
		if (m_GHost->m_CurrentGame)
			CONSOLE_Print(m_GHost->m_Language->GameIsInTheLobby(m_GHost->m_CurrentGame->GetDescription(), UTIL_ToString(m_GHost->m_Games.size()), UTIL_ToString(m_GHost->m_MaxGames)));
		else
			CONSOLE_Print(m_GHost->m_Language->ThereIsNoGameInTheLobby(UTIL_ToString(m_GHost->m_Games.size()), UTIL_ToString(m_GHost->m_MaxGames)));
	}

	//
	// !HOSTSG
	//

	else if (Command == "hostsg" && !Payload.empty())
	{
		// extract the owner and the game name
		// e.g. "Varlock dota 6.54b arem ~~~" -> owner: "Varlock", game name: "dota 6.54b arem ~~~"

		std::string Owner;
		std::string GameName;
		std::string::size_type GameNameStart = Payload.find(" ");

		if (GameNameStart != std::string::npos)
		{
			Owner = Payload.substr(0, GameNameStart);
			GameName = Payload.substr(GameNameStart + 1);
			m_GHost->CreateGame(m_GHost->m_Map, GAME_PRIVATE, true, GameName, Owner, "THE SERVER", std::string(), false);
		}
	}

	//
	// !HOSTSGPUB
	//

	else if (Command == "hostsgpub" && !Payload.empty())
	{
		// extract the owner and the game name
		// e.g. "Varlock dota 6.54b arem ~~~" -> owner: "Varlock", game name: "dota 6.54b arem ~~~"

		std::string Owner;
		std::string GameName;
		std::string::size_type GameNameStart = Payload.find(" ");

		if (GameNameStart != std::string::npos)
		{
			Owner = Payload.substr(0, GameNameStart);
			GameName = Payload.substr(GameNameStart + 1);
			m_GHost->CreateGame(m_GHost->m_Map, GAME_PUBLIC, true, GameName, Owner, "THE SERVER", std::string(), false);
		}
	}

	//
	// !LOAD (load config file)
	//

	else if (Command == "load")
	{
		if (Payload.empty())
			CONSOLE_Print(m_GHost->m_Language->CurrentlyLoadedMapCFGIs(m_GHost->m_Map->GetCFGFile()));
		else
		{
			std::string FoundMapConfigs;

			try
			{
				path MapCFGPath(m_GHost->m_MapCFGPath);
				std::string Pattern = Payload;
				transform(Pattern.begin(), Pattern.end(), Pattern.begin(), (int(*)(int))tolower);

				if (!exists(MapCFGPath))
				{
					CONSOLE_Print("[ADMINGAME] error listing map configs - map config path doesn't exist");
					CONSOLE_Print(m_GHost->m_Language->ErrorListingMapConfigs());
				}
				else
				{
					directory_iterator EndIterator;
					path LastMatch;
					uint32_t Matches = 0;

					for (directory_iterator i(MapCFGPath); i != EndIterator; ++i)
					{
						std::string FileName = i->path().filename().string();
						std::string Stem = i->path().stem().string();
						transform(FileName.begin(), FileName.end(), FileName.begin(), (int(*)(int))tolower);
						transform(Stem.begin(), Stem.end(), Stem.begin(), (int(*)(int))tolower);

						if (!is_directory(i->status()) && i->path().extension() == ".cfg" && FileName.find(Pattern) != std::string::npos)
						{
							LastMatch = i->path();
							++Matches;

							if (FoundMapConfigs.empty())
								FoundMapConfigs = i->path().filename().string();
							else
								FoundMapConfigs += ", " + i->path().filename().string();

							// if the pattern matches the filename exactly, with or without extension, stop any further matching

							if (FileName == Pattern || Stem == Pattern)
							{
								Matches = 1;
								break;
							}
						}
					}

					if (Matches == 0)
						CONSOLE_Print(m_GHost->m_Language->NoMapConfigsFound());
					else if (Matches == 1)
					{
						std::string File = LastMatch.filename().string();
						CONSOLE_Print(m_GHost->m_Language->LoadingConfigFile(m_GHost->m_MapCFGPath + File));
						CConfig MapCFG;
						MapCFG.Read(LastMatch.string());
						m_GHost->m_Map->Load(&MapCFG, m_GHost->m_MapCFGPath + File);
					}
					else
						CONSOLE_Print(m_GHost->m_Language->FoundMapConfigs(FoundMapConfigs));
				}
			}
			catch (const std::exception& ex)
			{
				CONSOLE_Print(std::string("[ADMINGAME] error listing map configs - caught exception [") + ex.what() + "]");
				CONSOLE_Print(m_GHost->m_Language->ErrorListingMapConfigs());
			}
		}
	}

	//
	// !LOADSG
	//

	else if (Command == "loadsg" && !Payload.empty())
	{
		// only load files in the current directory just to be safe

		if (Payload.find("/") != std::string::npos || Payload.find("\\") != std::string::npos)
			CONSOLE_Print(m_GHost->m_Language->UnableToLoadSaveGamesOutside());
		else
		{
			std::string File = m_GHost->m_SaveGamePath + Payload + ".w3z";
			std::string FileNoPath = Payload + ".w3z";

			if (UTIL_FileExists(File))
			{
				if (m_GHost->m_CurrentGame)
					CONSOLE_Print(m_GHost->m_Language->UnableToLoadSaveGameGameInLobby());
				else
				{
					CONSOLE_Print(m_GHost->m_Language->LoadingSaveGame(File));
					m_GHost->m_SaveGame->Load(File, false);
					m_GHost->m_SaveGame->ParseSaveGame();
					m_GHost->m_SaveGame->SetFileName(File);
					m_GHost->m_SaveGame->SetFileNameNoPath(FileNoPath);
				}
			}
			else
				CONSOLE_Print(m_GHost->m_Language->UnableToLoadSaveGameDoesntExist(File));
		}
	}

	//
	// !MAP (load map file)
	//

	else if (Command == "map")
	{
		if (Payload.empty())
			CONSOLE_Print(m_GHost->m_Language->CurrentlyLoadedMapCFGIs(m_GHost->m_Map->GetCFGFile()));
		else
		{
			std::string FoundMaps;

			try
			{
				path MapPath(m_GHost->m_MapPath);
				std::string Pattern = Payload;
				transform(Pattern.begin(), Pattern.end(), Pattern.begin(), (int(*)(int))tolower);

				if (!exists(MapPath))
				{
					CONSOLE_Print("[ADMINGAME] error listing maps - map path doesn't exist");
					CONSOLE_Print(m_GHost->m_Language->ErrorListingMaps());
				}
				else
				{
					directory_iterator EndIterator;
					path LastMatch;
					uint32_t Matches = 0;

					for (directory_iterator i(MapPath); i != EndIterator; ++i)
					{
						std::string FileName = i->path().filename().string();
						std::string Stem = i->path().stem().string();
						transform(FileName.begin(), FileName.end(), FileName.begin(), (int(*)(int))tolower);
						transform(Stem.begin(), Stem.end(), Stem.begin(), (int(*)(int))tolower);

						if (!is_directory(i->status()) && FileName.find(Pattern) != std::string::npos)
						{
							LastMatch = i->path();
							++Matches;

							if (FoundMaps.empty())
								FoundMaps = i->path().filename().string();
							else
								FoundMaps += ", " + i->path().filename().string();

							// if the pattern matches the filename exactly, with or without extension, stop any further matching

							if (FileName == Pattern || Stem == Pattern)
							{
								Matches = 1;
								break;
							}
						}
					}

					if (Matches == 0)
						CONSOLE_Print(m_GHost->m_Language->NoMapsFound());
					else if (Matches == 1)
					{
						std::string File = LastMatch.filename().string();
						CONSOLE_Print(m_GHost->m_Language->LoadingConfigFile(File));

						// hackhack: create a config file in memory with the required information to load the map

						CConfig MapCFG;
						MapCFG.Set("map_path", "Maps\\Download\\" + File);
						MapCFG.Set("map_localpath", File);
						m_GHost->m_Map->Load(&MapCFG, File);
					}
					else
						CONSOLE_Print(m_GHost->m_Language->FoundMaps(FoundMaps));
				}
			}
			catch (const std::exception& ex)
			{
				CONSOLE_Print(std::string("[ADMINGAME] error listing maps - caught exception [") + ex.what() + "]");
				CONSOLE_Print(m_GHost->m_Language->ErrorListingMaps());
			}
		}
	}
	// !PRIVBY (host private game by other player)
	//

	else if (Command == "privby" && !Payload.empty())
	{
		// extract the owner and the game name
		// e.g. "Varlock dota 6.54b arem ~~~" -> owner: "Varlock", game name: "dota 6.54b arem ~~~"

		std::string Owner;
		std::string GameName;
		std::string::size_type GameNameStart = Payload.find(" ");

		if (GameNameStart != std::string::npos)
		{
			Owner = Payload.substr(0, GameNameStart);
			GameName = Payload.substr(GameNameStart + 1);
			m_GHost->CreateGame(m_GHost->m_Map, GAME_PRIVATE, false, GameName, Owner, "THE SERVER", std::string(), false);
		}
	}

	//
	// !PUBBY (host public game by other player)
	//

	if (Command == "pubby" && !Payload.empty())
	{
		// extract the owner and the game name
		// e.g. "Varlock dota 6.54b arem ~~~" -> owner: "Varlock", game name: "dota 6.54b arem ~~~"

		std::string Owner;
		std::string GameName;
		std::string::size_type GameNameStart = Payload.find(" ");

		if (GameNameStart != std::string::npos)
		{
			Owner = Payload.substr(0, GameNameStart);
			GameName = Payload.substr(GameNameStart + 1);
			m_GHost->CreateGame(m_GHost->m_Map, GAME_PUBLIC, false, GameName, Owner, "THE SERVER", std::string(), false);
		}
	}

	//
	// !RELOAD
	//

	else if (Command == "reload")
	{
		CONSOLE_Print(m_GHost->m_Language->ReloadingConfigurationFiles());
		m_GHost->ReloadConfigs();
	}

	//
	// !SAY
	//

	else if (Command == "say" && !Payload.empty())
	{
		for (std::vector<CBNET*> ::iterator i = m_GHost->m_BNETs.begin(); i != m_GHost->m_BNETs.end(); ++i)
			(*i)->QueueChatCommand(Payload);
	}

	//
	// !SAYGAME
	//

	else if (Command == "saygame" && !Payload.empty())
	{
		// extract the game number and the message
		// e.g. "3 hello everyone" -> game number: "3", message: "hello everyone"

		uint32_t GameNumber;
		std::string Message;
		std::stringstream SS;
		SS << Payload;
		SS >> GameNumber;

		if (SS.fail())
			CONSOLE_Print("[ADMINGAME] bad input #1 to saygame command");
		else
		{
			if (SS.eof())
				CONSOLE_Print("[ADMINGAME] missing input #2 to saygame command");
			else
			{
				getline(SS, Message);
				std::string::size_type Start = Message.find_first_not_of(" ");

				if (Start != std::string::npos)
					Message = Message.substr(Start);

				if (GameNumber - 1 < m_GHost->m_Games.size())
					m_GHost->m_Games[GameNumber - 1]->SendAllChat("ADMIN: " + Message);
				else
					CONSOLE_Print(m_GHost->m_Language->GameNumberDoesntExist(UTIL_ToString(GameNumber)));
			}
		}
	}

	//
	// !SAYGAMES
	//

	else if (Command == "saygames" && !Payload.empty())
	{
		if (m_GHost->m_CurrentGame)
			m_GHost->m_CurrentGame->SendAllChat(Payload);

		for (std::vector<CBaseGame*> ::iterator i = m_GHost->m_Games.begin(); i != m_GHost->m_Games.end(); ++i)
			(*i)->SendAllChat("ADMIN: " + Payload);
	}

	//
	// !UNHOST
	//

	else if (Command == "unhost")
	{
		if (m_GHost->m_CurrentGame)
		{
			if (m_GHost->m_CurrentGame->GetCountDownStarted())
				CONSOLE_Print(m_GHost->m_Language->UnableToUnhostGameCountdownStarted(m_GHost->m_CurrentGame->GetDescription()));
			else
			{
				CONSOLE_Print(m_GHost->m_Language->UnhostingGame(m_GHost->m_CurrentGame->GetDescription()));
				m_GHost->m_CurrentGame->SetExiting(true);
			}
		}
		else
			CONSOLE_Print(m_GHost->m_Language->UnableToUnhostGameNoGameInLobby());
	}

	//
	// !W
	//

	else if (Command == "w" && !Payload.empty())
	{
		// extract the name and the message
		// e.g. "Varlock hello there!" -> name: "Varlock", message: "hello there!"

		std::string Name;
		std::string Message;
		std::string::size_type MessageStart = Payload.find(" ");

		if (MessageStart != std::string::npos)
		{
			Name = Payload.substr(0, MessageStart);
			Message = Payload.substr(MessageStart + 1);

			for (std::vector<CBNET*> ::iterator i = m_GHost->m_BNETs.begin(); i != m_GHost->m_BNETs.end(); ++i)
				(*i)->QueueChatCommand(Message, Name, true);
		}
	}
}