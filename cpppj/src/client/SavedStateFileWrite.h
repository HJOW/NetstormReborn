// 현재 Client의 사용자 저장 경로와 기존 VFS/FILE을 일반 저장상태 writer에 빌린다.
#pragma once
#include "o/RawSavedStateWrite.h"
namespace netstorm::client { // 생성만으로 파일을 열거나 설치 폴더를 변경하지 않는다.
class Client; // 실제 Client는 반환 장치/Session보다 오래 살아야 한다.
o::SavedStateWriteHooks MakeSavedStateFileWriteHooks(Client& client); // SP 진단/현재 잔액은 Session이 추가하고 경로/FILE/시계/진단을 연결한다.
}
