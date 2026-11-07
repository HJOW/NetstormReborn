// 현재 서버 목록의 시작 읽기와 Root0Fh 이후 사용자 홈 저장을 연결한다.
#pragma once
namespace netstorm::client { // 설치 자료는 읽고 자동 생성 파일은 사용자 Saves에만 쓴다.
class Client; // 실제 현재 VFS/설정/목록과 사용자 저장 정책의 장기 소유자다.
void LoadClientRootServerList(Client& client); // 사용자 !rootservers.dat→기존 DataDir !rootservers.dat→rootservers.dat 순서다.
void SaveClientRootServerList(Client& client); //00458e30/458bf0의 현재 행/선택 LF를 UTF-8 경로의 binary 파일에 기록한다.
}
