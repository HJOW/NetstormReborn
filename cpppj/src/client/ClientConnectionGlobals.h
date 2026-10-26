// Native 연결표와 실제 로컬 양끝의 주소/수명 장치를 Client 수명에 보관한다.
#pragma once
#include "o/RawConnectionRegistry.h"

namespace netstorm::client { //동기화 토큰과 서버 권한은 기존 실제 Client owner에서 빌린다.
// 최초 BSS0만 구성하며 Session 획득/반납이 표나 개수를 다시 초기화하지 않는다.
class ClientLocalStateGlobals; //동기화 DWORD의 실제 Client owner다.
class ClientConnectionGlobals final:public o::RawConnectionRegistry {
public:
    ClientConnectionGlobals()=default; //새 Client의 원문 고정 배열 수명이다.
    explicit ClientConnectionGlobals(std::shared_ptr<ClientLocalStateGlobals> local); //기존005c8bf4의 같은 주소와 수명을 빌린다.
    ClientConnectionGlobals(const ClientConnectionGlobals&)=delete; //실제 연결 주소 표를 복제하지 않는다.
    ClientConnectionGlobals& operator=(const ClientConnectionGlobals&)=delete; //생존 연결의 현재 칸을 값 대입으로 바꾸지 않는다.
    ClientConnectionGlobals(ClientConnectionGlobals&&)=delete; //연결된 실제 표의 주소를 이동하지 않는다.
    ClientConnectionGlobals& operator=(ClientConnectionGlobals&&)=delete; //현재 span의 주소 수명을 유지한다.
};
}
