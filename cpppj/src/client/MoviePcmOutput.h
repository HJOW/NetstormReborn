// 원본 Smackw32의 PCM 출력을 Windows10/11 x64의 waveOut 장치로 연결한다.
#pragma once
#include "client/SmackerDecoder.h"
#include <optional>

namespace netstorm::client { // 원래 PCM 규격/표본을 변환하지 않는 영상 전용 출력이다.
class MoviePcmOutput { // SDK와 다른64비트 출력 소유자이며 원시 PCM/표본 속도는 그대로 사용한다.
public:
    explicit MoviePcmOutput(bool enabled,bool mute=false); // noAudio는 장치를 열지 않는다. 검사 음소거는 제출 사본만 무음으로 바꾸고 시스템 음량은 유지한다.
    ~MoviePcmOutput(); // 예약된 모든 버퍼를 Reset/Unprepare한 뒤 장치를 닫는다.
    MoviePcmOutput(const MoviePcmOutput&)=delete;MoviePcmOutput& operator=(const MoviePcmOutput&)=delete; // waveOut와 WAVEHDR의 주소/수명은 복제하지 않는다.
    void Prepare(const std::array<SmackerAudioInfo,7>& formats); // 원문 SmackOpen처럼 시작 전 존재하는 트랙 장치를 준비한다. noAudio는 열지 않는다.
    bool Ready() const; // 실제 열린 트랙이 있는지 반환한다. 파일의 트랙 존재만으로 성공을 합성하지 않는다.
    std::optional<double> ElapsedSeconds() const; // 실제 장치 재생 커서의 초다. 조회/장치 부재는 빈 값이다.
    void Submit(std::uint32_t track,const SmackerAudioInfo& format,std::span<const std::uint8_t> pcm,double mediaSeconds); // 현재 청크를 복사해 비동기 출력하며 첫 청크의 영상 시각/장치 실패를 구별한다.
    void Stop(); // 소유한 일곱 트랙을 즉시 정지/반납하며 반복 호출할 수 있다.
    std::uint64_t SubmittedBytes() const; // 실제 waveOutWrite가 수락한 누적BYTE다. noAudio/장치 실패는0이며 제출 사본 음소거는 실제 수락량을 센다.
private:
    struct Impl;std::unique_ptr<Impl> impl_; // 고정 주소의 PCM 청크와 장치 핸들 소유자다.
};
}
