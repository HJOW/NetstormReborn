// 실제 ProcessFinder 순서로 부모의 프로세스들을 골라 제거한다.
#pragma once
#include "o/SquidProcess.h"
namespace netstorm::o {
// PATCH004b27a0: 다음 링크를 삭제 전에 보존하고 정확한 타입(0이면 전체)을 flags40으로 제거한다.
void RemoveAttachedProcesses(SquidProcessHost& host,Sid parent,std::uint32_t type);
}
