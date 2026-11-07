# 1048 — 실제 수락 순서·상한·진단 반환

연결 수락의 원문4e28b0/4e02d0/4e18d0/4dfb90/4e1130을 정밀 C와 보호 PE로 대조해 기존 제품의 세 차이를 교정했다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**이며 이번 산출물 변경은 없다. 신규 네트워크/게임 검사는 수행하지 않았다.

원문 Poll은005e4a10(zlocal)→005e4a08(TCP)→005e4a0c(SPX) listener를 호출하고 pending 각1회→정상0..21의 현재 칸 반복으로 진행한다. 기존 복합 전송의 TCP→zlocal 순서와 그 주석을 교정했다. 같은 프레임에 내부 자기 연결과 원격 연결이 준비돼 있으면 첫 pending 번호/실제 처리 순서도 이 순서를 따른다. 지원 대상의 SPX는 계속 제외하고 내부/Editor는 같은 zlocal 위치다.

대기 개수00543604는 DWORD 저장/unsigned 감산과 별개로4e02f1의jge/4e038c의jl에서 **signed31 비교**를 한다. 현재 CanAccept도 같은 비트를signed32로 해석한다. 수락된 노드마다 현재 첫 빈 pending1..31→sender 음수→개수 증가의 기존 Registry 부모를 사용하고 상한을 다시 읽는다. 실제 표/개수 불일치에서 원문 무한 반복/범위 밖 쓰기를 합성하지 않는 호스트 물리 오류 경계는 유지한다.

TCP 수락은4e18d0처럼 실제select의 읽기/exception부터 검사한다. 준비0이면 accept/노드/진단 없이 자연 반환한다. 준비 오류 또는 실제accept의음수는 마지막WSA오류와 원문 `listening socket shutdown` 진단 후 반환한다. 별도 C++소켓 예외로 전체 Poll을 중단하지 않고 listener를 임의로 닫지도 않는다. select뒤 would-block도 원문의음수 경로이며 새 성공으로 취급하지 않는다. 후속 Poll은 같은 listener를 다시 조회할 수 있다.

4dfb90은4e1130으로 부모 socket 속성과 수락된 핸들을 복제하며 추가ioctl을 호출하지 않는다. 현재 Windows backend의 수락 뒤 재설정/새 실패 분기를 제거했다. Windows accept가 부모 socket의 속성을 유지한다는 플랫폼 계약도 [Microsoft 원문](https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-accept)에서 확인했다. 실제 비공개 주소만 받는 LAN 정책과 호스트 핸들/할당 경계는 유지한다.

정상22칸flush, pending각1회/정상ID별 반복은 기존 복합 전송 경로와 재대조했다. 원문 제외 방송4e2e80의 server 진단은 정상 복귀할 수 있지만 현재 RawBattleBroadcast는 곧바로 예외를 던지는 차이가 있어 다음 송신 감사 대상으로 남긴다. endpoint/서버/boss·전체 writer/decoder·메타/Outpost/checking5를 이번 수락 교정으로 완료 처리하지 않는다.

NetstormCpp Release는 **종료0·경고/오류0**, EXE 12,168,192바이트/SHA-256 `0f9ad90e5841a70053fbe7adcce51426dc186c8911c7f4dfa8c0b7fa5c37c5ce`다. 보호 2800파일 불변·새fixture0이다. 신규 MP/단위/게임/통신 검사는 지시대로0이며 실제 수락/동시 접속/오류 재현 완료는 아니다. 다음은 나머지 송신/LAN→한국어 후속→Edit 시나리오 UI다.
