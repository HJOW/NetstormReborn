# 원본 월드 코어 W03 자산 삭제 재정의 조각 — 다리 밖 자산 세 종류(RawPriestPreDestroy·RawBombPreDestroy·RawFlyerPreDestroy)

2026-10-23, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W03(원본 월드 코어 ② 삭제·수명)** 의
열일곱째 조각이다. 열여섯째 조각에서 자산 루트의 다리 가상 삭제 재정의를 옮겼고, 이번에는
[현행 계획](../LEFT_JOBS.dotnetpj.md#dotnet-current-plan-20261022)의 "바로 다음 작업 4-b2" 가운데
**다리 밖 자산 타입별 가상 삭제 재정의**의 첫 세 종류 — 사제·Point Blast Bomb·플라이어 — 를 옮겼다.
선택·커서·Graph·장부·Kernel 프로세스 호스트를 묶는 실제 삭제 수명주기 조립은 아직이다.

근거는 커밋된 cpppj 소스(`RawPriestPreDestroy.h/.cpp`·`RawBombPreDestroy.h/.cpp`·`RawFlyerPreDestroy.h/.cpp`·
`RawPriestPostPop.h`)와 cpppj 검사(`PriestPreDestroyTests.cpp`·`LightningScreenTests.cpp` 의 Bomb 절·`FlyerPreDestroyTests.cpp`)뿐이다.
새 디컴파일 산출물을 읽지 않았고 원본/클론 게임·창 실행도 없다.
현재 PC는 공통 기록의 마지막 전체 정밀 디컴파일 PC(**VM-W11-Codex, 2026-10-23**)와 같으나 디컴파일 산출물을 읽지 않았으므로 재디컴파일 대상이 아니다.
마지막 디컴파일 PC 기록은 그대로다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다.
이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawPriestPreDestroy.cs` (신규) | `PriestPostPopState`·`PriestPreDestroyHooks`·`RawPriestPreDestroy`(`Handles`·`PreDestroy`·`Pool`·`WrapHooks`) | `RawPriestPreDestroy.h/.cpp`, 원본 004919b0 / CD 0040c330 |
| `RawBombPreDestroy.cs` (신규) | `BombPreDestroyHooks`·`RawBombPreDestroy`(`Handles`·`PreDestroy`·`Pool`) | `RawBombPreDestroy.h/.cpp`, 원본 0041e960 |
| `RawFlyerPreDestroy.cs` (신규) | `FlyerPreDestroyHooks`·`RawFlyerPreDestroy`(`PreDestroy`·`Pool`) | `RawFlyerPreDestroy.h/.cpp`, 원본 0045e7e0 |
| `X86RawPriestPreDestroyTests.cs` (신규) | `priestdestroy` 10.78 몫 2,184행 재생 | `PriestPreDestroyTests.cpp` 의 `PriestScene` |
| `X86RawBombPreDestroyTests.cs` (신규) | `bomb-predestroy` 96행 재생 | `LightningScreenTests.cpp` 의 `LightningScreen_BombPreDestroyNative` |
| `X86RawFlyerPreDestroyTests.cs` (신규) | `flyer-predestroy` 96행 재생 | `FlyerPreDestroyTests.cpp` |
| `RawPriestPreDestroyTests.cs`·`RawBombPreDestroyTests.cs`·`RawFlyerPreDestroyTests.cs` (신규) | 경계 계약 11건 | 같은 cpppj 검사의 guard·변형 절 |

### 사제 (`RawPriestPreDestroy`)

1. 진입 조건은 **번호가 5 이상 풀 크기 미만**이고 슬롯 `+10` 이 **타입 158**, 앞 4바이트가 **10.78 사제 가상 표**(`0x0050F210`)인 경우뿐이다.
2. 상태 바이트(`+11`)에 **1·8** 이 켜져 있으면 목록·보호막을 만지기 전에 예외로 거부한다. 원본은 공통 삭제가 dead 를 켠 뒤
   진입하므로 **dead 를 이유로 생략하지 않는다**.
3. 추가 비트(`+40`)의 **9** 가 꺼져 있으면 일반 경로다. 일반 경로만 사제 목록의 **활성 개수**를 먼저 검사한다
   (개수 > 저장소 길이면 원본 배열 접근 전 예외).
4. **Carrier 사전 검사**는 일반·추상 모두 목록·보호막보다 **먼저** 부른다.
5. 일반 경로는 목록을 **앞방향 압축**으로 일치 항목을 모두 빼고(비활성 꼬리는 보존), 보호막을 조회해
   반환 번호가 **1 이상 풀 크기 미만**이면 가상 삭제를 요청한다(타입·free·dead 재검사 없음 — 원본 그대로).
6. 마지막으로 **Carrier 전체 준비**를 같은 플래그로 부른다. 깊이 감소는 Carrier 의 책임이며 이 몸체는 Pop·공간 해제를 하지 않는다.
7. `WrapHooks` 는 cpppj `MakePriestPreDestroyHooks` 와 같은 어댑터다. 다른 풀이면 생성 시 거부하고, 후속 `Emit` 이 없으면
   거부하며, **사제의 `PreDestroy` 사건만** 가로채 나머지 사건은 그대로 넘긴다.

`PriestPostPopState` 는 아직 이식하지 않은 `RawPriestPostPop` 과 공유할 상태다. 이번에는 사전 파괴가 실제로 읽는
**사제 목록**만 두고, 원본 소유자 상태 배열(`00595428`)은 postPop 몸체를 옮길 때 같은 클래스에 더한다.

### Point Blast Bomb (`RawBombPreDestroy`)

1. 진입 조건은 앞 4바이트가 **10.78 Point Blast Bomb 가상 표**(`0x00502370`)인 경우다. 타입 이름으로 넓히지 않는다.
2. 추가 비트(`+40`)의 **9** 가 꺼져 있고 삭제 플래그에 **0x100000** 이 있을 때만 폭발과 소리를 낸다.
3. **폭발 뒤 현재 슬롯의 좌표(`+14`·`+18`)를 다시 읽어** 소리를 낸다(폭발이 좌표를 바꿀 수 있다).
4. 가상이 아닌 **공통 사전 파괴**는 폭발 여부와 무관하게 같은 플래그로 항상 마지막에 한 번 부른다.
5. 선택된 경계가 없으면 그 효과 전에 명시 거부한다.

### 플라이어 (`RawFlyerPreDestroy`)

1. 슬롯 `+8` 을 **WORD(2바이트)** 로만 읽는다. DWORD 로 넓히면 타입(`+10`)·상태(`+11`)가 대상에 섞인다.
2. 대상이 **0 이 아니면** 그 대상을 플래그 0 으로 가상 삭제한다.
3. 그 뒤 **원래 this·플래그**로 Damageable 사전 파괴를 한 번 부른다(callback 이 raw 를 바꿔도 인자는 그대로다).
4. 두 하위 경계가 모두 없으면 생성 시 거부한다.

## 2. 검증 결과

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,349개 통과**(Assets 363 + Core 986; 이번 +14).

| 검사 | 내용 |
| --- | --- |
| `X86RawPriestPreDestroyTests.PriestPreDestroyMatchOriginalX86` | `priestdestroy` 10.78 몫 **2,184행**. 행마다 목록 count·목록 저장소 전체(비활성 꼬리 포함)·효과 문자열(`F`/`D`/`C`, 시점의 개수·추가 비트)·슬롯 전체 50바이트·Carrier 사전 검사 1회를 대조 |
| `X86RawBombPreDestroyTests.BombPreDestroyMatchOriginalX86` | `bomb-predestroy` **96행**. 행마다 효과 문자열(폭발·소리·공통)과 슬롯 50바이트를 대조하고, 32비트·64비트 원본 관찰 열이 같은지 확인 |
| `X86RawFlyerPreDestroyTests.FlyerPreDestroyMatchOriginalX86` | `flyer-predestroy` **96행**. 네 슬롯(50·60·61·32767) **200바이트**와 효과 문자열·변형 3종을 대조 |
| `RawPriestPreDestroyTests.RejectsRawListAndCarrierBeforeWriting` | 0·51·풀 크기·65535 거부, 상태 비트 1·8 거부, 다른 가상 표 거부, 손상된 개수 7 거부가 목록·슬롯·효과를 바꾸지 않음 |
| `RawPriestPreDestroyTests.CarrierValidationFailurePreservesList` | Carrier 사전 검사 실패가 목록·보호막 효과 없이 중단 |
| `RawPriestPreDestroyTests.AbstractPrefixSkipsListAndReadsOnlyCarrier` | 추가 비트 9 는 손상된 개수도 읽지 않고 Carrier 만 부름(사전 검사는 1회) |
| `RawPriestPreDestroyTests.OrdinaryPathRemovesAllAndRequestsForcefieldOnce` | 세 항목 압축·보호막 1회 요청, 꼬리 보존, 풀 밖 보호막 번호는 요청 없음 |
| `RawPriestPreDestroyTests.WrapperInterceptsOnlyPriestPreDestroy` | 사제 사건만 가로채고 나머지 전달, 다른 풀·후속 누락·필수 훅 누락 거부 |
| `RawBombPreDestroyTests.ExplodesOnlyWithFlagAndVisibleExtra` | 플래그·추가 비트 조합 3종, 폭발이 바꾼 좌표를 소리가 다시 읽음, 공통 전처리는 항상 마지막 |
| `RawBombPreDestroyTests.RejectsForeignVtableBeforeEffects` | 가상 표만으로 판별하며 거부 시 슬롯 무변화 |
| `RawBombPreDestroyTests.RejectsMissingSelectedHook` | 폭발·공통 경계 누락을 그 효과 전에 거부 |
| `RawFlyerPreDestroyTests.ReadsOnlyWordAndKeepsOriginalArguments` | +8 WORD 만 읽음(4바이트였다면 0x02019002), callback 뒤에도 원래 this·flags 로 하위 호출 |
| `RawFlyerPreDestroyTests.SkipsDestroyWhenTargetIsZero` | 대상 0 이면 Damageable 만 |
| `RawFlyerPreDestroyTests.RejectsMissingHooks` | 두 하위 경계 누락을 생성 시 거부 |

**변이 확인:** 구현 5곳을 하나씩 망가뜨려 모두 실패로 잡혔다.

| 변이 | 결과 |
| --- | --- |
| 사제 보호막 하한 `>= 1` → `>= 0` | 10.78 2,184행 중 110행과 synthetic 1건이 실패로 잡힘 |
| 사제 `ValidateCarrier` 호출 제거 | 재생 1건·synthetic 3건이 실패로 잡힘 |
| Bomb 추가 비트 마스크 `9` → `0` | 재생 1건·synthetic 1건이 실패로 잡힘 |
| 플라이어 +8 을 DWORD 로 읽기 | synthetic 3건이 실패로 잡힘 |
| 사제 추가 비트 위치 `40` → `35`(CD 배치) | 재생 1건이 실패로 잡힘 |

소스는 모두 원복해 같은 검사가 다시 통과함을 확인했다.

**동기화:** 시작 검토 HEAD 는 **`acdb8c63`(20261023-32 DN)**, 목록의 반영 커밋은 **`f95a8db2` 그대로**다.
`cpp_sync.py` 종료 0, 연결 fixture **61개**(이번에 `priestdestroy`·`bomb-predestroy`·`flyer-predestroy` 를
`--update --add` 로 등록, 58→61)와 생성 자료 3개가 모두 일치했고, `cpppj *-x86.tsv` **514개 중 연결된 것은 58개**다.
반영 이후 cpppj 관련 커밋 **94개**, 새 fixture **56개**(추가 56, 수정·삭제 0)다. 이 가운데 이번에 옮긴 세 몸체와
`RawPriestPostPop.h`·세 fixture 를 바꾼 커밋은 **없다**(`git log f95a8db2..HEAD -- <해당 경로>` 가 비어 있다).
미이식 기능(삭제 수명주기 조립·실제 세션 연결 등)이 남아 있으므로 규칙대로 반영 커밋을 HEAD 로 올리지 않았다
([계획 7절 7번](dotnet-cpp-port-plan-20261016.md#7-cpppj-진행을-따라가는-절차-다른-ai가-cpppj를-계속-진행-중)).
작업 트리의 미커밋 cpppj 변경(`SoloMapCatalog`·`RawFortObjectSave` 등)과 추적되지 않은 `fort-*-save-x86.tsv` 는 쓰지 않았다.

## 3. 이번에 하지 않은 것 (경계)

- **삭제 수명주기 조립**(선택·커서·Graph·장부·Kernel 프로세스 호스트·실제 세션 연결)은 아직이다.
  세 몸체는 각각의 가상 사전 파괴만 소유하며 `RawCampaignSession` 의 분배 순서(1157~1165행)에 해당하는
  세션 연결은 옮기지 않았다.
- **나머지 자산 타입의 가상 삭제 재정의**는 아직이다(워크샾·공격체·신전·얼음 타워·애시드 바리케이드·데빌메이커 등).
  `wind-aviary-destroy-x86.tsv` 도 쓰지 않았다.
- **CD 10.72·10.37 몫**은 옮기지 않았다. 사제는 가상 표 값과 추가 비트 위치(`+35`)가 달라 10.78 만 재생했고,
  Bomb 도 CD 의 다른 전처리를 같은 것으로 추정하지 않았다.
- `RawPriestPostPop` 본체(목록 등록·회복 예약·소유자 상태)는 아직이다. `PriestPostPopState` 에는 그 몸체가 쓸
  소유자 상태 배열을 아직 더하지 않았다.
- 프로세스 Form(타입 10~62) 등록·`RawGraph` 의 비표면 소수 영역 연결은 이번 조각 범위가 아니다.
- 이 조각은 L1 모듈 단계이며 L2 이상으로 합산하지 않는다.

## 4. 다음 조각

삭제 수명주기 조립(선택·커서·Graph·장부·Kernel 프로세스 호스트) → 실제 세션 연결 → L2 시작 상태.
시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
