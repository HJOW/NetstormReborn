# 원본 월드 코어 W03 네째 조각 — 삭제 그래프 재생(destroygraph)

2026-10-21, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W03(원본 월드 코어 ② 삭제·수명)** 의 네째 조각이다.
셋째 조각의 공통 삭제 수명주기(`RawSquidDestroyLifecycle`)는 그래프를 **꺼 둔 상태**의 장면에서만 검증됐다.
이번 조각은 그 수명주기에 실제 표면 그래프(`RawGraph`)를 연결한 상태에서 원본의 삭제 전/사후 그래프 효과를 재생한다.
근거는 커밋된 cpppj 소스(`SquidDestroyLifecycle.cpp`·`RawGraph.cpp`·`SquidPostPop.h`)와 원본 fixture(`destroygraph-x86.tsv`·`island-graph-frame-x86.tsv`)뿐이다.
새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다.
이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `X86RawDestroyGraphTests.cs` (신규) | 삭제 그래프 원본 재생 4건(`Pre`·`Post`·`Destroy` 각 192행 + 섬 H 치환 24행) | `cpppj/tests/DestroyGraphTests.cpp` 의 `Scene`·`Run`·`Check` 열과 같다 |
| `Netstorm.Core.Tests.csproj` (확장) | `destroygraph-x86.tsv`·`island-graph-frame-x86.tsv` 링크 2건 | cpppj fixture 를 그대로 링크한다(사본을 만들지 않는다) |
| `cpp-fixture-manifest.tsv` (확장) | 위 두 fixture 등록(연결 fixture 43 → **45**개) | `python dotnetpj/tools/cpp_sync.py --update --add destroygraph:Core --add island-graph-frame:Core` |

이번 조각에서 **소스 구현은 바꾸지 않았다.** W02 열여덟째 조각의 `RawGraph.PostDestroy`·`Frame`과
W03 셋째 조각의 `RawSquidDestroyLifecycle.PreGraph`·`NeedsPostGraph`가 원본과 같은 결과를 냈음을
실제 그래프가 켜진 원본 장면 600행에서 확인했다.

재생 장면은 cpppj 원본 시험과 같은 입력을 쓴다.

- 슬롯 풀(32,768칸·서버)·4단계 공간 해시·65,536칸 점유 지도·188개 타입 표·타입별 프레임 코드 표.
- 클라이언트 슬롯 7개(5~11)를 잡고 뿌리 하나와 주변 여섯 개에 가상 표(0x501dd0)·타입·좌표·발자국·
  그래프 번호·추가 비트·프레임 번호를 넣은 뒤, 공간에서 빠지지 않은 슬롯만 해시 체인에 넣는다.
- 그래프 표 255행과 flood 스택 4,096칸은 fixture 입력값(초기 스택은 `seed + i`) 그대로 채운다.
- 타입 162(치환 표면)는 대상 타입이 162 가 아닐 때만 별도 플래그·발자국·프레임으로 준비한다.

비교 항목은 원본 기계어 결과와 같은 열이다.

| 칸 | 내용 |
| --- | --- |
| 효과 문자열 | 선택 해제(`S`)·보상(`Q`)·잃어버린 소리(`L`)·전송(`T`)·사전(`P`)·사후(`O`) 진입 순서 |
| 슬롯 50바이트 × 7 | 풀/해시/점유 변경까지 포함한 최종 슬롯 바이트 |
| 풀·공간 13칸 | 빈 번호 수·예측 커서·양쪽 첫 빈 번호·꼬리·풀 전체 Adler-32·해시 머리 Adler-32·점유 지도 Adler-32·삭제 기록 Adler-32·선택 번호·사전/사후 깊이 |
| 장부 18칸 | 총 비용·재계산 알림·깊이·세 타입 통계 표 Adler-32·공급원/작업장 목록 개수·소유자 0~8 목록 개수·목록 항목 Adler-32 |
| 좌표 5칸 | 배치 타입·배치 x/y·잃어버린 x/y 비트 |
| 그래프 표 255행 | 표면 수·사용 여부·예약 WORD 를 세 칸씩 직접 비교 |
| flood 스택 체크섬 | 스택 4,096칸 전체 Adler-32 |

## 2. 검증 결과

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,244개 통과**(Assets 363 + Core 881; 이번 +4).

| 검사 | 내용 |
| --- | --- |
| `Pre` 10.78 몫 192회 재생 | 사전 파괴(표면 분할·Free·특수 H 치환)와 선택/장부 순서가 위 모든 열에서 일치한다 |
| `Post` 10.78 몫 192회 재생 | 사후 파괴(발자국→일반 탐색→순차 Add)가 그래프 표 255행과 flood 스택까지 일치한다 |
| `Destroy` 10.78 몫 192회 재생 | 삭제 본체→사전 Graph/장부→Unpop→사후 Graph/비용→반납 전체가 일치한다 |
| 섬 H 치환 24회 재생 | 9→1 프레임 치환(157 의 H 프레임→162 단일 프레임) 장면에서 pre/Destroy 가 일치한다 |

`destroygraph-x86.tsv` 는 총 1,728행(Pre 576 + Post 576 + Destroy 576)이며 10.78 몫 576행(세 절 각 192행)만 재생했다.
`island-graph-frame-x86.tsv` 는 총 24행이고 전부 10.78이라 24행을 모두 재생했다. 재생한 원본 행은 합계 **600행**이다.
`originalCD`(CD 10.72)·`original1037` 행은 읽지 않는다(기준 판본은 10.78 뿐이다).

그래프 표가 실제로 바뀌는 행도 확인했다. 10.78 몫에서 표 255행이 입력과 달라진 행은
Pre 98/192 · Post 50/192 · Destroy 110/192 행이고, 섬 H 치환 24행 중 20행이다.
나머지 행은 그래프 효과가 없는 경우(그래프 꺼짐·분할 금지·매림·비표면 타입·H 아닌 특수 프레임)다.

**변이 확인:** 구현 7곳을 하나씩 망가뜨려 모두 실패로 잡혔다.

| 변이 | 잡힌 검사 |
| --- | --- |
| 대형 표면(162) 감소 수 9 → 1 | `Destroy`·`Pre` |
| 특수 표면의 H 방향 코드 검사 제거 | `Destroy`·`Pre` |
| 삭제 분할 정책(rebuild) 전달 무시 | `Destroy`·`Pre` |
| 사후 그래프 분기 마스크에서 비트 8 제거 | `Post`·`Destroy` |
| 사후 순차 Add 무력화 | `Post`·`Destroy` |
| 로컬 현재 타입 수 감소 누락 | `Destroy`·`Pre` |
| 그래프 Detach 의 9 감소 전달 무시 | `Destroy`·`Pre` |

소스는 모두 원복해 같은 검사가 다시 통과함을 확인했다.

## 3. 이번에 하지 않은 것 (경계)

- **공급원/작업장 목록 제거 분기는 이 fixture 로는 안 잡힌다.** 목록 항목이 0x60000000 계열이라 삭제 대상 번호가
  목록에 들어 있는 조합이 10.78 행에 없다(변이 검사에서 목록 제거를 흐리게 해도 `destroygraph` 는 통과한다).
  이 분기는 앞 조각의 `destroylifecycle` fixture 가 잡는다(확인함).
- **삭제 준비의 "안쪽(footprint interior)" 조기 반환도 이 fixture 로는 안 잡힌다.** 같은 이유로 `graphremove` fixture 가 잡는다(확인함).
- `Damageable`(피격·사전/사후 파괴·해방)·종속 조회·내용물·Form 수명·신전 삭제, `bridgeeffects` 전체(W06 다리 효과) 재생.
- 삭제 보상(SP·지갑·샘 풀) 계산: 바깥 보상 알림까지만 옮겼다.
- 그래프 분기 재생: 사전 분할·사후 주변 추가가 잡혔지만 그래프 분기(분할)·전체 재구성·소진 복구의 조합은
  W02 의 `graphremove`·`graphrebuild`·`graphrecovery` 가 맡는다.
- CD 10.72·10.37 몫, 파생 분배 허브·실제 세션 연결. 이 조각은 L1 모듈 단계이며 L2 이상으로 합산하지 않는다.

## 4. 다음 조각

Damageable·종속 조회·내용물·Form·신전 → 파생 분배 허브·실제 세션 연결.
시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
