// 개발자 전용2bh 입력은 일반 Fort 준비 뒤 기존1bh 수신 본문을 직접 호출한다.
#pragma once
#include <functional>

namespace netstorm::client { // Editor c2의 snapshot/Guide 전환과 구별한다.
class Client;struct NativeBattleSource; // 같은 Client와 실제 선택된 전송 장치를 빌린다.
void JoinNativeTestBattle(Client& client,const std::function<NativeBattleSource()>& selectSource); //4b8be0: 이름4개→현재 지역packet→server0/phase29→같은1bh 본문이다. Source는 실제 Run 지점에서만 선택한다.
}
