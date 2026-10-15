# dotnetpj 플레이어 참여·방향별 동맹 (감사 D03)

2026-10-15, 작업 PC **HJOW-X3D**. 근거는 [cpppj 반영 점검 문서](dotnet-cpp-audit-20261015.md)의 D03과 5절 2번 작업 순서다. 작업 시작 때 공통 기록의 마지막 정밀 디컴파일 PC는 같은 **HJOW-X3D**였으나 도중에 들어온 cpppj 커밋이 **HJOW-Athlon, 2026-10-15**로 갱신했다. 이번 작업은 디컴파일 산출물을 읽지 않았고(원본 실행 파일의 문자열 확인, cpppj 문서·fixture만 사용) 새 디컴파일·원본 기계어 관찰·원본/클론 게임 실행은 하지 않았다. 원본 근거는 cpppj가 만든 [참여·동맹·AI 부착 경계](exe/cpp-player-participation-20261015.md)·[미션 시작 경계](exe/cpp-mission-start-20261015.md)와 fixture `player-participation-x86.tsv`(원본 10.78 PE 독립 관찰 608개)다.

## 1. 원본 규칙 (이번에 확정·적용한 것)

| 항목 | 원본 | 변경 전 C# |
| --- | --- | --- |
| 동맹의 모양 | **방향별 9×9 표**. `allies[A][B]`는 A가 B를 동맹으로 선언했다는 뜻이며 반대 방향은 독립이다 | 두 플레이어 중 한쪽만 적어도 대칭 동맹 |
| 사람 쪽 목록 `myAllyList` | **읽지 않는다.** 두 실행 파일(10.78·CD)에 `myAllyList` 문자열이 없고 `ai%sAllyList` 형식만 있다 | 동맹으로 읽었다(2026-10-01 승패 오발동을 막으려던 우회로 추가) |
| 로컬 사람의 행 | 직접 정하지 않는다. AI가 로컬을 선언했을 때만 `AddAlly(local, AI)`로 **반대 방향을 더한다** | `myAllyList`로 정했다 |
| AI 사이 | 각 AI는 자기 행만 쓴다. 대칭화하지 않는다 | 대칭 |
| 번호 없는 `aiAllyList` | 공통 기본값으로 **모든 AI**에 적용된다(`Prepare = 기본값 → Apply(-1) → Apply(owner)`) | 플레이어 2 전용 |
| 목록 해석 | `;`로만 나누고 항목마다 `atol`. 앞 **10개**만. 양수만 `1 << (값 & 31)`로 OR. 쉼표·따옴표는 구분자가 아니다 | `;`·`,`·공백으로 나눔, 개수 제한 없음 |
| AI가 만들어지는 조건 | `owner != local`(또는 데모)이고 **실제 사제가 있을 때**. `aiName`의 유무는 조건이 아니다 | 맵에 소유자가 있으면 모두 플레이어 |
| 참여 조건 | Player flags `& 0x22`(사람 연결 0x02 또는 AI 부착 0x20). 0x01만으로는 참여가 아니다 | 없음 |

### 소비자가 읽는 방향 (원본 근거가 있는 것)

| 소비자 | 방향 | 근거 |
| --- | --- | --- |
| 사격 표적 | 공격자 → 대상 소유자 | `RawGunTarget`(`allyMasks[attacker] & (1<<owner)`), `RawGunProcess` |
| 방어선(fence) 표적 | 발사자 → 방어선 소유자 | `RawFenceTarget` |
| 승패의 같은 편 판정 | **양방향 모두 참** | `RawMissionOutcome`(`Ally(local,p) && Ally(p,local)`) |
| 수송 도착 동맹 픽업 | 방향별(`alliances[first*9+second]`) | `RawCarrierArrival` (C# 미연결) |

원본 근거가 없는 소비자(보상 지급의 `enemy` 판정, 비행체 표적, 포획 가능 여부, 방어선 기둥 연결, 임시 AI의 적 판정, 에너지 공급)는 **가장 가까운 원본 소비자의 방향**으로 두었고 코드 주석에 "근거 없음/임시"로 표시했다. 방어선 기둥 연결만은 기존 의미(서로 동맹)를 보존하려고 양방향으로 두었다. 이들은 각 소비자 항목(D10·D12·D13)에서 원본으로 확정한다.

## 2. 구현

- [`PlayerParticipation`](../dotnetpj/src/Netstorm.Core/Simulation/PlayerParticipation.cs): cpppj `RawPlayerParticipation`(로그인 `004a6dc0`·동맹 설정 `00490ea0`·방향 추가 `0048f6a0`·brain 부착/해제 `004911a0/00413ae0/00413b20`)의 C# 이식이다. Player 9×0xb4바이트와 81개 DWORD 표를 호출자 저장소로 받아 **같은 바이트를 읽고 쓴다**. 외부 효과(연결·Chat·소멸자)는 훅이며 실제로 도달했는데 비어 있으면 성공으로 바꾸지 않고 예외를 낸다. 쿼리 `Allies(from,to)`·`MutualAllies`·`Participates`를 제공한다.
- [`MissionParticipation.Create`](../dotnetpj/src/Netstorm.Core/Simulation/MissionParticipation.cs): 미션 시작 순서를 따른다 — 모든 Player가 자기 자신을 동맹으로 두고 → 로컬 사람 로그인 → owner 1~8을 차례로 방문해 로컬이 아니고 사제가 있는 소유자마다 `SetAlliance(owner, AllyMask)` → `allies[owner][local]`이 참이면 `AddAlly(local, owner)` → brain 부착(참여 비트 0x20). 클론의 AI에는 원본 brain 객체가 없으므로 토큰은 번호 기반 표식이며 **원본 AI 생성·전략이 복원됐다는 뜻이 아니다**.
- [`MissionStart.AllyMask`](../dotnetpj/src/Netstorm.Core/Rules/MissionStart.cs): 공통 `aiAllyList` + `ai{N}AllyList`를 원본 규칙으로 해석한다. `MyAllyList`·대칭 `AreAllied`는 제거했다. 해석 도우미 [`ConfigNumbers`](../dotnetpj/src/Netstorm.Assets/ConfigNumbers.cs)(`atol`, `;` 항목)를 Assets에 추가했다.
- [`BattleMap`](../dotnetpj/src/Netstorm.Core/Rules/BattleMap.cs): 대칭 `AreAllied`를 방향 질의 `IsAllyOf(from,to)`와 `AreMutualAllies`로 교체했다. 세션·팩토리는 같은 표를 맵과 세션에 공유한다.
- `BattleSession.Participation`: 승패 판정은 이제 **참여자만** 센다(사제가 없는 소유자는 AI가 만들어지지 않으므로 제외). 같은 편 판정은 양방향 조건이다. 사격·비행체·포획·방어선·임시 AI는 위 표의 방향 질의를 쓴다.

## 3. 검증

| 검사 | 내용 |
| --- | --- |
| `X86PlayerParticipationTests` | fixture **608행**(로그인 210·동맹 설정 122·방향 추가 72·부착 192·해제 6·전체 해제 6)의 **Player 1,620바이트·표 324바이트·외부 효과 호출 순서·반환값** 전부 일치(처음 실행부터). 방향 추가가 반대 방향도 켜는 변이·자기 비트 누락·로그인 비트 변경은 모두 실패로 잡혔다 |
| `ConfigNumbersTests` | C++ 정적 검사와 같은 벡터(`atol`·`;` 목록) + `"3,4"`·앞 공백·감김 |
| `MissionOutcomeTests` | 방향별·`myAllyList` 무시·참여 규칙, 마스크의 공통→소유자 순서·`;` 전용·10개 상한·`1<<(값&31)` |
| `PlayerRelationsSessionTests` | 캐논이 **자기 소유자가 선언한 대상만** 쏘지 않음(4조합), 맵 질의의 방향·양방향, 사제가 없는 소유자의 신전 파괴로 승패 사건이 나지 않음. 사격을 대칭으로 되돌린 변이와 참여 필터를 지운 변이는 각각 실패했다(원복함) |

기존 테스트 가운데 영향을 받은 것은 구 API(`AreAllied`/`MyAllyList`)를 쓰던 4곳뿐이다. `CppWorldStartTests`의 동맹 비트 비교는 cpppj의 **값형** 월드 덤프(`MissionPlayers`, x86 관찰 아님)가 `myAllyList`를 읽고 대칭화하므로 TEST01만 원본 규칙의 기대값(각자 자기 비트만)을 직접 적었다.

## 4. 실제 미션 데이터에서 달라지는 동작

원본 미션 스크립트(`originals/d/*.english`) 중 `AllyList`를 쓰는 미션을 변경 전 C# 규칙과 원본 규칙으로 각각 계산해 비교했다(AI는 `ai{N}*` 키가 있는 N, 사제 존재는 맵이 정하므로 근사). **21개 미션**에서 결과가 갈린다. 아래 "→"는 선언 방향이다.

| 유형 | 미션 |
| --- | --- |
| 로컬이 `myAllyList`로만 동맹을 적은 미션 — 원본에서는 AI가 로컬을 선언하지 않으면 **동맹이 아니다** | FW3(1·2), FW5(1·2, 1·3), Imp5(1·6), TEST01(1·2), tnron52a(1·5), tnron74(1·3) |
| AI 한쪽만 선언한 **단방향** — 선언한 AI의 사격만 건너뛴다 | Big_Brothers, FW8, Guard, Home, Razorianwar4, Revenge, Slaughter, Sun, Thunder_Command, TotalAnnihilation, george1, tnron3ex, tnron84, tnron8ex |
| 쉼표 구분 목록 (`"4,5"` → 원본은 4 하나만) | Big_Brothers, Slaughter, TotalAnnihilation |
| 번호 없는 공통 `aiAllyList` (모든 AI 적용) | freedom (`aiAllyList="4"` → 2·3·4 모두 4를 선언) |

예: FW3 은 `myAllyList="2"`지만 `ai2AllyList`에 1이 없어 원본에서는 사람과 AI 2가 서로 적이다. 이 변화는 원본 실행 파일의 키 목록이 근거이며, 각 미션의 실제 플레이 녹화로 다시 확인한 것은 아니다.

## 5. 남은 경계

- **D04(승패 규칙)는 그대로다.** 참여 필터와 양방향 같은 편 판정만 연결했고 `_seenAlive` 기록·엔티티 부재 기반 판정, 튜토리얼 미션 생략, 사건 잠금 순서는 원본과 다르다.
- 같은 AI 공통 키 규칙(`Apply(-1)`)은 `aiName`·`aiColor`·`aiTech`·`aiStartMoney` 등 **다른 키에도** 적용된다. C#은 아직 번호 없는 키를 플레이어 2 전용으로만 읽는다(예: `DarkLord`의 `aiColor`, `WOF6`, `FWOS5`의 `aiTech`). 이번에는 동맹 목록만 바꿨고 이들은 후속 항목이다.
- 참여자는 **사제가 시작 월드에 있는 소유자**로 근사한다. 원본은 `004921f0`의 실제 사제 조회이며 데모(`demo != 0`)에서는 로컬도 AI가 될 수 있다(미구현).
- `Chat`·전송 효과는 단일 플레이에 소비자가 없어 빈 훅이다(동맹 표 자체는 원본 순서로 갱신한다). Outpost·LAN은 범위 밖이다.
- 클론 창(GUI)에서 동맹이 갈리는 미션을 직접 플레이해 보지는 않았다. 위 효과는 단위·세션 테스트가 검증한다.
- D14(공유 Graph·내용물·장부)는 이번 단계에서 시작하지 않았다.
