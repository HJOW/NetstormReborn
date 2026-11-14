# cpppj 테스트 전용 캠페인/스커미시 자동 플레이 — 아군 AI·아군/적 난이도·시작 SP (2026-11-14)

마지막 디컴파일 PC 호스트명: HJOW-Athlon(변경 없음, 재디컴파일 없음). 사용자 요청(2026-11-14): 캠페인 플레이 검증은 우연성이 커서 재현이 어렵다. 게임에 이미 있는 AI를 **아군 쪽(로컬 플레이어)에도** 붙여 자동으로 플레이시키고, 아군 AI는 어려움·적 AI는 쉬움·시작 SP는 승리를 재현하기 쉽게 조절할 수 있어야 한다. **테스트 전용**이므로 일반 사용자 플레이에서는 켤 수 없어야 하며, 처음에는 캠페인만 대상으로 했다. 같은 날 후속 요청으로 **스커미시까지 확장**하고, 다른 AI도 쓸 수 있게 **[AI 에이전트용 사용 안내](../testing/cpp-autoplay-test-guide.md)**를 따로 만들었다(사용법은 그 문서가 기준이다).

## 1. 결론

- 원본 10.78에는 **데모 모드에서 로컬 플레이어에도 AI를 붙이는 분기**가 이미 있다(`RawMissionStart::Start`의 `owner!=LocalOwner() || demo!=0`, 원문 `004c2b20`). AI 몸체도 로컬 소유자 전용 처리(로컬 SP 동기화 `RawAiPreparation`, 로컬 드롭 음향 `RawAiCollectors`, 로컬 생산 등록 `RawAiStrategySetup`)를 갖고 있다. 그러나 일반 캠페인에서 이 분기를 켜는 방법은 없었다(데모 플래그를 켜면 360초 재시작·데모 안내·AI 지갑 하한 등 다른 동작까지 바뀐다). 원문 `MakeAi` 명령(`00417fa0`)도 있지만 키 바인딩/UI 스크립트로만 부를 수 있고 이름·동맹을 덮어쓴다.
- 그래서 **명령줄 전용 테스트 옵션**을 추가했다. 원문 복원 몸체(`o::RawMissionStart` 등)는 바꾸지 않고, 클라이언트 연결층(`RawCampaignMissionStartBindings`)이 원본 Start가 끝난 뒤 로컬 플레이어에 원문 `MakeAi` 규칙의 AI를 붙인다.
- 조사 중 **11차 난이도의 판정 결함**을 발견해 함께 고쳤다: `ai2AllyList="1"`처럼 미션이 정한 동맹 AI는 AI 생성자 안에서야 동맹표에 기록되는데, 난이도는 그 전에 정해져 **동맹 AI가 적으로 분류되어 옵션 난이도(쉬움/어려움)를 받았다**(GuardMyBack·EnemyTerritory·ToTheRescue 등). 이제 설정의 동맹 목록(+23c)도 함께 본다(3절).

## 2. 사용법

### 2.1 명령줄 옵션 (`--run <게임자료> ...` 또는 `--mcp ...` 뒤)

| 옵션 | 뜻 |
| --- | --- |
| `--test-autoplay` | 프리셋. 따로 지정하지 않은 아군 AI를 **어려움**, 적 AI를 **쉬움**으로 둔다. |
| `--test-ally-ai <easy\|normal\|hard>` | 로컬 플레이어에도 AI를 붙이고(아군 자동 플레이), 로컬 AI와 **미션 동맹 AI**의 난이도로 쓴다. `1/2/3`도 받는다. |
| `--test-enemy-ai <easy\|normal\|hard>` | 옵션 메뉴 난이도 대신 적 AI 난이도로 쓴다. |
| `--test-start-sp <0..10000000>` | 미션 `myStartMoney` 대신 로컬 시작 SP. 아군 AI의 지갑도 이 값에서 시작한다. 아군 AI 없이 사람이 플레이할 때도 쓸 수 있다. |
| `--test-enemy-start-sp <0..10000000>` | 미션 `ai%sStartMoney` 대신 **적** AI의 시작 SP(지갑). 동맹 AI는 미션 값을 유지한다. |

예(캠페인 1-1을 아군 어려움·적 쉬움, 시작 SP 30000, 적 시작 SP 2000으로):

```powershell
$env:NETSTORM_USER_DATA = "<빈 폴더>"   # 진행 기록이 실제 Saved Games에 섞이지 않게 격리(권장)
cpppj\build\bin\Release\Netstorm.exe --run assets\game-data --mission thewarbegins --window --test-autoplay --test-start-sp 30000 --test-enemy-start-sp 2000
```

- `--mission` 없이 메뉴(Campaign→미션)로 들어가도 적용된다. 결정론 재현이 필요하면 `--fixed-frame-ms 25`(가상 시계, [session-dump 문서](cpp-session-state-dump-20261109.md))를 함께 쓴다.
- 시작 시 stderr와 활동 기록에 `테스트 전용 캠페인 자동 플레이 활성: ...`, 미션 Start 뒤 `테스트 자동 플레이: 로컬 owner N AI 생성 완료 (...)`가 남는다.
- 잘못된 값(범위 밖 SP, 모르는 난이도)은 시작 시 오류로 거부한다. `--view`와는 함께 쓸 수 없다.

### 2.2 실행 도구 `tools/cpp_campaign_autoplay.py`

게임을 위 옵션으로 띄우고(`--mission <키>` 또는 `--skirmish`), 안내 버튼(`Play Mission`/`Go!`/`MORE`/`OK`)만 주기적으로 누르며, 일정 프레임마다 `session-dump`·`report`(선택: 화면 캡처)를 남겨 승패(`victory`/`defeat`/`undecided`)까지 요약한다. 스커미시 모드는 화면 설정 키를 `--set`으로 넣고 처음 1500프레임 동안만 `Campaign → Skirmish → Start`를 누른다. **게임 창이 뜨므로 AGENTS.md 실행 확인 규칙(예외 호스트 포함)을 따른다.** 인자·출력·결과 판정·문제 해결은 [AI 에이전트용 사용 안내](../testing/cpp-autoplay-test-guide.md)가 기준이다.

```powershell
python -X utf8 tools/cpp_campaign_autoplay.py --mission thewarbegins
python -X utf8 tools/cpp_campaign_autoplay.py --mission thewarbegins --start-sp 20000 --enemy-start-sp 1000 --frames 72000 --snapshot-every 12000
python -X utf8 tools/cpp_campaign_autoplay.py --skirmish --skirmish-ai-count 2 --skirmish-allies 3 --frames 72000
```

설정/진행 기록은 출력 폴더의 `user-data`로 격리한다(`NETSTORM_USER_DATA`).

## 3. 동작 상세

| 항목 | 구현 |
| --- | --- |
| 로컬 AI 부착 | 원본 Start(`004c2b20`)가 끝난 뒤 `RawCampaignMissionStartBindings::CreateLocalTestAi`가 실행된다. 로컬에 살아 있는 사제가 있고 아직 AI가 없을 때만 만든다(원본 데모 분기와 같은 조건). |
| 로컬 AI 설정 | 원문 `MakeAi` owner 비영 분기 규칙: 기본값(`0040ef80`)만 쓰고 미션 공통 `ai*` 키는 읽지 않는다. **기술 추가 없음**(`+1e`=0, 미션 `myTech`만 생산), **시작 자금=현재 로컬 SP**. 로컬 플레이어에 붙이기 위해 **이름=현재 로컬 이름**, **동맹 마스크=현재 로컬 동맹(Player+54)**, 강제 적/NoTemple/색/Off=0을 더했다. 공통 키를 읽지 않는 이유: 예) `finalconfrontation`의 `aiAllyList="2;3;4;5"`를 로컬에 적용하면 로컬이 적 전원과 동맹이 된다. |
| 난이도 | `services.ai.difficulty`를 `WrapCampaignTestPlayDifficulty`로 감싼다. 적은 `--test-enemy-ai`, 아군(로컬 AI·미션 동맹 AI)은 `--test-ally-ai`, 지정하지 않은 쪽은 기존 옵션/스커미시 규칙이다. 배율은 11차와 같다(어려움: 세 주기×0.5, 쉬움: ×4·채취 절반). |
| 시작 SP | `--test-start-sp`: 원본 Start의 `setSp(local, myStartMoney)` 위치에서 값을 바꾼다. 아군 AI는 그 뒤 현재 SP로 지갑을 시작한다. `--test-enemy-start-sp`: 적 AI의 600BYTE 설정 `+22c`만 바꾼다. |
| 적용 범위 | 캠페인/일반 미션과 **스커미시**(같은 Start 연결층을 쓴다). **편집 Test Battle(mode64)·데모·LAN/MP에는 적용하지 않는다**(`UberGump`의 적격 판정, 바인딩의 데모/편집 제외). |
| 관전 카메라 | `--test-follow-battle`(`--test-autoplay` 프리셋 포함). `client/CampaignTestSpectator`가 0.5초(게임 시각)마다 소유 객체(중립·섬·다리 제외)의 HP 감소와 **피해를 입은 적 있는 객체의 사라짐**(파괴, 상한 60)을 모아 최근 6초·반경 8칸 피해 합이 가장 큰 곳으로 카메라 목표를 옮긴다(최소 20, 4초 유지, 6칸 안 재이동 없음). 희생 의식은 제단 의식 중에만 재생되는 사슬/몸부림 위치 소리(`RunAltarRegular` 사건21)의 좌표를 그대로 받아 즉시 제단으로 옮기고 5초 유지한다. 카메라 목표는 미니맵 클릭과 같은 `GameWorld::CenterCamera` 경로로 저장되어 화면 출력 뒤 적용된다. 판단기는 해당 미션 World에만 묶이며(`SetCurrentTestSpectator(spectator, world)`), 옵션이 없으면 만들어지지 않는다. |
| 스커미시 | 스커미시 대본(`myStartMoney=5000`, `ai<N>StartMoney=5000`, 플레이어 편은 `ai<N>AllyList`에 1 포함)도 같은 규칙으로 시작 SP·적 SP·로컬 AI가 적용된다. 난이도는 `--test-ally-ai`(로컬 AI·플레이어 편 AI)·`--test-enemy-ai`(적 편 AI)가 **스커미시 화면의 AI별 난이도보다 우선**하고, 지정하지 않은 쪽은 화면 값이다. 로컬 AI는 `myTech="all"`·금지 유닛 규칙을 그대로 따른다. |
| 11차 판정 보정 | `AiCreateHooks::difficulty`가 강제 적 정리 뒤 설정을 함께 받고, `o::IsAiEnemyOfLocal`이 로컬 자신·로컬 동맹 마스크·**설정 동맹 목록에 로컬이 있는 AI**를 아군으로 판정한다. 일반 플레이에서도 미션 동맹 AI는 이제 보통(원본 그대로)이다. |

### 3.1 일반 사용자 실행에서 비활성인 근거

- 옵션은 **명령줄에서만** 해석한다(`main.cpp` `RunClient`). 무인자 실행(`Netstorm.exe` 더블클릭, 배포본 기본 경로)은 `RunClient({},...)`로 인자가 없어 항상 비활성이다.
- `options.cfg`·캠페인 진행 파일·게임 내 메뉴에 저장/노출하지 않는다. 프로세스 전역 값(`ConfigureCampaignTestPlay`)이며 다음 실행에는 남지 않는다. `--help` 출력에도 넣지 않았다(이 문서가 사용법 기준).
- 설정이 비어 있으면 Start 연결층의 모든 분기가 원래 경로를 그대로 탄다(`TestPlay()`가 nullptr). 관전 카메라 판단기도 만들어지지 않아 `UberGump::FinishKernelFrame`의 추가 처리와 의식 소리 관찰이 모두 생략된다(검사 `CampaignTestPlay_ParseOptions`가 일반 실행의 판단기 없음을 확인). 검사 `CampaignTestPlay_ActualStartAttachesLocalAi`가 설정 없는 Start에서 로컬 AI가 없고 SP/기술이 원본 값임을 확인한다.

## 4. 변경 파일

- 신규: `cpppj/src/client/CampaignTestPlay.{h,cpp}`(옵션 해석·전역 보관·난이도 감싸기·로컬 AI 설정), `cpppj/src/client/CampaignTestSpectator.{h,cpp}`(관전 카메라 판단기), `cpppj/src/o/AiAllianceSide.h`(생성 직전 아군/적 판정), `cpppj/tests/CampaignTestPlayTests.cpp`, `tools/cpp_campaign_autoplay.py`(캠페인/스커미시 실행 도구, 승패 확정 뒤 조기 종료·추가/조건부 버튼), `tools/cpp_campaign_batch.py`(캠페인 1~3 일괄 실행·결과표), `docs/testing/cpp-autoplay-test-guide.md`(AI 에이전트용 사용 안내). 1313에서 `cpppj/src/client/RawCampaignHitBindings.cpp`에 제단 총기 피격(`0x00448b70`) 분배를 추가했다(7.2절).
- 수정: `cpppj/src/app/main.cpp`(옵션), `cpppj/src/client/RawCampaignMissionStartBindings.{h,cpp}`(시작 SP·적 SP·로컬 AI), `cpppj/src/client/UberGump.cpp`(적격 판정·난이도 감싸기·관전 카메라 연결), `cpppj/src/o/RawAiCreate.{h,cpp}`·`cpppj/src/client/RawCampaignSession.cpp`(11차 판정 보정), CMake 목록, 새 집중 검사 대상 `netstorm_campaign_test_play_tests`.

## 5. 검증

### 5.1 창 없는 검사

| 검사 | 결과 |
| --- | --- |
| 새 집중 대상 `netstorm_campaign_test_play_tests` 16건 | **통과.** 옵션 해석(프리셋·명시값 보존·범위/형식 거부·일반 실행 비활성)·난이도 감싸기·로컬 AI 설정 필드·아군/적 판정, 실제 보호 미션 Start 두 건(아래), 관전 카메라 5건(가장 격렬한 전투의 가중 중심·작은 소모 무시·유지 시간·파괴 상한·피해 없는 사라짐 무시·의식 우선과 복귀·World 묶음), 기존 `AiDifficulty_*` 5건. |
| `CampaignTestPlay_ActualStartAttachesLocalAi`(캠페인1-1 TheWarBegins) | 설정 없음: 로컬 AI 없음·SP 3000·기술=myTech 4종(원본 그대로). 설정 있음(아군 어려움·적 쉬움·SP 30000·적 SP 1500): 로컬 AI 부착, SP 30000, 이름/동맹 마스크 불변, 기본 AI 기술 5종 미추가, 로컬 AI 주기 6.0→3.0·3.0→1.5, 적 지갑 1500·이동 간격 ×4. |
| `CampaignTestPlay_ActualAlliedMissionAi`(캠페인2-4 GuardMyBack, 비데모) | 동맹 AI2는 적이 아님(11차 보정), 적 AI3은 적. 로컬 AI를 붙여도 로컬↔2 동맹 유지, 적 시작 SP는 적(3)에만 적용, 동맹 2는 미션 자금·어려움, 적 3은 쉬움. |
| 변형 확인 | 판정만 옛 규칙(로컬 Player+54만 확인)으로 되돌리면 위 GuardMyBack 검사가 **2건 실패**(AI2를 적으로 분류, 아군 난이도 미적용) → 기존 결함이 실제였고 검사가 잡는다. 복원 뒤 재빌드·재실행 통과. |
| 기존 `netstorm_ai_difficulty_tests` 5건·`netstorm_cd_mission_start_tests` 10건 | 통과(`RawAiCreate` 훅 형식 변경 뒤 컴파일·동작 회귀 없음). |

### 5.2 실제 게임 실행 (HJOW-Athlon, 실행 확인 예외 호스트, `tools/cpp_campaign_autoplay.py`)

고정 시각 25ms/프레임, 1024×768 창, 음소거, 설정/진행은 출력 폴더 `user-data`로 격리. 출력은 `extracted/cpp-test-play-20261114/`(git 제외).

| 실행 | 결과 |
| --- | --- |
| `thewarbegins-hard-easy`: 아군 어려움·적 쉬움, SP 미션값(3000), 24000프레임(게임 600초, 실제 약 240초) | 종료 0, 오류 로그·AI 원본 진단 0. Start 직후 `로컬 owner 1 AI 생성 완료`. 아군 객체 939→1850: 다리 41→383, 소유 섬 3→35, 썬 에이비어리 0→37·썬 플라이어 0→37, 썬 캐논 0→11, 레인 배터리 1→5. 적(2)의 썬 아처 9→0·윈드 배터리 3→0·썬 블로커 8→2로 **적 시설 파괴**. 600초 안에 승리 판정은 없음. |
| `skirmish-2ai-ally3`: 스커미시 AI 2명(2 적, 3 플레이어 편), 아군 어려움·적 쉬움, 72000프레임(메뉴 `Campaign→Skirmish→Start` 클릭으로 시작) | 종료 0, 오류 로그·AI 원본 진단 0. MP 규칙 섬 생성 뒤 `로컬 owner 1 AI 생성 완료`, AI 부착 소유자 1·2·3. 아군 객체 749→2060, 동맹 3도 744→2422로 확장, 적 2는 1102(56000)→468(60000)로 급감. 30분 안에 최종 승리 판정은 없음(적 잔존). |
| `thewarbegins-follow`: 1-1, 관전 카메라 켬, SP 20000·적 SP 1000, 32000프레임 | 종료 0. 카메라 전투 이동 30회(대부분 적 기지 쪽 (146~156, 116~139) 교전). 이 실행에서 피해 없는 사라짐(건설 교체 등)이 기지 쪽 "피해합 60"으로 잡히는 오탐을 확인해 **피해 입은 객체의 사라짐만 파괴로 세도록 보정**했다(검사 `CampaignTestSpectator_UndamagedVanishIgnored`). 희생 의식은 이 실행에 없었다. |
| `thewarbegins-follow-72k`: 보정 뒤, 관전 카메라 켬(안내서 예시 설정) SP 20000·적 SP 1000, 72000프레임 | 종료 0, 오류 로그·AI 원본 진단 0. 카메라 전투 이동 25회, 모두 적 전선(x 132~156)이며 기지 쪽 오탐 없음. 프레임 28000 화면에서 불타는 적 신전과 공격 중인 아군 썬 플라이어 확인. **프레임 32000~36000 사이 승리(`BadTeamDead`, `DoneTheWarBegins=1`)** 재현. |
| `thewarbegins-sp20000`: 같은 난이도, **시작 SP 20000·적 시작 SP 1000**, 72000프레임 | 종료 0. 로컬 SP 20000에서 시작(4000프레임 16550), 적 SP 1000→200. 적 객체 567(16000)→85(20000). **프레임 24000~28000 사이 `BadTeamDead` 승리 안내("Success! Your island is now free!", Leave Missions/Next Mission)와 `DoneTheWarBegins=1`** 관측. 도구는 승리 창 버튼을 누르지 않으므로 이후 정지 상태 유지. |

- 실제 `%USERPROFILE%\Saved Games\Netstorm\Saves\options.cfg`는 마지막 수정 2026-11-12로 이번 실행의 영향이 없었다. originals/originalCD 변경 없음.
- **결정론(재현성)**: 고정 시각 25ms, 같은 인자로 짧은 실행 3회(`det-a-nofollow`·`det-b-follow`·`det-c-follow`, 6000프레임)를 비교했다. 카메라 켬 두 실행(B·C)은 프레임 2000·4000의 `random` 행(게임 난수·CRT)과 `pool` 해시가 **완전히 같았다** → 같은 인자면 재현된다. 카메라 끔(A)은 프레임 2000까지 B와 같다가(첫 카메라 이동 71초 전) 4000에서 달라졌다 → **화면 위치가 공용 난수 소비에 영향을 준다**(화면 의존 효과의 난수 소비로 추정, 사람이 미니맵으로 화면을 옮겨도 같은 현상이며 이번 변경이 만든 특성은 아니다. 원인 소비자는 미확정). 따라서 관전 카메라를 켜고 끈 실행끼리, 또는 `--set` 내용(예: 도구의 `intro=1`)이 다른 실행끼리는 전개를 비교하지 않는다.

## 6. 한계와 주의

- **테스트 전용**이다. 원본 동작 복원과 무관한 추가 기능이며, 로컬 AI는 원본 데모 분기와 같은 AI 몸체를 쓰지만 설정은 원문 `MakeAi` 규칙(기본값)이라 원본 데모(`ai1*` 키 사용)와 완전히 같지 않다.
- 로컬 AI가 켜져 있는 동안 사람이 직접 건설/지출하면 AI 지갑과 로컬 SP 동기화(`RawAiPreparation`의 로컬 SP 보정) 때문에 SP가 사람 플레이와 다르게 보일 수 있다. 자동 플레이 중에는 직접 조작하지 않는다.
- 승리는 AI 전략·난수에 좌우된다. 재현이 어려우면 `--test-start-sp`를 올리거나 `--test-enemy-start-sp`를 낮추고 `--frames`를 늘린다. 승리 창 이후(Next Mission 등)는 도구가 누르지 않는다.
- 캠페인과 스커미시가 대상이다. 편집 Test Battle·데모·LAN/MP에는 적용되지 않으며 MP 실행 검증은 하지 않았다.
- 관전 카메라의 **희생 의식(제단) 이동은 실제 실행에서 아직 관측하지 못했다**(이번 실행들은 모두 적 전멸로 끝나 의식이 없었다). 원문 의식 소리 위치를 받는 경로와 우선 규칙은 검사 `CampaignTestSpectator_RitualHasPriority`로만 확인했다. 의식이 나오는 실행에서 `camera_moves.ritual`과 화면을 확인하면 된다.
- 관전 카메라의 전투 판정은 피해 관찰에 기반한 근사다. 피해 합이 작은 교전(20 미만)이나 한 번에 끝난 짧은 교전은 놓칠 수 있다.
- 제품 코드가 바뀌었으므로(11차 판정 보정 포함) 배포 ZIP을 다시 만들 때는 M8 관문을 다시 거친다.

## 7. 캠페인 전체 자동 플레이 실행 (1313, 2026-11-14 후속)

사용자 요청: 캠페인 전 스테이지를 이 방식으로 테스트한다. 이어서 사용자가 **3-x 계열(캠페인 3)은 이 방식으로 안 되겠다며 제외**, **2-5 Surrounded도 제외**를 지시했다(해당 게임 창은 사용자가 직접 종료했다). 튜토리얼 성격의 Early Missions·Priest Training과 사용자 제작 캠페인은 처음부터 대상이 아니다. 실행은 HJOW-Athlon(실행 확인 예외 호스트)에서 일괄 도구 `tools/cpp_campaign_batch.py`로 했고, 출력은 `extracted/cpp-campaign-batch/pass1/`(Git 제외)다.

**공통 인자:** 아군(로컬·동맹) AI 어려움, 적 AI 쉬움, 로컬 시작 SP 20000, 적 시작 SP 1000, 관전 카메라 켬, 72000프레임(고정 25ms 기준 게임 시간 30분), 덤프 4000프레임마다, 3개 동시 실행, 승패 확정 뒤 조기 종료. 도구가 `Play Mission`/`More`/`Fight On`/`Continue Fighting`/`Ignore`/`Go to Hell`만 눌렀다(승패 창 버튼 제외, EnemyTerritory만 `Ai2PriestCaptured` 창에서 `Continue`).

### 7.1 결과 (범위 안: 캠페인 1·2)

| 미션 | 결과 | 비고 |
| --- | --- | --- |
| 1-1 TheWarBegins | **승리** (프레임 40000) | `BadTeamDead` |
| 1-2 MasterOfWhirligigs | **승리** (20000) | |
| 1-3 SaveTheIsland | 미결 | 로컬 SP가 ~1000으로 묶인 채 적 SP 11550까지 증가, 교전 정체 |
| 1-4 FragileFortune | 미결 | 로컬 객체 ~1100·SP 20000대 미지출, 적 객체 ~1920 불변 |
| 1-5 ThunderingPower | **승리** (32000) | 게임 중 선택창 `[@20]…[@900]`을 `Fight On`/`Continue Fighting` 등으로 통과 |
| 1-6 DissolvedAlliance | 미결 | 로컬 객체 1500대·SP 38800까지 누적, 두 적 객체 ~1100/~1000 불변 |
| 2-1 TheNoose | **패배** (24000) | 로컬 AI가 처음 2400 SP만 쓰고 이후 SP 17600 고정(건설·다리 없음), 로컬 객체가 698→28로 급감 |
| 2-2 RainVsRain | **승리** (20000) | |
| 2-3 RunForIt | **승리** (24000) | |
| 2-4 GuardMyBack | 1차 **비정상 종료** → 수정 → 2차 미결(종료 없이 완주) | 7.2절 |
| 2-5 Surrounded | 사용자 지시로 제외 | 창은 사용자가 종료(`UI script ended before all steps ran`), 게임 오류는 관측하지 못함 |

집계(2-5 제외 10개): 승리 5, 패배 1, 미결 4. 모든 승리 실행은 오류 로그 0·AI 원본 진단 0이다.

### 7.2 발견과 조치

1. **2-4 GuardMyBack 비정상 종료 — 수정 완료.** 게임 시간 약 853초에 `std::logic_error: 실제 세션 총 피해 미복원: 공격 TYPE128 → 표적 TYPE164`로 종료했다(7차 오류 로그 확인). 썬 캐논이 제단(TYPE164)을 쏘는 총기 피격 경로에서, 제단의 가상 `+0x98`(`0x00448b70`, `Dais.cpp`: 방향 글자 11=투명·12=희생 중이면 피해를 무시하고 1 반환, 그 밖에는 일반 피격 `0x0044c8e0`)이 분배되지 않고 미복원 예외로 떨어졌다. 같은 함수는 반환형 경로 `RawCampaignHitBindings::DamageResult`에는 이미 있었으나 총기 경로 `DamageHooks`에만 빠져 있었다. 본문을 파일 안 도우미 `AltarHit` 한 곳으로 모아 두 경로가 함께 쓰게 했다(`cpppj/src/client/RawCampaignHitBindings.cpp`). **검증:** 같은 인자로 다시 실행하면 이전 종료 지점을 넘어 게임 시간 30분까지 종료 코드 0·오류 로그 0으로 완주했다(`extracted/cpp-campaign-batch/pass2-guardmyback/`). 새 집중 검사는 만들지 못했다: 이 저장소의 `InstallScene` 검사 틀이 새 집중 대상에서 기본 미션 `Load()`부터 `RawNormalTerrainWorldBinding::GenerateOriginal`의 "지형 생성 상위의 현재 전역/장치 미연결"로 실패한다(2026-11-05 군도 생성 연결 이후의 기존 문제로 추정, 기준선 비교는 하지 않았다). 기존 `netstorm_campaign_test_play_tests` 16건·`netstorm_ai_difficulty_tests`·`netstorm_cd_mission_start_tests`는 변경 뒤에도 통과한다.
2. **아군 AI 정체 — 미해결(조사 과제).** 미결 4건과 패배 1건의 공통 양상은 로컬 AI가 초반에 성장한 뒤 SP를 쓰지 않고 정체하는 것이다(2-1은 거의 처음부터). 이 기능은 원본의 데모 분기와 같은 AI 몸체를 쓰지만 설정은 `MakeAi` 규칙의 기본값이라 미션이 조정한 `ai1*` 값이 없다. 2-1은 섬이 적 다리로 둘러싸인 미션이라 AI의 초기 전략(`0x258`, 자기/적 시작 지역이 있어야 완료)이 끝나지 않았을 가능성이 있으나 **확인하지 않았다**. 다음 단계 제안: 세션 덤프/보고에 AI 본체 필드(초기화 `+258`·적 `+264`·지갑 `+2d8`·마지막 행동 시각)를 넣어 정체 원인을 직접 관측.
3. **희생 의식 카메라 미관측.** 15개 실행 모두 `camera_moves.ritual=0`이었다(의식이 없었다). 전투 카메라는 모든 실행에서 작동했다(미션당 18~206회).
4. **3-x 참고(범위 밖, 수정하지 않음).** 3-1 BreakingThrough는 게임 시간 약 245초에 `std::logic_error: 다른 파생 실제 피해 미복원`(`RawDamageableHit.cpp:63`, 총기 피격의 표적 가상 `+0x98`이 일반/Base가 아닌 다른 파생이고 대체 처리 없음)으로 종료했다. 3-2 ToTheRescue는 `@120` 선택창에서 멈춰(원인 버튼 미확인) 사용자가 종료했고, 3-3 Vicious도 사용자가 종료했으며, 3-4 EnemyTerritory는 72000프레임 미결, 3-5 FinalConfrontation은 제가 시작 직후 종료했다. 모두 판정 근거로 쓰지 않는다.
5. **병렬 실행 비용.** 3개 동시 실행은 실행 속도를 크게 떨어뜨린다(단독 실행은 약 100프레임/초, 24000프레임 약 240초였지만 병렬에서 1-1은 40000프레임에 1464초, 약 27프레임/초). 시간 비교는 같은 병렬도에서만 한다.
