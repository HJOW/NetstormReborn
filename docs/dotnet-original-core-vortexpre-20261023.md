# 원본 월드 코어 W03 열네째 조각 — 신전 삭제 전처리(RawVortexPreDestroy)

2026-10-23, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W03(원본 월드 코어 ② 삭제·수명)** 의 열네째 조각이다.
열세째 조각에서 내용물 수명과 contained 생성을 옮겼고, 이번에는 [현행 계획](../LEFT_JOBS.dotnetpj.md#dotnet-current-plan-20261022)의 "바로 다음 작업 4-b" 가운데
신전 삭제 전처리(`vortex-pre`)를 옮겼다. 삭제 수명주기 조립(선택·커서·Graph·장부·파생 분배 허브)은 아직이다.

근거는 커밋된 cpppj 소스(`RawVortexPreDestroy.cpp`·`RawVortexPreDestroy.h`), 원본 fixture
(`vortex-pre-x86.tsv`)와 기존 cpppj 검사(`VortexPreDestroyTests.cpp`)뿐이다.
새 디컴파일 산출물을 읽지 않았고 원본/클론 게임·창 실행도 없다. 현재 PC는 공통 기록의 마지막 전체 정밀 디컴파일 PC
(**VM-W11-Codex, 2026-10-23**)와 같으나 디컴파일 산출물을 읽지 않았으므로 재디컴파일 대상이 아니다.
마지막 디컴파일 PC 기록은 그대로다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다.
이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawVortexPreDestroy.cs` (신규) | `VortexPreDestroyState`·`VortexPreDestroyHooks`·`RawVortexPreDestroy`(`PreDestroy`·`Remove`·`Position`·`Field`) | `RawVortexPreDestroy.h/.cpp` 의 004dca50, 목록 제거 0040ea00, 섬 0046f530/0046ee30/0046f260/004db890/0046ef10, 지역 00455380, 공통 0044b4b0, 생산 0047d890, 진단 0x2ab/0x2ac |
| `X86RawVortexPreDestroyTests.cs` (신규) | `vortex-pre` 10.78 몫 102행 재생 1건 | `cpppj/tests/VortexPreDestroyTests.cpp` `VortexPreDestroy_Native102` |
| `RawVortexPreDestroyTests.cs` (신규) | 합성 경계 7건(필수 하위·저장소·가림·범위 2건·로딩·Player 정리) | `VortexPreDestroy_RequiredDamageableAndStorage` 와 L1 경계 |
| `Netstorm.Core.Tests.csproj` | fixture 링크 1건, `cpp-fixture-manifest.tsv` 갱신(57→**58**개) | `cpp_sync.py --update --add vortex-pre:Core` |

### 삭제 전처리 (`RawVortexPreDestroy`)

원본 004dca50 의 순서와 폭을 그대로 지킨다.

1. 가림 비트(+40 & 9)가 없으면 신전·거주 목록에서 삭제 번호를 앞방향 압축으로 뺀다(물리 꼬리 보존).
2. 삭제 전 좌표로 섬 번호(0046f530)를 조회하고, 존재(0046ee30)·신전 일치(0046f260)가 깨지면 진단(0x2ab·0x2ac)을 남기고 계속한다.
3. 섬 지우기(004db890 region,0)→소유자 0 전파(0046ef10)→삭제 후 좌표로 지역 갱신(00455380) 순서로 부른다.
4. outpost 목록의 현재 count 만큼 순회하며, 번호가 풀·16비트 범위를 벗어나면 거부하고, 부모와 소유자가 같은 outpost 좌표마다 지역 갱신을 부른다.
5. 공통 Damageable 사전 파괴(0044b4b0)를 같은 플래그로 부른다.
6. 로딩 깊이가 0 이고 가림 비트가 없으면 생산 갱신(0047d890)을 부른다. 로딩·가림은 효과 뒤 현재 값을 다시 읽는다.
7. 소유자가 0 이 아니면 현재 Player 두 참조(+0x7c·+0x84)가 삭제 번호일 때 0 으로 지우고 표시 필드(+0x80·+0x88)에 같은 값을 복사한다. 소유자·Player 주소는 효과 뒤 현재 소유자로 다시 구한다.

다른 풀의 목록·상태와 길이가 1,620 이 아닌 Player 저장소는 생성 시점에 거부한다.
개별 훅이 null 이면 앞선 목록·지역 쓰기를 보존한 채 그 호출 시점에 명시 거부한다(원본 `Need` 와 같다).
`Position`·`Field`·Player 읽기는 모두 리틀 엔디언 DWORD 로 원본 바이트 순서를 보존한다.

## 2. 검증 결과

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,318개 통과**(Assets 363 + Core 955; 이번 +8).

| 검사 | 내용 |
| --- | --- |
| `vortex-pre` 10.78 몫 102행 재생 | 매 행마다 Player 1,620바이트·세 슬롯 150바이트·목록 48바이트·로딩·효과 문자열을 대조한다 |
| 합성 경계 7건 | 필수 하위 누락 뒤 앞선 쓰기 보존·저장소 크기·가림 건너뛰기·outpost 범위·소유자 범위·로딩 억제·Player 미러 정리 |

- `vortex-pre-x86.tsv` 는 헤더 1행 + 102행이며 판본 열이 없어 102행 모두 10.78 로 재생했다.
  입력은 `variant,mutation,flags` 이고 flags 는 항상 0x400000(4,194,304)이다.
- 효과 5종은 전체 삭제(`Island;Exists;Temple;Clear;Owner;Region;Region;Base;Refresh`)·로딩 억제(Refresh 없음)·
  가림(Base 만)·존재 진단(Assert,683)·신전 진단(Assert,684)이다.
- 변형 1~4 는 공통 하위 뒤 소유자·가림·로딩·생산 뒤 소유자 변경을, variant 0 은 소유자 0 의 Player 정리 생략을 검증한다.
- `originalCD` 행은 없다(기준 판본은 10.78 뿐이다).

**변이 확인:** 구현 8곳을 하나씩 망가뜨려 모두 실패로 잡혔다.

| 변이 | 잡힌 검사 |
| --- | --- |
| 가림 마스크 9→0 | 원본 재생 |
| 거주 목록 제거 누락 | 원본 재생 |
| 두 번째 지역 갱신 좌표 고정 | 원본 재생 |
| outpost 소유자 비교 무시 | 원본 재생 |
| 로딩 검사 제거 | 원본 재생 |
| 중립이어도 Player 정리 | 원본 재생 |
| 첫 번째 미러 복사 제거 | 원본 재생 |
| outpost 범위 검사 제거(70000 투입) | 합성 경계 |

소스는 모두 원복해 같은 검사가 다시 통과함을 확인했다.

**동기화:** 시작 검토 HEAD 는 **`9f401229`(20261023-20 CP)**, 목록의 반영 커밋은 **`f95a8db2` 그대로**다.
`cpp_sync.py` 종료 0, 연결 fixture **58개**(`vortex-pre` 추가)와 생성 자료 3개 모두 일치했다.
cpppj의 `*-x86.tsv` 505개 중 연결된 것은 55개이며, 나머지 연결 자료는 vtable 표 2개와 dotnetpj 소유 clockkernel 1개다.
반영 이후 cpppj 관련 커밋은 시작 시점 85개·종료 시점 **86개**, 새 fixture 48개다. 이번에 옮긴
`RawVortexPreDestroy` 와 의존 모듈(`RawPostPopList`·`RawSidPool`)을 바꾼 커밋은 그 사이에 없었다.
미이식 기능이 남아 있으므로 규칙대로 반영 커밋을 HEAD 로 올리지 않았다
([계획 7절 7번](dotnet-cpp-port-plan-20261016.md#7-cpppj-진행을-따라가는-절차-다른-ai가-cpppj를-계속-진행-중)).
작업 트리의 미커밋 cpppj 변경(`SoloMapCatalog` 등)은 쓰지 않았다.

## 3. 이번에 하지 않은 것 (경계)

- **삭제 수명주기 조립**(선택·커서·Graph·장부·파생 분배 허브·`ContentFormHost.BindCreate`)은 아직이다.
  이번 몸체는 신전 자신의 전처리 효과만 내며, 실제 월드 분배기에 꽂지 않았다.
- `GumpFormHost`·`SquidFormDispatch`·파생 Form 분배 허브, 실제 세션 연결(`BattleSession`)도 아직이다.
- CD 10.72·10.37 몫은 옮기지 않았다.
- 이 조각은 L1 모듈 단계이며 L2 이상으로 합산하지 않는다.

## 4. 다음 조각

삭제 수명주기 조립(선택·커서·Graph·장부·파생 분배 허브) → 실제 세션 연결.
시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
