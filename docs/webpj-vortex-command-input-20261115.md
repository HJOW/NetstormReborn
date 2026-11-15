# 신전 명령·Descend·ViewMeta 일반 입력 이식 (WB04-E536)

6단계 "게임 메뉴 명령 표(`GameMenuCommands`)"가 이름표 조회로 쓰는 `client/VortexCommandInput`을 `webpj/www/core/vortex-command-input.js`로 옮겼다.
기준선 `225553df` 이후 cpppj 소스 변경은 없다. 호출자는 cpppj `UberGump`(일반 입력 분배)이며, 웹의 분배 조립은 5-C의 Uber 허브 단위에서 연결한다.

## 옮긴 범위

- `vortexCommandScriptCode(name)`: 보호 PE 이름표(542f00/542f68/5430c0/542c20)의 `WipeOut`(0x0300006f)·`Ascend`(0x0300007c)·`Descend`(0x030000a7)·`ViewMeta`(0x02000013)를
  ASCII 대소문자 무시로 조회한다. 별칭·부분 일치·NUL 뒤 글자는 미등록(0xffffffff)이며 공통 조회를 이어간다.
- `dispatchVortexCommandInput(event, command)`: 수정키를 지운 코드가 세 신전 명령이면 소비한다. 뗌은 효과 없이 소비,
  `y`의 낮은 WORD가 대상 SID(0이면 장치를 요구하지 않고 소비), 높은 WORD가 확인 인자로 `command(target, event.code 전체, parameter)`에 전달된다(반환값 무시).
- `dispatchViewMetaInput(event, command, hooks)`: 현재 ViewMeta 명령(호출 순간 DWORD)만 소비한다. 뗌은 소비, `y >> 16`(산술)이 signed 서버 WORD이며
  `-1`은 허용·`-2` 이하는 서버 개수를 읽지 않고(단락) 거부, `서버 >= serverCount()`도 거부해 `invalidServer`(19번 진단 뒤 curServer "0" 쓰기는 소비자 몫)를 부르고,
  그 밖은 낮은 WORD 지역으로 `descend`를 요청한다. 서버 선택 전역은 여기서 쓰지 않는다.
- 선택된 분기에서 도달한 장치만 요구하며 미연결은 `Error`(`/미연결/`)로 거부한다.

## 시험

`webpj/tests/vortex-command-input.test.mjs` 3건. 원문에 이 분배기만 따로 관측한 fixture가 없고 cpppj에도 전용 시험이 없어(호출자 시험에 간접 포함),
**cpppj 소스의 분기·단락·WORD 폭을 직접 대조하는 계약 시험**이다: 이름표(대소문자·별칭·NUL), 세 신전 명령의 WORD 분해·수정키 제거 비교·
음수 `y` 하위 32비트·대상 0 무장치·뗌, ViewMeta의 서버 WORD 경계(−1/−2/상한)·단락 평가·지역 WORD 마스크·장치 누락.
**변형 확인:** 서버 하한·산술 시프트·대상 0 보호·확인 인자 폭·뗌 처리·상한 경계·지역 마스크·명령 코드 전달·이름 코드·수정키 마스크·`asciiLower` 제거 등 12가지를
넣어 시험이 모두 실패함을 확인했다. manifest 항목은 없다(fixture 없음).

## 남은 경계

- `GameMenuCommands`가 이 이름표를 공통 조회에 묶는다(다음 단위). 실제 객체 명령 부모(`Session.CommandObject`)·`RequestFortDescend`·진단 로그는 아직 연결하지 않았다(PORT_MAP `port`).
