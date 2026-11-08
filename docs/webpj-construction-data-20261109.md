# webpj 건설 상태 메시지(RawConstructionData) 이식과 기준선 판 정정 — WB04-E141

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백마흔한 번째 부분이다. 건설 프로세스 상태 메시지 0x37 — 서버 쪽 쓰기(`004426d0`)·실제 프로세스 전송 래퍼(`00443bd0`)·읽기(`00442760`)·수신(`004427b0`) — 를 옮긴 `o/RawConstructionData`이다. 이 메시지는 E132 건설 프로세스의 시각 필드를 서버에서 클라이언트로 동기화한다. 선행은 E132 `ConstructionProcess`/`RawConstructionProcess`, 기존 `SquidProcessHost`(`attach`/`fromForm`), `Kernel`이다.

## 1. 옮긴 것

`www/core/raw-construction-data.js`

- `createConstructionDataHeader()` — 헤더 바이트 기본값 `{protocol 1, message 0x37}`(패치 005325b9/005325b8).
- `writeConstructionData(fields, packet, form, parent, force, header)` (004426d0) — 현재 필드를 리틀엔디언으로 쓰고 dirty 를 0 으로 만든다. force 이면 먼저 dirty 0xff. 부모는 인자가 0 이 아닐 때만 쓰며 dirty bit0 도 켠다. 시작/끝 시각은 double 비트 그대로(산술 변환 없음), 추가 바이트는 dirty 비트에 따라 쓴다. 헤더 9바이트(protocol, message, 선언 길이 WORD, form DWORD, dirty). 부족한 버퍼는 쓰기/dirty 변경 전에 거부한다. 반환은 기록한 길이(최대 30).
- `writeConstructionProcessData(process, packet, includeParent, force, header)` (00443bd0) — 실제 프로세스의 부모(`includeParent` 또는 `force` 일 때)·form 으로 쓴다.
- `readConstructionData(fields, packet)` (00442760) — 선택된 시각/추가 바이트만 복원하고 부모 DWORD(없으면 0)를 반환한다. dirty/소리는 보존한다. 선택된 실제 저장 직전에만 길이를 검사하므로 앞선 부분 쓰기는 남는다(시작 시각은 21바이트, 끝 시각은 29바이트부터). `fields=null` 은 시각/추가 비트가 없을 때만 허용한다.
- `RawConstructionData(pool, state, { hooks | logic+host, host, assertion })` — 새 form 부착을 명시 경계(`hooks.allocate`/`attach`/`lookup`)로 두거나, 같은 풀의 실제 `RawConstructionProcess`+`SquidProcessHost` 에 직접 연결한다.
  - `receive(packet)` (004427b0): 헤더 9바이트·form 번호(0 이 아니고 풀 안)를 확인한다. form 이 free 이면 dirty 가 0xff 가 아닐 때 원문 진단 `0x42` 뒤 계속, 새 객체를 만들어(hooks.allocate 또는 실제 `ConstructionProcess`) 필드를 `0xff/0/0` 으로 초기화하고 읽은 뒤 부착한다(hooks.attach(필드, 현재 프로세스 타입, 부모, 캡처한 form, 0x10) 또는 `host.attach(…, 0x10, form)`; 부모 DWORD 가 65535 초과·풀 밖이면 Kernel 객체를 남기지 않고 거부). live form 이면 부모 비트가 있을 때 진단 `0x4a` 뒤, 현재 프로세스를 host(`fromForm`) 또는 `hooks.lookup(Kernel 번호 DWORD)` 로 찾아 필드만 갱신한다(건설 프로세스가 아니거나 없으면 읽기만 끝내고, 필드가 필요한 비트가 있으면 거부). 진단 장치가 없으면 성공을 합성하지 않고 거부한다.

## 2. 검증

`tests/construction-data.test.mjs`(6건):

1. `constructiondata-x86.tsv` 의 `originals` 1780행(`originalCD`·`original1037` 은 다른 판본이라 제외) — Write 1034·Read 512·ReadNull 32·Receive 162·Wrapper 40 에서 사건 순서(A 할당/J 부착/K Kernel 조회)·필드 7항목(dirty·시작/끝 시각 비트·소리·추가)·부모 출력·패킷 전/후 전체·주변 form raw 가 모두 같다(NaN 시각 비트 포함).
2. 시작의 전체 전송(30바이트, dirty 0xff)과 끝 시각의 부분 전송(17바이트)을 실제 host 수신→Kernel 진행(HP 53)→지연 완료(HP 100·abstract 해제)로 잇는다.
3. 부족한 모든 버퍼 길이(0..29)에서 쓰기는 필드/패킷 변경 전에 거부하고, 읽기는 거부하되 앞선 필드만 보존한다. 선언 길이 WORD(0xffff)는 읽기 판단에 쓰지 않는다. `fields=null` 은 비트가 있으면 거부.
4. 진단 장치 없는 부분 상태 free form·live 부모 비트·범위 밖 form·필수 경계 누락·다른 풀의 host/logic 거부(부착 전). 전체 상태 free form 은 부착 경계를 한 번 부른다.
5. 상위 비트 Kernel 번호(`0x10001`)와 건설이 아닌 프로세스(Regular)의 form 은 건설 필드를 덮어쓰지 못한다.
6. 실제 새 소유 수신: 서버 월드의 건설 프로세스(form 서버 영역 +50)를 보낸 전체 상태 패킷이 클라이언트 월드에 같은 form 으로 부착되고(Kernel 1개, 필드 `255/0/8/0x5a`, 소리 0), 부분 상태 free form 은 진단 `0x42` 뒤 계속하며, 범위를 넘는 부모 DWORD 는 Kernel 객체를 남기지 않고 거부한다.

최초 실패와 교정: 새 모듈은 1780행 fixture 를 첫 실행에 통과했다. 시험의 기대(읽기 부분 쓰기 보존)와 cpppj 시험 한 건(오래된 form 거부)은 fixture 로 확정된 실제 의미와 맞지 않아 옮기지 않았다: cpppj `ConstructionData_ActualHostRejectsWrongAndStaleForms` 의 "같은 Kernel 번호를 가진 오래된 form 거부"는 `SquidProcessHost::FromForm` 이 form 번호를 검증하지 않아(원문과 같음) JS 에서도 같은 프로세스가 갱신되므로 이 항목은 시험하지 않았다. cpppj 쪽 확인이 필요한 항목으로 남긴다.

## 3. 기준선 판 정정 (E134~E139 포함)

[LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.0절의 원칙은 "이식 원본은 언제나 기준선(`1f00a25a`) 판"이다. 그런데 E133~E141 에서는 기준선 판을 따로 꺼내 읽지 않고 작업 트리(= 커밋된 HEAD)의 소스를 읽었다. 확인 결과 기준선 뒤에 바뀐 파일은 다음뿐이며, 모두 **실제 wire 경로**(`Actual*`)와 현재 전역을 빌리는 `shared*` 접근자가 추가된 CP 작업이다(기준선 판에는 `ActualHandle`/`ActualPlace`/`ActualConfirm` 이 없음을 `git show 1f00a25a:…` 로 확인).

| 파일 | 옮긴 판(마지막 변경 커밋) | 기준선 대비 변경 |
| --- | --- | --- |
| `RawConstructionPacket` | `77a0c3a4` (20261108-151 CP) | +41/−2 (뷰·초기화 helper) |
| `RawConstructionConfirm` | `77a0c3a4` | +47/−3 (`ActualConfirm`) |
| `RawConstructionNotice` | `976f2b7b` (20261108-145 CP) | +50/−0 (`ActualHandle`) |
| `RawConstructionPlace` | `976f2b7b` | +75/−5 (`ActualPlace`·실제 Player 기록) |
| `RawConstructionEffects` | `4717dff5` (20261107-47 CP) | +15/−10 (공유 전역) |
| `RawConstructionData` | `a942a75a` (20261107-45 CP) | +26/−21 (실제 수신·진단 복귀) |

나머지(`Arrival`·`Selection`·`CanonTypeDecoder`·`CanonDecoder`·`Clear`·`Request`·`Local`)는 기준선 뒤 변경이 없다. 이 여섯 파일의 PORT_MAP `base` 를 옮긴 판의 커밋으로 바로잡았다(Data 포함). 이식한 소스가 기준선 대비 상위집합이고 모든 fixture 재생이 같으므로 판정은 ③(실제 경로 추가·리팩터, 단독 경로 동작 동일)에 해당한다. 향후 단위는 기준선 판과 HEAD 판을 구별해 기록한다.

## 4. 경계와 남은 것

- 서버→클라이언트 분배 등록(`RegisterConstructionData`: 메시지 55, 버전 1, 길이 65535)과 `RawPacketDispatch` 위의 전체/부분 상태 분배 시험은 `RawPacketDispatch` 이식 뒤 옮긴다.
- 10.72(CD) 경로(진단 파일 이름 대소문자)는 범위 밖이다.

## 5. 다음 시작점

`RawConstructionBuilder`·`Command`(선행 `RawPriestPlacementGeometry`·`RawPacketDispatch`), `RawManaBoltArrival`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
