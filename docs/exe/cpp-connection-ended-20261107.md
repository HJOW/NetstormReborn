# 1045 — 실제 연결 종료 통지와 선택 활동표 정리

이월 목록의 frame22는22h 패킷이 아니라 **005c84e8의22DWORD 활동 프레임 표**다. 전체 clear는 기존 RawStateWorldCleanup의22개 직접 쓰기와 ClientStateWorldGlobals의 실제 owner에 연결돼 있다.64h 수신/주기 송신·timeout 안내/자동 실패는1015 연결이다. 이번 감사에서는 이와 별개인 **004b6660 개별 종료 수신기가 실제 분배기에 없고 선택 Player의 frame0 쓰기도 없던 누락**을 찾아 구현했다.

마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**이다.004b6660/004e2270/00491270의 정밀 C와 보호 PE를 읽기 전용으로 대조했으며 산출물을 변경하지 않았다.4b6660은 현재 연결과005c8bf4가 같으면 동기화0을 쓴다. 비서버9번은 기존004b6560의 flags8행/저장을 실행한 뒤 현재 server가 비영이면 즉시 반환한다. 그 외는 실제 Registry Disconnect를 호출하되 서버9번 자기 끝은 건너뛴다. 수신 깊이 안의 실제 종료는 기존 DeferConnectionClose/외곽 free를 재사용하며 여기서 직접 노드를 반납하지 않는다.

이후 현재server이고 sender1..8이면 현재 Player base를 각 store에서 다시 읽어 flags100 설정→flags1 해제→flags40 해제를 별도로 수행한다. 기존 RawMissionServerPackets의 실제 Player writer/주소 formatter를 재사용해 현재 접속자1..8에 보내고 **방송이 정상 반환한 뒤** 해당 활동 frame 한 칸만0으로 쓴다. 방송 실패에서 앞선 flags를 되돌리거나 frame을 미리 지우지 않는다. 벽시각 표와 Zone 간극은 지우지 않는다.10번 종료는 현재23이며 다음24/12가 아닐 때 다음20만 예약하고 즉시 World를 교체하지 않는다.

실제 Client의 복합 전송 생성은 Session 대여 전에2번 bootstrap 수신기를 등록한다. 버전/정책/observer는 원문4e2270처럼 새로 덮지 않는다. 콜백은 현재 World를 약하게 참조하다 호출 동안 전체 owner를 고정하고, 같은 장기 Registry token/권한/현재·다음 State/활동표를 사용한다. 실제 Session 바인딩 때 동일 수신기를 대여에도 포함해 callback 중 Native 정리 관문과 실행 pin을 적용하고 detach 때 이전 bootstrap 등록을 복원한다. 특정 Login this를 backend에 영구 저장하지 않는다.

Player 상태 방송은 Login 없는 Editor도 동일 원문 writer/현재 Registry를 사용할 수 있게 했다. 원문491270의 서버 진단 뒤 현재 접속자 표를 순회한다. 서버 장애 저장도 flags8행을 먼저 적용한 뒤 기존 writer의 missionPost/미known 자연 반환을 유지한다. Editor에 새 Login이나 저장 성공을 합성하지 않는다.

bootstrap1의004e2f00은 **빈 함수가 아니다.** 양수 ID의 별도22송신버퍼 칸이 비영이면 같은 현재 연결의 가상+4로 보내고 free→0한다. 이 몸체와 현재 OS backend의 접속 전 큐·전역버퍼 도달/수명은 다음 감사 대상으로 명시한다. endpoint/server/boss·리슨/수락 전체, 나머지 writer/decoder·메타/Outpost/checking5를 이번 종료 통지만으로 완료 처리하지 않는다.

첫 Release는 기존 Read에 없는 세 번째 인자를 넘긴 한 C2660 오류/종료1이었다. 원문 BYTE flags 직접 조회로 교정하고 callback 대여도 보완한 최종 NetstormCpp Release는 **종료0·경고/오류0**, EXE 12,160,000바이트/SHA-256 `7d11881eec76c95b00f1da87eccd4a43fabfbabf5c4d5aa0c3ee4146f7674a0d`다. 보호 2800파일 불변·새 fixture0이다. 신규 MP/단위/게임/통신·실제 접속 종료 검사는 지시대로0이다. 다음은 남은 LAN→한국어 후속→Edit 시나리오 UI다.
