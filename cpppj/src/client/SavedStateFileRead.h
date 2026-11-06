// 저장상태 reader를 실제 Client의 사용자 파일/Player/메타데이터 물리에 연결한다.
#pragma once
#include <cstdint>
namespace netstorm::client {
class Client; // 현재 Player/이름/Core/State 소유자가 동기 읽기보다 오래 살아야 한다.
std::int32_t ReadClientSavedState(Client& client); //004b5890의 실제 파일 부재/짧은 읽기/부분 복원 효과와 signed 저장 벽시각을 반환한다.
}
