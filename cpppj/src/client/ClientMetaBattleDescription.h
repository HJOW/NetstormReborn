// 실제 CS 설명의 로그·현재 객체 가상88·특정 전투 표시 갱신을 같은 Client에 연결한다.
#pragma once
#include "client/NativeMetaBattleDescription.h"
namespace netstorm::client { //장치 생성만으로 World/CS/화면/통신을 실행하지 않는다.
class Client; //이 실제 소유자는 반환 장치의 동기 사용보다 오래 살아야 한다.
NativeMetaBattleDescriptionHooks MakeClientMetaBattleDescriptionHooks(Client& client); //선택된 원문 효과 위치에서만 현재 물리와 Renderer를 조회한다.
}
