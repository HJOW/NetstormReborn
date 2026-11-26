# 직접 LAN 시작의 이전 Source 반납과 수신 장치 — 단위1366

2026-11-26, HJOW-Athlon. 마지막 디컴파일 PC도 HJOW-Athlon이다. 재디컴파일·원본 실행/변경 없이 기존10.78 C와 현재 장치를 대조했다.

직접 대기실에서 전투를 시작할 때 이전 Fort의 내부 서버9가 남았다. 원문 State7의 reason1 정리는 서버9를 보존할 수 있고, 장기 등록표의 실제 Source pin이 이전 Editor 전송을 유지한다. 새 TCP의 원문 재사용 관문은 이 이전 연결의 준비값1을 보고 새 socket 생성을 생략했다. 기존 코드의 CloseListener/reset만으로는 두 끝을 반납할 수 없었다.

`EditorLocalTransport::CloseOwnedConnections`로 자기 실제 정상/음수 노드와 listener만 반납한다. `LoadGeneralBattleWorldBody`는 이전 Editor가 실제 network.editor로 인계되지 않을 때만 이를 호출한다. 같은 공용 표의 다른 Source·인계된 기존 network·원문 reason1 정리는 보존한다. 순환/소유자 검사를 완화하거나 새 연결 성공값을 만들지 않았다.

새 집중 검사는 이전 Fort9가 실제 TCP의 재사용 관문을 선택함을 먼저 확인한다. 자기 Source를 반납한 뒤9/pending이 사라지고 다른 Source5가 보존되며, 새 접속의 잘못된 숫자 주소 검사가 선택됨을 확인한다. DNS/socket을 열지 않는다. 반복 반납도 close/release를 늘리지 않는다.

실제 클라이언트 수신 조립의 누락6개(assertion/configInt/tell/peerAddress/removeChat/saveReconnect)도 같은 진단·설정·Tell·전송·저장 부모에 연결했다.28필드 전체에 대입이 있는지 정적으로 대조했다. 새 채널 제거는 원본00444ff0을 복원했다. 모든100기록의 일치 subscriber(0 포함)에서 mask 하위BYTE 비트만 지우며 이름/ID/다른 채널/무관 행·Refresh를 보존한다. 중복ID/0/상위 mask·4800바이트 보존 검사를 추가했다. 최초 빌드에서 존재하지 않는 참여서비스 removeChat에 연결한 C2039는 공통 Login 서비스와 실제 RawChatChannels 몸체를 추가해 교정했다.

Release 제품·집중 대상 빌드 성공, 연결/소유권30/30·채팅7/7 통과다. 실제 두 클라이언트 재검사에서는 이전 `다른 owner의 복합 연결 status 조회`와 `미션 클라이언트 패킷의 실제 소비자 미연결`이 사라지고 다수 Fort 전송/객체 설치로 진행했다. **전투 로그인은 아직 실패**다. 호스트는 `Fort Objects 직접 BYTE 물리범위`, 상대는 `Done` 뒤 상태17에서 `미복원상태진단미연결`로 종료1이다. 다음1367은 이 객체 읽기/상태17의 원문 부모를 대조한다. 입력/상태를 주입하거나 로그인14 성공으로 세지 않았다.

EXE SHA-256 `b844b195763104548617ffe77b9c8c59ce309f3cf03a2db48f670c4a51b67957`. 보호3212파일/기존 fixture 불변. [검증 JSON](../../cpppj/lan-source-owner-1366-20261126.json). 원시는 `extracted/cpp-lan-1364/build-source-1366*.log`·`tests-source-1366.log`·`tests-chat-1366.log`·`bot-battle-4`다. 봇의 성공 조건은 실제14와PlayerKnown을 함께 요구한다. 오류 EOF/프로세스 종료 경합도 실제 종료 코드로 수집하며 실패를 성공으로 바꾸지 않는다. 최신ZIP1362는1363~1366 변경 전이다.

[DN 후속](../dotnet-cpp-lan-followup-20261126.md)·DN 인계의 DN-MP-04에도 같은 확정 결과와 다음 두 실패를 기록했다. DN 코드/실행/검사 수는 바꾸지 않았다. 사용자 직접 확인·MP 포획/희생·특정 공격·승리는 계속 제외한다.
