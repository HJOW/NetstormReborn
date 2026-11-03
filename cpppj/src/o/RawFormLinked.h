//원문 Form::isRobust004ae080의 부모 체인 전체 검사다.
#pragma once
#include "o/SidPool.h"
#include <functional>

namespace netstorm::o { //ContentForm의 목록 flags 진단과 구별하는 공통 Form 연결 검사다.
Sid ReadRawProcessFormParent(const SidPool& pool,Sid form); //알려진 ProcessForm 가상 표의 실제0번 getter는 상태/TYPE 검사 없이+10 WORD만 반환한다. 미복원 가상 표는 명시 거부한다.
bool CheckRawFormChain(std::span<const std::uint8_t> bytes,std::uint32_t stride,std::uint32_t capacity,Sid form,Sid parent); //명시 물리/논리 범위의 전체 체인만 검사한다. 소형 Native 입력도 실제 게임 풀 재구성 없이 대조한다.
bool CheckRawFormLinked(const SidPool& pool,Sid form,const std::function<Sid(Sid)>& parent); //가상 부모를 한 번 읽고 끝까지 순회한다. 자기 뒤의 범위 실패도 false이며 순환은 호스트 명시 오류다.
}
