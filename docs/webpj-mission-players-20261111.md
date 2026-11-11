# webpj 미션 플레이어 목록(MissionPlayers) — WB04-E321 (2026-11-11, 4-C)

3.1절 4-C 의 **Player**(`o/Player.cpp` 74 + `Player.h` 48 = 기준선 122줄, 기준선 뒤 cpppj 변경 없음)를 `core/mission-players.js` 로 옮겼다. Player.cpp/Totalmade.cpp 의 미션 시작 상태 중 자원·기술·동맹·색만 연결하는 `MissionPlayers`(미션 플레이어 목록과 기술·회수·승급·AI 허용 상태)이다. cpppj 에서는 화면용 사본(10.72 어댑터와 `GameWorld` 표시)이고 실제 Player9 는 raw 모듈이 읽으므로, 웹에서도 같은 위치의 자료 모델로 둔다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다. 이 모듈은 DOM 을 쓰지 않는다.

## 1. 규칙

- `MissionPlayers.load(get, fort, types, fallbackMoney)`: `get(key)` 는 미션 스크립트 머리 값 조회(없으면 `undefined`/`null`, 있으면 문자열이며 빈 문자열도 있는 값; 문자열이 아니면 거부), `fort` 는 `parseFort` 결과(저장 기술/덱을 복사), `types` 는 `createFortTypes` 결과(`records` 188개). 조회 순서는 `denySalvage`·`denyAscend`·`aiOff`·`techAllowed` 뒤 플레이어 1~8 순이며 사람(1번)은 이름을 읽지 않는다(`Player` 고정).
- 숫자 머리 값은 **C `strtod`** 처럼 앞의 숫자를 읽는다(`cStringToDouble`): 앞 공백(공백·탭·LF·VT·FF·CR)·부호·`inf`/`infinity`/`nan(…)`·16진 실수(`0x1.8p1`)·십진 실수(`1e3`, `.5`, `5.`), 숫자가 없으면 0, 범위를 넘으면 ±Infinity, 첫 NUL 에서 멈춘다. `denySalvage`/`denyAscend`/`aiOff` 는 값이 0 이 아니면 참(NaN·inf 포함, -0 은 거짓). 시작 SP 는 머리 값이 있으면(빈 문자열 포함) 그 값을 읽고, **값이 아예 없을 때만** 사람은 대체 SP(`fallbackMoney`)를, AI 는 0 을 쓴다.
- 기술 허용표(`techAllowed`): 길이 = 타입 레코드 수, 처음 전부 허용. `allow`/`deny` 토큰이 상태를 바꾸고 `all` 은 표 전체를 현재 상태로 채우며 그 밖의 토큰은 **모든 레코드의 이름**(자산이 아닌 내장 타입 포함)을 ASCII 대소문자 무시로 비교한 첫 일치 번호(0 보다 큰 것)에 현재 상태를 쓴다. 토큰은 다듬지 않고(` all` 은 다른 이름), 빈 조각에서 읽기를 끝낸다.
- 플레이어 1~8: 번호·색(기본 번호)·`computer`(1번만 거짓)·동맹 비트(자기 비트). 이름은 사람 `Player`, AI 는 `ai<번호>Name`(없으면 빈 문자열이고 비활성). **번호 있는 키가 없을 때 2번만** 번호 없는 구식 `ai…` 키를 대신 읽는다(번호 있는 값이 빈 문자열이어도 있는 값이라 대체하지 않음). 시작 기술 `myTech`/`ai<번호>Tech`, 능력 `ai<번호>Ability`(사람도 `ai1Ability` 를 읽음), 색 `ai<번호>Color`(색 이름 8종, 대소문자 무시, `none`·알 수 없는 이름은 번호 색 유지), 동맹 `myAllyList`/`ai<번호>AllyList`.
- 시작 지식: 세미콜론 목록의 `all` 은 타입 레코드 70 부터 **자산 레코드 중 `group < 9`**(생산 그룹 0~8과 알 수 없는 그룹 -1, NO_GROUP 10 과 misc 9 제외), 이름 토큰은 0 보다 큰 번호만 모아 중복 없이 오름차순이다.
- 동맹 목록은 `ConfigParseLong` 규칙(32비트로 감김)으로 읽어 1~8 만 더한다. 적재 마지막에 한쪽에만 지정된 동맹을 양쪽 비트로 대칭화한다(전이 없음). `allied(a, b)` 는 중립/범위 밖/정수가 아닌 번호는 거짓, 두 방향 중 한쪽 비트만 있어도 참(자기 자신은 참)이다.
- 저장 기술(`storedTechnology`)·덱(`storedDeck`)은 사람 1번에만 `structuredClone` 복사로 두어 원본 Fort 와 독립이다. 생산 덱 재구성은 다음 단계의 몫이다.

## 2. 기준선 뒤 변경 판정

- 기준선 `1f00a25a` 뒤 `o/Player.cpp/.h` 를 바꾼 cpppj 커밋이 없다(3.6절 ①). 옮기지 않은 것도 없다. `configListItem` 은 `raw-mission-ai-settings.js` 의 기존 구현을 재사용하며(같은 의미) 새로 만들지 않았다.

## 3. 검사 (`tests/mission-players.test.mjs` 10건)

- 원본 관측 fixture 가 없는 모듈이라 기대값을 모두 손으로 계산했다: 머리 값이 없을 때의 기본 상태와 **조회 순서 전체**, `strtod` 판독표(공백류·부호·지수·16진·inf/nan·범위·NUL·1e-400)와 참/거짓, 사람·AI 의 이름·SP·기술·능력과 구식 `ai` 대체, 색 이름, 동맹 목록·대칭화·`allied`(수동 상태 포함), 기술 허용표(토큰 상태·대소문자·중복 이름·공백 토큰), 시작 지식(all/이름/중복/정렬/자산 아님/70 앞), 저장 기술/덱 복사, 입력 검사와 기본 생성.
- **실제 타입 표**(`netstorm.tarc` 의 `.type` 116개)로 `all` 지식을 정의의 `group` 속성 이름 표로 독립 계산한 값과 대조하고, `bridge`=82·`sunArcher`=71 이름 조회와 `deny;all;allow;sunArcher` 허용표를 확인한다.

## 4. 변형 확인과 남은 경계

- 변형 112개 중 첫 실행에서 20개가 남았고, 시험을 보강한 뒤 **4개만 남았으며 모두 동등**하다: `infinity` 철자 일부만 읽어도 값은 같은 `Infinity`·찾지 못한 타입의 -1 대 0(호출자가 모두 `> 0` 만 받음)·사람(1번)의 활성 조건(이름이 항상 `Player`)·색 이름이 빈 문자열일 때의 조건(어차피 일치 없음).
- 변형 확인으로 조인 경계: `all` 지식의 시작 번호(70)·자산 표시 조건·마지막 레코드(187), 이름 조회의 0·1번 경계, 동맹 대칭화에서 번호가 큰 쪽만 지정한 경우, `ConfigParseLong` 의 32비트 감김(`4294967298`·`-4294967294` 모두 2), 빈 문자열 시작 SP, 입력 검사의 메시지, 중립 자리 비트 무시.
- 3.1절 4-C 는 18/30 이다. 다음: RawConstructionCommand(로컬 건설 요청 0x3e 25바이트와 서버 수신 444af0).
