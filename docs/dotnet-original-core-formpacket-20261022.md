# 원본 월드 코어 W03 열두째 조각 — Form 패킷(RawFormPacket)

2026-10-22, 작업 PC **HJOW-Athlon**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W03(원본 월드 코어 ② 삭제·수명)** 의 열두째 조각이다.
열한째 조각에서 Form 생성·지정 Take·ContentForm·감시 쌍 수명을 옮겼고, 이번에는 그 Form 들이 바깥으로 내보내는
**가상 +0x54 데이터 패킷**과 **공통 삭제 가상 +0x58 삭제 패킷**의 실제 바이트 작성 몸체를 옮겼다.
[현행 계획](../LEFT_JOBS.dotnetpj.md#dotnet-current-plan-20261022)의 "바로 다음 작업 2번" 가운데 남아 있던 `RawFormPacket`(`form-packet`) 부분이다.

근거는 커밋된 cpppj 소스(`RawFormPacket.cpp`·`RawFormPacket.h`)와 원본 fixture(`form-packet-x86.tsv`)뿐이다.
새 디컴파일 산출물을 읽지 않았고 원본/클론 게임·창 실행도 없다. 현재 PC는 마지막 전체 정밀 디컴파일 PC
(**VM-W11-Codex, 2026-10-22**)와 다르지만 디컴파일 소스를 분석하지 않았으므로 재디컴파일 대상이 아니다.
마지막 디컴파일 PC 기록은 그대로다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다.
이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawFormPacket.cs` (신규) | `FormPacketHeader`·`FormPacketHeaders`·`FormDeletionStamp`, `WriteData`·`WriteDeletion` | `RawFormPacket.cpp` 의 004ae560/004aeb40(데이터 writer)·004ac340(삭제 writer)·004df590(헤더 erase0) |
| `X86RawFormPacketTests.cs` (신규) | `form-packet` 10.78 몫 576행 재생 1건 + 합성 6건 | `cpppj/tests/FormPacketTests.cpp` 의 `CheckFormPackets`·`ShortDeleteBufferPreservesStamp` 및 계약 검사 |
| `Netstorm.Core.Tests.csproj` | fixture 링크 1건, `cpp-fixture-manifest.tsv` 갱신(53→**54**개) | `cpp_sync.py --update --add form-packet:Core` |

### 헤더

원본 등록 표의 헤더 세 값(메시지 BYTE, 프로토콜 BYTE, 선언 길이 WORD)은 writer 가 호출마다 현재 값을 읽는다.
원본 erase0 경로는 다음 순서로 패킷 +0..3 만 쓴다.

1. 물리 버퍼가 반환 크기보다 짧으면 **아무것도 쓰기 전에** 거부한다.
2. +0 에 프로토콜의 bit7 을 지운 값(bit6..0 만)을 쓴다.
3. +1 에 메시지를 쓴다.
4. +2 에 선언 길이와 4 중 큰 값을 쓴다. 선언 길이가 1~3 이어도 4 가 기록된다.

반환 크기 뒤의 버퍼 바이트는 보존한다(고정 64바이트 버퍼의 나머지를 덮지 않는다).

### 데이터 writer (`WriteData`, 13 또는 18바이트)

- 현재 슬롯 +0 의 가상 표로 writer 를 고른다. 10.78 감시 Form 은 `0x512110`, 내용물 Form 은 `0x512208` 이며
  다른 가상 표는 writer 선택 전에 거부한다.
- 부모(+16 WORD)를 읽을 때도 현재 가상 표가 네 Form 생성자(타입 1·2·3·6) 중 하나인지 확인한다.
  free/dead/void 상태 필터는 더하지 않고, 타입 BYTE 가 프로세스 번호여도 같은 생성자 가상 표로 본다.
- 부모 번호가 일반 client 번호(5~14999)면 원본 assert 경로로 거부한다. 0~4 번은 원본처럼 client 로 분류하지 않는다.
- **감시 writer(13바이트):** 서버 풀이 아니면 거부한다. +18/+20 의 상대 Form 이 0 이 아니면 그 번호와 그 부모도
  server 번호여야 한다. 출력은 `sid, 부모, 상태 BYTE, +18 WORD, +20 WORD` 다.
- **내용물 writer(18바이트):** `sid, +12 WORD, +18 WORD, +20 WORD, +24 WORD, 부모 WORD, +23 BYTE(순번), 상태 BYTE` 다.
  10.78 의 +24 WORD 와 +23 BYTE 를 읽는다.
- writer 는 입력 슬롯을 바꾸지 않는다(원본도 읽기 전용).

### 삭제 writer (`WriteDeletion`, 8바이트)

- SID 를 먼저 client 번호 검사한 뒤 헤더와 SID 를 쓴다.
- 최근 삭제 표식의 SID 가 이 SID 와 같을 때만 표식 SID 를 0 으로 소비하고, 표식 token 은 그대로 둔다.
- 출력 플래그 WORD 는 `(flags >> 16) & 0xfff0` 과 소비한 token 의 낮은 WORD 의 합집합이다.
  SID 가 다르면 token 없이 같은 마스크만 쓴다.
- 버퍼가 7바이트 이하이면 헤더 단계에서 거부하며 표식과 버퍼는 바뀌지 않는다.

## 2. 검증 결과

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,295개 통과**(Assets 363 + Core 932; 이번 +7).

| 검사 | 내용 |
| --- | --- |
| `form-packet` 10.78 몫 576행 재생 | 매 행마다 Form 슬롯 50바이트·패킷 64바이트·헤더 세 값·삭제 표식·삭제 플래그를 입력으로 되돌리고, 반환 크기·패킷 전체 64바이트·표식 소비 결과·슬롯 무변화를 대조한다 |
| 합성 6건 | 짧은 삭제 버퍼와 표식 보존·헤더 정규화와 꼬리 보존·client 부모/상대 Form 거부·상대 Form 가상 표 검사·감시 writer 서버 권한·삭제 표식 소비와 플래그 비트 |

- `form-packet-x86.tsv` 는 총 1,152행(2 판본 × 576행)이며 10.78 몫 576행만 재생했다.
  진입 경로는 D(감시)·W(감시 wrapper)·C(내용물)·V(내용물 wrapper)·X(삭제)·Z(삭제 wrapper)가 각 96행이고,
  반환 크기는 13·18·8 이 각 192행이다. wrapper 경로 288행·native 경로 288행이 모두 쓰인다.
- 헤더 기대값에는 프로토콜 bit7 이 켜진 입력(198 등)과 선언 길이 1~3 입력이 포함되어 정규화 규칙이 실제로 검증된다.
- 입력 슬롯 50바이트와 패킷 버퍼 64바이트가 매 행 대조되므로, writer 가 반환 크기 뒤를 덮거나 슬롯을 바꾸면 실패한다.
- `originalCD` 행은 읽지 않는다(기준 판본은 10.78 뿐이다).

**변이 확인:** 구현 12곳을 하나씩 망가뜨려 모두 실패로 잡혔다.

| 변이 | 잡힌 검사 |
| --- | --- |
| 헤더 프로토콜 bit7 마스크 제거 | 원본 재생 |
| 헤더 최소 길이 4 보정 제거 | 원본 재생 |
| client SID 하한 5 제거(0~4 도 거부) | 합성(삭제 SID 경계) |
| 부모 가상 표 검사 제거 | 합성(상대 Form 가상 표) |
| 감시 가상 표 선택 값 변경 | 원본 재생 |
| 내용물 추가 WORD 위치 24→23 | 원본 재생 |
| 내용물 순번 BYTE 위치 23→22 | 원본 재생 |
| 삭제 플래그 0xfff0 마스크 제거 | 원본 재생 |
| 삭제 표식 SID 일치 조건 제거 | 원본 재생 |
| 표식 소비 뒤 SID 유지 | 원본 재생 |
| 감시 writer 서버 권한 검사 제거 | 합성 |
| 감시 상대 Form 서버 번호 검사 제거 | 합성 |

소스는 모두 원복해 같은 검사가 다시 통과함을 확인했다.

## 3. 이번에 하지 않은 것 (경계)

- **방송 요청 wrapper(세 번째 인자·네 번째 요청 플래그)는 옮기지 않았다.** fixture 의 마지막 열은 그 wrapper 가
  쓰였는지만 기록하며, C++ 테스트도 이 값을 native writer 구현의 주장으로 쓰지 않는다. 실제 송신·방송 요청은
  세션 연결 조각의 범위다.
- `GumpFormHost`·`SquidFormDispatch`·파생 Form 분배 허브는 옮기지 않았다.
- 내용물 수명(`ContentFormLifetime`, `contentlifetime`)과 contained 생성기 연결(`RawContainedCreate`)은 옮기지 않았다.
- 신전 삭제, 삭제 보상(SP·지갑·샘 풀) 계산, CD 10.72·10.37 몫, 실제 세션 연결. 이 조각은 L1 모듈 단계이며 L2 이상으로 합산하지 않는다.
- 동기화 목록의 반영 커밋은 **`f95a8db2` 그대로** 두었다. HEAD(`5d34fc2d`) 사이 cpppj 커밋 66개 가운데 이 writer 를 바꾼 커밋은
  없지만, 새 fixture 30개(AI·워커·클라우드 플로터·wind aviary·dissolve 등)가 아직 이식되지 않은 기능을 담고 있어
  ["미반영 동작이 남으면 반영 커밋을 무조건 HEAD 로 올리지 않는다"](dotnet-cpp-port-plan-20261016.md#7-cpppj-진행을-따라가는-절차-다른-ai가-cpppj를-계속-진행-중)는 규칙을 지켰다.
  작업 트리의 미커밋 cpppj 변경은 쓰지 않았다.

## 4. 다음 조각

내용물 수명(`contentlifetime`·`RawContainedCreate`) → 신전 삭제 → 삭제 보상(SP·지갑·샘 풀) 계산 →
파생 분배 허브·실제 세션 연결. 시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를
`--update --add` 로 등록한다.
