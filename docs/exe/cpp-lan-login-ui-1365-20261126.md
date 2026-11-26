# 일반 LAN 로그인 안내 두 경계 연결 — 단위1365

2026-11-26, HJOW-Athlon. 마지막 디컴파일 PC도 HJOW-Athlon이고 재디컴파일·원문 변경은 없다. 기존 원문 State8의 missionPost0 연결/재접속 안내와72h 중앙섬 안내를 현재 공통 UI에 연결했다. 원본을 실행하지 않았다.

1364의 두 참가자 시작에서 `일반 연결/재접속 안내 미연결`로 종료됐다. `RunCampaignMissionStarting`은 World 교체 중 Session이 사라져도 유지되는 Client 장기 부모지만, 같은 호출의 즉시8에 필요한 progress를 설치하지 않았다. Session·메타 부모에만 있던 `ShowLoginProgress`를 이 부모에도 지연 조회하도록 연결했다. 그 뒤 양쪽이 State9, 호스트가State10까지 진행했다.

일반 로그인 공통 서비스의 creatingIslands도 빠져 `RawMissionServerPackets::ReceiveMetadata`에서 실패했다. 기존 미션 시작은 따로 이 콜백을 설치해 미션 검사로는 드러나지 않았다. 공통 서비스에 현재 번역·표시 인코딩·동일 transient HTML 창을 사용하는 콜백을 연결했다. 수정 후 실제 호스트 로그의 `createArchipelago... predictableCount = 4505`와 Fort 전송까지 확인했다. 빈 콜백이나 단순 refresh로 대체하지 않았으며 다음 오류에서 계속 명시 실패한다.

Release 빌드 성공. 실제 두 참가자 재검사2회로 위 순서의 진전을 확인했고, 최종 EXE의 기존 캠페인1-1 진입/실제 입력·Session 관측1회는 종료0이다. 새 순수 단위 검사를 만들거나 기존 메뉴170 전체 검사를 반복한 것으로 세지 않는다. 보호3212파일과 기존 fixture는 불변이다. 상대 봇의 오류 종료 시 깨진 stdin도 명시 닫아 Python 종료 때 보조 I/O 예외가 출력되지 않게 했다.

**전체 전투 로그인은 미완료다.** 상대 클라이언트는 State9의 `다른 owner의 복합 연결 status 조회`, 호스트는 중앙섬/Fort 전송 뒤 `미션 클라이언트 패킷의 실제 소비자 미연결`로 각각 종료1이다. 새TCP와 이전Fort 내부Source의 장기 등록/반납·실제 수신 소비자를1366부터 대조한다. 원인의 추정과 확정 수정을 구별하며 전역표 삭제·무조건 성공·가짜 ACK로 우회하지 않는다.

최종 EXE SHA-256 `1f0bd53191751885ae5d62fe4e25868819e728218925f8e4518c3c7245d04883`. [검증 JSON](../../cpppj/lan-login-ui-1365-20261126.json). 원시는 `extracted/cpp-lan-1364/bot-battle-2`/`bot-battle-3`·`build-login-1365*.log`와 `extracted/cpp-followups-1328/lan-ui-hooks-1365-mission*`다. 최신 ZIP1362는1363~1365 수정 전이다.

사용자의 추가 지시에 따라 [DN 후속 문서](../dotnet-cpp-lan-followup-20261126.md)의 DN-MP-03/04와 DN 인계에 같은 경계를 기록했다. DN 코드/실행·진행률은 변경하지 않았다. MP 포획/희생·특정 공격·승리와 명시 사용자 확인 항목은 계속 제외한다.
