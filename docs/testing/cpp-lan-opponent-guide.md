# TCP/IP 로컬 상대 봇 사용법

`tools/cpp_lan_opponent.py`는 실제 `Netstorm.exe` 두 개를 로컬 MCP로 조작한다. 호스트와 상대 봇이 각각 새 Fort·사용자 폴더를 사용한다. 게임 프로토콜은 제품의 원본 복원 코드가 처리하며 Python이 게임 패킷/ACK/Player/월드를 생성하지 않는다. 실제 창이 두 개 열리므로 AGENTS.md의 실행 허용 규칙을 따른다.

```powershell
python -X utf8 tools/cpp_lan_opponent.py --exe cpppj/build/bin/Release/Netstorm.exe --game extracted/cpp-reset-all-1325/game --output extracted/lan-new-run --lobby-only
```

출력 경로는 존재하지 않는 새 경로를 지정한다. 각 참가자의 `user` 하위에 설정·맵·캐시·로그가 저장되고 `.tsv`/`.png`/`mcp.json`/`stderr.log`와 최상위 `result.json`에 관측/결과가 남는다. 실제 사용자 저장소나 기존 원본/설치 파일을 고치지 않는다. 기본 대기실16800·게임16799포트는 `--lobby-port`/`--game-port`로 바꿀 수 있다. 상대 주소는127.0.0.1이며 외부 공식 서버를 사용하지 않는다.

봇은 Multiplayer → 새 Fort/이름 입력 → Host/Join → 실제 양쪽 명단을 확인한다. `--lobby-only`는 여기서 정상 종료한다. 이 옵션이 없으면 Start Battle → 실제 phase14/PlayerKnown → 짧은 대기를 검사한다. 아직 이 경로의 성공이 확인되지 않은 시점에는 전체 LAN 플레이 검증으로 세지 않는다. 단위1364는 대기실을 통과했다.1365는 일반 연결/중앙섬 안내를 연결해 State9/10·중앙섬 생성/Fort 전송까지 진행했으며, 1366은비인계Fort Source반납·수신6콜백/채널해제를연결해위두오류를해소했다(집중37/37). 다음은호스트Fort Objects BYTE범위·상대State17경계이며전체로그인/플레이는미완료다.

입력은 영어1024×768의 실제 라벨과 전용 이름/옵션창의 실제 좌표를 사용한다. `--fixed-frame-ms`나 전투 결과 주입을 쓰지 않는다. 요청30초·작업별 제한 시간과 자기 자식 종료 처리가 있어 응답 없는 게임을 무한 대기하지 않는다. 포획·희생·특정 공격·승리는 이 도구의 기본 검사에 포함하지 않는다.
