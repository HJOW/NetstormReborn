// 객체 덤프의 현재 슬롯 순회와 두 파일 단계/마크업 제거를 복원한다.
#pragma once
#include "client/RawTraceLog.h"
#include "o/RawColorMapCache.h"
#include "o/SidPool.h"
#include "o/RiftType.h"
#include "o/RawObjectTrace.h"
#include <functional>
#include <span>
#include <string>
#include <vector>

namespace netstorm::client { // 같은 현재 trace FILE/모드와 물리 풀을 빌린다.
// 기존 API 이름을 유지한 같은 원본 EX_UNABLE_TO_OPEN_FILE typed 예외다.
using ObjectDumpUnableToOpen=o::ColorCacheUnableToOpen;
// 원문 소비 시점마다 현재값을 읽는 덤프 전역이다.
struct ObjectDumpGlobals {
    std::uint32_t& suppress; // 005c8454: 진입1, 정상/typed catch 뒤0이며 초기값을 복원하지 않는다.
    const std::uint32_t& count; // 005c847c: 순회 끝의 현재값이며 최초 snapshot과 구별한다.
    const std::uint32_t& landType; // 005411dc: 좌표만 간단히 기록하는 현재 TYPE다.
    const std::uint32_t& fringeType; // 005411f0: 두 번째 단순 기록 TYPE다.
    const std::uint32_t& simpleType; // 0054123c: 세 번째 단순 기록 TYPE다.
};
// 경로/CRT FILE/할당과 실제24BYTE BaseFile을 원래 호출 위치에 연결한다.
struct ObjectDumpHooks {
    std::function<std::string()> path; // 459c60(-2,현재540cec,-3,"squids.txt",0)의 현재 경로다.
    std::function<TraceLogStream(std::string_view,std::string_view)> open; // fopen(path,"wt")의 실제 반환값이다.
    std::function<void()> openFailure; // traceStream!=NULL 진단이며 복귀 뒤 현재FILE을 다시 검사한다.
    std::function<void(TraceLogStream)> close; // fclose가 받는 그때의 현재 trace FILE이다.
    std::function<void(std::uint32_t)> trace; // 0044dab0의 실제 현재 재귀/가상 진단이다.
    std::function<std::vector<std::uint8_t>(std::string_view,std::uint32_t,std::uint32_t)> read; // 41baa0(path,0,&size,0)의 실제 전체 바이트다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> allocate; // 원문 max(signedSize,4)의 요청이며 호스트는 마지막NUL의 여유BYTE도 제공한다.
    std::function<void(std::span<std::uint8_t,24>)> construct; // 0041a5a0의 실제24BYTE 객체 생성이다.
    std::function<void(std::span<std::uint8_t,24>,std::string_view,std::uint32_t,std::uint32_t)> openWrite; // 41b4c0(path,2,0)이다.
    std::function<std::uint32_t(std::span<std::uint8_t,24>,std::span<const std::uint8_t>,std::uint32_t)> write; // 41a910(data,첫NUL전길이,ffffffff)이며 반환길이는 부모가 무시한다.
    std::function<void(std::span<std::uint8_t,24>)> destruct; // 41b410의 같은 객체다. 정상 close 예외를 중복 호출하지 않는다.
};
// 복원된고정trace format의접두쌍을같은로그에전달한다. END/가상진단이같은물리깊이/버퍼를쓴다.
void TraceRawObjectToLog(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,const std::uint32_t& suppress,
    const o::ObjectTraceGlobals& globals,TraceLogState& log,const TraceLogHooks& logHooks,std::uint32_t sid,const std::function<void()>& deadParent);
// 004d6020의 정상/열기거부/typed catch와 일반예외 순서다. 실제EH typed metadata는558문서를 따른다.
// 24BYTE 파일 물리와 할당 장치의 버퍼는 동기 호출이 끝날 때까지 유지해야 한다.
void DumpRawObjects(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,TraceLogState& log,
    const ObjectDumpGlobals& globals,const TraceLogHooks& logHooks,const ObjectDumpHooks& hooks,std::span<std::uint8_t,24> file);
}
