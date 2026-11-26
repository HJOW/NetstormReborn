# cpppj TCP/IP 실행에서 발견한 문제의 dotnetpj 후속 작업

2026-11-26 사용자 요청. dotnetpj 코드 수정·실행은 하지 않았다. 다음 DN 재개 시 아래 공통 도움말 차이를 먼저 교정하고, 멀티플레이어 UI·로그인·수송을 연결하는 R6 SL-9/R11에서 해당 항목을 필수 확인한다. cpppj의 추가 실행 결과가 나오면 이 문서와 인계 체크를 갱신한다.

현재 cpppj는 실제 클론 두 개를 MCP로 조작하는 상대 봇으로 loopback TCP 대기실 명단과 정상 종료를 확인했다(1364). 1367에서는두참가자시작/짧은대기/두정상종료까지통과했다. 1368 시계 경계·60초 반복2회,1369 상태17/18·실제 서버 인수도 확인했다. 1370 Wind 신전 한 건의 객체/SP·복구 저장도 확인했다. 유닛 생산·추가 건설·새 참가자 재접속은 남았다. 아래 cpppj 재현 사실을 DN의 실행 결과로 간주하지 않는다. [CP 인계](../LEFT_JOBS.md#now) · [CP1364](exe/cpp-lan-opponent-lobby-1364-20261126.md) · [봇 사용법](testing/cpp-lan-opponent-guide.md).

| ID | cpppj에서 확인한 사실 | dotnetpj 확인 위치와 완료 기준 |
| --- | --- | --- |
| DN-MP-01 | 관련 공통 결함1363: `SetNodeHelp`가 LOOK1000/HELP200을 쓰지 않아 예산 우클릭 About이 열리지 않았다. 빈 LOOK는1000 해제, 빈 HELP는 포인터만 해제/200 유지, 첫 NUL 처리가 필요하다. CP는 교정 후 메뉴169/언어52·세 언어 실제 GUI를 통과했다. | **정적 차이 확인:** `src/Netstorm.Client/Gump/GumpButtonTree.Ownership.cs:33`도 플래그 쓰기가 없고 빈 문자열까지 새로 소유한다. `tests/Netstorm.Client.Tests/HostGumpOwnershipTests.cs:65`는 잘못된 빈 문자열 소유를 기대한다. 다음 DN 작업에서 setter·설명 주석·검사를 교정한다. 기존 소유 토큰 해제·다른 비트 보존·빈 값/재설정/첫 NUL을 검사하고, 생산 HUD 연결 후 숫자 칸/생산창 우클릭을 실제 확인한다. [CP1363](exe/cpp-help-flags-1363-20261126.md). |
| DN-MP-02 |1364: 새 Fort 이름 확정 때 옵션 루트/자식의 형제 순환으로 종료됐다. 원본004cfc80의 가상+18은 Push,004cc970의 자식+10은 Move인데 Redraw/직접 Attach로 연결한 오류였다.004bc800→004bb430 스타일 파서 누락으로 `~w~3~E`/`~y`도 화면에 노출됐다. CP는 원문 순서/파서 교정 후 전체 메뉴170/170과 실제 양쪽 명단·종료0을 확인했다. | 일반 Multiplayer Options를 이식/연결할 때 `Netstorm.Client/Gump/GumpTree.cs`의 Push→Attach·Move를 사용한다. 현재 DN에 대응 일반 옵션 창이 이식됐다고 확인하지 않았다. 반복 이름/서버 갱신·두 통지·자식 부모/형제 링크·실제 창 닫기를 검증하고 스타일 제어 문자가 표시되지 않는지 확인한다. 공통 순환 검사를 완화하거나 트리를 새로 만드는 방식으로 숨기지 않는다. |
| DN-MP-03 |1365 해당 범위 완료: 일반 LAN 시작의 Client 장기 State7/즉시8 부모에 Progress가 빠져 `일반 연결/재접속 안내 미연결`로 종료됐다. Session/메타 부모에는 이미 있어 미션 검사로는 드러나지 않았다. 공통 로그인 서비스에 CreatingIslands가 없어72h 중앙섬 메타데이터에서도 종료됐다. 두 안내 콜백 연결 후 State9/10·`createArchipelago... predictableCount = 4505`·Fort 전송까지 진행했다. 당시 전투 시작은 실패했고1367 후속 교정 뒤 통과했다. | `Netstorm.Core/Original/LocalMissionStateTypes.cs`의 Progress/ConnectedProgress/CloseProgress와 `RawMissionServerPackets.cs`의 CreatingIslands를 실제 호스트/Client 장기 부모·Session 부모 양쪽에 연결한다. R6 SL-9/R11에서 일반 missionPost0·저장 재접속·즉시8·중앙섬 유/무를 구별한다. 기존 transient HTML 창/현재 번역/글꼴/닫기 수명을 사용한다. 빈 콜백·refresh만으로 통과시키지 않는다. DN 제품 호출자 미연결 여부는 해당 조립 때 다시 대조한다. |
| DN-MP-04 |1366은 비인계Fort Source반납·실제수신6콜백/채널제거,1367은일반전투의객체Chaff/공유후처리3모드를교정해Fort범위오류를해결했다. 실제두참가자14/Ready·시작안내소멸·짧은대기/두종료0을확인했다. | DN도TCP재사용·인계/비인계Source pin/반납·수신28필드/444ff0채널해제·Client장기후처리chaff/special/fort를대조한다. 원문17/18의 레벨1d 결과 로그·즉시14/출처0+next3은 CP1369에서 교정/인수 검증했고 DN 분배기에는 두 상태가 빠져 있다. DN도 전체144BYTE 보존·예외와 실제 인수를 점검한다. 빈데이터/임의성공/전역표삭제로우회하지않는다. |
| DN-MP-05 | 단일 미션·정적 소스/단위 검사만으로 위 일반 LAN 조립 누락이 발견되지 않았다. CP 봇은 새 사용자/실시간/새 Fort로 실제 두 참가자를 실행한다. | R11 실행 검증에 DN 상대 봇 또는 같은 MCP 조작 경로를 마련한다. 실제 TCP 연결→두 고유 subscriber/명단→Start/실제 ACK→로그인14/PlayerKnown→동일 월드·단순 생산/건설/수신→이탈/정상 종료를 단계별로 검증한다. CP 봇은 현재 CP 전용이므로 DN 지원으로 세지 않는다. 사용자 직접 확인·물리 LAN·MP 포획/희생/특정 공격/승리는 기존 제외 범위를 유지한다. |

CP1366은 비인계Fort두끝/리스너와 수신28필드 중 누락6개(assertion/configInt/tell/peerAddress/removeChat/saveReconnect)를 실제장치에 연결하고00444ff0채널해제를 복원했다. 연결30/채팅7검사와 실제재검증에서 기존두오류가 없어졌다. DN은 `Netstorm.Client/Chat/RawChatChannels.cs`의 대응 API·중복ID/0/하위BYTE mask·무관필드보존도 확인한다. [CP1366](exe/cpp-lan-source-owner-1366-20261126.md).

CP1367은 일반전투의객체Chaff와 이전Fort에서남은 Client장기후처리chaff/special/fort를 모두현재일반모드false로 맞췄다. Chaff만 교정한중간실행은실패했으며special이남으면RawIslandRuntime.Matches가owner를무시해다른참가자의같은Terr까지소비했다(stream1,pos21,size21). 최종두참가자시작/대기/종료0과같은명단/ID/Ready·SP6500·7660객체를확인했다. DN도새지역상태false검사뿐아니라 이전Fort→일반LAN의같은장기owner로검증한다. [CP1367](exe/cpp-lan-fort-modes-1367-20261126.md).

**원문상태17/18 후속:** 호스트종료뒤상대가서버인수15/16을진행하다17에서명시실패했다. 보호PE4b7cd5/4b7cf3와원문4b6dd0에서Reconnection accepted/denied(레벨1d),17의즉시14,18의sourceType0/next3을확인했다. CP1369에서 구현/실제 서버 인수를 확인했다. DN 분배기의 누락은 아래1369 정적 확인을 따르며 새 참가자 재접속은 아직 CP 잔여다.

| 추가 ID | 재현/정적 대응 | DN 다음 작업·완료 기준 |
| --- | --- | --- |
| DN-MP-06 | CP1367 별도 실행의 간헐 Archer 오류와1368 중간 실행의 월드 역행 거부를 확인했다. DN `Netstorm.Client/Mission/RawCampaignArcherBindings.cs:135`의 Delta<0, `Netstorm.Core/Original/RawSurfaceWorld.cs:786`의 Game<이전Game도 정적으로 확인했다. CP1368은 원문00460e90/00467c80에 맞춰 두 부호 제약을 교정하고 집중21/21·실제60초 이상 반복2회/각 두 정상 종료를 통과했다. | R5/R6 프레임과R11 시계 보정을 함께 연결할 때 두 경계를 교정한다. 원문은 부호 있는 차분을 보존하고 Archer의 비양수 적분만 건너뛴다. NaN/무한대 거부·상태 보존은 유지하고 시각/경과를 임의 고정하거나0으로 제한하지 않는다. 공유 주소·보정 증가/감소·정지/재개·인수 경계와 실제 두 참가자 반복/긴 대기·단일 미션 회귀를 확인한다. DN 실행 재현/교정 완료로 세지 않는다. [CP1368](exe/cpp-lan-clock-1368-20261126.md). |

DN의 제품 기준1339·마지막 완료1.076·기존 검사 수/진행률을 이 기록만으로 올리지 않는다. 완료 표시는 실제 코드 교정과 해당 단위 검증 뒤에만 한다. 원본/fixture·AGENTS.md·README 개요는 변경하지 않았다.

1368 확정: 원문 시간 계약과 실제 두 참가자 반복 결과를 DN-MP-06에 반영했다. 상태17/18·서버 인수는 CP1369에서 검증했고 DN-MP-04에 반영했다.

1369 확정: DN `Netstorm.Core/Original/RawLocalMissionState.cs:127`~131은15와4/12/14/16만 처리하고17/18은 InvalidState로 보낸다. DN-MP-04에서 원문 accepted/denied 레벨1d 로그·공통144BYTE 확정과 즉시14/출처0+next3을 복원한다. CP1369은 집중14/14와 실제 호스트 종료 뒤 상대15→16→17→14·공유 권한1/1·수락 로그·상대 정상 종료0을 확인했다. 로그는 표준 tracelevel29 선택/정상 종료 flush 뒤 확인한다. DN 실행 완료로 세지 않는다. [CP1369](exe/cpp-lan-reconnect-1369-20261126.md).

| 추가 ID | CP 재현·원문 의미 | DN 다음 작업·완료 기준 |
| --- | --- | --- |
| DN-MP-07 | CP1370의 실제 LAN에서 사제→Construct→Wind 신전5000→유효 배치 클릭 뒤 `전투 표시 갱신` 콜백 누락으로 종료됐다. 원문004473e0의 해당004b54e0은 UI 갱신이 아니라 비용 차감 뒤 복구 상태 저장이다. 미션은 missionPost 관문으로 이 호출을 생략해 기존 미션 검사에서 드러나지 않았다. | R6 커서 Drop/건설과 R11 일반 LAN을 연결할 때 비용 차감→비용 표시→battle이며!missionPost이면 기존 공통 복구 작성자 순서를 보존한다. DN에는 현재 Input/CursorDropState.cs 데이터만 확인했으며 같은 본체 결함 재현으로 세지 않는다. UI redraw/no-op으로 대체하지 말고 같은 SP/Player·저장 슬롯/사용자 파일을 사용한다. 실제 일반 LAN 배치 후 SP와 저장 파일 값을 확인하고 미션 자연 반환·로그인/수신 저장 작성자 공유도 검증한다. CP1370은 같은 공통 저장 연결·집중12/12·실제 양쪽 신전 객체/SP1500·호스트 저장 파일owner1/SP1500·두 종료0을 확인했다. [CP1370](exe/cpp-lan-cursor-save-1370-20261126.md). |

1370 마감/일시 중지 인계: DN-MP-07의 저장 작성자는 기존 `Host/HostSession.SavedState.cs`·`Original/RawSavedState.cs`와 실제 장치를 공유한다. CP의 실제 신전 자산명은windVortex이며 단순temple 문자열 필터로 건설 성공을 판정하지 않는다. DN은 계속 다음 작업으로 남기고 제품1.076·진행률·검사 수를 올리지 않는다.

1373의 DN-MP-05 참고 검증: CP의 새 두 Fort/실시간 TCP에서 Wind 신전 완성 뒤 실제 생산 아이콘으로 골렘(sunWalker/TYPE133)을 배치했다. 양쪽 신규 SID15249/owner1/좌표116,206/HP50·SP1500→1100·호스트 복구 저장owner1/SP1100·두 종료0을 확인했다. 비용400은 실제 TYPE 후처리의 운반 기본 단가이며 metadata의 후처리 전 cost0과 구별한다. stockEnabled=0에서는 수량0만으로 취득을 막지 않는다. 이 실행은 DN의 생산/통신 재현으로 세지 않는다. [CP1373 근거](exe/cpp-lan-production-1373-20261127.md)·[봇 사용법](testing/cpp-lan-opponent-guide.md).

1374의 DN-MP-05 참고 검증: CP의 같은 실제 TCP에서 썬 워크샾800 SP와 단일 다리0 SP를 추가 건설해 양쪽 객체/비용·호스트 복구 저장SP300·두 종료0을 확인했다. Factory는 건설 중에도 TYPE/VT가 같으므로 객체 존재로 완료를 판정하지 않는다. 실제 HP3000과 extra의 abstract bit0 해제를 기다린다. 다리는 자산명 bridge만 선택하고 별도 bridgeConnector를 섞지 않는다. 원본 pattern0은 신규 bridge1개이며 HP가 없는 타입이므로 보고HP0이 정상이다. DN 재현/완료로 세지 않는다. [CP1374 근거](exe/cpp-lan-extra-build-1374-20261127.md).

| 추가 ID | CP에서 확정한 원문 계약 | DN 후속 |
| --- | --- | --- |
| DN-MP-08 | CP1375의 실제 새 프로세스 복구에서 ①00461090의 time32−0x32c85780 기준 초/초 변환 전 DWORD 밀리초 감김 누락 ②State13의004477d0 로컬 known/Editor/SP/HUD 장치 누락 ③Residence PostPop의005c85d4(SavedGame)를 애니메이션 bool로 오인한 미결선을 교정했다. 복구 frame13에서 테마12를 중복 가산해25가 되는 실제 오류도 확인했다. | R11의 기존 참가자 재시작·저장 복구에 같은 실제 시계/Player/SP/SavedGame을 연결한다. Native reader가 사용하지 않는 저장 SP 필드를 새 정책으로 읽거나 SP·ACK·프레임을 주입하지 않는다. SavedGame은 Notify 뒤 현재값을 읽으며 독립 fixture 입력과 구별한다. DN 대응 코드 결함·재현·완료로 세지 않는다. |

CP1375는 공유 SID객체6,525개·로컬 생성 표면45개·SP300/6500과 기존 owner2/ID·세 종료0, 집중130/130·튜토리얼 실제 회귀를 확인했다. 로컬 player는 별도 참가자/owner로 확인하고, SID<15000의 island/islandStalag/bridgeConnector는 frame 포함 나머지 행의 다중집합으로 대조한다. 공유 객체는 SID를 유지하며 비동기 그림 프레임만 제외하고 다리 frame/수명은 보존한다. 원본은 알 수 없는 새 ID의 중도 참가를 거부하며, 기존 ID 복구와 새 ID 기능 추가는 별도 범위다. [CP1375](exe/cpp-lan-restart-1375-20261127.md).

1380 사용자 범위 결정(2026-11-27): CP는 원본대로 기존 ID의 재시작·저장 복구만 유지하며 전투 중 새 ID 참가 기능은 추가하지 않는다.1375의 원본 프로토콜 계약은 유지하고 D9 미결 상태를 해소했다. DN 코드/진행률/검증 완료로 세지 않는다.
