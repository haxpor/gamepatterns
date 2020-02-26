#include <iostream>
#include <memory>
#include <functional>

class AudioPlayer {
public:
    static AudioPlayer& instance() {
        static AudioPlayer inst;
        return inst;
    }

    void playJumpSfx() {
        std::cout << "Play jump sfx\n";
    }
    void playCoinSfx() {
        std::cout << "Play coin sfx\n";
    }
private:
    AudioPlayer() {}
    ~AudioPlayer() {}

    AudioPlayer& operator=(const AudioPlayer&) = delete;
    AudioPlayer& operator=(AudioPlayer&&) = delete;
};

int main() {
    AudioPlayer::instance().playJumpSfx();
    AudioPlayer::instance().playCoinSfx();
    return 0;
}
