#pragma once
#include <string>
#include <sstream>
#include <boost/filesystem.hpp>

using namespace boost::filesystem;

#include "includes.h"
#include "ghost.h"
#include "bnet.h"
#include "config.h"
#include "game_base.h"
#include "gameprotocol.h"
#include "ghostdb.h"
#include "language.h"
#include "map.h"
#include "packed.h"
#include "replay.h"
#include "savegame.h"
#include "util.h"

#include "consolecommand.h"

void ProcessConsole(std::string Command, std::string Payload, CGHost* m_GHost);