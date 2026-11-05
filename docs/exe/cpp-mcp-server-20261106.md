# 4차 로컬 MCP 서버 — 조작·상태·화면 캡처 (단위935)

2026-11-06, HJOW-Athlon(마지막 전체 정밀 디컴파일과 같은 PC, 실제 게임 실행 확인 면제 호스트). 4차 요구(dotnetpj 분석을 위해 게임 내 모든 조작과 화면 캡처·소리 포함 녹화)의 **첫 구현 묶음**이다. 소리 포함 녹화와 16:9 1920×1080은 남아 있다.

## 사용법

`Netstorm.exe`가 stdio MCP 서버가 된다(한 줄에 JSON-RPC 메시지 하나, 프로토콜 2024-11-05/2025-03-26/2025-06-18 협상).

- 배포 배치(실행 파일 옆 `game/`): `Netstorm.exe --mcp --window --no-audio`
- 개발/명시 경로: `Netstorm.exe --run <게임 디렉터리> --mcp --window --no-audio`
- Claude Code 등록 예: `claude mcp add netstorm-cpp -- <Netstorm.exe 경로> --run <게임 디렉터리> --mcp --window --no-audio`
- 사용자 데이터를 격리하려면 환경 변수 `NETSTORM_USER_DATA`를 임시 폴더로 준다(5차 저장 정책).

stdout은 MCP 전용으로 분리한다(게임의 CRT stdout 출력은 stderr로 우회). stdin이 닫히면 게임도 닫힌다. `--mcp`는 `--ui-script`/`--view`와 함께 쓸 수 없다.

## 도구

| 도구 | 설명 |
| --- | --- |
| `netstorm_status` | 프레임 번호, 화면 크기, 단계(phase)/페이지(page) 요약 |
| `netstorm_report` | 게임 내부 관측 보고서(탭 구분 텍스트, 기존 UI 검사 보고서와 같음, `camera_only` 옵션) |
| `netstorm_screenshot` | 현재 논리 화면을 PNG 이미지로 반환(8비트 인덱스 PNG, 원본 팔레트 색 번호 보존) |
| `netstorm_input` | `actions:[{op,arg}]`를 프레임마다 하나씩 입력 큐에 넣고 `wait_frames`(기본 3) 뒤 상태와 화면을 반환 |
| `netstorm_wait_frames` | 지정 프레임 수만큼 진행을 기다린 뒤 상태(선택: 화면) 반환 |
| `netstorm_quit` | 게임 창을 닫고 종료 |

입력 동작(`op`)은 기존 UI 스크립트 엔진과 같은 어휘다: `click`(컨트롤 라벨 또는 `screen:x,y`, 라벨 접두 `hud:`/`world:`/`cell:` 등), `click-if`, `down`, `up`, `up-if`, `right`, `outside`, `pointer`, `esc`, `key`/`keyup`(문자, `enter`, `f5`, `shift+a` 등), `script-command`(`Tell,Blank` 같은 원문 명령), `command`. 검사 전용 `test-*` 동작과 상태 합성 동작은 노출하지 않는다. 한 호출은 최대 200동작이다.

## 구조

- `platform/Json`(파서/직렬화, 깊이 64 제한), `platform/Png`(CRC-32/Adler-32/zlib 고정 Huffman+LZ77/PNG 인덱스·RGBA): 외부 라이브러리 없는 순수 부품이다.
- `app/McpServer`의 `McpProtocol`: 전송과 무관한 프로토콜 처리(initialize/ping/tools/list/tools/call 검증, 알림 무응답, 모르는 도구는 -32602, 모르는 메서드는 -32601).
- `McpServer`: stdin 읽기 스레드가 메시지를 해석해 대기열에 넣고, 게임 스레드가 `beforeInput`/`afterFrame` 훅에서 도구를 실행한다. 입력 도구의 단계는 기존 UI 스크립트의 `steps`에 현재 프레임으로 덧붙여 **같은 실행 코드**를 쓴다. 잘못된 입력(없는 컨트롤 등)은 `Client::inputHookError`로 받아 게임을 죽이지 않고 오류 응답으로 돌린다.
- `Client::RequestClose()`가 원문 종료 경로와 같은 `WM_CLOSE`를 보낸다.

## 검증

- 집중 검사 `netstorm_mcp_parts_tests` 9개 통과(첫 실행부터): 표준 벡터의 CRC/Adler/Base64, 독립 inflate로 zlib 왕복(빈/1바이트/7만 바이트 반복/잡음 포함)과 PNG 구조·CRC·픽셀, JSON 파싱·직렬화·거부 사례(깊이/대리쌍/잡문자), 프로토콜 협상·목록·대기열·오류 응답. 빌드 경고/오류 0.
- 실제 게임 스모크 `tools/mcp_cpp_smoke.py`(게임 창 실행): initialize→tools/list→ping→모르는 도구 거부→status→wait_frames→screenshot(표준 zlib로 해제, 1024×768 색 종류 3, CRC 일치)→report→`AskEditFort,0`으로 `LoadEditable` 페이지 진입→없는 컨트롤 클릭은 오류 응답이고 게임 유지→검사 전용 동작 거부→quit→종료코드 0. 전송된 화면을 PNG로 저장해 확인했고 메인 메뉴가 정확히 나왔다.

## 한계와 남은 4차

1. **소리 포함 녹화 미구현**(다음 단위): 게임 믹서 출력과 프레임을 같은 시간축으로 AVI/WAV에 기록해야 한다.
2. **16:9 최대 1920×1080 해상도 지원 미구현.**
3. 게임 시계는 벽시각 기반이라 프레임 단위 결정적 진행(step)은 제공하지 않는다. `wait_frames`는 실시간 진행을 기다린다.
4. 컨트롤 목록 조회 도구는 없다(`report`와 라벨 규칙으로 대체). 필요하면 후속으로 추가한다.
5. 실제 창이 필요하므로 서버 세션마다 게임 창이 뜬다(AGENTS.md 실행 확인 규칙 적용).
