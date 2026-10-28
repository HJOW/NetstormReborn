// 객체 Base와 내장 Form의 원문 진단 서식/현재 필드 조회를 복원한다.
#pragma once
#include "o/RiftType.h"
#include "o/SidPool.h"
#include <functional>

namespace netstorm::o { // 파생 자산의 Main/Task는 이 다섯 본체와 별도 연결한다.
// 실제 가상+70 또는 자산의 선행 직접 호출이 선택하는 원문 본체다.
enum class ObjectTraceFieldKind {
    Base, // 004ac400: 공통TYPE/링크/상태 한 줄만 기록한다.
    Form, // 004ae1e0: 공통 줄 뒤 현재 부모/이전WORD를 기록한다.
    Process, // 004ae3f0: 공통 줄 뒤 현재 signed PID DWORD를 기록한다.
    Depend, // 004ae530: 공통 줄 뒤 현재 감시Form 쌍을 기록한다.
    Content // 004aea90: 공통 줄 뒤 현재 내용TYPE/수량/flags를 기록한다.
};
// 같은 로그 장치의 level/서식완료 바이트를 빌리며 반환 뒤 현재 필드를 다시 읽는다.
using ObjectTraceFieldLog=std::function<void(std::int32_t,std::string_view)>;
// 같은 풀/TYPE와005c8454 현재값을 빌린다. 이름은 첫NUL·Base20BYTE 축약·Content의XXX 치환을 보존한다.
void TraceRawObjectFields(const SidPool& pool,std::span<const RiftTypeRecord> types,const std::uint32_t& suppress,
    std::uint32_t sid,ObjectTraceFieldKind kind,const ObjectTraceFieldLog& log);
}
