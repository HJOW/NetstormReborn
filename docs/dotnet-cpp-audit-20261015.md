# cpppj 원본 분석 결과의 dotnetpj 반영 점검

> **2026-10-16 후속:** 이 점검 이후 cpppj에 추가된 로그인·Guide·생산 Deck·AI 초기화·타입별 후처리·데모 등을 포함한 현행 이식 순서는 [cpppj → dotnetpj 이식 계획](dotnet-cpp-port-plan-20261016.md)이다. 아래 D01~D16의 항목 정의는 유지하고, 그 문서 6절이 상태와 이식 단위(W01~W20)의 대응을 갱신한다.

2026-10-15, 작업 PC **VM-W11-Codex**. 점검 기준 커밋은 **`39c76fc7fb90b54f95a484d2be5b68e20a1ba89d`**다. AGENTS.md와 양쪽 인수인계를 읽고 dotnetpj의 대응 소스·기존 검사와 cpppj의 복원 문서·일부 대응 소스를 정적으로 비교했다. 마지막 정밀 디컴파일 PC는 공통 인수인계의 **VM-W11-Codex, 2026-10-15**로 현재 PC와 같다. 새 디컴파일·원본 기계어 관찰·빌드·단위 테스트·게임/GUI 실행은 하지 않았다. 이 문서는 **수정 계획**이며 게임 구현은 변경하지 않았다.

## 1. 현재 진행상황과 판정 기준

| 프로젝트 | 실행·플레이 가능 여부 | 원본 10.78과의 동일성 |
| --- | --- | --- |
| cpppj | 창 모드 메뉴·캠페인·브리핑·월드 표시·사제 이동·일부 실제 교전이 연결됐다. Construct는 비활성이며 Early Missions/캠페인 1-1·1-2 자연 완주는 미완료다. | 다수 함수의 독립 기계어 대조와 같은 세션 연결 검사가 있다. 모든 모듈이 실제 플레이에 연결된 것은 아니다. AI 전략·건설 전체 연결·승패/저장 등은 남아 있다. |
| dotnetpj | 캠페인 1-1·1-2의 건설·생산·이동·전투 조작, 영어/한국어 UI·도움말·옵션·진행값 저장 경로가 구현됐다. | 최근 세션 시계·안내/튜토리얼 프레임 연결은 빌드 확인까지다. 아래에서 확인한 규칙 차이와 실제 완주→저장→재실행→해제의 검증이 남아 있다. 플레이 기능이 있다는 사실을 원본 동일성 완료로 해석하면 안 된다. |

기존 검증 기록은 cpppj **2026-10-15 Release 경고/오류 0, CTest 내부 1,233개 통과**, dotnetpj **2026-10-14 솔루션 Release 경고/오류 0**이다. dotnetpj의 **990개 단위 검사와 GUI 12회/188개 단언 통과는 2026-10-12 앞 단계** 기록이다. 이후 세션 시계·튜토리얼 연결까지 검증한 수치가 아니며 이번 점검에서 재실행하지 않았다. 최신 범위는 [공통 인수인계](../LEFT_JOBS.md)와 [dotnetpj 인수인계](../LEFT_JOBS.dotnetpj.md)를 따른다.

이번 점검은 시간/프레임·튜토리얼·플레이어/동맹·승패·안내 해석/명령·전투/사제 HP·경로·SP/보상·건설·월드 연결·정렬/신전 표시·AI 초기화를 대상으로 했다. 모든 C# 파일, 모든 오디오 장치 경로, 모든 미션/튜토리얼을 전수 대조한 결과는 아니다. 원본 근거는 아래 연결한 복원 보고서와 그 보고서가 명시한 fixture의 범위에 한정한다.

| 분류 | 의미 |
| --- | --- |
| **수정** | 현재 C# 코드와 검증된 원본 계약 사이의 차이를 소스에서 확인했다. 실제 화면에서의 영향 크기는 별도 검증한다. |
| **부분 반영·검증** | 대응 기능이 이미 있으므로 보존하면서 남은 호출 순서·경계·실제 세션 연결을 확인한다. |
| **보류** | cpppj에서도 미검증 초안이거나 미복원이다. 확정 동작으로 이식하지 않는다. |

P0는 시간·월드·플레이어·미션 순서 같은 선행 구조, P1은 확인된 플레이/표시 차이, P2는 후속 분류·범위 확장이다. 우선순위와 항목 ID는 이번 점검의 제안이다. 2026-10-10 사용자 결정대로 **현재 C# 구조로 원본 동작을 재현할 수 있으면 유지하고, 불가능하면 구조를 바꾼다.** 원본의 포인터 배치·메모리 손상·Win32 장치를 그대로 복제하는 것이 완료 기준은 아니다.

## 2. 이미 반영되어 보존할 부분

- 원본 숫자 시계·중첩 정지, 실제 화면 간격의 그리기 제한, 명령 처리 중 시간 진행 방지, 화면 프레임 단위 다리 스캔이 있다. 고정 틱을 사용하는 나머지 규칙과 구분해야 한다. [BattleSession.cs](../dotnetpj/src/Netstorm.Core/Simulation/BattleSession.cs#L305), [세션 시계 작업 기록](dotnet-session-clock-20261012.md).
- 초기 10프레임·재개 7프레임, 후속 0.5초/10프레임과 튜토리얼 2의 double 2/4초 타이머가 이미 구현됐다. `FortMapViewer`의 `finally`에서 화면별 단계 판정도 연결되어 있다. 이를 새 미구현 항목으로 다시 만들지 않는다. 안내 게이트(점검 당시 `MissionDialogFrameGate.cs`, 2026-10-15에 [MissionFrameGate.cs](../dotnetpj/src/Netstorm.Core/Simulation/MissionFrameGate.cs)로 교체), [화면 연결](../dotnetpj/src/Netstorm.Core/Simulation/BattleSession.TutorialFrames.cs), [튜토리얼 타이머 작업 기록](dotnet-stage-frames-20261013.md).
- SID 계산 라이브러리만 있는 상태는 지났다. 저장 객체·다리·건설·생산 등이 공유 서버 번호 풀을 사용한다. 다만 실제 표면 Graph와 파생 수명 전체 연결까지 완료된 것은 아니다. [SidPool.cs](../dotnetpj/src/Netstorm.Core/Simulation/SidPool.cs), [BattleSessionFactory.cs](../dotnetpj/src/Netstorm.Core/Simulation/BattleSessionFactory.cs).
- 공통 월드 표시 목록과 선택 순서 연결, 신전 기본 그림/겹침 그림, 원본 영어 글꼴·커서, 공유 한국어 명칭표, 캠페인 조건/완료값 저장 경로가 있다. 특히 레인 신전 기본 그림이 C#에서도 전혀 없다고 판단해서는 안 된다. [StructureAnimation.cs](../dotnetpj/src/Netstorm.Assets/StructureAnimation.cs#L104), [월드 표시](dotnet-world-display-20261011.md), [진행 저장 검토](dotnet-review-20261011.md).

## 3. 수정·재검증 목록

| ID | 우선순위 | 판정 | 대상 |
| --- | --- | --- | --- |
| D01 | P0 | 수정·부분 반영 | 시간 모델과 24Hz/8틱 상한 |
| D02 | P0 | 부분 반영·검증 | 공통 미션 프레임과 정지 소유권 |
| D03 | P0 | 수정 | 플레이어 참여·방향별 동맹·초기화 |
| D04 | P0 | 수정 | 승패·사제 상태·사건 잠금 |
| D05 | P1 | 수정 | 튜토리얼 1 간헐천·Graph·SP 조건 |
| D06 | P1 | 수정·부분 반영 | 튜토리얼 2 명령·선택·타이머 부수효과 |
| D07 | P1 | 수정 | HTML 조건·토큰·Tell의 두 번 순회 |
| D08 | P1 | 수정 | 스크립트 명령 큐·완료값·비모달 Warning |
| D09 | P1 | 수정 | 사제 직접 피격 HP1·회복·보호막 |
| D10 | P1 | 수정 | 총 피해 시각·표적·투사체 |
| D11 | P1 | 수정 | 표면 기반 경로 탐색·실제 위치 |
| D12 | P1 | 수정 | 비용·환불·보상·SP·공유 난수 |
| D13 | P1 | 수정·부분 반영 | 배치·건설 과정·사제 메뉴 |
| D14 | P0 | 수정·부분 반영 | 표면 Graph·내용물·생성/삭제·장부 |
| D15 | P1 | 수정·부분 반영 | 실제 CRT 정렬·상태별 그림/애니메이션 |
| D16 | P2 | 수정·보류 구분 | AI 초기화와 전략 |

### D01. 시간 모델: 화면 시각은 연결됐지만 규칙은 여전히 틱 기반

**현재 코드:** [BattleSession.cs:113](../dotnetpj/src/Netstorm.Core/Simulation/BattleSession.cs#L113)의 `Seconds`는 `Tick / TicksPerSecond`다. `AdvanceTo`는 원본 게임 시각을 받지만 `AdvanceFrame`이 [FixedTimestep.cs](../dotnetpj/src/Netstorm.Core/Simulation/FixedTimestep.cs)를 통해 규칙 틱을 생성한다. 기본 24Hz·최대 8틱을 넘는 밀린 시간은 버린다. 이동·건설·생산·수확·전투·의식·AI와 여러 애니메이션이 계속 `TicksFor`, `*Tick` 필드를 사용한다.

**반영:** 벽 시각, 정지 가능한 게임 시각, 화면 호출 횟수, 객체별 다음 실행 시각을 구분한다. 최근 연결을 유지하면서 규칙별 원본 기한/정밀도/호출 순서로 옮긴다. `Seconds` 속성 하나를 바꾸거나 기본 틱 수를 늘리는 것으로 완료 처리하지 않는다. 원본 [미션 게이트](exe/cpp-mission-frame-gate-20261015.md), [Regular/Kernel](exe/cpp-process-reconstruction.md), [총 시각](exe/cpp-gun-hit-20261013.md), [실제 Path](exe/cpp-raw-path-search.md)를 기준으로 소비자를 나눈다.

**검증:** 0틱·여러 틱·700ms 이상 지연, 반복/중첩 정지, 기한 직전/동일/직후, 미션 교체와 DWORD 감김을 같은 시간열로 재생한다. 시간열이 다르면 처리 횟수도 달라질 수 있으므로 모든 FPS에서 검사합이 같아야 한다는 가정을 추가하지 않는다.

**진행(2026-10-15, HJOW-X3D):** 5절 1번의 기준선(Release 경고/오류 0, 단위 990개 통과 → 이후 1,012개)을 새로 기록했고, 아직 고정 틱을 쓰는 시간 소비자의 **규칙별 목록을 확정**했다. 이행 자체(24Hz/8틱 상한 제거)는 시작하지 않았다. [상세와 목록](dotnet-mission-frame-gate-20261015.md#5-d01--아직-고정-틱24hz최대-8틱을-쓰는-시간-소비-목록).

### D02. 공통 미션 프레임 전체 순서와 정지 소유권

**현재 코드:** `MissionDialogFrameGate.cs`(점검 당시 파일, 2026-10-15에 [MissionFrameGate.cs](../dotnetpj/src/Netstorm.Core/Simulation/MissionFrameGate.cs)로 교체)는 안내 예약/재개 중심이다. [FortMapViewer.TutorialDialog.cs:107](../dotnetpj/src/Netstorm.Game/FortMapViewer.TutorialDialog.cs#L107)에서 안내→재개→정지 동기화→튜토리얼을 실행하고, 승패는 `BattleSession.UpdateMissionEvents`가 다른 위치에서 호출한다. 원본의 started·tellAt·demo·사건 반환을 포함한 공통 프레임 전체는 하나의 경로가 아니다.

**반영:** 원본 `004c34c0`의 시작 관문→승패→안내 기한/잠금→tellAt→재개→demo→튜토리얼 순서를 같은 현재 상태에 적용한다. `tellAt < 경과`의 엄격 경계와 demo의 포함 경계를 나눈다. 각 효과 뒤 공유 상태를 다시 읽는다. 기존 10/7/0.5초 구현과 F6 지식창/브리핑 위 도움말의 정지 분류는 보존한다. `CapturePause`의 bool·앱의 정지 사유 연결이 이미 정지 중인 상태에서도 원본의 자기 소유 깊이를 보존하는지 대조해야 하며, 정적 읽기만으로 고장이라고 단정하지 않는다. [원본 계약](exe/cpp-mission-frame-gate-20261015.md).

**검증:** `missionframe-x86.tsv`의 상태/호출 순서와 실제 화면 경로를 각각 대조한다. 안내 중 새 창·반복 Tell·다른 정지 소유자·재개 7번째 프레임·효과 중 상태 변경을 포함한다.

**진행(2026-10-15, HJOW-X3D) — 구현·검증 완료:** 원본 004c34c0 전체(시작 여부→승패→안내 잠금→tellAt→재개→데모→튜토리얼 분배)를 C# `MissionFrameGate`로 옮기고 `missionframe-x86.tsv` **328행의 상태 14필드·정지 깊이·호출 순서·반환값을 전부 대조**했다. 실제 뷰어는 구 부분 구현을 버리고 이 게이트의 `RunFrame`을 화면마다 한 번 호출하며, 튜토리얼 단계·타이머는 게이트 상태가 단일 소유한다. 튜토리얼 2 A~I를 화면 경로로 완주하는 헤드리스 검사와 클론 창 스모크(승인 후 실행)도 통과했다. **남은 것:** 승패 규칙 자체(D04), `Postpone` 상위 호출자, tellAt/데모 예약(출하 미션에 없음). [상세](dotnet-mission-frame-gate-20261015.md).

### D03. 플레이어 참여와 동맹을 방향별 상태로 분리

**현재 코드:** [MissionStart.cs:201](../dotnetpj/src/Netstorm.Core/Rules/MissionStart.cs#L201)는 `ListedBy(a,b) || ListedBy(b,a)`로 모든 동맹을 대칭화한다. [BattleSession.cs](../dotnetpj/src/Netstorm.Core/Simulation/BattleSession.cs#L165)는 저장 객체 소유자와 사람 번호로 플레이어를 만들며, `PlayerState`에는 원본 참여 flags/연결·brain 부착 상태가 없다. [BattleSessionFactory.cs](../dotnetpj/src/Netstorm.Core/Simulation/BattleSessionFactory.cs)는 위 단일 동맹 판정을 공급·전투 등에 주입한다.

**반영:** 방향별 9×9 관계, 참여 상태, 실제 사제 존재와 초기화 단계를 구분한다. 원본 참여 기준은 `flags & 0x22`이며 `aiName`의 존재나 맵에 등장했다는 사실과 다르다. AI의 AllyList 행 설정 뒤 **AI→로컬** 관계에 따라 로컬의 반대 방향을 더하는 예외가 있으나 모든 AI 사이를 대칭화하지 않는다. 소비자마다 원본이 읽는 방향/양방향 조건을 적용한다. [실제 로그인·동맹·AI 부착](exe/cpp-player-participation-20261015.md), [미션 시작](exe/cpp-mission-start-20261015.md).

**검증:** `player-participation-x86.tsv`·`mission-start-x86.tsv`를 재사용해 한 방향만 지정, 로컬 번호 변경, demo, 이름 없는 AI, 사제가 없는 플레이어, 초기화 전 승패 거부를 확인한다. 단지 기존 `AreAllied`를 AND로 바꾸는 것도 원본 시작 시 설정하는 반대 방향을 놓친다.

**진행(2026-10-15, HJOW-X3D) — 구현·검증 완료(D04의 승패 규칙 자체는 제외):** `player-participation-x86.tsv` **608행**을 Player 1,620바이트·표 324바이트·호출 순서·반환값까지 C# `PlayerParticipation`으로 대조했고, 미션 시작 순서(`MissionParticipation`)·공통 `aiAllyList`·`;`/`atol`/10개 규칙을 적용했다. `myAllyList`는 **원본 실행 파일에 없는 키**라 읽지 않으며 대칭 `AreAllied`는 방향 질의로 교체했다. 원본 미션 21개에서 동작이 달라진다. **남은 것:** 데모의 로컬 AI, 사제 조회 근사, 공통 `ai*` 키 전체, 근거 없는 소비자 방향(D10·D12·D13), `mission-start-x86.tsv` 연결. [상세](dotnet-player-relations-20261015.md).

### D04. 승패 판정을 Seen 기록·엔티티 부재 대신 원본 상태에 맞춤

**현재 코드:** [BattleSession.Sacrifice.cs:679](../dotnetpj/src/Netstorm.Core/Simulation/BattleSession.Sacrifice.cs#L679)는 튜토리얼을 건너뛰고, 사람 외 플레이어의 신전/사제 엔티티 존재와 `_seenAlive`로 조건을 판단한다. 한 번 본 사제가 모두 없어졌을 때 팀 전멸을 알린다. `TellOnce`는 이벤트 발행용 집합을 사용한다.

**반영:** 원본은 활성 Player 1~8, 사제 소유자 상태 1/2, 현재 신전 SID/HP/상태, 로컬↔대상의 양방향 관계와 현재 지면 소유자를 사용한다. 신전 부재에서 halfDead/dead가 참인 조건도 보존한다. 섹션 없음·Tell 차단·demo·섹션 효과 뒤 잠금 설정 순서를 구분한다. 초기화가 갖춰지기 전에 자동 승패를 켜거나 이전의 생존 기록으로 초기화 부족을 가리지 않는다. [RawMissionOutcome 계약](exe/cpp-mission-outcome.md), [참여 선행조건](exe/cpp-player-participation-20261015.md).

**검증:** `missionoutcome-x86.tsv`와 참여 상태를 조합하고 신전 HP 정확히 절반/그 아래, 포획/회수, 섹션 없음/차단, 효과가 플레이어 상태를 바꾸는 경우를 확인한다. 실제 플레이 승리·실패·저장/재시작은 별도 완료 조건이다.

### D05. 튜토리얼 1의 임시 간헐천과 연결 판정 교체

**현재 코드:** [BattleSessionFactory.cs:54](../dotnetpj/src/Netstorm.Core/Simulation/BattleSessionFactory.cs#L54)는 저장 간헐천이 없으면 세션 생성 시 [TutorialGeysers.cs](../dotnetpj/src/Netstorm.Core/Simulation/TutorialGeysers.cs)의 Gap=4/5×5 받침과 간헐천 하나를 추가한다. [TutorialStages.cs](../dotnetpj/src/Netstorm.Core/Simulation/TutorialStages.cs#L177)의 C에는 생성 호출이 없고, D는 다리 버전별 `CanReachAnyGeyser` 결과를 캐시한다. E/F는 `battleShown` 관문 없이 정수 SP를 검사한다.

**반영:** A의 현재 F4/reclaim 명령과 집계 조회 생략·반환, C의 19개 기준 뒤 생성 호출, D의 **현재 신전 Graph와 최근 생성 최대 5개 목록** 비교, E/F의 battleShown·float32 SP 200/600 조건을 적용한다. C의 Advance가 정지/잠금으로 거절돼도 생성 효과는 남는다. 전체 간헐천 목록이나 경로가 닿는다는 결과로 최근 목록/Graph를 대신하지 않는다. 생성 후보·지역 사각형·공유 보상 풀은 원본 모듈 계약부터 대조하고, cpppj의 실제 Session 전체 연결이 끝났다고 가정하지 않는다. [Early1](exe/cpp-tutorial-one-20261015.md), [로컬 생성](exe/cpp-local-geyser-20261015.md), [지역 사각형](exe/cpp-player-geyser-bounds-20261015.md).

**검증:** `tutorial-one-x86.tsv`·`local-geyser-x86.tsv`·`player-geyser-bounds-x86.tsv`, 생성 전/후·목록 변경·다른 간헐천만 연결·SP 경계/NaN·reclaim 반환을 대조한다. 기존 누적 다리 1/8/19개 검사는 보존한다.

**진행(2026-10-15, HJOW-X3D) — 판정 부분 완료:** `RawTutorialOne`을 C# `TutorialOne`으로 옮겨 `Stage` 행 **266개**(반환값·호출 순서)를 대조하고 `TutorialStages`가 이를 호출하도록 연결했다(A의 F4/누적 수, C의 생성 호출, D의 다중 Advance·최근 5개 목록, E/F의 조회 허용 + float32 SP). 표면 Graph 번호는 "사제가 걸어 닿는가"로 근사하고, **간헐천 생성은 아직 세션 생성 시점의 근사**이며 reclaim 토큰은 쓰이지 않는다. 생성 후보·지역 사각형·공유 보상 풀(`local-geyser`·`player-geyser-bounds`)은 남았다. [상세](dotnet-ledger-tutorial-one-20261015.md).

### D06. 튜토리얼 2의 성공 사건 대체와 부수효과 순서

**현재 코드:** [TutorialStages.cs](../dotnetpj/src/Netstorm.Core/Simulation/TutorialStages.cs)는 C/F의 double 타이머와 선택 해제 후 예약 유지를 이미 구현했다. H는 현재 명령 ID 대신 **Salvaged 성공 이벤트**를 사용한다. G는 `Advance` 안에서 타이머를 지우므로 Advance가 거절되면 원본의 마지막 timer=0 쓰기가 보장되지 않는다. 선택은 단일 엔티티 조회, 장부는 이름 기반 `_made`다.

**반영:** H는 상위 비트를 제거한 현재 reclaim 명령으로 예약한다. G의 마지막 timer=0과 B/E/F/G의 선택 정리는 단계 증가 성공 여부와 별개로 유지한다. C의 두 번 현재 선택 조회, NotVortex 성공 즉시 정지 뒤 후속 Advance 거부, A의 옵션 두 BYTE/공급 범위 갱신과 공용 장부 조회 순서를 맞춘다. 기존 2/4초 포함 경계와 프레임 경로는 유지한다. [원본 A~H 전체 계약](exe/cpp-tutorial-two-20261015.md).

**검증:** `tutorial-two-x86.tsv`의 346개 입력을 재사용한다. 단순 단계 글자뿐 아니라 선택·옵션·타이머·정지 깊이·외부 효과 순서를 비교한다. 현재 `TutorialStagesTests`의 헤드리스 경로 통과만으로 `ConnectTutorialScreenFrames` 이후를 검증했다고 하지 않는다.

### D07. 조건 필터와 Tell 파이프라인

**현재 코드:** [MissionConditions.cs](../dotnetpj/src/Netstorm.Assets/MissionConditions.cs)는 단일 visible 플래그로 조건을 제거하며 `Evaluate`는 왼쪽부터 치환한다. 인용된 `>`를 구별하는 전체 HTML cursor 대신 `IndexOf('>')`를 사용하고 토큰 뒤 공백을 모두 TrimEnd한다. [MissionScript.cs:98](../dotnetpj/src/Netstorm.Assets/MissionScript.cs#L98)와 [TutorialDialogScript.cs:353](../dotnetpj/src/Netstorm.Assets/TutorialDialogScript.cs#L353)는 필터 결과에서 제목/버튼을 뽑아 HTML 부분집합을 그린다.

**반영:** 선행 전체 Config 치환은 유지한다. 그 뒤 현재 조건 토큰을 **오른쪽→왼쪽**으로 다시 치환하고 shown/depth, 인용/78입력바이트 제한, 첫 출력 공백 보존, 현재 cursor 규칙을 맞춘다. 원본 Tell의 1차 HTML 효과→내용 관문→종류 우선 메타 제거→LF/BR→같은 상태의 2차 HTML 순서를 대응시킨다. 첫 순회의 명령 효과와 표시용 두 번째 순회를 중복 실행하지 않는다. 한국어 본문 판정을 영문 ASCII alnum 관문으로 일괄 바꾸지 않는다. 그 영역은 cpppj에서도 미검증이다. [조건](exe/cpp-html-condition-20261015.md), [실제 두 순회 연결과 제한](exe/cpp-dialog-condition-flow-20261015.md).

**검증:** `htmlcondition-x86.tsv`·`htmltext-x86.tsv`·`htmlpicture-x86.tsv`, 숨김 중 조건 중첩, 인용 꺾쇠, 공백 한 글자/78바이트, Config 변경 시 조회 순서, 명령만 있는 페이지, 메타 삭제 범위·스타일/그림을 확인한다.

### D08. 명령 예약과 비모달 Warning

**현재 코드:** [TutorialDialogScript.cs:226](../dotnetpj/src/Netstorm.Assets/TutorialDialogScript.cs#L226)는 `OpenSection`에서 `ConfigHandler`를 즉시 호출한다. 정규식으로 Config만 골라 저장하고 기본 닫기 버튼도 합성한다. 전체 `<$...>`를 원본 공통 입력 큐에서 순서대로 소비하는 경로와 20칸 Warning 게시판은 대응 소스에서 확인되지 않았다.

**반영:** 원본은 inline 명령을 예약한 뒤 **다음 Input**에서 Config/Warning을 처리한다. 같은 큐에 원문 순서와 문자열 수명을 보존하고, 명령만 있는 Tell은 기존 페이지·정지·캡처를 유지한다. `PriestImmobile`은 OK가 있는 모달이 아니라 중복을 허용하는 20칸 게시판의 **벽 시각 10초 Warning**이다. 게임 정지 가능한 시각과 분리한다. 원본 40칸 큐/80칸 문자열 고리의 정상 동작 경계는 대조하되 손상 포인터를 C#에 재현할 필요는 없다. [명령 큐/저장](exe/cpp-script-command-20261015.md), [Warning](exe/cpp-run35-20261015.md).

**검증:** `script-command-x86.tsv`·`warnings-x86.tsv`를 사용하고, 예약 전/다음 입력 후 Done 값, 여러 Config/Warning의 순서, 명령 전용 섹션의 페이지 보존, 20칸 포화·중복·10초 만료·정지 중 만료를 검사한다. 기본 OK와 링크/입력 컨트롤은 cpppj의 미완료 경계를 확인하고 별도로 다룬다.

### D09. 사제 HP1과 회복 이벤트

**현재 코드:** [BattleSession.Combat.cs:278](../dotnetpj/src/Netstorm.Core/Simulation/BattleSession.Combat.cs#L278)는 피해 후 사제가 최대 HP 절반 이하이면 **절반으로 다시 올리고** 기절시킨다. 같은 파일의 `PriestRecoveryPerSecond=5`로 매 틱 회복한다. [CombatTests.cs:155](../dotnetpj/tests/Netstorm.Core.Tests/CombatTests.cs#L155)도 절반 HP를 정답으로 고정한다.

**반영:** 원본 직접 피격은 HP를 **1 이상**으로 제한하고 별도 HP setter가 절반 경계의 공간/프레임 전환을 처리한다. HP1에서 표적 감시 해제·보호막·소리·비모달 안내가 이어진다. 현재의 단일 bool/직접 HP 쓰기로 회복 예약 취소·피격 통지·공간 변경을 생략하지 않는다. 회복 `0x25a`, 관련 Regular와 공유 난수/SP 추적 효과는 별도 원본 분기대로 이식한다. cpppj의 현재 getter 연결이 미완료인 부분은 이식 완료로 취급하지 않는다. [피격](exe/cpp-priest-damage-20261013.md), [회복](exe/cpp-priest-regen-reconstruction.md), [같은 월드의 HP1 교전](exe/cpp-run23-20261015.md), [SP 현재 조회의 남은 경계](exe/cpp-sp-store-notice-20261015.md).

**검증:** `priestdamage-x86.tsv`·`priestworldevents-x86.tsv`·`priestregen-x86.tsv`를 기준으로 절반 직전/동일/아래·HP1·반복 피격·회복 취소·표적 종료를 확인한다. 기존 잘못된 절반 HP 기대를 원본 fixture와 충돌하는 회귀 기준으로 유지하지 않는다.

### D10. 총의 양자화 시각·표적 검색·투사체 수명

**현재 코드:** [BattleSession.Combat.cs](../dotnetpj/src/Netstorm.Core/Simulation/BattleSession.Combat.cs#L110)는 거리/ID로 표적을 고르고, 발사 시 `damageRate × interval`을 `CombatShot`에 저장해 도착 틱에 적용한다. 그림 상태와 별도로 기본 1초 간격 및 틱 예약을 사용한다.

**반영:** 원본 GunHit은 WORD의 1/8초 시각·4096초 감김과 DPS×실제 양자화 경과를 사용한다. 경과가 음수 또는 30초 초과이면 3초분이며 정확히 30초는 전체 피해다. 표적 검색은 원본 공간 순서·group별 사거리/관측각·현재 프레임 허용을 사용한다. 실제 썬 디스크 타입83/효과160, 현재 표적 SID/감시와 공통 삭제의 연결을 대조한다. 발사기 타입71과 투사체83을 혼동하지 않는다. [피해](exe/cpp-gun-hit-20261013.md), [표적](exe/cpp-gun-target.md), [실제 충돌/종료](exe/cpp-run23-20261015.md).

**검증:** `gunhit-x86.tsv`·`guntarget-x86.tsv`·`gunimpact-x86.tsv`·`campaign-hit-x86.tsv`로 0.125초 경계·30초·감김·대상 소멸·HP1 이후 감시 해제를 검사한다. 모듈별 기대값과 실제 세션 교전 대조를 구분한다.

### D11. 경로 탐색과 시각 위치

**현재 코드:** [BattleSession.Harvest.cs](../dotnetpj/src/Netstorm.Core/Simulation/BattleSession.Harvest.cs#L196)는 칸 거리표와 PriorityQueue를 사용하고 대각선 선행 동률 순서로 경로를 고른다. [BattleSession.Movement.cs](../dotnetpj/src/Netstorm.Core/Simulation/BattleSession.Movement.cs#L62)는 틱별 진행량과 별도 화면 보간을 사용한다. 길 변화 재탐색/대기는 이미 있다.

**반영:** 원본의 현재 표면 SID/Graph·방향별 Bridge 연결을 기반으로 BFS·역추적·축약·Init·경로 append를 맞춘다. 원본은 단순 최단 거리뿐 아니라 방문 방향과 이전 방향 비용을 사용한다. 화면과 전투가 읽는 좌표, 정지 중 경로 변경과 도착 효과도 같은 Path 상태에서 파생해야 한다. [검색/Init](exe/cpp-raw-path-search.md), [서버 Path 시각 준비](exe/cpp-raw-path-data.md).

**검증:** `pathsearch-x86.tsv`·`pathinit-x86.tsv`·`pathruntime-x86.tsv`·`pathdata-x86.tsv`로 분기 길·모서리·같은 표면·표면0·append/재탐색·double 기한을 대조한 뒤 실제 사제/수확/운반 도착을 검사한다.

### D12. SP·보상·환불과 난수 소비

**현재 코드:** [PlayerState.cs:13](../dotnetpj/src/Netstorm.Core/Simulation/PlayerState.cs#L13)는 SP를 int로 저장한다. [GameEntity.cs:183](../dotnetpj/src/Netstorm.Core/Simulation/GameEntity.cs#L183)의 비용은 단일 타입 비용이고 회수는 HP와 무관한 25%다. [전투 보상](../dotnetpj/src/Netstorm.Core/Simulation/BattleSession.Combat.cs#L296)은 자기/동맹이면 임시로 지급을 막으며, 공통 삭제 보상의 잔여 가치를 간헐천 자원 풀로 모으는 대응 상태가 없다.

**반영:** float32 SP와 비용/개수 곡선·HP 비례 절삭 환불·타입별 예외·삭제 flags 수신자·AI 지갑/잔여 간헐천 풀을 분리한다. 원본의 상태 변경/통지 순서를 적용한다. 10.78은 SP 저장 시 전역 게임 난수 160회를 소비하고 회복/소리도 난수 순서에 영향을 줄 수 있다. 난독화 배열을 C#에 그대로 복제할지는 행동 동일성으로 판단하되, 공유 난수 소비·float 비트·현재 조회를 생략한 채 같은 게임이라고 판정하지 않는다. 저장소 회전 자체는 추가 난수를 소비하지 않는다. [보상/저장](exe/cpp-reward-reconstruction.md), [회전](exe/cpp-sp-rotation.md), [현재 SP/Player 통지](exe/cpp-sp-store-notice-20261015.md).

**검증:** `reward-x86.tsv`·`sprotate-x86.tsv`·`player-content-notice-x86.tsv`로 분수 비용/손상 HP·nugget/altar/rainBlocker 예외·수신자·풀·난수 상태를 비교한다. 이어 SP 쓰기 후 다리 추첨/회복의 같은 난수열을 검증한다.

### D13. 배치·건설 과정과 사제 메뉴

**현재 코드:** [BattleMap.cs](../dotnetpj/src/Netstorm.Core/Rules/BattleMap.cs#L202)의 영역/점유/에너지 검사와 `BattleSession.Construction/Commands/Production`의 직접 배치·차감·완료 경로를 사용한다. [ConstructionTimes.cs](../dotnetpj/src/Netstorm.Core/Simulation/ConstructionTimes.cs)의 10초는 기존 관찰에서 잡은 상수다. [사제 메뉴](../dotnetpj/src/Netstorm.Game/FortMapViewer.ContextMenu.cs#L224)는 종류/목록 중심으로 별도 조립한다.

**반영:** 이미 원본 대조된 MayPlace·CanonDecoder의 조각 순서, 예측→확정→통지→예측 제거→실제 배치, 비용 부족/환불, 사제 도착/건설 Process·완료·삭제를 동작 단계로 비교한다. 원본도 특정 끝 시각 재설정은 현재+10초이므로 **10이라는 값 자체를 잘못이라고 바꾸지 않는다.** 문제는 호출 시점·기한·객체/사제 수명이다. 메뉴는 현재 기술 내용물·원소·사제 사건/프레임·신전 SID·제외 목록과 선택 시 SP 재조회까지 맞춘다. [MayPlace](exe/cpp-canon-mayplace-reconstruction.md), [배치](exe/cpp-construction-place-reconstruction.md), [건설 Process](exe/cpp-construction-process-reconstruction.md), [끝 시각](exe/cpp-construction-time-reconstruction.md), [메뉴](exe/cpp-priest-menu-20261015.md).

512바이트 메시지와 실제 전송 길이는 원본 계약 근거지만, 이번 싱글플레이 규칙 이식의 첫 작업으로 TCP/IP 전송을 구현할 필요는 없다. 내부 명령으로 같은 효과 순서를 보장할 수 있는지 먼저 판단한다. cpppj도 실제 Session의 Confirm/Notice/Clear/Place 전체 연결은 미완료다. [패킷의 범위](exe/cpp-construction-packet-20261015.md).

**검증:** `canonmayplace-x86.tsv`·`canonpermission-x86.tsv`·`canonplacement-x86.tsv`·`constructionplace-x86.tsv`·`constructionconfirm-x86.tsv`·`constructionclear-x86.tsv`·`constructionnotice-x86.tsv`·`constructionprocess-x86.tsv`·`constructiontime-x86.tsv`·`priestmenu-x86.tsv`를 필요한 순서로 연결한다. 비용 부족·취소·도착 전 사제 소실·중복 배치·완공 후 장부·재충전을 검사한다.

### D14. 공유 월드·Graph·내용물·장부의 실제 연결

**현재 코드:** [BattleSessionFactory.cs](../dotnetpj/src/Netstorm.Core/Simulation/BattleSessionFactory.cs)는 미리보기 지면과 영역 표를 월드 입력으로 쓰며 실제 `SurfaceGraph`를 조립하지 않는다. `BridgeGrid.graphSurfaces` 공급자도 여기서 주입하지 않아 별도 연결 크기 계산을 사용한다. [BattleMap.cs](../dotnetpj/src/Netstorm.Core/Rules/BattleMap.cs#L76)는 noIsland에 SID를 주지만 엔티티/점유 등록에서는 제외한다. [PlayerState.cs](../dotnetpj/src/Netstorm.Core/Simulation/PlayerState.cs#L38)는 이름 기반 단일 누적 표를 사용한다. [RemoveEntity](../dotnetpj/src/Netstorm.Core/Simulation/BattleSession.Combat.cs#L347)는 덱·점유·SID·일부 작업을 직접 정리한다.

**반영:** 표면 객체 identity/Graph·영역 소유·SID·내용물·선택·감시·통계/누적 제작·보상·UI 통지가 같은 현재 객체 상태를 보게 한다. 생성/Pop과 Unpop/삭제의 원본 순서를 대응시킨다. 기존 공유 SID와 표면/점유 계산 라이브러리는 재사용한다. `RawTotalMade`의 세 장부와 허용표, 범위/감김 규칙을 단순 `Dictionary.Sum`으로 대체하지 않는다. 파일에서 Deck/Technology/Contents를 파싱하는 것과 실제 소유자·기술 내용물/생산으로 복원하는 것은 다른 완료 항목이다. 초기 포트 설치/내용물 적용은 별도 대조가 필요하다. [현재 세션 수명](exe/cpp-session-lifetime-20261015.md), [기술 내용물](exe/cpp-player-technology-20261015.md), [집계 계약](exe/cpp-tutorial-one-20261015.md), [캠페인 설치 경계](exe/cpp-raw-fort-install-boundaries-audit-2-72d4d.md).

**검증:** 기존 graph/spot/SID fixture를 보존하고 새로 원본 Pop/삭제/내용물/집계 자료를 연결한다. 신전·워크샾·다리 제거 시 Graph 분할/소유권·선택·생산·감시·누적 수·보상/UI가 함께 일관되는지 같은 세션에서 확인한다. 복사한 지도와 별도 Graph를 각각 통과시킨 검사만으로 통합 완료 처리하지 않는다.

**진행(2026-10-15, HJOW-X3D) — 장부 부분 완료:** `RawTotalMade`를 C# `TotalMade`로 옮겨 `tutorial-one-x86.tsv` `Total` 행 **PATCH 52개**(표 4개 전체 바이트)를 대조하고, 세션에 로컬 소유자의 현재 수·누적 제작 수·전체 현재 수 장부로 연결했다(완공/실체화/배치/다리 칸 등록, 삭제·붕괴 때 현재 수만 감소). 이름 기반 `PlayerState` 누적 표는 삭제했다. **남은 것:** 실제 표면 `SurfaceGraph` 조립과 `BridgeGrid.graphSurfaces` 연결, 영역 소유·내용물·선택·감시의 공유 상태, 금지 표와 `TechPermissions`의 일원화, `ResetCounts` 호출 시점 확정. [상세](dotnet-ledger-tutorial-one-20261015.md).

### D15. CRT 동률 정렬과 신전/애니메이션 출력

**현재 코드:** [DrawOrder.cs:34](../dotnetpj/src/Netstorm.Core/Display/DrawOrder.cs#L34)는 “원본 qsort 미확정” 설명과 안정 정렬을 유지한다. 이 설명은 이미 cpppj의 2026-10-11 복원으로 오래된 상태다. [StructureAnimation.VortexFrames](../dotnetpj/src/Netstorm.Assets/StructureAnimation.cs#L104)는 기본 그림+겹침 그림을 지원하지만 시간 기반 일반 프레임 선택이며 원본 레인 신전의 WORD 상태 분기를 직접 모델링하지 않는다. 그림자/효과도 근사 경로가 남아 있다.

**반영:** 정렬은 10.78 CRT의 작은 구간/피벗/교환과 동률 순서를 이식한다. 같은 키 1~9의 원본 출력은 **9,5,3,4,2,6,7,8,1**이며 안정 정렬과 다르다. 한 객체의 기본/물결/상태별 후속 그림을 전체 정렬에서 흩뜨리지 않는다. 레인 신전의 기본0→현재 물결→WORD 상태별 그림/상승음 순서와 실제 원본 타이머/프레임을 반영한다. 기존 기본/겹침 그림과 공통 표시 목록은 보존한다. [CRT 정렬](exe/cpp-draw-sort-reconstruction.md), [레인 신전](exe/cpp-run34-20261015.md), [공통 프레임](exe/cpp-frame-advance-reconstruction.md).

**검증:** `drawsort-x86.tsv`·`rain-draw-x86.tsv`의 순서와 같은 픽셀 최종색, 부분 갱신/클립·클릭 우선순위를 확인한다. 그림자/팔레트·다른 유닛의 전체 픽셀·실제 청취는 별도 대조다. 기존 C#의 겹침 509픽셀 검사를 전체 원본 렌더러 동일성으로 확대하지 않는다.

### D16. AI는 초기화 확정 부분과 미복원 전략을 분리

**현재 코드:** [BattleSession.CampaignAi.cs](../dotnetpj/src/Netstorm.Core/Simulation/BattleSession.CampaignAi.cs)는 명시적으로 임시 방어 AI다. 고정 group 우선순위·신전 주변 위치 탐색·틱 간격·수집자 생성·방어 상한을 사용하며 `AiKnowledgeOf`는 공통 `Mission.AiKnowledge`에서 고른다. 실제 플레이어별 현재 기술 내용물의 원본 분류기가 아니다.

**반영:** 검증된 생성자/초기 갱신의 소유자별 기술·이름·builder/원소 분류·최대 요구에너지 길이·**가장 비싼 워크샾 선택**부터 대응 API를 설계한다. 초기 갱신은 기술을 일괄 지우거나 SP/지갑을 초기화하지 않는다. 기존 임시 AI를 원본 AI라고 표기하지 않는다. [생성자](exe/cpp-ai-brain-20261015.md), [초기 갱신](exe/cpp-ai-initial-update-20261015.md).

**보류:** mana 메타데이터 4파일은 CMake 미등록·빌드/새 native 대조 미실행 초안이다. `invalidGraph` 현재 getter와 실제 Session AI 활성화/전략도 남아 있다. 이 초안을 C# 기대값으로 가져오거나 원본 전략을 새 추정으로 채우지 않는다. [초안 경계](exe/cpp-ai-required-energy-20261015.md).

**검증:** `ai-brain-x86.tsv`·`ai-initial-update-x86.tsv`로 소유자/기술·재호출 누적·비용 동률/최댓값·원소 분류를 비교한다. 전투 승리만으로 초기화·전략이 맞았다고 판정하지 않는다.

## 4. 검증 자료의 이식 방법

[Core 검사 프로젝트](../dotnetpj/tests/Netstorm.Core.Tests/Netstorm.Core.Tests.csproj)에는 bridge/surface/bridgedecay/graph/SID/hash/renderer/display의 cpppj fixture 링크와 별도 clockkernel/world-start 자료가 있다. 최신 missionframe/tutorial/participation/outcome/HTML/script/gunhit/reward/path/drawsort/AI 자료는 위 목록과 같은 방식으로 모두 연결된 상태가 아니다. `world-start-1078.tsv`는 **C++ 월드 덤프**이며 원본 기계어 fixture와 출처 수준이 다르다. [공유 로더](../dotnetpj/tests/Shared/X86Fixture.cs)를 확장할 때도 이 구분을 유지한다.

1. 기존 C# 검사를 먼저 수집·실행해 현재 통과/실패와 SDK·커밋을 새로 기록한다. 과거 990개 숫자에 맞추기 위해 기대값을 바꾸지 않는다. 화면 경로를 연결하지 않는 기존 헤드리스 검사와 실제 앱 경로를 구분한다.
2. 각 원본 보고서의 허용 입력·외부 관찰 경계·판본 열·단위를 읽고 **10.78** 기대값을 연결한다. 기존 fixture를 복사하거나 C# 구현으로 재생성하지 않는다. C++ 검사와 생성 도구는 열 의미를 확인하는 데 사용한다.
3. 반환값뿐 아니라 float/double 비트·정수 감김·호출 순서·현재 상태 재조회·난수·큐·선택/장부/수명 효과를 비교한다. 기존 추정 테스트와 충돌하면 원본 자료로 이유를 기록하고 추정 기대를 교정한다.
4. 모듈 대조 뒤 실제 한 세션에 조립해 같은 객체·시계·SP·Graph·플레이어를 사용하는지 검사한다. fixture의 외부 관찰 콜백을 빈 성공으로 연결한 통과는 실제 플레이 완료가 아니다.
5. 마지막에 영어/4:3 기본 경로의 자연 플레이를 먼저 검증한다. Early1/2·캠페인1-1/1-2의 입력→진행→승패→저장→종료→재실행→다음 미션 해제와 도움말/옵션/정지/복귀를 확인하고, 기존 한국어·와이드·전체화면 저장/재실행 회귀를 확인한다. 강제 완료 섹션·합성 전투를 자연 완주로 합산하지 않는다.

## 5. 제안 작업 순서와 완료 조건

| 순서 | 작업 단위 | 선행·완료 기준 |
| --- | --- | --- |
| 1 | 현재 검사 기준선 + D01/D02 화면 연결 대조 | 최근 시계/프레임 구현을 실제 연결 경로로 검증하고 남은 규칙별 시간 소비 목록 확정 — **완료(2026-10-15)**, [상세](dotnet-mission-frame-gate-20261015.md) |
| 2 | D14 공유 상태·장부와 D03 참여/방향별 동맹 | 같은 월드의 생성/삭제·플레이어 상태/초기화 계약을 갖춤. 기존 SID/Graph 계산 검사 보존 — **D03 완료(2026-10-15)**, [상세](dotnet-player-relations-20261015.md). D14 남음 |
| 3 | D12 SP/보상·D09 사제·D10 피해·D11 경로 | 원본 수치/시간/수명 fixture와 실제 객체 연결을 대조. 회복 getter 등 미완료 경계는 남겨서 표시 |
| 4 | D07/D08 안내 처리와 D04 승패 | 같은 프레임·현재 상태·명령 큐·사건 잠금/정지 소유권을 연결. 초기화 이전 자동 승패 금지 |
| 5 | D13 건설·D05/D06 Early1/2 | 공통 배치·생산·장부·간헐천/Graph와 연결하고 실제 입력으로 단계 진행 검증 — D05의 판정기·D14의 장부는 2026-10-15 선행 완료([상세](dotnet-ledger-tutorial-one-20261015.md)), 생성·Graph·D06·D13 남음 |
| 6 | D15 표시 복원·D16 검증된 AI 초기화 | 동률 픽셀/표적 우선순위와 원본 초기 분류 대조. 미복원 전략은 보류 표시 유지 |
| 7 | 자연 완주·저장·재실행·다음 미션 | 모듈·세션·GUI·자연 플레이 결과를 각각 기록. 다른 미션 공개는 해당 규칙 검증 후 판단 |

한 항목을 끝낼 때 코드 변경·근거 fixture/버전·새 검사와 실제 실행 범위·남은 효과 경계를 함께 기록하고 [dotnetpj 인수인계](../LEFT_JOBS.dotnetpj.md)의 해당 항목을 완료 또는 부분 완료로 갱신한다. cpppj에서 검증되지 않은 전략, LAN/아웃포스트·MCP, 새 60/120프레임·Linux 확장, D2Coding 1.4.0 교체를 이 점검의 선행 구현으로 추가하지 않는다. 기존 기능의 회귀는 보존한다. 원본/클론 GUI 실행은 [실행 확인 규칙](execution-confirmation.md)을 따른다.

## 6. 이번 문서 작업의 검증 범위

- 게임 코드·검사 코드·원본 자료·디컴파일 산출물 변경 없음. cpppj 완주 개발의 기존 일시 중지는 유지한다.
- 문서에 연결한 저장소 파일·소스 줄 참조·fixture 파일명과 UTF-8·공백을 검사한다. 문서 검사 결과는 인수인계에 기록한다.
- 이 문서의 “수정”은 정적 대조 결과다. 새 테스트 통과·실제 증상 재현·원본 관찰을 수행한 것으로 해석하지 않는다.
