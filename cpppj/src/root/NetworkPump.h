// R.exe 메인 통신 부모의256/32/51×40 순회와64칸 select 집합을 복원한다.
#pragma once
#include "root/ConnectionState.h"
#include <array>
#include <cstddef>

namespace netstorm::root { // 게임 클라이언트의22칸 Poll과 다른 R00476b80의 경계다.
struct NetworkSocketSet { // 원문 FD_SETSIZE64와 원문 직접 추가의 중복 보존을 표현한다.
    std::array<std::uintptr_t,64> sockets{}; // 실제 OS 장치가 쓰는 포인터 폭을 유지하며 DWORD로 절삭하지 않는다.
    std::size_t count{}; // 원문 fd_count이며 가득 찬 집합에는 추가하지 않는다.
    void Add(std::uintptr_t socket); // FD_SET의 중복 제거를 적용하지 않고 원문 배열에 직접 추가한다.
};
struct NetworkSocketSets { // select가 같은 세 집합의 실제 count/원문을 바꿀 수 있다.
    NetworkSocketSet read,write,error; // 일반/대기는 read+error, 미연결 일반만 write, Challenge는 read에 추가한다.
};
struct NetworkState { // R의 옵션/유휴 전역을 같은 프로세스 수명으로 소유한다.
    std::int32_t mainTimeout{},pendingTimeout{},challengeTimeout{},emptyExitTimeout{}; //004d21e8/ec/f0/21bc의 BSS0이며 원문 옵션 해석이 현재값을 쓴다.
    std::uint32_t previousChallengeParticipants{1},emptySince{}; //004cd804의 PE초기1과004d2208의BSS0이다.
};
struct NetworkHooks { // 선택된 실제 장치만 요청하며 미연결을 정상 서버 통신으로 처리하지 않는다.
    std::function<std::uint32_t()> clock; //4739b0의 지역 FullTime을 최초 한 번 만들고 ftol64 하위 DWORD를 공급한다. 순회 중 시각을 새로 읽지 않는다.
    std::function<std::uint32_t(std::uint32_t)> lastReceived; //478a30의 같은 실제 노드+44 현재 DWORD다.
    std::function<std::uint32_t(std::uint32_t)> noMainTimeout; // 일반 노드+90이며0일 때만 수신 타임아웃을 적용한다.
    std::function<std::uint32_t(std::uint32_t)> socketFlags; // 실제 하위 socket+0의 현재 플래그이며0x80은 listener 관문이다.
    std::function<std::uintptr_t(std::uint32_t)> socket; // 현재 노드의 하위 socket+4 실제 OS 핸들을 폭 보존해 조회한다.
    std::function<std::int32_t(std::uint32_t)> socketStatus; //88590의 현재 준비 판정이며 오류/닫힘 부작용도 보존한다.
    std::function<std::string(std::uint32_t)> describe; //59760의 주소 읽기와864b0의1/0/0 서식을 같은 실제 노드에 실행한다.
    std::function<void(std::int32_t,std::string_view)> log; // 원문 level과 이미 서식화한 전체 로그를 전달해 문자열을 가변 인자의 정수로 오해하지 않게 한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // count<64의 실제 r.cpp 진단이다. 복귀하면 순회를 계속한다.
    std::function<void(std::uint32_t)> terminate; //84890의 실제2번 종료 통지/깊이 반납 부모다. 슬롯0을 합성하지 않는다.
    std::function<void(std::int32_t)> closePending; // 대기 타임아웃에서858d0에 실제 음수 ID를 넘긴다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> challengeConnection; // 현재 Challenge+2ad8의40개 참가자 전송 토큰을 매번 읽는다.
    std::function<std::int32_t(std::uintptr_t,NetworkSocketSets&,std::int32_t)> select; // 최고핸들+1/현재세집합/초 단위 timeout이며 usec0이다. 반환값으로 후속 순회를 생략하지 않는다.
    std::function<void(std::uint32_t,std::uint32_t)> receive; // 같은 현재 transceiver 가상+0(1)이다. 반환값은 원문처럼 사용하지 않는다.
    std::function<void(std::uint32_t)> flush; // 수신 복귀 뒤 현재 일반 슬롯의 가상+0c다.
    std::function<void(std::uint32_t)> receiveChallenge,sendChallenge; //578f0→579d0이며 두 호출 사이 현재 Challenge를 다시 읽는다.
    std::function<bool()> auxiliaryActive; //004d23f8에 대한479bb0의 현재 판정이다. 장치의 의미를 추측해 LAN 성공으로 바꾸지 않는다.
    std::function<std::uintptr_t(NetworkSocketSets&)> auxiliarySets; //7f510의 read/error 집합 추가와 최대핸들 반환이다.
    std::function<void()> auxiliaryPoll; // 실제7f2b0이며 일반 수신/flush 뒤에 호출한다.
    std::function<void(std::int32_t)> exit; // LAN 유휴 기한 초과의 실제_exit(1)이며 정상 반환하지 않는다.
};
struct NetworkPumpResult { // 원문이 분기에서 사용하지 않은 select 값과 실제 순회 개수의 진단 결과다.
    std::int32_t selected{}; // OS select의 반환값이며 성공 여부를 뜻하는 bool로 정규화하지 않는다.
    std::uint32_t sockets{},challengeParticipants{}; // 타임아웃을 통과한 소켓 수와 타임아웃 전 Challenge 참가자 수다.
};
NetworkPumpResult PumpNetwork(ConnectionState& connections,NetworkState& state,std::int32_t waitSeconds,const NetworkHooks& hooks); //00476b80의 부모이며 실제 OS/Challenge 장치는 호출자가 제공한다.
}
