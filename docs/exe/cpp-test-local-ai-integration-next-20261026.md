# 일반 미션의 테스트용 로컬 AI 연결 조사와 다음 구현안

- 조사일/PC: 2026-10-26, HJOW-Athlon.
- 마지막 전체 정밀 디컴파일: HJOW-Athlon / 2026-10-23. 디컴파일 소스를 변경하지 않았다.
- 이 문서는 **읽기 전용 조사와 미구현 제안**이다. 새 옵션·로컬 AI 배선을 구현하거나 빌드·게임·GUI·플레이 검사를 실행하지 않았다.
- 현행 사용자 결정: 일반 미션의 demo0을 유지하고, 테스트용 로컬 owner에도 복원된 공통 AI를 연결한다. TEST02 포획·희생 검사는 이 옵션 구현 뒤 진행한다. 기본 게임·적 설정·일반 프로필에 테스트 지원 상태를 저장하지 않는다.
- 과거 검토: [2026-10-21 가능성 검토](cpp-test-local-ai-and-sp-feasibility-20261021.md). 그 문서의 ‘자동 전략 프레임 미연결’ 등 당시 상태를 현재 상태로 읽지 않는다.

## 현재 연결 상태

`services.localAI`나 테스트용 로컬 AI CLI는 없다. 현재 `--test-bridge-patterns`는 로컬 생산 UI의 별도 명시 테스트 옵션이며 AI 활성화 옵션이 아니다.

반면 공통 AI의 실제 생성·부착·초기 기술 분류·월드 등록·피해/생성/삭제 통지·보상 지갑·자동 프레임은 현재 Session에 연결되어 있다.

| 위치 | 현재 역할 |
| --- | --- |
| [app/main.cpp](../../cpppj/src/app/main.cpp), `RunClient` | `--run` 옵션을 파싱한다. `--test-bridge-patterns`와 같은 곳에 새 명시 테스트 옵션을 넣을 수 있다. |
| [ClientMain.h](../../cpppj/src/client/ClientMain.h), `ClientOptions` | 일반 설정 파일과 별개인 실행 인자를 Client 수명에 보관한다. 현재 로컬 AI 필드는 없다. |
| [UberGump.cpp](../../cpppj/src/client/UberGump.cpp), `StartMissionWorld` | Guide 단계3 및 일반 로드의 실제 Start를 `missionRuntime_->start->Start()`로 호출한다. 현재 Session·MissionScript·gate가 모두 살아 있다. |
| [RawCampaignMissionStartBindings.cpp](../../cpppj/src/client/RawCampaignMissionStartBindings.cpp), `Start` | 현재는 `start_.Start()` 한 번이다. 정상 반환 직후의 호스트 테스트 배선을 두기 좋은 단일 지점이다. |
| 같은 파일의 `CreateAi` | 실제 caller 바이트 → 원본600BYTE 설정 준비 → `Session::CreateAi` 경로다. 기존 함수는 공통/owner 미션 설정을 읽으므로 로컬 테스트에 그대로 쓰면 적의 설정을 상속한다. |
| [RawCampaignSession.cpp](../../cpppj/src/client/RawCampaignSession.cpp), `CreateAi` | 실제 설치 후 owner1..8만 허용하고 기존 `RawAiCreate`를 호출한다. |
| 같은 파일의 `PrepareFrame` | 정지하지 않은 실제 프레임에서 `aiRuntime->Frame()`을 호출한다. `RawAiRuntime::Frame`이 같은 Player1..8의 brain을 순회한다. 새 로컬 전용 프레임이나 Kernel 추가 호출은 필요 없다. |

일부 공개 API 주석에는 과거의 ‘자동 프레임 후속’이 남아 있지만 실제 `PrepareFrame` 호출은 이미 연결되어 있다. 주석만 보고 미연결로 판단하거나 자동 AI 루프를 중복 추가하지 않는다.

## 실제 Start 순서와 테스트 경계

[RawMissionStart::Start](../../cpppj/src/o/RawMissionStart.cpp)는 금지/간헐천/타이머 → TellAt 목록 → 현재 로컬 시작 SP → 화면 → 로컬 기술 Clear/Add(myTech) → 사제가 있는 다른 owner의 AI 생성 → 명령창/랜덤 간헐천/카메라 → 현재 시작 시각 → 같은 `gate.started=1` 순서다. 일반 미션은 로컬 owner를 건너뛰고, demo 비영일 때만 기존 경로가 로컬도 생성한다.

다음 구현은 원본 `RawMissionStart`의 순회 조건을 바꾸지 않고, `RawCampaignMissionStartBindings::Start`의 정상 반환 뒤 테스트 전용 조정기에서 로컬만 추가 생성하는 방향을 권한다. 이때 적의 원본 AI 생성은 이미 원래 순서로 끝났고, 다음 자동 AI 프레임은 아직 실행되지 않았다. `gate.demo`, Tutorial 번호/단계, 승패 사건이나 데모 브리핑을 수정할 필요가 없다.

테스트 조정기의 적용 조건은 명시 옵션 활성·demo0·실제 설치된 Session·PlayerKnown 비영·현재 LocalOwner1..8·해당 owner의 실제 살아 있는 사제다. Editor나 기존 데모에서는 적용하지 않고 이유를 기록한다. `LocalOwner()`를 읽으며1을 고정하지 않는다. Start가 중간 실패한 경우 지원 성공을 기록하거나 로컬 brain을 먼저 만들지 않는다.

동일 Start 객체에서 중복 적용을 막는 호스트 기록은 원본 `gate.started`의 대체 상태가 아니다. 적용 전/후의 native started와 실제 brain 토큰을 기록하고, 같은 Session에서 이미 brain이 있으면 새 프로필을 적용했다고 표시하지 않는다. `RawAiCreate::Create`는 기존 brain이 있으면 초기 갱신을 다시 하지 않고 기존 토큰을 반환한다. 특히 그 앞의 Color 변경은 먼저 실행되므로 기존 brain 검사도 테스트 조정기에서 먼저 하는 편이 명확하다.

## 제안 CLI와 수명

아래 옵션은 모두 **미구현**이다.

| 제안 | 기본값/의미 |
| --- | --- |
| `--test-local-ai original` | 옵션이 없으면 비활성. 초기 구현은 복원된 원본 공통 AI만 지원한다. |
| `--test-local-sp 50000` | 선택 사항인 일회성 로컬 시작 SP 하한. 더 높은 원래 시작 금액은 낮추지 않는다. |
| `--test-local-ai-tech "sunWalker;…"` | 선택 사항인 로컬 전용 테스트 기술 목록. 미지정이면 실제 Start가 준비한 로컬 기술을 그대로 사용한다. |

로컬 속도 배수나 별도의 상황 판단기는 이번 최소 구현에 넣지 않는다. 원본 AI의 현재 시간/행동 루프를 재사용한다. 최소변경 위치는 `main.cpp` 도움말/파서, ClientOptions의 독립 실행값/getter, StartBindings가 빌리는 테스트 프로필/배선, `MakeCampaignMissionStartEffects`의 전달, 읽기 전용 보고 항목이다. Raw AI·FrameGate·Outcome·적의 설정 파서를 수정할 이유는 없다.

실행 옵션은 Client에만 보관하고 `--set`·일반 Config에 쓰지 않는다. 새 Session/미션/재시작에는 같은 명시 CLI 정책을 다시 평가하되, 현재 새 owner·native started·brain 생성 횟수와 지원 내용을 따로 기록한다. 기존 Session의 토큰이나 이전 성공 bool을 다음 Session에 재사용하지 않는다. 기본 실행에는 테스트 이름·금액·기술·자기 AI가 남지 않아야 한다.

## 독립600BYTE 로컬 프로필

[RawMissionAiSettings](../../cpppj/src/o/RawMissionAiSettings.cpp)의 `Prepare`는 Initialize → 공통 ai… → ai&lt;owner&gt;… 순서다. 일반 미션의 공통 aiOff/aiNoTemple/aiName/aiTech/aiStartMoney/능력 금지/동맹을 로컬에 그대로 상속하면 사용자의 상태를 바꾼다. 기존 `CreateAi(localOwner)`만 호출하는 방식은 이 문제를 해결하지 못한다.

로컬용600BYTE는 기존 `settingsBytes(owner,bytes)` 장치를 사용한 뒤 원본 Initialize를 실행하고, 공통/적 미션 조회와 분리된 명시 테스트 입력으로 준비한다. 새 메모리 전체를0으로 채워 caller의 미정 바이트 의미를 바꾸지 않는다. 기본 원문 시간값6/3/3과 능력 기본값은 기존 원본 Initialize를 사용한다.

| 원문 항목 | offset | 로컬 테스트 준비 기준 |
| --- | --- | --- |
| Name | +0 | 현재 로컬 사용자 이름을 보존하는 독립 값. 공통 적 이름을 상속하지 않는다. |
| Tech | +0x1e | 미지정이면 이미 준비된 실제 로컬 기술을 사용하고 새 기술을 추가하지 않는다. Initialize의 기본 Tech 문자열도 로컬에 무조건 추가하지 않는다. |
| BridgeDrawRate / StuffRefreshRate / TimeBetweenMoves | +0x220 / +0x224 / +0x228 | 원본 기본값을 사용한다. 적의 값이나 전체 시계를 조정하지 않는다. |
| StartMoney | +0x22c | 원본 Start가 실제 지정한 로컬 SP와 일치하는 정수 값. |
| GeyserAttachments / Collectors / MaxInHand | +0x230 / +0x234 / +0x238 | 별도 요청이 없으면 원본 기본값3/3/4를 사용한다. |
| AllyList의 mask | +0x23c | 현재 로컬 Player+0x54의 실제32BIT 동맹 mask를 보존한다. 적/아군 구성을 새로 정하지 않는다. |
| Enemy | +0x240 | 독립 기본0으로 시작하면 기존 원본 전략이 현재 실제 적을 고른다. 공통 적 owner 설정을 가져오지 않는다. |
| NoTemple / Ability / Color / Off | +0x244 / +0x248 / +0x250 / +0x254 | NoTemple0·원본 기본 능력·Color0·Off0. 기술/플레이어 이름/동맹/신전을 의도하지 않게 바꾸는 상속을 막는다. |

`RawAiCreate`는 NoTemple 비영이면 실제 신전을 삭제할 수 있다. Off 비영은 실제 프레임 종료/소멸 경로다. 둘을 활성화 스위치로 쓰지 않는다. Color0은 색 변경을 요청하지 않는 원본 입력이다. 현재 이름은 `Session::CurrentHandle()` 및 실제 Player 이름의 차이를 확인하여 로컬 표시 이름을 보존하는 쪽을 선택해야 한다.

동맹 mask는 기본0으로 남기지 않는다. 원본 생성자가 `SetAlliance(owner,settings+0x23c)`를 실행하여 해당 실제 행을 갱신하기 때문이다. 단순 문자열 AllyList는0을 무시하므로 현재 mask의 상위 비트/neutral0까지 보존하려면 실제 Player+0x54 DWORD를 복사하는 명시 테스트 프로필 필드가 더 정확하다. 이 방식도 생성자의 정상 자기 동맹/Chat 통지를 실행하며, 적의 행을 직접 쓰지 않는다.

## 기술 목록의 주의점

기본 Tech 문자열을 없애는 것과 실제 로컬 기술을 없애는 것은 다르다. 원본 AI 초기 갱신은 설정 Tech를 **추가한 뒤 실제 Player 기술 부모의 flags4 ContentForm 전체를 분류**한다. 따라서 현재 로컬 기술이 all인데600BYTE Tech에 일부 이름만 넣어도 기존 all이 제한되지 않는다.

미지정 기본 모드에서는 새 AI 프로필의 Tech 추가를 비활성화하고 Start가 만든 로컬 기술을 그대로 분류하도록 한다. `CurrentTechnologyParent()`·`ProductionFinder()`·`Core().pool`의 실제 ContentForm 순회로 flags4와 +18의 TYPE WORD를 읽어 관측할 수 있다. `mission.myTech`만 저장해 실제 기술 상태를 대신했다고 주장하지 않는다.

명시 `--test-local-ai-tech`로 기술을 제한할 때는 테스트 경계에서 현재 로컬 기술에만 `Session::ClearTechnology(local)` → `AddTechnology(local,list)`를 실행한 뒤 같은 CreateAi/초기 분류를 사용해야 한다. 적의 기술·공통 aiTech·원본 파일은 바꾸지 않는다. 이 실행은 기술이 바뀐 명시 테스트 실행으로 표시한다. 현재 생산 중인 임의 프레임에 적용하지 않고 새 Start 직후로 제한한다.

TEST02는 읽기 전용 현황에서 `myStartMoney=500000`, `myTech="all"`이다. 실제 검사를 재개하기 전 Fort/언어 파일의 SHA와 배치를 다시 확인해야 한다. 기존 결정의 크로스보우·썬 디스크 발사기·맨오워풀·데빌메이커 우선, 훨리베이스 제외를 적용하려면 이 ‘추가형 Tech’ 특성을 고려해 실제 테스트 기술 목록을 명시해야 한다. 성공 결과를 일반 all 기술의 자연 완주로 합산하지 않는다.

## 시작 SP와 실제 지갑

기본 로컬 AI의 StartMoney는 원본 Start 완료 뒤의 `session.Sp(local)`와 맞춘다. 생성자는 +0x22c를 실제 지갑+0x2d8로 복사한다. 로컬 `RawAiPreparation`은 지갑과 실제 SP가 다르면 실제 로컬 SP 가산 경로로 맞추며, `RawAiEconomy`는 행동 말미에 지갑을 분산 SP로 저장한다. SP만 별도로 올리고 지갑을 낮게 남기면 첫 AI 프레임이 금액을 되돌릴 수 있다.

SP 하한 옵션은 StartBindings의 `hooks.get("myStartMoney")`에서 해당 값만 메모리의 테스트 값으로 선택해 **기존 원본 Start의 단 한 번의 SetSp**를 쓰는 방식을 권한다. 원본 RawMissionStart 본체나 공유 Config/미션 파일을 수정하지 않는다. 하한 미지정은 기존 조회를 그대로 사용한다. Start 효과 뒤 별도 SetSp를 추가하거나 매 프레임 잔액을 고정하지 않는다.

입력 금액은 유한한0이상 정수·signed32 범위로 확인하고, 지원금 상한은 테스트 정책으로 별도 정한다. 초기 제안1,000,000은 원본 한도가 아니다. 지갑 생성 전에 실제 SP float가 정수 지갑과 같은 값으로 표현되는지도 검증한다. 이미 더 높은 원본 금액은 낮추지 않는다. 기본 프로필·기존 체크포인트·캠페인 완료값에 테스트 지원 상태를 저장하지 않는다.

## demo0과 공유 난수

gate.demo를1로 바꾸면 Early 판정 생략, Demo 접두 승패 절, 데모 타이머 및 별도 브리핑까지 달라진다. 새 옵션은 실제 미션 demo0을 유지한다. 이 때문에 기존 데모 전용 지갑 하한, 일부 로컬 다리 보충/차단창 조건도 원본의 demo0 분기를 그대로 탄다. 이 분기 차이를 숨기기 위해 AI 서비스의 demo getter만1로 속이지 않는다.

로컬 brain의 초기 전략·다리·배치·보상은 같은 Client GameRandom을 쓴다. 적의 설정과 프레임 경로가 유지되어도 로컬 AI의 추가 난수 소비로 이후 적 행동이 달라질 수 있다. 별도 AI용 난수를 만들어 원본 공통 흐름을 바꾸지 않는다. 기존 SetSp는 이미 초기화된 저장소에서160회 난수를 소비하므로 기존 한 번의 Start 쓰기를 재사용하면 추가 SP 쓰기의160회는 피할 수 있다. 로컬 AI 자체의 난수 소비까지 없어지는 것은 아니다.

기록에는 지원 활성·로컬 owner·미션/세션 토큰·demo·native started·SP/지갑·실제 기술·동맹·brain 토큰·생성 횟수·공유 난수 전후를 넣는다. 적 설정 준비600BYTE 및 적 brain의 처음600BYTE를 기본 실행과 대조해 ‘설정 불변’과 ‘추가 난수에 따른 이후 행동 변화’를 구별한다.

## 후속 검증

1. 옵션 미지정의 기존 Start/AI/미션 검사와 실제 메뉴 경로를 유지한다. 파서의 잘못된 모드·금액·빈 기술 목록은 게임 생성 전에 명시 거부한다.
2. 일반 demo0의 로컬 owner1 및1이 아닌 입력에서 실제 사제·SP·기술·동맹·이름을 확인한다. 공통 aiOff1/aiNoTemple1/적 이름·적 Tech·StartMoney0이 로컬 프로필에 새지 않는 사례를 넣는다.
3. 같은 Start의 재호출, 기존 brain, 사제 부재, PlayerKnown0, 원본 Start 중단, 실제 생성 중단을 확인한다. 부분 실패를 지원 성공으로 표시하지 않는다.
4. 재시작·다음 미션·메뉴 복귀에서 이전 AI 본체/목록/Player 포인터를 원본 정리로 반납하고 새 Session에 정확히 한 번 다시 적용되는지 확인한다. 기존 실제 데모에는 추가 brain/테스트 SP를 적용하지 않는다.
5. 같은 자동 Frame/Kernel에서 로컬 AI가 정상 생산·다리·채취·공격 명령을 내는지 확인하고, 실제 SP/지갑·기술·목록·통지의 일치를 관측한다. 빈 효과나 강제 단계/HP/포획 상태로 성공을 만들지 않는다.
6. TEST02/캠페인1-1의 포획·운반·제단·희생·자연 승패를 확대한다. 테스트 AI/지원금/기술 조정의 결과는 별도 표시하고 사용자 입력의 자연 플레이·기본 자원 완주·원본 화면/음향 동일성과 구별한다.

이 문서의 API 연결 가능성은 자동 클리어 성공의 증거가 아니다. 실제 제품 구현·집중 검사·GUI·자연 진행·인수인계 및 커밋은 후속 작업이다.
