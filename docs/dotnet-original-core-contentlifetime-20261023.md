# 원본 월드 코어 W03 열세째 조각 — 내용물 수명(RawContentLifetime)과 contained 생성

2026-10-23, 작업 PC **HJOW-Athlon**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W03(원본 월드 코어 ② 삭제·수명)** 의 열세째 조각이다.
열두째 조각에서 Form 패킷 writer 를 옮겼고, 이번에는 생산 칸 **ContentForm(타입 6)** 의 삭제 수명
(`ContentFormLifetime`)과 그 내용물을 만드는 **contained 생성기**(`RawContainedCreate`)를 옮겼다.
[현행 계획](../LEFT_JOBS.dotnetpj.md#dotnet-current-plan-20261022)의 "바로 다음 작업 4번" 가운데
`ContentFormLifetime`(`contentlifetime`)·`RawContainedCreate` 부분이다.

근거는 커밋된 cpppj 소스(`ContentFormLifetime.cpp`·`RawContainedCreate.cpp`·`SquidFactory.cpp`), 원본 fixture
(`contentlifetime-x86.tsv`)와 기존 cpppj 합성 검사(`ContentLifetimeTests.cpp`·`ReturnPieceTests.cpp`)뿐이다.
새 디컴파일 산출물을 읽지 않았고 원본/클론 게임·창 실행도 없다. 현재 PC는 마지막 전체 정밀 디컴파일 PC
(**VM-W11-Codex, 2026-10-22**)와 다르지만 디컴파일 산출물을 읽지 않았으므로 재디컴파일 대상이 아니다.
마지막 디컴파일 PC 기록은 그대로다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다.
이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawContentLifetime.cs` (신규) | `ContentLifetimeState`·`ContentLifetimeHooks`·`RawContentLifetime`(`PreDestroy`·`Destroy`·`Hooks`·`IsContentForm`) | `ContentFormLifetime.h/.cpp` 의 004aeeb0/CD 004b0300, 00452fe0·0040abe0·00413bb0·0040eec0·0041df80·004a9d70·004adea0 |
| `RawContainedCreate.cs` (신규) | `ContainedCreateState`·`ContainedCreateHooks`·`RawContainedCreate`(`Create`·`ConnectFactory`) | `RawContainedCreate.h/.cpp` 의 004afa70/CD 004ac4d0, `SquidFactory::CreateContained` |
| `RawSquidFactory.cs` (확장) | `CreateContained(flags)` — 내장 Form 생성기(타입 6)로 위임 | `SquidFactory.cpp` `CreateContained` = `SquidForms(pool).Create(6, flags)` |
| `X86RawContentLifetimeTests.cs` (신규) | `contentlifetime` 10.78 몫 198행 재생 1건 | `cpppj/tests/ContentLifetimeTests.cpp` `ContentLifetime_IndependentPeFixture` |
| `RawContentLifetimeTests.cs` (신규) | 실제 SP 저장소·AI 지갑 연결 1건 | 같은 파일 `ContentLifetime_ActualSpAndWalletIntegration` (10.78 몫) |
| `RawContainedCreateTests.cs` (신규) | 계약 합성 5건(쓰기 순서·기본값·다리 통지·거부·팩토리 연결) | `ReturnPieceTests.cpp`·`FormLifecycleTests.cpp` 가 함께 쓰는 계약 |
| `Netstorm.Core.Tests.csproj` | fixture 링크 1건, `cpp-fixture-manifest.tsv` 갱신(56→**57**개) | `cpp_sync.py --update --add contentlifetime:Core` |

### 삭제 수명 (`RawContentLifetime`)

원본 `ContentFormLifetime` 은 ContentForm 만 자기 몸체로 분배하고 나머지 파생 Form·자산 효과는 fallback 훅으로 넘긴다.
공통 삭제 본체(`RawSquidDestroy`)가 내는 효과를 다음처럼 가로챈다.

| 공통 효과 | ContentForm 일 때 | 그 밖 |
| --- | --- | --- |
| `PreDestroy` | `PreDestroy(form)` (아래 순서) | fallback `Emit` |
| `PostDestroy` | 공통 사후 깊이 반납만 | fallback `Emit` |
| `ReleaseDependent` | 아무것도 하지 않음(부모가 이미 사라짐) | fallback `Emit` |
| `DestroyDependent` | 같은 수명으로 `Destroy(form, flags)` 재진입 | fallback `Emit` |
| 그 밖(전송·선택) | fallback `Emit` | fallback `Emit` |
| `UnpopForm` | `RawContentFormHost.Unpop` | fallback `UnpopForm` |

`PreDestroy` 의 순서는 원본 그대로다.

1. 타입 6·비어 있지 않은 슬롯을 확인한다(반납된 dead 는 허용).
2. 현재 커서(세션 콜백 또는 값형)가 이 Form 이거나 자기 부모(+16 WORD)가 자기 번호이면 **커서 취소**를 요구한다.
   콜백은 슬롯을 바꿀 수 있으므로 이후 분기는 슬롯을 다시 읽는다.
3. 내용물 종류(+18 WORD)가 현재 너겟 타입이면 원본 패치 효과를 수행한다.
   - 부모(+16 WORD)가 범위 밖이면 거부하고, 부모에 죽음 비트가 없을 때만 진행한다.
   - 부모 소유자가 로컬 소유자면 로컬 수입 알림, 아니면 권한이 있을 때만 원격 알림을 보낸다.
   - **선지급 억제 BYTE(+22)가 0 일 때만** 현재 비용을 0 방향 절삭한 AI 지갑 가산 → 현재 소유자·비용·로컬 여부를
     **다시 읽은 뒤** SP 가산 → 발자국 절반을 뺀 중심으로 위치 소리 순서로 진행한다.
4. 공통 사전 깊이를 정확히 한 번 반납한다.

`RawSquidReward` 가 있으면 지갑·SP 는 그 저장소로 연결하고, 없을 때만 `Wallet`·`AddSp` 콜백을 요구한다.
다른 풀의 Form·삭제·보상 모듈과 10.78 이 아닌 타입 표는 생성 시점에 거부한다.

### contained 생성 (`RawContainedCreate`)

원본 004afa70 의 순서와 폭을 그대로 지킨다.

1. 부모가 0 이거나 풀 범위 밖이면 **아무것도 만들기 전에** 거부한다.
2. `flags` 가 DWORD 전체로 0 일 때만 `0x20` 을 쓴다.
3. 부착 플래그가 `0x20`(예측)이면 1, 아니면 `0x10`(클라이언트)이면 2, 둘 다 아니면 0(서버)을 생성 영역으로 넘긴다.
4. 생성 결과 번호가 5 미만이거나 범위 밖이면 거부한다.
5. 저장 타입(+18 WORD) → 인자(+12 WORD) → **현재 슬롯 타입 바이트가 다리 타입(82)이면 표면 통지** →
   다시 받은 슬롯에 품질 BYTE(+30) → 목록 플래그(+20 WORD) → 부착 콜백 순서로 쓴다.

`RawSquidFactory.CreateContained` 는 자산 생성자 표가 아니라 내장 Form 생성기를 써서 가상 표(`0x512208`)와 타입만 쓰고
postCreate 는 부르지 않는다. `ConnectFactory` 는 같은 풀의 팩토리만 받고 타입 6 이 아닌 요청을 거부한다.

## 2. 검증 결과

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,310개 통과**(Assets 363 + Core 947; 이번 +7).

| 검사 | 내용 |
| --- | --- |
| `contentlifetime` 10.78 몫 198행 재생 | 매 행마다 부모(100)·ContentForm(80) 슬롯 50바이트를 입력으로 되돌리고, 반환 값·**효과 문자열**(커서 취소·로컬/원격 알림·지갑·SP·소리·포획)·**부모 알림 문자열**·관찰 슬롯 전체 바이트·**풀 메타 11개**(크기·빈 수·예측 커서·두 영역 머리/꼬리·전체 풀 Adler-32·사전/사후 깊이)를 대조한다 |
| 실제 SP·지갑 연결 | 너겟 삭제 뒤 SP 1,000 → **1,050.75**, AI 지갑 0 → **100**(선지급 50 + SP 가산 50), 로컬 알림 1회·소리 1회(중심 18.5/27.5)·SP 표시 갱신 1회, 부모 알림 1(부착)·2(제거), 부모 종속 머리 0, 클라이언트 반납 기록 타입 6 |
| contained 생성 계약 5건 | 기본 flags `0x20`·저장 타입/인자/품질/플래그 위치·예측 우선순위·다리 표면 통지·부모/생성 번호/경계 거부·같은 풀 팩토리 연결과 타입 6 전용 |

- `contentlifetime-x86.tsv` 는 총 396행(2 판본 × 198행)이며 10.78 몫 198행만 재생했다.
  연산은 Destroy 158·Linked 24·Attach 16, 풀 권한은 서버 99·클라이언트 99, 변형은 0 이 166행,
  1~8 과 100~107 이 각 2행이다.
- 효과 호출은 취소 68·로컬 알림 44·지갑 42·SP 42·소리 42·원격 알림 8·포획 2행에서 실제로 잡힌다.
  변형 1~8 은 콜백 뒤 재조회(종류·선지급·소유자·죽은 부모·좌표)를, 100~107 은 전투 부착의 포획 경로를 검증한다.
- 풀 전체 Adler-32 와 사전/사후 깊이를 매 행 대조하므로, 이 몸체가 슬롯을 여분으로 건드리거나 완료 호출을 빠뜨리면 실패한다.
- `originalCD` 행은 읽지 않는다(기준 판본은 10.78 뿐이다).
- contained 생성은 cpppj 에도 전용 x86 fixture 가 없어 합성 계약으로만 검증했다. 이 한계를 L1 근거 범위로 기록한다.

**변이 확인:** 구현 16곳을 하나씩 망가뜨려 모두 실패로 잡혔다.

| 변이 | 잡힌 검사 |
| --- | --- |
| 커서/자기 부모 취소 조건 제거 | 원본 재생 |
| 로컬 알림 분기 고정 | 원본 재생 |
| 원격 권한 검사 제거 | 원본 재생 |
| 선지급 억제(+22) 제거 | 원본 재생 |
| SP 의 로컬 여부 고정 | 원본 재생 |
| 중심 x 발자국 절반 제거 | 원본 재생 |
| 지갑 절삭을 반올림으로 | 원본 재생 |
| 공통 사전 깊이 반납 제거 | 원본 재생 |
| 공통 사후 깊이 반납 제거 | 원본 재생 |
| 종류 비교 반전 | 원본 재생 |
| SP 비용 재조회 고정 | 원본 재생 |
| contained flags 기본값 `0x20` 제거 | 합성 계약 |
| 예측/클라이언트 우선순위 반전 | 합성 계약 |
| 품질 BYTE 위치(+30→+26) | 합성 계약 |
| 다리 표면 통지 제거 | 합성 계약 |
| 저장 타입/인자 뒤바꿈 | 합성 계약 |

소스는 모두 원복해 같은 검사가 다시 통과함을 확인했다.

**동기화:** 검토 HEAD는 **`145190d1`(20261023-16 CP)**, 목록의 반영 커밋은 **`f95a8db2` 그대로**다.
`cpp_sync.py` 종료 0, 연결 fixture **57개**(`contentlifetime` 추가)와 생성 자료 3개 모두 일치했다.
cpppj의 `*-x86.tsv` 505개 중 연결된 것은 54개이며, 나머지 연결 자료는 vtable 표 2개와 dotnetpj 소유 clockkernel 1개다.
반영 이후 cpppj 관련 커밋은 **81개**, 새 fixture 48개다. 이번에 옮긴 `ContentFormLifetime`·`RawContainedCreate` 와
의존 모듈(`ContentFormHost`·`SquidForms`·`RawSquidDestroy`·`SquidReward`)을 바꾼 커밋은 그 사이에 없었다.
미이식 기능이 남아 있으므로 규칙대로 반영 커밋을 HEAD 로 올리지 않았다
([계획 7절 7번](dotnet-cpp-port-plan-20261016.md#7-cpppj-진행을-따라가는-절차-다른-ai가-cpppj를-계속-진행-중)).
작업 트리의 미커밋 cpppj 변경은 쓰지 않았다.

## 3. 이번에 하지 않은 것 (경계)

- **신전 삭제(`vortex-pre`)와 삭제 수명주기 조립**(선택·커서·Graph·장부·파생 분배 허브)은 아직이다.
  이번 몸체는 ContentForm 자신의 삭제 효과만 분배하며, 실제 월드 분배기에 꽂지 않았다.
- `RawContentFormHost.BindCreate`(cpppj 의 contained 생성기 부착 어댑터)는 옮기지 않았다.
  이번에는 `RawContainedCreate.ConnectFactory` 와 `RawSquidFactory.CreateContained` 만 마련했다.
- contained 생성은 cpppj 전용 x86 fixture 가 없어 **L1 원본 재생 근거가 없다**. 합성 계약 검사만 통과했다.
  fixture 가 생기면 그때 재생 검사를 추가한다.
- CD 10.72 몫(너겟 효과 없음, contained 품질 BYTE +26 과 쓰기 순서)과 10.37 은 옮기지 않았다.
- `GumpFormHost`·`SquidFormDispatch`·파생 Form 분배 허브, 실제 세션 연결(`BattleSession`)도 아직이다.
  이 조각은 L1 모듈 단계이며 L2 이상으로 합산하지 않는다.

## 4. 다음 조각

신전 삭제(`vortex-pre`) → 삭제 수명주기 조립(선택·커서·Graph·장부·파생 분배 허브) → 실제 세션 연결.
시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
