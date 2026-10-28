// 실제 로그의 공유1024BYTE 버퍼·현재 스트림과 출력 순서를 복원한다.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>

namespace netstorm::client { // 파일/화면 장치는 같은 Client 수명에서 빌린다.
// 원본 FILE*는 호스트의 실제 FILE 포인터 또는 검사의 명시 토큰으로 표현한다.
using TraceLogStream=std::uintptr_t;
// 004c2630/004c2600/004c2610이 같은 주소에서 읽고 쓰는 실제 전역이다.
struct TraceLogState {
    std::int32_t level{29}; // 00542658의 PE초기값29이며 덤프 부모는 잠시30으로 쓴다.
    TraceLogStream stream{}; // 005ca8d0의 현재 FILE이며 최초0이다.
    std::uint32_t indent{}; // 005ca8d4의 현재 DWORD이며 signed양수에서만 공백을 넣는다.
    std::uint32_t mirror{},flush{}; // 005ca8dc/005ca8d8의 현재 출력 복제/즉시flush 관문이다.
    std::array<char,1024> buffer{}; // 005ca4d0의 같은 물리 버퍼이며 NUL 뒤 stale BYTE도 보존한다.
};
// 게임 formatter 이후의 실제 출력 장치이며 콜백 뒤 현재 상태/버퍼를 다시 읽는다.
struct TraceLogHooks {
    TraceLogStream standardOut{1},standardError{2}; // 실제 CRT의 두 고정 FILE을 연결하는 호스트 별칭이다.
    std::function<void(std::string_view)> console; // 00434f40의 trace→Chat 전달 몸체이며 조건 판단도 그 실제 장치가 수행한다.
    std::function<void(TraceLogStream,std::string_view)> write; // fprintf(FILE,"%s",현재공유버퍼)다. NULL도 이 경계로 전달한다.
    std::function<void(TraceLogStream)> flush; // fflush의 현재 FILE이며 콜백 뒤 예전 포인터를 재사용하지 않는다.
};
// 입력은 서식변환 완료 바이트와 원문 format의 선행~쌍 BYTE수다. 문자열 인자의~를 접두로 오인하지 않는다.
// 필터된 호출은 입력/장치를 읽지 않는다. 선택된 출력은 prefix→공백→본문/개행→console→mirror→현재FILE→flush 순서다.
void WriteRawTraceLog(TraceLogState& state,std::int32_t level,std::string_view formatted,std::size_t prefixBytes,const TraceLogHooks& hooks);
// 004c2610: 현재 FILE이 비0일 때만 같은 실제 flush 장치를 부른다.
void FlushRawTraceLog(TraceLogState& state,const TraceLogHooks& hooks);
}
