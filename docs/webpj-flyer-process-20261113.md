# webpj 공통 비행 프로세스 RawFlyerProcess (WB04-E352, 2026-11-13, 7-P1 비행 유닛 부품)

3.1절 7-P1 첫 항목 "RawFlyerProcess 238줄(+53)"(기준선 `.h` 115 + `.cpp` 164 = 279줄 중 대응표 줄 수 238, 기준선 뒤 `20261107-42`·`20261108-162`·`20261108-205 CP` 세 커밋이 수신·Native 송신 인자·명령 추가)을 `core/raw-flyer-process.js` 로 옮겼다. 맨 오 워(144)·훨리기그(73)·더스트 데빌이 공유하는 **원문 112바이트 비행 프로세스 객체**의 생성/상태 전송/수신/홈 연결/명령 기한/종료와, 세 전용 비행체의 **최초 후처리**(45e6b0 공통 + 45fac0/45ef30 전용)를 담는다. 이후 RawSunAviaryProcess·RawRainAviaryProcess·RawFlyerFrame·RawFlyerTarget·RawSunFlyerFrame·RawWindFlyer 가 이 필드 형식(`FlyerProcessFields`)을 요구한다.

## 1. 규칙

- **`FlyerProcessFields`:** `bytes`(112바이트: +8 dirty·+10 홈 WORD·+12/16/20/24 상태·+32/40 시각 double·+48 inner VT·+52 Form·+56 부모·+64/72 시각·+84 단계·+96/104 전용 타이머)와 훨리기그 꼬리 `sunTail`(24바이트)를 소유한다. 생성자가 건드리지 않는 padding 은 호출자의 입력 그대로이며, `form`/`parent`/`home` 은 각각 inner Form/부모의 하위 WORD, 홈 WORD 이다.
- **`construct(fields, parent, home, speed, parameter)` (0045c340):** 시각 32/40 을 0, dirty 를 0xff 로 두고 inner VT(0x5019ec → 0x509ca8)·외부 VT 0x509cc8·시각 64/72·단계 84 를 쓰며, 서버가 아니면 진단 0x2e3(정상 복귀 시 계속) 뒤 홈 WORD·dirty |= 0x1e·속도·상태 12(WORD)·파라미터 float 를 쓰고 dirty |= 0x60·시각 32 = now·상태 24 = 0 으로 한 뒤 `attach(fields, parent, processType, 8)` 를 부른다. 맨 오 워 전용 표(`specializeRain`)는 부착 뒤의 별도 쓰기다.
- **전용 표 쓰기:** `specializeRain`(0x509f60·inner 0x509f40·타이머 96/104·단계 84), `specializeSun`(0x509f24·inner 0x509f04·타이머 104·꼬리 앞 16바이트·단계 84; +80/+84 는 전용 첫 Frame 이 쓴다), `specializeWind`(0x509f98·inner 0x509f78·타이머 96·단계 84; 후속 RawWindFlyer 가 같은 쓰기를 재사용).
- **수신:** `initializeReceived(fields, theme)` 는 기본 생성자 쓰기만 하고 theme 0=Wind·1=Rain·그 외=Sun 의 전용 표만 쓴다(홈/위치/속도 padding 보존, 첫 Frame 은 호출하지 않음). `readData(fields, packet)` 은 머리 9바이트와 dirty 비트(1=부모 DWORD, 2..128=WORD·WORD·DWORD·DWORD·DWORD·8바이트·8바이트)에 따라 `bytes` 로 복사하고 패킷의 부모 DWORD 를 돌려준다(필드가 모자라면 앞선 복사를 남기고 거부; 외부 VT 가 네 등록 표가 아니면 거부). `destructFields` 는 inner VT 를 0x5019ec, Form/부모 DWORD 전체를 −12345(0xffffcfc7)로 둔다.
- **홈 연결/조회:** `bind(flyer, home)`(0045c1b0)은 TYPE 범위·genus(0x100000) 진단 0x904(정상 복귀 시 계속) 뒤 `findProcess(flyer, processType)` 로 첫 nondead 비행 Form 을 찾아 없으면 자연 반환, 있으면 `fields(form)` 의 dirty |= 2 와 홈 WORD 를 쓴다. `home(flyer)`(0045c100)는 genus 진단 0x8ee 는 무조건, dead/free(상태 비트 1·2)면 0, Form 이 없으면 현재 `checking` 이 켜졌을 때만 진단 0x8f5 뒤 0, 있으면 필드의 홈 WORD 이다.
- **명령/종료/기한:** `returnDelay`(현재 외부 VT 가 기본/Rain 이면 1e7초·Sun 60초·Wind 25초, 그 밖은 거부), `command(fields, 1)` 은 타이머 104 를 `min(남은 시간 + 15, 60) + now`(NaN 은 60, 음수 하한 없음), 2 는 `returnDelay + now` 를 시각 40 에, 3 은 시각 40 을 double 0 으로, 그 밖은 무동작이다. `finish`(45bc30)는 서버가 아니면 진단 0x41d 뒤 `pieces(부모)` → `destroy(부모, 8)` 이고 파편 효과 뒤 필드의 부모가 바뀌면 그 현재 부모를 다시 읽는다. `frame`/`notify`/`parentDying`(큰 하위 몸체)은 훅으로 위임하며 없으면 조용히 생략하지 않고 거부한다.
- **`writeProcess(fields, packet, includeParent, force, protocol, message)` (0045c310→0045aed0):** 부모 DWORD 는 (`includeParent` 또는 `force`)이고 부모가 0 이 아닐 때만 쓰고(그때 dirty |= 1), `force` 는 dirty 를 0xff 로 한다. **선택 필드들의 총 길이(9 + 부모 4 + 필드 폭 합)를 쓰기 전에 검사**하고 모자라면 필드를 바꾸지 않고 거부한다. 쓰고 나면 패킷 길이 WORD·dirty 바이트를 쓰고 필드 dirty 를 0 으로 되돌리며 길이를 돌려준다. 8바이트 필드는 바이트 복사라 NaN/미정 값도 보존한다.
- **`FlyerProcess`:** `SquidProcess` 파생. 필드를 소유하고 `runFrame`/`notify`/`onParentDying` 을 로직에 전달하며, `onAttached` 와 `writePacket` 이 Host 의 현재 Form/부모를 inner 에 기록한다(부착 전 writer 도 같다).
- **`RawFlyerPostPop` (45e6b0 + 전용):** 외부 VT 0x50a388(맨 오 워)·0x50a1e8(훨리기그)·0x50a050(더스트 데빌)만 처리한다. 더스트 데빌: flags bit0 이고 event 0 Regular 가 없고 확보가 되면 event 0·페이로드 0.25 Regular 를 만든다. 훨리기그(서버): 홈(+12 WORD)을 Unpop → 홈 프레임 10 → Repop 하고 전용 프로세스(속도·0.6)를 만들며, 서버·클라이언트 모두 impact/active 소리를 요청한다. 맨 오 워(서버, 훨리기그·더스트 데빌 아님): Rain 프로세스(속도·0.5)를 만든다. 공통: 프레임이 TYPE 프레임 수보다 작고 TYPE flags1 0x400000 이면 그림자를 만들어 프레임(프레임 수 + 부모 프레임)·+40 의 8 비트·부모의 그림자 WORD 를 쓰고 부모 좌표에 Pop 한 뒤 단위를 초기화하며, 그림자가 있으면 프레임을 먼저 쓰고 Unpop → 현재 좌표 + 오프셋 두 값으로 Pop, 마지막은 Damageable 후처리이다. 입력 TYPE/보충 표는 만들 때 사본이다.

## 2. 웹 범위와 기준선 뒤 변경

- 10.72(CD)는 범위 밖이다(PATCH 188칸 TYPE 만 받는다). 기준선 뒤 세 커밋의 변경(수신 `initializeReceived`·`readData`·`destructFields`, `returnDelay`·`command`, writer 의 `protocol`/`message` 인자, 데이터 필드 표 상수)을 모두 반영했다(PORT_MAP base `2b5ee903`).
- 전역은 현재 값을 읽는 함수 묶음 `{ server(), now(), processType(), checking() }`(`checking` 은 숫자/함수/생략=0), 서버는 `RawFlyerPostPop` 생성자의 함수이다. 훅 구현(부착·Frame·통지·파편·삭제와 후처리의 생성/공간/소리/Regular)은 후속 단위와 4-D/6 단계 조립 몫이다.
- 이 모듈은 DOM 을 쓰지 않는다.

## 3. 검사 (`tests/raw-flyer-process.test.mjs` 18건)

- **원본 독립 관측:** `flyer-postpop-x86.tsv` 의 1,421행 — 생성자 C 38행(기본 표 부착 이전의 모든 쓰기와 padding/현재 전역 16바이트), 후처리 P 329행(맨 오 워 확장·그림자 생성/재배치·공통 단위 초기화의 호출 순서와 실제 객체 250바이트·부착 직전 이미지), 홈 연결 B 30행(첫 nondead Form 조회와 WORD 홈/dirty 쓰기), 상태 writer W 1,024행(모든 dirty/부모/force 의 64바이트 출력과 길이)이 **첫 실행에서 전부** 일치했다.
- **cpppj 시험 이식:** `Flyer_NativeConstructor38`·`_NativePostPop329`·`_NativeBind30`·`_NativeWriter1024`·`Flyer_RequiredBoundaries`(선택 누락은 앞선 쓰기를 남기고 거부, 짧은 패킷은 사전 거부).
- **웹 계약:** 수신용 기본 생성자의 theme 별 전용 표·padding 보존, 수신 복사의 dirty 순서/부분 쓰기/관찰자 표, 해제 표식, `returnDelay`·`command` 의 기한 계산(NaN·음수·상한), 종료 순서와 부모 재조회, 홈 조회/연결의 진단 순서, `FlyerProcess` 의 Kernel 전달과 Host 번호 기록, 더스트 데빌·훨리기그 후처리 분기, 상태 패킷의 쓰기 전 길이 검사(force 부모 포함 45바이트 경계)와 필드 불변.
- 시험이 먼저 실패한 곳은 모두 시험 쪽이었다: force 패킷 길이(부모 0 이면 41바이트), free 상태는 `pool.physical` 로만 쓸 수 있음. 모듈은 바뀌지 않았다.

## 4. 변형 확인과 남은 경계

- 한 문자열 치환 변형 88회(1차 75개 + 재확인 13개)를 적용했다. 1차에서 12개가 남았고 11개는 **시험의 공백**이라 보강해 잡았다: free 상태의 홈 조회, writer 의 force 가 가정한 총 길이(부모 4바이트 포함, 44·41·20·9 바이트 거부와 필드 불변), `writePacket` 의 Host 번호 기록, 더스트 데빌 Regular 페이로드 0.25 와 Rain 프로세스 비요청, 그림자 프레임을 Unpop 전에 쓰는 순서, 비대칭 오프셋 두 값의 각각 더하기, 입력 TYPE/보충 표 사본.
- 남은 동등 변형 1개: `readData` 의 `offset > packet.length ||` — 같은 조건이 `width > packet.length − offset`(음수 우변)에 이미 포함된다.
- 변형하지 않은 방어선: TYPE 범위 오류 메시지, 훅 누락 메시지 문구.
- 앱 수명 연결(실제 부착/Frame/통지/파편/삭제와 후처리 훅 구현)은 후속 7-P1 단위와 4-D/6 단계 조립 몫이다. 다음 7-P1 항목은 **RawSunAviaryProcess**(214줄 +20)이다.
