// 음악 WAVE를 열린 상태로 보관해 채널의 부분 읽기/되감기/닫기에 연결한다. 오디오 장치는 열지 않는다.
#pragma once
#include "client/SoundDevice.h"
#include <memory>

namespace netstorm::client {
// Winmm 파일 핸들은 호스트 객체가 소유하고 raw 채널에는 재사용하지 않는 32비트 토큰만 저장한다.
// 사용: 이 객체를 채널보다 오래 보관한다. Runtime의 worker 종료/합류 후 장치와 파일 소유자를 종료한다. 파일 표/각 IO는 재귀 잠금으로 보호한다.
class MusicFileStore {
public:
    // 진단 경계만 보관한다. 파일/장치는 확보하지 않는다.
    explicit MusicFileStore(SoundAssertReport report={});
    // 아직 열린 모든 음악 파일을 닫는다. 이미 닫힌 토큰의 재닫기는 안전하게 무시한다.
    ~MusicFileStore();
    // 소유권/토큰의 복제와 이동을 금지해 이미 만든 콜백의 대상을 유지한다.
    MusicFileStore(const MusicFileStore&)=delete;
    MusicFileStore& operator=(const MusicFileStore&)=delete;
    // 기존 입출력으로 실제 헤더를 읽는다. 실패 시 토큰/format의 부분 변경도 남기고 파일은 닫는다.
    bool Open(const std::filesystem::path& path,MusicFileHeader& header);
    // 실제 절대 위치로 이동한다. 닫힌 토큰/범위 밖 위치는 -1이다.
    std::int32_t Seek(std::uint32_t token,std::uint32_t absolute);
    // 현재 위치에서 요청 span을 읽고 실제 길이 또는 -1을 돌려준다. 표본 전체를 적재하지 않는다.
    std::int32_t Read(std::uint32_t token,std::span<std::uint8_t> target);
    // 해당 토큰을 반납한다. 0/이미 닫힌 토큰은 다른 열린 파일에 영향을 주지 않는다.
    void Close(std::uint32_t token);
    // 현재 파일 위치 또는 닫힌 토큰의 -1을 조회한다. 실제 파일 연결 검사에도 사용한다.
    std::int32_t Position(std::uint32_t token) const;
    // 기존 버퍼/기록 경계를 보존하면서 파일 seek/read/close를 이 소유자에 연결한다.
    MusicChannelHooks FileHooks(MusicChannelHooks hooks={});
    // resolver는 구성된 경로 자체를 조회한다. UTF-8 경로를 Windows 유니코드 파일로 열고 상위 fallback을 보관한다.
    MusicOpenHooks OpenHooks(SoundFileResolver resolver,std::function<void(std::string_view)> fallback={});
private:
    // Winmm 핸들 표와 다음 토큰/진단을 cpp 구현에 숨긴다.
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
