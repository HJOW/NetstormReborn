# 원본 월드 코어 W03 열째 조각 — 종속 조회·삭제(RawContainedLookup)

2026-10-21, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W03(원본 월드 코어 ② 삭제·수명)** 의 열째 조각이다.
아홉째 조각까지는 사제 피격까지 이식했다. 이번 조각은 **종속(내용물) 조회·삭제** 몸체로,
건설 완료·사제 HP 1 감시 같은 상위 경로가 "타입과 순번으로 종속을 찾아 지운다"고 말할 때의 그 몸체다.
근거는 커밋된 cpppj 소스(`RawContainedLookup.cpp`)와 원본 fixture(`contained-x86.tsv`)뿐이다.
새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다.
이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawContainedLookup.cs` (신규) | `RawContainedLookup`(`Find`) | `RawContainedLookup.cpp` 의 004b1a90 조회·삭제 몸체. 타입 일치와 dead 제외를 통과한 항목만 순번을 소비하고, 다음 링크는 삭제 뒤 현재 슬롯에서 읽는다 |
| `X86RawContainedLookupTests.cs` (신규) | 원본 10.78 몫 805행 재생 1건 | `cpppj/tests/ContainedLookupTests.cpp` 의 `Scene`·`Prepare`·`Delete`·`Dump` 열과 같다 |
| `RawContainedLookupTests.cs` (신규) | 합성 2건(삭제 경계 누락·손상된 링크) | `ContainedLookupTests.cpp` 의 `ContainedLookup_RejectsMalformedLinksIncludingAfterDelete` 와 같은 구성 |

### 순서와 규칙

`RawContainedLookup.Find(parent, type, ordinal, remove)` 는 부모 슬롯의 head WORD 부터 종속 체인을 훑는다.

- 타입이 같고 죽음 비트가 꺼진 항목만 순번을 소비한다. 선택한 뒤에도 부모 head 로 되돌아가지 않는다.
- `remove` 가 거짓이면 찾은 번호를 그대로 돌려준다.
- `remove` 가 참이면 **서버 풀이거나 일반 client 번호(5~14999)일 때만** 플래그 0 으로 가상 삭제를 요청하고,
  권한이 없어도 순번은 줄어든다. 그래서 삭제가 일어나지 않은 경우에도 뒤의 일치 개수는 같이 줄어든다.
- 순번은 DWORD 로 감소하며 0 에서 감긴다(삭제 뒤 순번이 `0xFFFFFFFF` 가 되어 다음 항목은 선택되지 않는다).
- 다음 링크는 삭제 효과가 끝난 뒤 현재 슬롯에서 읽는다.
- 범위 밖 번호와 순환 체인은 방문 시점에 거부한다.

## 2. 검증 결과

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,277개 통과**(Assets 363 + Core 914; 이번 +3).

| 검사 | 내용 |
| --- | --- |
| `contained` 10.78 몫 805행 재생 | 서버 권한 입력에 따라 서버 풀·client 풀 두 장면을 나눠 쓴다. 매 행마다 반환 번호·삭제 요청 순서·지정 슬롯 8개 전체 바이트가 일치한다 |
| 합성 2건 | 삭제 경계 누락·범위 밖/순환 링크·삭제 뒤 손상된 next 를 조회 전에 거부한다 |

`contained-x86.tsv` 는 총 2,415행(3 판본 × 805행)이며 10.78 몫 805행만 재생했다.
`originalCD`·`original1037` 행은 읽지 않는다(기준 판본은 10.78 뿐이다).

10.78 몫에서 실제로 잡히는 경우가 많다. 반환 번호는 0 이 773행이고, 찾은 번호로
1·4·5·6·14999·15000·23002 가 각각 2~6행씩 잡힌다.
삭제 요청은 `D:1:0` 7행·`D:5:0` 6행·`D:14999:0` 6행·`D:6:0` 4행·`D:4:0` 3행이며,
서버 첫 번호(15000)와 예측 번호(23002)는 client 풀에서 삭제되지 않는다(권한 경계).
입력 패턴 0~4·제거 여부 0/1·변형 0~5·순번 0/1/2/5/6/7/`0xFFFFFFFF`/`0x80000000` 가 모두 쓰인다.

**변이 확인:** 구현 4곳을 하나씩 망가뜨려 모두 실패로 잡혔다.

| 변이 | 잡힌 검사 |
| --- | --- |
| 죽음 비트 제외 조건 제거 | 원본 재생 |
| 삭제 권한의 서버 영역 상한 제거(15000 이상도 삭제) | 원본 재생 |
| 순번 감소의 0 감김 제거 | 원본 재생 |
| 삭제 경계 누락 허용 | 합성 |

소스는 모두 원복해 같은 검사가 다시 통과함을 확인했다.

## 3. 이번에 하지 않은 것 (경계)

- **내용물 수명(`ContentFormLifetime`)은 아직 옮기지 않았다.** 그 몸체는 Form host(`ContentFormHost`)와
  SP·지갑 보상(`SquidReward`)을 함께 요구하므로 다음 조각으로 미뤘다.
- `MakeContainedConstructionHooks`(건설 완료의 진행음 프로세스 제거 경계)는 어댑터라 파생 분배 허브 조각에서 옮긴다.
- Form 수명(`formlifecycle`·`formtake`·`form-packet`), 신전 삭제, 삭제 보상(SP·지갑·샘 풀) 계산은 아직 옮기지 않았다.
- CD 10.72·10.37 몫, 파생 분배 허브·실제 세션 연결. 이 조각은 L1 모듈 단계이며 L2 이상으로 합산하지 않는다.

## 4. 다음 조각

내용물 수명(`contentlifetime`) → Form 수명(`formlifecycle`·`formtake`·`form-packet`) →
신전 삭제 → 삭제 보상(SP·지갑·샘 풀) 계산 → 파생 분배 허브·실제 세션 연결.
시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
