# TCP/IP 로컬 상대 봇 사용법

`tools/cpp_lan_opponent.py`는 실제 `Netstorm.exe` 두 개를 로컬 MCP로 조작한다. 호스트와 상대 봇이 각각 새 Fort·사용자 폴더를 사용한다. 게임 프로토콜은 제품의 원본 복원 코드가 처리하며 Python이 게임 패킷/ACK/Player/월드를 생성하지 않는다. 실제 창이 두 개 열리므로 AGENTS.md의 실행 허용 규칙을 따른다.

```powershell
python -X utf8 tools/cpp_lan_opponent.py --exe cpppj/build/bin/Release/Netstorm.exe --game extracted/cpp-reset-all-1325/game --output extracted/lan-new-run --lobby-only
```

출력 경로는 존재하지 않는 새 경로를 지정한다. 각 참가자의 `user` 하위에 설정·맵·캐시·로그가 저장되고 `.tsv`/`.png`/`mcp.json`/`stderr.log`와 최상위 `result.json`에 관측/결과가 남는다. 실제 사용자 저장소나 기존 원본/설치 파일을 고치지 않는다. 기본 대기실16800·게임16799포트는 `--lobby-port`/`--game-port`로 바꿀 수 있다. 상대 주소는127.0.0.1이며 외부 공식 서버를 사용하지 않는다.

봇은 Multiplayer → 새 Fort/이름 입력 → Host/Join → 실제 양쪽 명단을 확인한다. `--lobby-only`는 여기서 정상 종료한다. 이 옵션이 없으면 Start Battle → 실제 phase14/PlayerKnown → 자연 대기 → 양쪽 명단/Ready·월드 규모·시작창 소멸과 정상 종료를 검사한다. `--steady-seconds 60`처럼 실제 OS 경과 대기를 선택할 수 있다(기본4초,0~3600초). 실제 프레임120개씩 기다리므로 요청 시간보다 약간 길 수 있다.

입력은 영어1024×768의 실제 라벨과 전용 이름/옵션창의 실제 좌표를 사용한다. `--fixed-frame-ms`나 전투 결과 주입을 쓰지 않는다. 요청30초·작업별 제한 시간과 자기 자식 종료 처리가 있어 응답 없는 게임을 무한 대기하지 않는다. 포획·희생·특정 공격·승리는 이 도구의 기본 검사에 포함하지 않는다.

1367은 일반 Fort 객체/공유 모드를 교정해 두 참가자 시작을 통과했다.1368은 원문 시계 보정의 음수 경과/역행을 Archer와 월드에서 허용하고 실제60초 이상 대기2회/각 두 정상 종료를 확인했다. 새 프로필은 표준 registeredSubId에6356993/6356994를 설정한다. 생산/건설·개별 객체/SP 동기화·상태17/18·서버 인수/재접속은 잔여다. 단순 월드 규모 일치를 개별 객체 전체 일치로 세지 않는다.

`cpppj/build/bin/Release/netstorm_lan_clock_tests.exe`는 창 없이 시계 Reset·Archer 양판본·월드 회귀와 LAN 보정 경계를 실행한다. `--list`로 사례 목록, `--filter CampaignClock`으로 새 관련 사례만 선택한다. 기본 전체는21개이며 기존 보호 자산/fixture를 읽기만 한다.
