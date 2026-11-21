// 15차 Reset All의 사용자 설정·진행 파일 초기화다. 설치 자료와 커스텀 맵·로그는 건드리지 않는다.
#pragma once
#include "platform/UserDataRoot.h"

namespace netstorm::client { // 초기화 요청의 실제 디스크 변경 경계다.
// 사용자 options.cfg를 빈 파일로 바꿔 기존 설치 설정의 재이관을 막고 체크포인트·화면 표식·서버 목록을 지운다.
// 지정 파일이 디렉터리/링크이거나 읽기·쓰기·삭제가 실패하면 예외를 던진다. 폴더 전체를 재귀 삭제하지 않는다.
void ResetAllUserSettings(const platform::UserDataRoot& user);
}
