// 일반 전투의!state.dat writer를 현재 Player/전역과 실제 File 장치에 연결한다.
#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::o { // 파일 배치와 순서는10.78을 따르며 호스트 FILE ABI를 직렬화하지 않는다.
struct SavedStateWriteGlobals { // 같은 Client/Session의 현재 값을 선택 위치마다 다시 조회한다.
    std::function<bool()> missionPost,playerKnown,checking; // 원문 바깥 관문과 두 checking 위치다.
    std::function<std::uint32_t()> localOwner,mySubscriber; //00540c70/74의 현재 실제 값이다.
    std::function<std::span<const std::uint8_t>()> handle,players; // 현재 이름NUL/9×180BYTE를 빌린다.
    std::uint32_t& restoreOwner;std::uint32_t& restoreSubscriber; //00542528/2c의 실제 출력 물리다.
    std::span<std::uint8_t> previousHandle; //005c84c0의 현재 NUL 포함 이름 복사 대상이다.
    std::array<std::function<std::uint32_t()>,3> tail; //00595090/0055a420/0055b4ac를 각 write 직전에 읽는다.
    const double& loginEnd; // 마지막 DWORD는 현재005c85f0!=0이며 예약 시각 자체를 쓰지 않는다.
};
struct SavedStateWriteHooks { // 파일 장치와 실제 SP 진단/시간/현재 Fort 이름의 경계다.
    std::function<std::span<const std::uint8_t>()> currentFort; //0055b490의 같은256BYTE 물리이며 checking/복사 시 재조회한다.
    std::function<void()> diagnose; // 원문의 SP 추적 비교/잔여횟수/배열 진단 접두를 현재 실제 저장소에 실행한다.
    std::function<void(std::uint32_t,std::uint32_t)> open; // 실제004b5470 이름을 현재 위치에서 작성하고41b4c0(mode2,create1)으로 연다.
    std::function<void(std::span<const std::uint8_t>,std::int32_t)> write; //41a910의 같은 File에 현재 물리/offset-1을 동기 기록한다.
    std::function<void()> close; // 정상/선택 예외에서 같은 File의41b410을 한 번 실행한다.
    std::function<double()> wallSeconds; //46 0d70의 현재 벽시각이며 프레임 게임시각이 아니다.
    std::function<float()> localSp; // 현재 PlayerKnown/owner를 다시 읽는40ee90이며 미known은0.0이다.
    std::function<void(std::string_view,std::uint32_t)> assertion; //State.cpp의f79/fa3 진단 뒤 계속 실행한다.
};
//004b54e0: post 또는 미known이면 장치 없이 반환하며 부분 쓰기를 전체 트랜잭션으로 되돌리지 않는다.
void WriteSavedState(const SavedStateWriteGlobals& globals,const SavedStateWriteHooks& hooks);
}
