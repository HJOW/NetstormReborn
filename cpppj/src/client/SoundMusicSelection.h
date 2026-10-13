// 원본 상위 음악 옵션/현재 곡 이름과 곡 전환·기본 곡 fallback을 기존 공개 Play/Stop에 연결한다.
#pragma once
#include "client/SoundMusic.h"

namespace netstorm::client {
// 원본 현재 곡 이름의 256바이트 저장 공간이다. strncpy는 첫 255바이트만 쓰고 마지막 바이트를 보존한다.
class MusicSelectionState {
public:
    // 원본 음악 옵션의 signed DWORD다. 0만 꺼짐이며 래퍼 Select가 이 값을 사용한다.
    std::int32_t enabled=1;
    // 현재 이름의 전체 저장 공간을 관찰/초기 배치한다. 정상 이름은 NUL로 끝나야 한다.
    std::span<std::uint8_t> Raw();
    // 쓰지 않는 마지막 바이트까지 확인하는 읽기 전용 보기다.
    std::span<const std::uint8_t> Raw() const;
    // 첫 NUL까지 현재 이름을 읽는다. 종료 없는 호스트 상태는 범위 예외다.
    std::string_view Current() const;
    // 원본 strncpy(...,255)처럼 복사/0 패딩하되 마지막 바이트는 남긴다. name은 저장 공간 밖의 문자열이어야 한다.
    void Assign(std::string_view name);
private:
    // 원본 전역과 같이 처음에는 빈 이름이다.
    std::array<std::uint8_t,256> name_{};
};
// 원본 00435200→00435160 / CD 00484aa0→00484a10의 음악 선택 계층이다.
// 사용: 참조 상태/음악·파일/버퍼 콜백 대상이 이 객체보다 오래 살아야 한다. Select/옵션/현재 이름은 주 스레드에서 관리하고 worker는 SoundMusic 명령 잠금으로 분리한다.
class MusicSelection {
public:
    // 파일/버퍼 경계를 보관하고 열기 실패의 fallback을 이 객체의 옵션 래퍼로 연결한다. OS 자원은 열지 않는다.
    MusicSelection(MusicSelectionState& state,SoundMusic& music,MusicDirectories directories,MusicOpenHooks files,MusicBufferHooks buffers);
    // 저장한 fallback 콜백의 this 주소를 유지하므로 복제/이동하지 않는다.
    MusicSelection(const MusicSelection&)=delete;
    MusicSelection& operator=(const MusicSelection&)=delete;
    // 전역 음악 옵션으로 선택한다. 원본 공개 래퍼와 demo 재선택이 사용하는 경로다.
    void Select(const char* name);
    // 이름 변경 시 새 파일 존재를 확인한 뒤 이전 곡을 정지/이름 복사한다. 옵션이 켜져 있으면 선택 곡을 시작한다.
    // fanfare.mus/defeat.mus만 한 번 재생한다. 꺼짐은 Stop이며 결과 이름은 먼저 저장된다. null은 호스트 진단 예외다.
    void Select(const char* name,std::int32_t enabled);
private:
    // 외부 상태·공개 음악과 경계를 같은 수명 안에 보관한다. 상위 fallback은 files_에서 재귀 Select로 연결된다.
    MusicSelectionState& state_;/* music_: 효과음과 음악의 공통 음소거 제어다. 최초 PushMute에서 음량을 예약하고 마지막 PopMute에서 최신 예약값을 복원한다 (SoundMusic). */ SoundMusic& music_;/* directories_: 게임 자산을 검색할 디렉터리 목록. */ MusicDirectories directories_;/* files_: 경로 탐색·열린 파일 헤더와 상위 옵션의 demo.mus 선택 경계다. 파일 탐색에는 효과음의 언어/와일드카드 규칙을 적용하지 않는다 (MusicOpenHooks). */ MusicOpenHooks files_;/* buffers_: 음악 버퍼의 생성/갱신 COM 경계다. HRESULT는 음수일 때 실패하며 status/cursor/lock 출력은 성공 시 유효하다 (MusicBufferHooks). */ MusicBufferHooks buffers_;
};
}
