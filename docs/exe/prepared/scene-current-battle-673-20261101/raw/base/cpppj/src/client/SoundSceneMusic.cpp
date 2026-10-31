// 장면별 음악 감독의 판단 순서·시계 읽기 횟수·x87 비교의 비교 불가 처리를 원본 기계어 순서대로 복원한다.
#include "client/SoundSceneMusic.h"
#include <bit>
#include <stdexcept>
#include <string>
#include <utility>

namespace netstorm::client {
namespace {
// 원본 _stricmp(C 로캘)처럼 ASCII A~Z만 접어 두 이름이 같은지 본다. 길이가 다르면 다르다.
// 사용: 곡 이름 비교에만 쓴다. 파일 시스템 비교가 아니다.
// 원본: 004e65ed → 004ef510 / CD 004f2980.
// 이력: 2026-10-10 추가.
bool SameName(std::string_view left,std::string_view right) {
    if (left.size()!=right.size()) return false;
    // 길이가 같은 이름의 각 바이트를 대소문자 구분 없이 비교한다.
    for (std::size_t i=0;i<left.size();++i) {
        // ASCII 대문자 한 바이트만 소문자로 접는다.
        const auto fold=[](char value) { return value>='A' && value<='Z' ? static_cast<char>(value-'A'+'a') : value; };
        if (fold(left[i])!=fold(right[i])) return false;
    }
    return true;
}
// 원본 fcomp + `test ah,1`의 "작음"이다: 비교 불가(NaN)이면 참이다. a < b 이거나 a >= b가 아닐 때 참이다.
// 사용: 시계 대 끝 시각 비교(결과 곡 잠금)에 쓴다. 일반 C 비교와 달리 NaN을 "작음"으로 본다.
// 이력: 2026-10-10 추가.
bool BelowOrUnordered(double left,double right) { return !(left>=right); }
// 색인이 0~3일 때만 표에서 꺼낸다. 원본은 범위를 검사하지 않고 표 앞뒤 메모리를 읽으므로 여기서는 진단 예외로 막는다.
// 사용: 원소 곡·팔레트·효과음·날씨 색 표를 읽을 때 쓴다.
// 이력: 2026-10-10 추가.
template<class Table> auto& At(Table& table,std::int32_t index) {
    if (index<0 || static_cast<std::size_t>(index)>=table.size()) throw std::out_of_range("장면 음악 색인이 0~3을 벗어났습니다");
    return table[static_cast<std::size_t>(index)];
}
}

// 채널 raw의 +0x18부터 여덟 바이트를 리틀 엔디언 double로 읽는다. 파일을 연 적이 없거나 닫은 채널은 0이다.
double MusicChannelDuration(const MusicChannelState& primary) {
    const auto raw=primary.Raw();
    std::uint64_t bits=0;
    // 낮은 DWORD부터 여덟 바이트를 조합한다. 채널 +0x18에 파일을 열 때 쓴 double이 있다.
    for (std::size_t i=0;i<8;++i) bits|=static_cast<std::uint64_t>(raw[static_cast<std::size_t>(MusicField::Duration)+i])<<(8*i);
    return std::bit_cast<double>(bits);
}

// 경계는 첫 호출 전에 모두 연결돼 있어야 한다.
SceneMusic::SceneMusic(SceneMusicState& state,o::GameRandom& random,SceneMusicHooks hooks):state_(state),random_(random),hooks_(std::move(hooks)) {
    if (!hooks_.wallSeconds || !hooks_.currentName || !hooks_.select || !hooks_.duration || !hooks_.sacrificing || !hooks_.waitingRoom ||
        !hooks_.refresh || !hooks_.ascendancyPalette || !hooks_.loadPalette || !hooks_.thunderFlash || !hooks_.weatherSound)
        throw std::invalid_argument("장면 음악의 필수 경계 누락");
}

// 현재 곡 이름을 먼저 비교하고, 맞으면 곧바로 시계를 읽는다. 두 번째 비교는 현재 곡이 fanfare/defeat일 때만 참이 될 수 있다.
bool SceneMusic::ResultMusicLocked() const {
    const auto current=hooks_.currentName();
    if (SameName(current,kFanfareMusic) && BelowOrUnordered(hooks_.wallSeconds(),state_.fanfareEnd)) return true;
    return SameName(hooks_.currentName(),kDefeatMusic) && BelowOrUnordered(hooks_.wallSeconds(),state_.defeatEnd);
}

// 시계 읽기 횟수가 원본과 같다: fanfare는 두 번(끝 시각, songEnd), defeat는 두 번, 그 밖은 한 번이며 곡 길이는 필요할 때마다 다시 읽는다.
void SceneMusic::Request(std::string_view name) {
    if (SameName(hooks_.currentName(),name)) return;
    if (ResultMusicLocked() && !SameName(name,kAnticipationMusic)) return;
    if (SameName(name,kFanfareMusic) && state_.resultState!=1) return;
    hooks_.select(name);
    if (SameName(name,kFanfareMusic)) {
        const double length=hooks_.duration();
        state_.fanfareEnd=length+hooks_.wallSeconds();
    } else if (SameName(name,kDefeatMusic)) {
        const double length=hooks_.duration();
        state_.defeatEnd=length+hooks_.wallSeconds();
    }
    // 30초보다 긴 곡만 그 길이를 믿는다. 길이가 0이거나 NaN이면 180초 뒤에 다시 확인한다.
    if (kLongSongSeconds<hooks_.duration()) {
        const double length=hooks_.duration();
        state_.songEnd=length+hooks_.wallSeconds();
        return;
    }
    state_.songEnd=hooks_.wallSeconds()+kShortSongSeconds;
}

// 색인은 먼저 저장하고 나서 희생 여부를 묻는다. 희생 곡을 고르면 날씨 효과는 적용하지 않는다.
void SceneMusic::Next() {
    // 32비트로 하나 올린 뒤 C의 % 4를 한다(음수는 0 이하, 오버플로는 감김).
    const auto next=static_cast<std::int32_t>(static_cast<std::uint32_t>(state_.index)+1U);
    state_.index=next%4;
    if (state_.playersReady!=0 && hooks_.sacrificing(state_.localPlayer)) { Request(kSacrificeMusic);return; }
    Request(At(kElementMusic,state_.index));
    ApplyWeather();
}

// 시계를 한 번 읽어 songEnd와 비교한다. 곡이 끝나기 전이거나 비교 불가면 아무것도 하지 않는다.
void SceneMusic::Frame() {
    if (BelowOrUnordered(hooks_.wallSeconds(),state_.songEnd)) return;
    if (state_.battle!=0) { Next();return; }
    if (hooks_.waitingRoom()) { Request(kAnticipationMusic);return; }
    Request(kMenuMusic);
}

// 전투 밖은 색인 3과 메뉴 곡, 전투 중은 난수로 고른 색인의 다음 곡이다. 그 뒤 Frame을 한 번 불러 곡 끝 시각이 이미 지났으면 곧바로 다음 곡으로 넘어간다.
void SceneMusic::Start() {
    if (state_.battle==0) {
        state_.index=kMenuMusicIndex;
        Request(kMenuMusic);
    } else {
        state_.index=static_cast<std::int32_t>(random_.Next(kFirstSongRandomLimit)/kFirstSongRandomStep);
        Next();
    }
    Frame();
    state_.tint=At(state_.tints,state_.index);
}

// 설정이 켜진 때의 색 대입은 팔레트 적재보다 먼저다. 효과음 색인은 팔레트 단계 뒤에 다시 읽는다.
void SceneMusic::ApplyWeather() {
    hooks_.refresh();
    if (hooks_.ascendancyPalette()) {
        state_.tint=At(state_.tints,state_.index);
        hooks_.loadPalette(At(kAscendancyPalettes,state_.index));
        state_.paletteDirty=1;
    }
    const std::string_view sound=At(kWeatherSounds,state_.index);
    if (sound.empty()) return;
    if (state_.index==kThunderMusicIndex) hooks_.thunderFlash();
    hooks_.weatherSound(sound);
}

// 실제 모듈에 잇는 네 경계를 채우고 나머지는 호출자의 것을 쓴다. select의 이름은 NUL 종료 문자열로 넘긴다.
SceneMusicHooks MakeSceneMusicHooks(MusicSelection& selection,const MusicSelectionState& names,const MusicChannelState& primary,
    SoundPlayer& sounds,SceneMusicHooks external) {
    external.currentName=[&names] { return names.Current(); };
    external.select=[&selection](std::string_view name) { const std::string text(name);selection.Select(text.c_str()); };
    external.duration=[&primary] { return MusicChannelDuration(primary); };
    external.weatherSound=[&sounds](std::string_view name) { static_cast<void>(sounds.PlayByName(name,0,0,0,0,0)); };
    return external;
}
}
