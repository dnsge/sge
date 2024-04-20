#pragma once

#include "game/Game.hpp"
#include "game/Scene.hpp"

namespace sge {

game::Game &CurrentGame();
game::Scene &CurrentScene();
bool GameOffline();

} // namespace sge
