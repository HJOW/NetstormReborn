# TCP/IP 로컬 상대 봇 사용법

`tools/cpp_lan_opponent.py`는 실제 `Netstorm.exe` 두 개를 로컬 MCP로 조작한다. 호스트와 상대 봇이 각각 새 Fort·사용자 폴더를 사용한다. 게임 프로토콜은 제품의 원본 복원 코드가 처리하며 Python이 게임 패킷/ACK/Player/월드를 생성하지 않는다. 실제 창이 두 개 열리므로 AGENTS.md의 실행 허용 규칙을 따른다.

```powershell
python -X utf8 tools/cpp_lan_opponent.py --exe cpppj/build/bin/Release/Netstorm.exe --game extracted/cpp-reset-all-1325/game --output extracted/lan-new-run --lobby-only
```

출력 경로는 존재하지 않는 새 경로를 지정한다. 각 참가자의 `user` 하위에 설정·맵·캐시·로그가 저장되고 `.tsv`/`.png`/`mcp.json`/`stderr.log`와 최상위 `result.json`에 관측/결과가 남는다. 실제 사용자 저장소나 기존 원본/설치 파일을 고치지 않는다. 기본 대기실16800·게임16799포트는 `--lobby-port`/`--game-port`로 바꿀 수 있다. 상대 주소는127.0.0.1이며 외부 공식 서버를 사용하지 않는다.

봇은 Multiplayer → 새 Fort/이름 입력 → Host/Join → 실제 양쪽 명단을 확인한다. `--lobby-only`는 여기서 정상 종료한다. 이 옵션이 없으면 Start Battle → 실제 phase14/PlayerKnown → 자연 대기 → 양쪽 명단/Ready·월드 규모·시작창 소멸과 정상 종료를 검사한다. `--steady-seconds 60`처럼 실제 OS 경과 대기를 선택할 수 있다(기본4초,0~3600초). 실제 프레임120개씩 기다리므로 요청 시간보다 약간 길 수 있다.

입력은 영어1024×768의 실제 라벨과 전용 이름/옵션창의 실제 좌표를 사용한다. `--fixed-frame-ms`나 전투 결과 주입을 쓰지 않는다. 요청30초·작업별 제한 시간과 자기 자식 종료 처리가 있어 응답 없는 게임을 무한 대기하지 않는다. 포획·희생·특정 공격·승리는 이 도구의 기본 검사에 포함하지 않는다.

1367은 일반 Fort 객체/공유 모드를 교정해 두 참가자 시작을 통과했다.1368은 원문 시계 보정의 음수 경과/역행을 Archer와 월드에서 허용하고 실제60초 이상 대기2회/각 두 정상 종료를 확인했다. 새 프로필은 표준 registeredSubId에6356993/6356994를 설정한다. 1369에서17/18과 실제 서버 인수를 확인했다. 1370은 Wind 신전 한 건의 실제 객체/SP·복구 저장을 확인했다. 유닛 생산·추가 건설·이탈 뒤 새 참가자 재접속은 잔여다. 단순 월드 규모 일치를 개별 객체 전체 일치로 세지 않는다.

`cpppj/build/bin/Release/netstorm_lan_clock_tests.exe`는 창 없이 시계 Reset·Archer 양판본·월드 회귀와 LAN 보정 경계를 실행한다. `--list`로 사례 목록, `--filter CampaignClock`으로 새 관련 사례만 선택한다. 기본 전체는21개이며 기존 보호 자산/fixture를 읽기만 한다.

`--takeover`는 시작 검사 뒤 이 도구의 호스트 자식만 강제 종료해 실제 소켓 장애를 만든다. 상대의 client 권한0/0→server/boss1/1·로그인14/PlayerKnown과 추가120프레임을 확인하고 정상 종료 뒤 실제 수락 로그도 검사한다. 표준 `tracelevel=29`를 선택한다. 호스트의 예상 강제 종료는 `hostTermination`에 별도 기록하며, 상대는 종료0이어야 한다. `--lobby-only`와 함께 쓸 수 없다. 물리 LAN/다른 사용자 프로세스에는 접근하지 않는다.

재접속 상태의 창 없는 검사는 `cpppj/build/bin/Release/netstorm_cd_presence_tests.exe --filter LocalMissionState`로 실행한다. 인자 없이 실행하면 기존 CD/경로 회귀를 포함한14개를 검사하며 보류된 CD 복원 작업을 확장하지 않는다.

1370의 커서 비용 차감/복구 저장 경로 회귀는 `cpppj/build/bin/Release/netstorm_cursor_drop_tests.exe`로 실행한다. 창 없이 기존 커서/파일 경로12사례를 검사하며 `--list`·`--filter Cursor`도 지원한다. 실제 Wind 신전 한 건의 객체/SP/저장 파일과 두 정상 종료는1370 원시 TCP 관측으로 확인했다.

1373에서 `--produce-golem`을 추가했다. 실제 사제 메뉴로 Wind 신전을 완성한 뒤 `production:133`의 실제 아이콘을 클릭하여 골렘 한 개를 배치한다. 같은 객체의 SID/자산/owner/좌표/HP·양쪽 SP400 차감·호스트 복구 저장을 대조한다. 기본1024×768/영어와 표준 시작6500 SP를 사용하며 현재 보이는 메뉴와 실제 커서 허용 후보만 선택한다. `--lobby-only`·`--takeover`와 함께 사용할 수 없다.

```powershell
python -X utf8 tools/cpp_lan_opponent.py --exe cpppj/build/bin/Release/Netstorm.exe --game extracted/cpp-reset-all-1325/game --output extracted/lan-production-new --produce-golem
```

골렘 생산의 관련 창 없는 검사는 `cpppj/build/bin/Release/netstorm_selection_key_input_tests.exe --filter InventoryItemGump`다. 실제 자산/아이콘/트리 클릭과 독립 원본 입력2사례를 검사하며1373에서 기존 커서12사례와 함께 통과했다. 실제 TCP에서 골렘HP50/SP1500→1100·두 정상 종료를 확인했다. 추가 건설과 새 참가자 재접속은 아직 잔여다.
