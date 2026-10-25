// 공통 시작의 실제 Player 할당과 방향별 동맹 초기화 장치를 연결한다.
#pragma once
namespace netstorm::client { // Native의 원문 초기화 위치에서만 명시 호출한다.
class Client; // 실제 Core/Player/현재 known과 Chat 채널의 장기 소유자다.
void InitializeClientPlayerRecords(Client& client); //0043952c의 실제 초기화다. 초기 known0 경로만 요구하며 없는 World/Chat을 빈 성공으로 채우지 않는다.
}
