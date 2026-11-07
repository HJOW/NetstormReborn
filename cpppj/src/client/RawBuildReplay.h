// Recorder.cpp의 200개 이벤트 판독·프레임 실행·라벨 조회를 같은 실제 현재 상태에 적용한다.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::client { // 파일/Kernel/화면의 실제 부모는 이 원문 몸체에 선택된 장치로 연결한다.
inline constexpr std::size_t kBuildReplayEventLimit=200; //4955b0의 NUM_EVENTS이며 진단 뒤에도 호스트 배열 범위는 넘지 않는다.
// 원문 각 이벤트의 +20 이름/+34·38 좌표/+3c canon/+40 회전/+44 지연에 해당하는40BYTE다.
struct BuildReplayEvent {
    std::array<char,20> name{}; // NUL 포함 원문20BYTE 이름이며 RL 라벨도 같은 영역을 쓴다.
    float x{},y{}; // 두 단정도 좌표다. 라벨 판정은 문자열 접두 대신40e800의 현재 보드 범위를 쓴다.
    std::int32_t argument{},rotation{}; // 읽기는 signed DWORD이며 송신은 각각 lowWORD/lowBYTE만 사용한다.
    float delay{}; // 이전 소비 시각과의 차이에 strict greater를 적용할 단정도 기한이다.
};
static_assert(sizeof(BuildReplayEvent)==40 && offsetof(BuildReplayEvent,x)==20 && offsetof(BuildReplayEvent,delay)==36); // 호스트 패딩이 원문 이벤트 행 배치를 바꾸지 않게 한다.
// 원문 이벤트 배열과 +1f68 시각/+1f70 위치/+1f74 개수다. 미판독 tail은 호출자가 제공한 물리를 보존한다.
struct BuildReplayState {
    std::array<BuildReplayEvent,kBuildReplayEventLimit> events{}; // 실제 판독한 필드만 loader가 바꾸며 매번 전체 배열을 지우지 않는다.
    double previousTime{}; //4957a0은 새 이벤트를 소비할 때 현재 double 시각을 먼저 저장한다.
    std::uint32_t current{},count{}; // 원문 signed 비교를 수행하는 두 raw DWORD다.
};
// 실제 원문 하위만 선택된 시점에 요구한다. 검사 결과나 가짜 전송 성공을 만들지 않는다.
struct BuildReplayHooks {
    std::function<double()> now; //0055b4d0의 현재 프레임 시각을 각 원문 읽기 위치에서 조회한다.
    std::function<bool(float,float)> validPoint; //0040e800의 현재 WorldCells/ordered 좌표 판정이다.
    std::function<std::int32_t(std::string_view)> findType; //0049a860의 현재 TYPE 이름 조회이며 -1도 lowBYTE로 보낸다.
    std::function<void(std::span<std::uint8_t>)> constructPacket; //40a7c0의 현재 헤더/0시각만 쓴다. 품질BYTE16은 원문처럼 보존한다.
    std::function<void(std::uint32_t,std::span<const std::uint8_t>)> send; //4e2dc0의 실제 목적지9 송신이다. 예측 배치를 추가하지 않는다.
    std::function<void()> kill; //41bf10(this,0)의 실제 ProcessForm/Kernel 반납이다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; //4955b0의 i < NUM_EVENTS 진단이며 정상 복귀 뒤 EOF 판독도 계속한다.
    std::function<void(std::string_view,std::uint32_t)> label; //495730의 현재 선택 콜백이다. 부재는 원문 NULL callback으로서 자연 반환한다.
};
//4955b0의 fscanf6필드 반복과 마지막 current/count/시각 쓰기다. publish/playing/Host 부착은 실제 생성 부모가 담당한다.
void ReadBuildReplay(FILE* file,BuildReplayState& state,const BuildReplayHooks& hooks);
//4957a0의 한 프레임 한 이벤트 실행이다. packet은 최소25BYTE이며 constructor/필드 쓰기 전의 caller 품질BYTE를 유지한다.
void RunBuildReplayFrame(BuildReplayState& state,std::uint32_t& playing,std::span<std::uint8_t> packet,const BuildReplayHooks& hooks);
//495710: 현재 이벤트의 좌표가 보드 밖이면 true다. count 이후 미판독 tail 자체 일치는 주장하지 않는다.
bool IsBuildReplayLabel(const BuildReplayState& state,const BuildReplayHooks& hooks);
//495730: callback이 있으면 현재 좌표를 재조회하고 라벨의 RL 길이2를 생략하거나 빈 이름과 SID를 전달한다.
void NotifyBuildReplayLabel(const BuildReplayState& state,std::uint32_t sid,const BuildReplayHooks& hooks);
}
