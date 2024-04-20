#include "AudioPlayer.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

#include "gea/AudioHelper.h"
#include "resources/Resources.hpp"

#include <string_view>

namespace sge {

void PlayAudio(int channel, std::string_view name, bool loop) {
    if (name.empty()) {
        return;
    }
    auto* chunk = resources::GetAudio(name);
    PlayAudio(channel, chunk, loop);
}

void PlayAudio(int channel, Mix_Chunk* chunk, bool loop) {
    AudioHelper::Mix_PlayChannel498(channel, chunk, loop ? -1 : 0);
}

void StopAudio(int channel) {
    AudioHelper::Mix_HaltChannel498(channel);
}

} // namespace sge
