// 원본32비트 SDK 대신 Windows10/11 x64에서 같은 PCM을 재생하기 위한 장치 경계다.
#define NOMINMAX
#include "client/MoviePcmOutput.h"
#include <windows.h>
#include <mmsystem.h>
#include <algorithm>
#include <deque>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace netstorm::client { // 게임 효과음/음악과 별도인 Smacker 일곱 트랙의 수명을 소유한다.
struct MoviePcmOutput::Impl { // 완료되지 않은 WAVEHDR와 PCM은 OS가 반납할 때까지 같은 주소를 유지한다.
    struct Block { std::vector<std::uint8_t> bytes;WAVEHDR header{}; }; // 한 비동기 청크의 실제 소유 메모리다.
    struct Track { HWAVEOUT device{};bool failed{},submitted{};double mediaOrigin{};std::uint32_t rate{},bytesPerSecond{};std::deque<std::unique_ptr<Block>> blocks; }; // 한 원래 트랙의 장치/제출 여부/첫 청크 영상 시각/원래 표본 속도/현재 제출 순서다.
    bool enabled{},mute{};std::uint64_t submitted{};std::array<Track,7> tracks; // 검사 옵션과 성공한 실제 누적BYTE다.
    explicit Impl(bool value,bool silent):enabled(value),mute(silent) {} // 생성만으로 장치를 열지 않는다.
    void Open(Track& track,const SmackerAudioInfo& format) { // 원래 PCM 규격으로 공유 출력 장치를 열며 실패한 트랙은 반복 열지 않는다.
        if (!enabled || !format.exists || track.device || track.failed) return;
        WAVEFORMATEX wave{};wave.wFormatTag=WAVE_FORMAT_PCM;wave.nChannels=format.channels;wave.nSamplesPerSec=format.sampleRate;wave.wBitsPerSample=format.bitsPerSample;wave.nBlockAlign=static_cast<WORD>(wave.nChannels*wave.wBitsPerSample/8);wave.nAvgBytesPerSec=wave.nSamplesPerSec*wave.nBlockAlign;
        const auto result=waveOutOpen(&track.device,WAVE_MAPPER,&wave,0,0,CALLBACK_NULL);
        if (result!=MMSYSERR_NOERROR) { track.device=nullptr;track.failed=true;std::clog<<"영상 PCM 장치 열기 실패 "<<result<<'\n';return; }
        track.rate=wave.nSamplesPerSec;track.bytesPerSecond=wave.nAvgBytesPerSec;
    }
    void Reap(Track& track) { // 재생 완료한 앞 청크만 실제 장치에서 해제한다.
        while (!track.blocks.empty() && (track.blocks.front()->header.dwFlags&WHDR_DONE)) { auto& block=*track.blocks.front();if (waveOutUnprepareHeader(track.device,&block.header,sizeof(WAVEHDR))!=MMSYSERR_NOERROR) break;track.blocks.pop_front(); }
    }
    void Close(Track& track) { // Reset이 모든 예약을 반환한 뒤에만 WAVEHDR/PCM을 반납한다.
        if (!track.device) return;waveOutReset(track.device);
        // 아직 준비 상태인 각 소유 청크를 같은 핸들로 반납한다.
        for (auto& block:track.blocks) if (block->header.dwFlags&WHDR_PREPARED) waveOutUnprepareHeader(track.device,&block->header,sizeof(WAVEHDR));
        track.blocks.clear();waveOutClose(track.device);track.device=nullptr;
    }
};
MoviePcmOutput::MoviePcmOutput(bool enabled,bool mute):impl_(std::make_unique<Impl>(enabled,mute)) {} // 옵션은 출력 수명 동안 고정한다.
MoviePcmOutput::~MoviePcmOutput() { Stop(); } // OS가 참조하는 청크를 먼저 반납한다.
void MoviePcmOutput::Prepare(const std::array<SmackerAudioInfo,7>& formats) { // 파일 열기 단계에서 실제 트랙만 준비한다.
    for (std::size_t index=0;index<formats.size();++index) impl_->Open(impl_->tracks[index],formats[index]); // 존재하는 원래 일곱 트랙을 순서대로 연다.
}
bool MoviePcmOutput::Ready() const { // 적어도 하나의 실제 출력 핸들이 있어야 오디오 시계 가지를 선택한다.
    for (const auto& track:impl_->tracks) if (track.device && !track.failed) return true;return false; // 실패한 핸들을 실제 시계로 사용하지 않는다.
}
std::optional<double> MoviePcmOutput::ElapsedSeconds() const { // 파일 예상 시간이 아니라 실제 장치가 소비한 표본 위치를 읽는다.
    for (const auto& track:impl_->tracks) if (track.device && !track.failed && track.submitted) { // 아직 청크가 없거나 실패한 트랙은 시계로 삼지 않는다.
        MMTIME position{};position.wType=TIME_SAMPLES;if (waveOutGetPosition(track.device,&position,sizeof(position))!=MMSYSERR_NOERROR) continue;
        if (position.wType==TIME_SAMPLES && track.rate) return track.mediaOrigin+static_cast<double>(position.u.sample)/track.rate;
        if (position.wType==TIME_BYTES && track.bytesPerSecond) return track.mediaOrigin+static_cast<double>(position.u.cb)/track.bytesPerSecond;
        if (position.wType==TIME_MS) return track.mediaOrigin+static_cast<double>(position.u.ms)/1000.0;
    }return {};
}
void MoviePcmOutput::Submit(std::uint32_t index,const SmackerAudioInfo& format,std::span<const std::uint8_t> pcm,double mediaSeconds) { // 첫 성공 제출의 영상 시각을 장치 커서0과 대응시킨다.
    auto& s=*impl_;if (index>=s.tracks.size()) throw std::out_of_range("영상 PCM 트랙 범위");if (!s.enabled || pcm.empty() || !format.exists) return;auto& track=s.tracks[index];if (track.failed) return;
    s.Open(track,format);if (!track.device) return;
    s.Reap(track);if (pcm.size()>(std::numeric_limits<DWORD>::max)()) throw std::length_error("영상 PCM 청크 범위");auto block=std::make_unique<Impl::Block>();block->bytes.assign(pcm.begin(),pcm.end());block->header.lpData=reinterpret_cast<LPSTR>(block->bytes.data());block->header.dwBufferLength=static_cast<DWORD>(block->bytes.size());
    if (s.mute) std::fill(block->bytes.begin(),block->bytes.end(),static_cast<std::uint8_t>(format.bitsPerSample==8 ? 128 : 0)); // --mute-audio 검사는 제출 사본만 무음으로 바꾼다. 시스템/장치 전역 음량은 바꾸지 않는다.
    auto result=waveOutPrepareHeader(track.device,&block->header,sizeof(WAVEHDR));if (result!=MMSYSERR_NOERROR) { track.failed=true;std::clog<<"영상 PCM 버퍼 준비 실패 "<<result<<'\n';return; }
    result=waveOutWrite(track.device,&block->header,sizeof(WAVEHDR));if (result!=MMSYSERR_NOERROR) { waveOutUnprepareHeader(track.device,&block->header,sizeof(WAVEHDR));track.failed=true;std::clog<<"영상 PCM 제출 실패 "<<result<<'\n';return; }
    if (!track.submitted) { track.mediaOrigin=mediaSeconds;track.submitted=true; }s.submitted+=pcm.size();track.blocks.push_back(std::move(block));
}
void MoviePcmOutput::Stop() { for (auto& track:impl_->tracks) impl_->Close(track); } // 각 트랙의 원래 속도를 유지한 채 모든 장치를 닫는다.
std::uint64_t MoviePcmOutput::SubmittedBytes() const { return impl_->submitted; } // 실제 장치 수락량만 반환한다.
}
