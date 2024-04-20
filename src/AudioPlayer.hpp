#pragma once

#include <SDL2/SDL_mixer.h>

#include <string_view>

namespace sge {

void PlayAudio(int channel, std::string_view name, bool loop = false);
void PlayAudio(int channel, Mix_Chunk* chunk, bool loop = false);
void StopAudio(int channel);

} // namespace sge
