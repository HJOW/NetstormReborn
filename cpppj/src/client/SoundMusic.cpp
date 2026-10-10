// 음악 파일의 부분 읽기·한 번 되감기·재귀 정지와 공유 음소거를 원본 상태 변경 순서대로 실행한다.
#include "client/SoundMusic.h"
#include <algorithm>
#include <climits>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
// 원본의 읽기/쓰기 상태를 그대로 관찰할 수 있는 바이트 보기다.
std::span<std::uint8_t> MusicChannelState::Raw() { return raw_; }
// 읽기 전용 상태 보기다.
std::span<const std::uint8_t> MusicChannelState::Raw() const { return raw_; }
// 비정렬 DWORD도 호스트 alias/alignment에 의존하지 않고 읽는다.
std::uint32_t MusicChannelState::Field(MusicField field) const {
    const auto at=static_cast<std::size_t>(field);if (at+4>raw_.size()) throw std::out_of_range("음악 필드 범위 밖");
    std::uint32_t value=0;
    // 낮은 바이트부터 DWORD를 합친다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw_[at+i])<<(8*i);
    return value;
}
// 지정한 DWORD 이외의 필드는 보존한다.
void MusicChannelState::SetField(MusicField field,std::uint32_t value) {
    const auto at=static_cast<std::size_t>(field);if (at+4>raw_.size()) throw std::out_of_range("음악 필드 범위 밖");
    // 낮은 바이트부터 순서대로 쓴다.
    for (std::size_t i=0;i<4;++i) raw_[at+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 필수 장치/파일 경계를 검사하고 OS 자원 없이 보관한다.
MusicChannel::MusicChannel(o::OriginalEdition edition,MusicChannelState& state,MusicChannelHooks hooks)
    :edition_(edition),state_(state),hooks_(std::move(hooks)) {
    if (!hooks_.setVolume || !hooks_.stop || !hooks_.release || !hooks_.seek || !hooks_.read || !hooks_.close)
        throw std::invalid_argument("음악 채널의 필수 경계 누락");
}
// 재귀 잠금은 실제 mutex가 수행하고 선택 관찰자는 원본 호출 순서 검증에 쓴다.
MusicChannel::Guard::Guard(MusicChannel& owner):channel(owner) {
    channel.mutex_.lock();if (channel.hooks_.lockEvent) channel.hooks_.lockEvent(true);
}
// 정상 반환과 예외 모두에서 획득한 잠금 한 번을 해제한다.
MusicChannel::Guard::~Guard() { if (channel.hooks_.lockEvent) channel.hooks_.lockEvent(false);channel.mutex_.unlock(); }
// 문장의 줄바꿈도 원본 그대로 전달한다.
void MusicChannel::Log(std::string_view text) { if (hooks_.log) hooks_.log(9,text); }
// 원본 256바이트 조회 버퍼의 null/빈 경로·이름 재붙이기를 존재 검사와 열기에서 공통으로 사용한다.
std::optional<std::string> FindMusicFile(std::string_view name,const MusicDirectories& directories,
    const std::function<std::optional<std::string>(std::string_view)>& find) {
    if (!find) throw std::invalid_argument("음악 파일 조회 경계 누락");
    std::string query;
    // 원본은 디렉터리가 null일 때 이전 검색 문자열을 남긴다. 빈 문자열은 역슬래시부터 시작하는 별개 경로다.
    const auto append=[&](const std::optional<std::string>& directory) {
        if (directory) query=*directory+"\\";
        query+=name;
        if (query.size()>255 || query.find('\0')!=std::string::npos) throw std::length_error("음악 경로가 원본 버퍼 범위를 벗어났습니다");
    };
    append(directories.primary);auto found=find(query);
    if (!found) { append(directories.secondary);found=find(query); }
    return found;
}
// 조회/헤더는 채널 잠금 안에서 처리하고 실패한 헤더의 부분 출력도 원본처럼 보존한다.
bool MusicChannel::Open(std::string_view name,const MusicDirectories& directories,const MusicOpenHooks& hooks) {
    if (!hooks.find || !hooks.open) throw std::invalid_argument("음악 파일 열기 경계 누락");
    Guard guard(*this);auto found=FindMusicFile(name,directories,hooks.find);
    if (!found) {
        if (hooks_.log) hooks_.log(0x13,"Missing music: \"(null)\"\n");
        return false;
    }
    MusicFileHeader header;header.file=state_.Field(MusicField::File);header.length=state_.Field(MusicField::Length);
    header.dataOffset=state_.Field(MusicField::DataOffset);auto raw=state_.Raw();std::copy_n(raw.begin()+0x28,18,header.format.begin());
    const bool opened=hooks.open(*found,header);
    state_.SetField(MusicField::File,header.file);state_.SetField(MusicField::Length,header.length);state_.SetField(MusicField::DataOffset,header.dataOffset);
    std::copy(header.format.begin(),header.format.end(),raw.begin()+0x28);
    if (!opened) { Log("Failed to open music file: \""+*found+"\"\n");return false; }
    // Ghidra의 float 캐스트와 달리 실제 x87에는 중간 float32 저장이 없다. C++는 53비트 연산으로 판본별 순서를 유지한다.
    const auto word=[&](std::size_t at) { return static_cast<std::uint32_t>(header.format[at])|(static_cast<std::uint32_t>(header.format[at+1])<<8); }; // format WORD를 읽는다.
    std::uint32_t rate{};std::memcpy(&rate,header.format.data()+4,4);const auto channels=word(2),bits=word(14);
    if (header.length>INT_MAX || !rate || !channels || !bits) throw std::out_of_range("음악 길이 계산 인자가 올바르지 않습니다");
    double duration;
    if (edition_==o::OriginalEdition::Patch1078) {
        duration=static_cast<double>(header.length)/(static_cast<double>(bits)*0.125);
        duration=duration/static_cast<double>(rate);duration=duration/static_cast<double>(channels);
    } else {
        double denominator=(static_cast<double>(bits)*0.125)*static_cast<double>(channels);
        denominator=denominator*static_cast<double>(rate);duration=static_cast<double>(header.length)/denominator;
    }
    std::memcpy(raw.data()+0x18,&duration,sizeof(duration));return true;
}
// 활성 여부는 원본과 같이 flags의 최하위 비트만 사용한다.
bool MusicChannel::Active() const { return (state_.Field(MusicField::Flags)&1U)!=0; }
// 버퍼가 없어도 잠금은 수행한다. 음량은 자르지 않고 실패만 기록한다.
void MusicChannel::SetVolume(std::int32_t volume) {
    Guard guard(*this);const auto buffer=state_.Field(MusicField::Buffer);
    if (buffer!=0 && hooks_.setVolume(buffer,volume)<0) Log("Failed to set music buffer volume.\n");
}
// 되감기 실패는 잠금을 푼 뒤 오류를 기록한다(중첩 호출이면 외부 잠금은 유지된다).
bool MusicChannel::Rewind() {
    bool success;
    {
        Guard guard(*this);success=hooks_.seek(state_.Field(MusicField::File),state_.Field(MusicField::DataOffset))!=-1;
        if (success) state_.SetField(MusicField::ReadOffset,0);else hooks_.close(state_.Field(MusicField::File));
    }
    if (!success) Log("Failed to rewind music file.\n");
    return success;
}
// 부분 읽기의 출력은 유지하며 실패한 파일 토큰도 상태에 남긴다. 원본 read 도우미와 같다.
bool MusicChannel::ReadExact(std::span<std::uint8_t> target) {
    if (hooks_.read(state_.Field(MusicField::File),target)==static_cast<std::int32_t>(target.size())) return true;
    hooks_.close(state_.Field(MusicField::File));return false;
}
// 원본처럼 파일 끝에서 한 번만 되감는다. 여러 번 반복해서 채우거나 0을 unsigned PCM 무음으로 바꾸지 않는다.
std::int32_t MusicChannel::Read(std::span<std::uint8_t> target) {
    Guard guard(*this);
    if (target.empty()) return 0;
    const auto length=state_.Field(MusicField::Length),offset=state_.Field(MusicField::ReadOffset);
    if (target.size()>INT_MAX || length>INT_MAX || offset>length) throw std::out_of_range("음악 읽기 길이/위치 범위 밖");
    const auto remaining=length-offset;
    if (target.size()<remaining) {
        if (!ReadExact(target)) return 0;
        state_.SetField(MusicField::ReadOffset,offset+static_cast<std::uint32_t>(target.size()));
        return static_cast<std::int32_t>(target.size());
    }
    if (remaining!=0) {
        if (!ReadExact(target.first(remaining))) return 0;
        state_.SetField(MusicField::ReadOffset,offset+remaining);
    }
    if ((state_.Field(MusicField::Flags)&2U)==0) {
        std::fill(target.begin()+remaining,target.end(),std::uint8_t{});return static_cast<std::int32_t>(remaining);
    }
    static_cast<void>(Rewind()); // 원본은 실패 반환을 검사하지 않고 다음 읽기 또는 현재 길이 반환으로 진행한다.
    if (target.size()==remaining) return static_cast<std::int32_t>(remaining);
    const auto rest=target.subspan(remaining);
    if (!ReadExact(rest)) return 0;
    state_.SetField(MusicField::ReadOffset,state_.Field(MusicField::ReadOffset)+static_cast<std::uint32_t>(rest.size()));
    return static_cast<std::int32_t>(target.size());
}
// 파일/버퍼 계약 진단은 잠금 전에 보고하고, 되감기 성공 후에만 활성/반복 플래그를 기록한다.
bool MusicChannel::Start(std::uint32_t loop) {
    if (hooks_.report) {
        const bool patch=edition_==o::OriginalEdition::Patch1078;
        if (state_.Field(MusicField::Length)==0) hooks_.report("length != 0",patch ? 0x509 : 0x504);
        if (state_.Field(MusicField::Buffer)==0) hooks_.report("buffer != NULL",patch ? 0x50a : 0x505);
    }
    Guard guard(*this);if (!Rewind()) return false;
    state_.SetField(MusicField::Flags,loop!=0 ? 3U : 1U);state_.SetField(MusicField::WriteOffset,0);return true;
}
// 채널의 읽기 정보만 지우며 +0x3a 패딩 WORD와 버퍼/flags/쓰기 위치를 보존한다.
void MusicChannel::Close() {
    Guard guard(*this);hooks_.close(state_.Field(MusicField::File));state_.SetField(MusicField::File,0);
    state_.SetField(MusicField::Length,0);state_.SetField(MusicField::ReadOffset,0);state_.SetField(MusicField::DataOffset,0);
    auto raw=state_.Raw();std::fill(raw.begin()+0x18,raw.begin()+0x20,std::uint8_t{});
    std::fill(raw.begin()+0x28,raw.begin()+0x3a,std::uint8_t{});
}
// 원본의 중첩 잠금 구간과 버퍼/파일 분리 정리를 유지한다. 깊이는 DWORD로 넘치도록 증가한다.
void MusicChannel::Stop() {
    Guard outer(*this);
    {
        Guard inner(*this);
        if (state_.Field(MusicField::StopDepth)==0 && state_.Field(MusicField::Buffer)!=0) hooks_.stop(state_.Field(MusicField::Buffer));
        state_.SetField(MusicField::StopDepth,state_.Field(MusicField::StopDepth)+1U);
        state_.SetField(MusicField::Flags,state_.Field(MusicField::Flags)&~1U);
    }
    {
        Guard release(*this);const auto buffer=state_.Field(MusicField::Buffer);
        if (buffer!=0 && hooks_.release(buffer)<0) Log("Failed to release the sound buffer.\n");
        state_.SetField(MusicField::Buffer,0);state_.SetField(MusicField::BufferBytes,0);
    }
    Close();
    state_.SetField(MusicField::StopDepth,0);
}
// 생성 실패 기록은 원본처럼 잠금을 푼 뒤 남긴다. COM이 쓴 출력 토큰은 HRESULT와 무관하게 보존한다.
bool MusicChannel::EnsureBuffer(const MusicBufferHooks& hooks) {
    {
        Guard guard(*this);
        if (state_.Field(MusicField::Buffer)!=0) return true;
        if (!hooks.create) throw std::invalid_argument("음악 버퍼 생성 경계 누락");
        auto buffer=state_.Field(MusicField::Buffer);
        const auto result=hooks.create(state_.Raw().subspan(0x28,18),kMusicBufferFlags,kMusicBufferBytes,buffer);
        state_.SetField(MusicField::Buffer,buffer);
        if (result>=0) { state_.SetField(MusicField::BufferBytes,kMusicBufferBytes);return true; }
    }
    Log("Failed to create a music buffer.\n");return false;
}
// 원본은 status의 재생/반복 비트(5)로 첫 채우기와 부분 갱신을 구별하고 장치 쓰기 커서는 사용하지 않는다.
bool MusicChannel::FillBuffer(const MusicBufferHooks& hooks,const SoundState& sound) {
    Guard guard(*this);
    if ((state_.Field(MusicField::Flags)&1U)==0) return false;
    if (!hooks.status || !hooks.restore || !hooks.cursor || !hooks.lock || !hooks.unlock || !hooks.play)
        throw std::invalid_argument("음악 버퍼 갱신 경계 누락");
    const auto buffer=state_.Field(MusicField::Buffer);
    if (buffer==0) throw std::invalid_argument("활성 음악 버퍼 누락");
    // 장치 오류를 기록한 뒤 중첩 정리하고 반환한다. 표본 읽기 실패에는 별도 기록 문장이 없다.
    const auto fail=[&](std::string_view text) { if (!text.empty()) Log(text);Stop();return false; };
    std::uint32_t status{};
    if (hooks.status(buffer,status)<0) return fail("Failed to get music buffer status.\n");
    if ((status&2U)!=0 && hooks.restore(buffer)<0) return fail("Failed to restore music buffer.\n");
    if (hooks_.setVolume(buffer,sound.musicVolume)<0) Log("Failed to set music buffer volume in fillBuffer().\n");
    const auto bytes=state_.Field(MusicField::BufferBytes);
    std::uint32_t count{};
    if ((status&5U)==0) { state_.SetField(MusicField::WriteOffset,0);count=bytes; }
    else {
        std::uint32_t play{},write{};
        if (hooks.cursor(buffer,play,write)<0) return fail("Failed to get current music buffer position.\n");
        const auto offset=state_.Field(MusicField::WriteOffset);
        if (play>=bytes || offset>=bytes) throw std::out_of_range("음악 버퍼 커서 범위 밖");
        count=play<offset ? bytes-offset+play : play-offset;
    }
    if (count!=0) {
        if (bytes>INT_MAX) throw std::out_of_range("음악 버퍼 크기 범위 밖");
        MusicBufferRegions regions;
        if (hooks.lock(buffer,state_.Field(MusicField::WriteOffset),count,regions)<0) return fail("Failed to lock music buffer.\n");
        bool shortRead=false;
        try {
            if (regions.first.size()>count || regions.second.size()!=count-regions.first.size())
                throw std::out_of_range("음악 버퍼 잠금 구간 크기 불일치");
            const auto first=Read(regions.first);
            if (first==static_cast<std::int32_t>(regions.first.size())) {
                state_.SetField(MusicField::WriteOffset,state_.Field(MusicField::WriteOffset)+static_cast<std::uint32_t>(first));
                if (regions.first.size()!=count) {
                    const auto second=Read(regions.second);
                    if (second!=static_cast<std::int32_t>(regions.second.size())) shortRead=true;
                    else state_.SetField(MusicField::WriteOffset,state_.Field(MusicField::WriteOffset)+static_cast<std::uint32_t>(second));
                }
            } else shortRead=true;
        } catch (...) {
            hooks.unlock(buffer,regions);throw; // 원본 밖의 호스트 IO 예외도 실제 DirectSound 잠금을 남기지 않는다.
        }
        state_.SetField(MusicField::WriteOffset,state_.Field(MusicField::WriteOffset)%bytes);
        if (hooks.unlock(buffer,regions)<0) return fail("Failed to unlock music buffer.\n");
        if (shortRead) return fail({});
    }
    if ((status&5U)==0 && hooks.play(buffer,1)<0) return fail("Failed to play music buffer.\n");
    return true;
}
// 같은 공유 전역과 두 재생 계층을 참조한다.
SoundMusic::SoundMusic(o::OriginalEdition edition,SoundState& state,SoundPlayer& effects,MusicChannel& channel)
    :edition_(edition),state_(state),effects_(effects),channel_(channel) {}
// 열기 실패 뒤의 fallback은 상위 옵션 관리자 경계다. 생성/되감기 실패에는 곡을 바꾸지 않는다.
bool SoundMusic::Play(const char* name,std::uint32_t loop,const MusicDirectories& directories,const MusicOpenHooks& files,const MusicBufferHooks& buffers) {
    if (!state_.musicInitialized || !state_.device || channel_.Active() || !name) return false;
    if (!channel_.Open(name,directories,files)) {
        std::string folded(name);
        // 원본 demo.mus 비교에 필요한 ASCII 대소문자만 접는다. 음악 파일 이름 자체는 변경하지 않는다.
        for (auto& ch:folded) if (ch>='A' && ch<='Z') ch=static_cast<char>(ch-'A'+'a');
        if (folded=="demo.mus") return false;
        if (files.fallback) files.fallback("demo.mus");
        return channel_.Active();
    }
    return channel_.EnsureBuffer(buffers) && channel_.Start(loop);
}
// 음악 준비 전에도 현재/예약 전역은 갱신하며 음량은 자르지 않는다.
void SoundMusic::SetVolume(std::int32_t volume) {
    if (state_.volumeHoldDepth!=0) { state_.pendingMusicVolume=volume;return; }
    state_.musicVolume=volume;if (state_.musicInitialized) channel_.SetVolume(volume);
}
// 효과음 적용 중 재진입해 깊이가 바뀌면 음악 변경은 현재 보류 상태를 다시 따라간다.
void SoundMusic::PushMute() {
    if (state_.volumeHoldDepth==0) {
        state_.pendingMasterVolume=state_.masterVolume;state_.pendingMusicVolume=state_.musicVolume;
        effects_.SetMasterVolume(kSoundMinimum);SetVolume(kSoundMinimum);
    }
    state_.volumeHoldDepth=static_cast<std::int32_t>(static_cast<std::uint32_t>(state_.volumeHoldDepth)+1U);
}
// 양수만 감소시킨다. 효과음 복원 중 깊이가 바뀌면 원본처럼 음악 즉시 복원을 건너뛴다.
void SoundMusic::PopMute() {
    if (state_.volumeHoldDepth<=0) return;
    --state_.volumeHoldDepth;
    if (state_.volumeHoldDepth==0) {
        effects_.SetMasterVolume(state_.pendingMasterVolume);
        if (edition_!=o::OriginalEdition::Patch1078 || state_.volumeHoldDepth==0) SetVolume(state_.pendingMusicVolume);
    }
}
// 준비된 음악의 기본 채널을 정지한다. 두 판본 모두 WAVEFORMATEX 뒤의 패딩 WORD는 보존한다.
void SoundMusic::Stop() {
    if (!state_.musicInitialized) return;
    channel_.Stop();
}
// 초기화된 음악만 갱신한다. 실제 작업 스레드/이벤트 수명은 후속 계층이 맡는다.
bool SoundMusic::Update(const MusicBufferHooks& hooks) { return state_.musicInitialized && channel_.FillBuffer(hooks,state_); }
}
