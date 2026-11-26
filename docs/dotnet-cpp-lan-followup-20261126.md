# cpppj TCP/IP 실행에서 발견한 문제의 dotnetpj 후속 작업

2026-11-26 사용자 요청. dotnetpj 코드 수정·실행은 하지 않았다. 다음 DN 재개 시 아래 공통 도움말 차이를 먼저 교정하고, 멀티플레이어 UI·로그인·수송을 연결하는 R6 SL-9/R11에서 해당 항목을 필수 확인한다. cpppj의 추가 실행 결과가 나오면 이 문서와 인계 체크를 갱신한다.

현재 cpppj는 실제 클론 두 개를 MCP로 조작하는 상대 봇으로 loopback TCP 대기실 명단과 정상 종료를 확인했다(1364). 1367에서는두참가자시작/짧은대기/두정상종료까지통과했다. 간헐시계·서버인수후속·생산건설동기화는남았다. 아래 cpppj 재현 사실을 DN의 실행 결과로 간주하지 않는다. [CP 인계](../LEFT_JOBS.md#now) · [CP1364](exe/cpp-lan-opponent-lobby-1364-20261126.md) · [봇 사용법](testing/cpp-lan-opponent-guide.md).

| ID | cpppj에서 확인한 사실 | dotnetpj 확인 위치와 완료 기준 |
| --- | --- | --- |
| DN-MP-01 | 관련 공통 결함1363: `SetNodeHelp`가 LOOK1000/HELP200을 쓰지 않아 예산 우클릭 About이 열리지 않았다. 빈 LOOK는1000 해제, 빈 HELP는 포인터만 해제/200 유지, 첫 NUL 처리가 필요하다. CP는 교정 후 메뉴169/언어52·세 언어 실제 GUI를 통과했다. | **정적 차이 확인:** `src/Netstorm.Client/Gump/GumpButtonTree.Ownership.cs:33`도 플래그 쓰기가 없고 빈 문자열까지 새로 소유한다. `tests/Netstorm.Client.Tests/HostGumpOwnershipTests.cs:65`는 잘못된 빈 문자열 소유를 기대한다. 다음 DN 작업에서 setter·설명 주석·검사를 교정한다. 기존 소유 토큰 해제·다른 비트 보존·빈 값/재설정/첫 NUL을 검사하고, 생산 HUD 연결 후 숫자 칸/생산창 우클릭을 실제 확인한다. [CP1363](exe/cpp-help-flags-1363-20261126.md). |
| DN-MP-02 |1364: 새 Fort 이름 확정 때 옵션 루트/자식의 형제 순환으로 종료됐다. 원본004cfc80의 가상+18은 Push,004cc970의 자식+10은 Move인데 Redraw/직접 Attach로 연결한 오류였다.004bc800→004bb430 스타일 파서 누락으로 `~w~3~E`/`~y`도 화면에 노출됐다. CP는 원문 순서/파서 교정 후 전체 메뉴170/170과 실제 양쪽 명단·종료0을 확인했다. | 일반 Multiplayer Options를 이식/연결할 때 `Netstorm.Client/Gump/GumpTree.cs`의 Push→Attach·Move를 사용한다. 현재 DN에 대응 일반 옵션 창이 이식됐다고 확인하지 않았다. 반복 이름/서버 갱신·두 통지·자식 부모/형제 링크·실제 창 닫기를 검증하고 스타일 제어 문자가 표시되지 않는지 확인한다. 공통 순환 검사를 완화하거나 트리를 새로 만드는 방식으로 숨기지 않는다. |
| DN-MP-03 |1365 해당 범위 완료: 일반 LAN 시작의 Client 장기 State7/즉시8 부모에 Progress가 빠져 `일반 연결/재접속 안내 미연결`로 종료됐다. Session/메타 부모에는 이미 있어 미션 검사로는 드러나지 않았다. 공통 로그인 서비스에 CreatingIslands가 없어72h 중앙섬 메타데이터에서도 종료됐다. 두 안내 콜백 연결 후 State9/10·`createArchipelago... predictableCount = 4505`·Fort 전송까지 진행했다. 전투 완료는 아직 실패다. | `Netstorm.Core/Original/LocalMissionStateTypes.cs`의 Progress/ConnectedProgress/CloseProgress와 `RawMissionServerPackets.cs`의 CreatingIslands를 실제 호스트/Client 장기 부모·Session 부모 양쪽에 연결한다. R6 SL-9/R11에서 일반 missionPost0·저장 재접속·즉시8·중앙섬 유/무를 구별한다. 기존 transient HTML 창/현재 번역/글꼴/닫기 수명을 사용한다. 빈 콜백·refresh만으로 통과시키지 않는다. DN 제품 호출자 미연결 여부는 해당 조립 때 다시 대조한다. |
| DN-MP-04 |1366은 비인계Fort Source반납·실제수신6콜백/채널제거,1367은일반전투의객체Chaff/공유후처리3모드를교정해Fort범위오류를해결했다. 실제두참가자14/Ready·시작안내소멸·짧은대기/두종료0을확인했다. | DN도TCP재사용·인계/비인계Source pin/반납·수신28필드/444ff0채널해제·Client장기후처리chaff/special/fort를대조한다. 원문17/18의accepted/denied로그·즉시14/출처0+next3은CP1369잔여로DN도점검한다. 빈데이터/임의성공/전역표삭제로우회하지않는다. |
| DN-MP-05 | 단일 미션·정적 소스/단위 검사만으로 위 일반 LAN 조립 누락이 발견되지 않았다. CP 봇은 새 사용자/실시간/새 Fort로 실제 두 참가자를 실행한다. | R11 실행 검증에 DN 상대 봇 또는 같은 MCP 조작 경로를 마련한다. 실제 TCP 연결→두 고유 subscriber/명단→Start/실제 ACK→로그인14/PlayerKnown→동일 월드·단순 생산/건설/수신→이탈/정상 종료를 단계별로 검증한다. CP 봇은 현재 CP 전용이므로 DN 지원으로 세지 않는다. 사용자 직접 확인·물리 LAN·MP 포획/희생/특정 공격/승리는 기존 제외 범위를 유지한다. |

CP1366은 비인계Fort두끝/리스너와 수신28필드 중 누락6개(assertion/configInt/tell/peerAddress/removeChat/saveReconnect)를 실제장치에 연결하고00444ff0채널해제를 복원했다. 연결30/채팅7검사와 실제재검증에서 기존두오류가 없어졌다. DN은 `Netstorm.Client/Chat/RawChatChannels.cs`의 대응 API·중복ID/0/하위BYTE mask·무관필드보존도 확인한다. [CP1366](exe/cpp-lan-source-owner-1366-20261126.md).

CP1367은 일반전투의객체Chaff와 이전Fort에서남은 Client장기후처리chaff/special/fort를 모두현재일반모드false로 맞췄다. Chaff만 교정한중간실행은실패했으며special이남으면RawIslandRuntime.Matches가owner를무시해다른참가자의같은Terr까지소비했다(stream1,pos21,size21). 최종두참가자시작/대기/종료0과같은명단/ID/Ready·SP6500·7660객체를확인했다. DN도새지역상태false검사뿐아니라 이전Fort→일반LAN의같은장기owner로검증한다. [CP1367](exe/cpp-lan-fort-modes-1367-20261126.md).

**원문상태17/18 후속:** 호스트종료뒤상대가서버인수15/16을진행하다17에서명시실패했다. 보호PE4b7cd5/4b7cf3와원문4b6dd0에서Reconnection accepted/denied(레벨1d),17의즉시14,18의sourceType0/next3을확인했다. CP아직미구현이며DN일반상태분배기에서도확인한다. 실제서버인수/재접속이완료됐다고세지않는다.

| 추가 ID | 재현/정적 대응 | DN 다음 작업·완료 기준 |
| --- | --- | --- |
| DN-MP-06 | CP1367 별도 실행의 간헐 Archer 오류와1368 중간 실행의 월드 역행 거부를 확인했다. DN `Netstorm.Client/Mission/RawCampaignArcherBindings.cs:135`의 Delta<0, `Netstorm.Core/Original/RawSurfaceWorld.cs:786`의 Game<이전Game도 정적으로 확인했다. CP1368은 원문00460e90/00467c80에 맞춰 두 부호 제약을 교정하고 집중21/21·실제60초 이상 반복2회/각 두 정상 종료를 통과했다. | R5/R6 프레임과R11 시계 보정을 함께 연결할 때 두 경계를 교정한다. 원문은 부호 있는 차분을 보존하고 Archer의 비양수 적분만 건너뛴다. NaN/무한대 거부·상태 보존은 유지하고 시각/경과를 임의 고정하거나0으로 제한하지 않는다. 공유 주소·보정 증가/감소·정지/재개·인수 경계와 실제 두 참가자 반복/긴 대기·단일 미션 회귀를 확인한다. DN 실행 재현/교정 완료로 세지 않는다. [CP1368](exe/cpp-lan-clock-1368-20261126.md). |

DN의 제품 기준1339·마지막 완료1.076·기존 검사 수/진행률을 이 기록만으로 올리지 않는다. 완료 표시는 실제 코드 교정과 해당 단위 검증 뒤에만 한다. 원본/fixture·AGENTS.md·README 개요는 변경하지 않았다.

1368 확정: 원문 시간 계약과 실제 두 참가자 반복 결과를 DN-MP-06에 반영했다. 상태17/18·서버 인수는 CP1369에서 계속하며, 새 결과가 나오면 DN-MP-04에도 갱신한다.
