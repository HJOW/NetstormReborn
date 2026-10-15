# dotnetpj Totalmade 장부와 튜토리얼 1 단계 판정 (감사 D14·D05)

2026-10-15, 작업 PC **HJOW-X3D**. 근거는 [cpppj 반영 점검 문서](dotnet-cpp-audit-20261015.md)의 D14(누적 제작 장부)와 D05(튜토리얼 1 판정)다. 작업 시작 때 원격과 동기화했고(fetch만, 새 커밋 없음) 다른 PC의 cpppj 작업과 겹치는 파일은 건드리지 않았다. 이번 작업에서 새 디컴파일·원본 기계어 관찰·원본/클론 게임 실행은 없다. 원본 근거는 cpppj 문서([튜토리얼 1과 Totalmade](exe/cpp-tutorial-one-20261015.md), [공통 postPop](exe/cpp-postpop-reconstruction.md))와 fixture `tutorial-one-x86.tsv`(원본 10.78 PE 독립 관찰: `Total` 52행·`Stage` 266행)다.

## 1. 원본 장부의 의미 (확정한 것)

원본 공통 postPop과 삭제 준비가 갱신하는 **세 개의 256칸 DWORD 표**와 금지 표다. 타입 번호로 접근한다.

| 표 | 올리는 때 | 내리는 때 | 읽는 곳 |
| --- | --- | --- | --- |
| 로컬 소유자의 **현재 수** (`localCounts`) | 로컬 소유자 객체의 첫 등록(flags 1, abstract/buried 아님) | 로컬 소유자 일반 객체 삭제 | `Local(type)` |
| 로컬 소유자의 **누적 제작 수** (`localSecondaryCounts`) | 위와 같은 때 | **내리지 않는다**(장부 초기화만) | `Built(type)`, `BuiltForGenus(마스크)` — 튜토리얼 단계 판정 |
| **모든 소유자**의 전체 현재 수 (`globalCounts`) | 모든 소유자 객체의 첫 등록 | 일반 객체 삭제 | `Global(type)` |
| 금지 표 (`restrictions`) | — | — | `Allowed(type)`: DWORD 0이 허용(허용 값을 뒤집어 저장) |

세부 규칙: 부류 합산(`BuiltForGenus`)은 **타입 70부터** 플래그2가 마스크와 **한 비트라도 겹치는** 타입의 누적 수를 DWORD로 더한다(감김). 초기화(`ResetCounts`/`AllowAll`)는 **앞 255칸만** 쓰고 256번째 물리 칸은 유지한다. 잘못된 타입의 getter는 진단을 보고하고 0/거짓이다.

## 2. 구현

- [`TotalMade`](../dotnetpj/src/Netstorm.Core/Simulation/TotalMade.cs): cpppj `RawTotalMade`의 C# 이식. 표 4개(배열 노출)와 `AllowAll/Allow/Allowed/ResetCounts/Local/Built/Global/BuiltForGenus`, 그리고 postPop/삭제 때 쓰는 `RecordPopped/RecordDestroyed`(`RecordDestroyed`는 등록하지 않은 객체를 내리면 원본처럼 DWORD가 감긴다). CD 10.72 는 대상이 아니다.
- [`TutorialOne`](../dotnetpj/src/Netstorm.Core/Simulation/TutorialOne.cs): 원본 `004c3a20`의 단계 A~F 판정 전체를 이식했다. A는 F4 명령 또는 다리 누적 수 > 0, B는 > 7, C는 > 18에서 Advance 뒤 **Advance 성공과 무관하게** 간헐천 생성 호출, D는 신전 Graph와 최근 간헐천(최대 5개) 각각의 Graph가 같을 때마다 Advance(여러 번 불릴 수 있다), E·F는 SP 조회 허용 + float32 SP ≥ 200·600이다.
- 세션 연결: `BattleSession.Ledger`(로컬 소유자 = 사람 플레이어), `BuiltCount(이름)`·`BuiltForGenus(마스크)`. 건물 완공·유닛 실체화·개발 배치는 `RecordPopped`, 놓은 다리 칸은 **칸마다** 등록하고, 객체 삭제(`RemoveEntity`)와 다리 칸 붕괴/약화(`decay.Removed`, `WeakenAround`)에서 등록한 것만 현재 수를 내린다. 구 `PlayerState`의 이름 기반 누적 표(`Made`·`MadeWithFlags`·`MadeSnapshot`)는 삭제했고 검사합에는 장부 표의 0이 아닌 칸을 넣었다.
- `TutorialStages`: 튜토리얼 1은 `TutorialOne`에 위임하고, 튜토리얼 2의 누적 수 조회도 장부로 바꿨다. 같은 판정 안의 중복 Advance는 원본의 10프레임 잠금 효과를 헤드리스 경로에도 주려고 한 번만 허용한다.

## 3. 검증

| 검사 | 내용 |
| --- | --- |
| `X86TotalMadeTests` | `Total` 행 **PATCH 52개**(반환값·assert 횟수·표 4개 전체 1,024바이트)가 처음부터 전부 일치. 부류 "겹침→전체 일치", 시작 타입 70→0, 초기화 255→256 변이는 모두 실패로 잡혔다. CD 52행은 개수만 확인(재생 안 함) |
| `X86TutorialOneTests` | `Stage` 행 **266개**(단락 평가·중복 Advance·다중 Graph·float32 SP 경계·NaN)의 반환값과 **호출 순서**(집계·Graph·SP·Advance·생성)가 처음부터 전부 일치. D의 첫 일치 뒤 중단, E 경계 `>=`→`>`, SP 조회 허용 무시, B 기준 7→8 변이는 모두 실패 |
| `TutorialOneSessionTests` | 실제 세션에서 장부 입력으로 B(7/8)·C(18/19) 경계, 현재 수 표를 바꿔도 단계가 안 변함, 연결된 간헐천 3개가 있어도 D에서 한 단계만 오름, 끊긴 간헐천은 D에 머묾, SP 199/200/599/600 → E/F/F/G. 중복 Advance 방지 제거·누적 대신 현재 수 읽기·연결 판정 항상 참 변이는 각각 실패 |
| `SessionLedgerTests` | 맵 초기 객체는 장부에 없고 회수해도 감기지 않음, 워크샾 완공 시 (현재, 누적, 전체) = (1,1,1) → 회수 뒤 (0,1,0), 다리 칸 등록, 검사합 포함. 등록 표식 제거·삭제 때 누적도 내림 변이는 각각 실패 |

Release 경고/오류 **0**, 단위 **1,057개 통과**(Assets 343 + Core 714). 직전 커밋 `7c67b99`의 1,035개에서 **22개**가 늘었다(Total 4 + 튜토리얼 1 판정 3 + 세션 11 + 장부 4).

## 4. 동작이 달라진 점

1. 누적 제작 수는 **사람(로컬) 플레이어만** 센다(원본 장부는 로컬 소유자 한 명). AI 플레이어별 누적 표는 없어졌고 소비자도 없었다.
2. 튜토리얼 1의 E·F는 SP 조회 허용과 float32 비교를 쓴다(동작은 같다). D는 **최근 간헐천 최대 5개**만 비교한다(맵 전체 간헐천이 아니다).
3. `PlayerState.Made*` API 삭제와 검사합 값 변경.

## 5. 남은 경계 (정직한 한계)

- **표면 Graph 번호는 세션에 없다.** D의 `graphFor`는 "현재 사제가 그 간헐천까지 걸어갈 수 있는가"로 근사한다(닿으면 신전과 같은 번호). 실제 `SurfaceGraph` 조립·`BridgeGrid.graphSurfaces` 연결·영역 소유·내용물·선택·감시의 같은 현재 상태 공유는 **D14의 나머지**이며 이번에 하지 않았다.
- **간헐천 생성은 아직 원본 시점이 아니다.** 원본은 단계 C에서 지역 사각형·공유 보상 풀 규칙으로 생성(`00486360`, `local-geyser`·`player-geyser-bounds` fixture)하는데 클론은 세션 생성 때 결정적 근사 위치에 미리 만든다. 생성 경계는 비어 있다(D05 후속).
- **금지 표(`Restrictions`)는 세션에 있으나 소비자가 없다.** 사람 플레이어의 기술 허용은 여전히 `TechPermissions`(이름 키)가 맡는다. 원본의 단일 금지 표와 `AllowAll/Allow` 로의 일원화는 후속이다.
- **장부 초기화(`ResetCounts`)의 원본 호출 시점은 미확인**이다. 맵 초기 객체를 세지 않는 현재 동작은 "초기화가 로딩 뒤에 있다"는 가정과 녹화 기반 기존 동작을 따른 것이다. 다리 칸의 현재·전체 수는 `decay.Removed`/`WeakenAround`의 두 경로에서만 내리며, 그 밖의 칸 제거 경로가 생기면 연결해야 한다.
- 현재 수·전체 수 표는 소비자가 아직 없다(AI·메뉴가 읽을 때 사용). 회수 명령 토큰(단계 A의 `reclaimCommand`)을 보내는 사건은 클론에 없어 반환 1 경로는 쓰이지 않는다.
- 튜토리얼 2의 판정기(`004c3bb0`, D06) 이식은 하지 않았다(장부 조회만 교체).
- 클론 창(GUI)은 이번 단계에서 실행하지 않았다(변경은 Core 한정, 헤드리스로 검증).
