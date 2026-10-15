# MissionStage3 공통 시작과 실제 AI 생성 경계

현재/마지막 정밀 디컴파일 PC는 **VM-W11-Codex, 2026-10-15**다. 보호 PE와 현재 정밀 프로젝트를 읽기 전용으로 사용했다. 원본 게임·복사본·GUI를 실행하지 않았고 `originals`/`originalCD` 파일을 변경하지 않았다. 구현은 10.78 PATCH 전용이다. CD10.72의 몸체를 이 주소에 대응한다고 추정하지 않는다.

## 복원 범위와 같은 저장소

| 원본 함수 | 새 모듈/API | 정확히 복원한 범위 |
| --- | --- | --- |
| `004c2b20` | `RawMissionStart::Start` | 현재 설정 조회, 시작 전역/시간, tellAtList, 실제 SP/기술 경계, owner별 사제 검사→AI 요청, 명령 Gump/랜덤 간헐천/카메라, 최종 시작 표식의 순서 |
| `004c3100`, `00482c80` | `RefreshTellAt`, Start 내부 | 기존 tellAtIndex의 첫 글자 뒤 `atol`, double 예약, base sequence 증가/flags bit0 해제 |
| `0040ef80` | `RawMissionAiSettings::Initialize` | 실제 기본값이 쓰는 600바이트 중 일부만 초기화하고 나머지 caller 바이트 보존 |
| `004c2d90`, `0040f030` | `Apply`, `SetAbility` | 빈 값 유지, 실제 필드/조회 순서, 색 표, 동맹 가산, 능력 표/미등록 진단 후 비트 |
| `004c2d40` | `Prepare` | 기본값→공통 `-1`→owner 설정 누적. 실제 생성 요청은 상위에서 `RawAiCreate`에 연결 |
| `00417e50` | `RawAiCreate::Create` | 색 변경→기존 brain 반환→NoTemple→실제 생성자 요청→현재 Player 부착→실제 초기 갱신 요청 |

`RawMissionStart`는 기존 `MissionFrameGateState`의 demo/startSeconds/started/demoEndSeconds/tellAtSeconds를 참조한다. 별도 시작 성공 사본을 만들지 않는다. base Mission `+54/+58`, Tutorial `+a0/+c0/+c4`만 `MissionStartState`에 둔다. `+c0/+c4`의 의미는 아직 확인하지 못했으므로 원본 offset 이름을 유지한다. `Start`는 `tellAtIndex`를 0으로 바꾸지 않는다.

현재 owner/known/editor 및 deny 전역은 `MissionStartGlobals`의 같은 인스턴스를 공유한다. 실제 세션 연결 시 이 값이 기존 `RawPlayerWorld::State()`와 따로 변하지 않도록 조회/수명 계약을 조립해야 한다. 이 단위에서는 Session/GameWorld와 자동 Outcome을 연결하지 않았다.

## Start의 실제 효과 순서

1. `+c0/+c4=0`, denySalvage/denyAscend/allowAnyCapture를 그대로 `atol` DWORD에 쓰고 `ordinaryGeysers=(moreGeysers==0)`을 쓴다. 입력을 bool로 먼저 정규화하지 않는다.
2. demoRestartTimer가 비제로일 때만 **현재 double 게임 시각+정수**로 demoEndSeconds를 바꾼다. 0은 이전 값을 보존한다. float 시각으로 좁히지 않는다.
3. 같은 Config의 `SectionNames("@")`를 `Set("tellAtList",...)`로 쓴 뒤 현재 index를 예약한다. 비어 있지 않은 항목은 첫 글자를 무조건 건너뛰고 `atol`을 읽는다. `'@'` 검사를 추가하지 않는다. 그 뒤 base sequence를 DWORD 증가하고 flags bit0을 지운다.
4. myStartMoney를 int32→float로 변환한다. editor이면 194162.0f다. playerKnown이 비제로인 경우에만 현재 owner에 실제 직접 SP 지정 `0040edb0`을 요청한다. AI 지갑 가산이나 `AddSp` 정책으로 바꾸지 않는다. `0043dad0`은 이후 항상 호출된다.
5. 현재 owner의 기존 기술 `004903b0`을 제거한다. **제거 뒤의 owner를 먼저 저장하고** myTech를 조회한 뒤 `004915c0(savedOwner,text)`를 실행한다. 빈 myTech도 호출한다.
6. owner1..8을 방문한다. 각 방문 때 현재 localOwner/demo를 다시 읽고 `owner!=local || demo`이면 실제 사제 `004921f0`를 찾는다. 사제가 비제로인 owner에만 `004c2d40`을 요청한다. **aiName 유무는 AI 존재 조건이 아니다.**
7. `0042e440`의 현재 Gump 갱신을 요청한다. 이 몸체는 `00548740`이 비제로이면 `00464e90(gump,00542914,0,0)`을 호출한다. 최근 생성 목록으로 추정하지 않는다.
8. randGeysers가 비제로일 때만 실제 세계 전체 생성 `00486440`을 요청하고 카메라 `004340d0`를 초기화한다. 마지막에 다시 현재 double 시각을 읽어 startSeconds를 쓰고 started=1을 쓴다.

모든 실제 효과는 도달한 위치에서 필수 callback이다. 누락되거나 예외가 발생하면 그 이후 원본 효과/최종 started 쓰기를 실행하지 않는다. 앞에서 이미 발생한 원본 상태 변경은 임의로 rollback하지 않는다.

## 600바이트 설정의 세부 규칙

기본 설정은 name 첫 바이트만 0, tech의 NUL 포함75바이트만 복사한다. 문자열의 남은 공간이나 두 문자열 뒤 gap을 0으로 지우지 않는다. 저장소는 caller가 준비한 실제600바이트여야 한다. 원본 strcpy처럼 문자열은 NUL까지 쓴다. name의 선언 폭30을 넘는 입력도 실제600바이트 안에서는 이후 필드를 덮어쓰는 원래 쓰기를 보존한다. 저장소 밖의 원본 메모리 손상은 호스트의 명시 범위 오류로 거부한다.

| offset | 기본값/설정 키 |
| --- | --- |
| `0` | 빈 Name, name 문자열 시작 |
| `1e` | `sunWalker;windBattery;sunCannon;sunBalloon;sunBlocker;sunArcher;windArcher` |
| `220/224/228` | BridgeDrawRate=6.0f, StuffRefreshRate=`FILD int005318d4`→float, TimeBetweenMoves=3.0f |
| `22c` | `FILD int0052f3c8 + float00500ffc(4000.0)`→CRT ftol 하위 DWORD. 실제 기본 전역2500이면6500 |
| `230/234/238` | GeyserAttachments=3, Collectors=3, MaxInHand=4 |
| `23c/240/244` | AllyList=0, Enemy=0, NoTemple=0 |
| `248/24c/250/254` | Ability mask=ffff, 미명명 DWORD=0, Color=0, Off=0 |

`Apply(-1)`/`Apply(0)`는 aiName 꼴이며 양수 owner는 ai1Name~ai8Name 꼴이다. 원본은 1~8 이외 양수도 `%d`로 처리하므로 클래스가 임의 owner 필터를 넣지 않는다. 빈 문자열은 이전 값에 쓰지 않는다. 문자열/세 rate/StartMoney/GeyserAttachments/MaxInHand/Collectors/Enemy/NoTemple/Off/Color/AllyList 열 항목/Ability 열 항목 순서를 지킨다.

세 rate의 `atof`는 **원본 CRT004e4359 결과를 받는 명시 callback**이다. 현대 `strtod`의 hex/inf/nan 해석을 원본과 같다고 간주해 기본 공급하지 않는다. 현재 검사의 십진 유한 입력에 대해서만 외부 CRT 결과를 명시했다. 정수는 기존 `ConfigParseLong`, 목록은 기존 세미콜론 전용 `ConfigListItem`을 재사용한다.

Color는 원본 none/blue/red/white/green/purple/yellow/lightblue/orange 순서다. ASCII 대소문자를 무시하며 index>0만 쓴다. none/미등록/빈 값은 이전 Color를 지우지 않는다. AllyList는 열 항목의 양수 `atol`에 대해 `1<<(value&31)`을 **OR**한다. 8을 넘는 양수를 지우거나 owner override로 기존 mask를 대체하지 않는다.

Ability는 PE의 실제13개 이름 순서를 보존한다. 선두 느낌표는 하나만 제거한다. 빈 이름은 효과 없음이다. 미등록 이름은 checkingAsserts이면 assertion을 보고하고 log(group29)한 뒤 **index13 bit도 실제로 수정**한다. 미등록 항목을 무시하지 않는다. 공통/owner 목록은 앞의 bit 상태에 누적된다.

## 실제 AI 생성의 제한과 후속

`RawAiCreate`는 Color를 기존 brain 조회보다 먼저 바꾼다. 기존 brain은 그대로 반환하며 새로운 설정을 `initialUpdate`에 재적용하지 않는다. brain이 없고 NoTemple이 비제로이면 Player+78의 현재 첫 지역으로 `RawIslandRuntime::Temple`에 해당하는 `0046f260`을 요청한다. 비제로 신전만 실제 가상삭제 flags400000을 호출하고0을 반환한다.

정상 생성 경로는 실제 new 크기0xb08/생성자 `00416860` 경계와 초기 갱신 `00414710` 경계가 **둘 다 공급되어야** 시작한다. 그 뒤 실제 생성자가 돌려준 opaque32 소유자만 기존 `RawPlayerParticipation::AttachBrain`으로 등록하고 초기 갱신을 실행한다. 이 wrapper에 가짜 brain allocator/빈 초기 갱신은 없다. 생성자가0을 돌려주면 명시 오류다. 원본의 할당 실패 뒤 NULL 부착/NULL 초기 갱신으로 이어지는 비정상 경로를 호스트에서 재현하는 범위는 아니다. 검사는 정상 비NULL 생성과 기존brain/NoTemple 분기를 대조한다.

**실제 생성자/초기 갱신은 아직 미복원이다.** `00416860`에는 실제 동적 목록 할당,600바이트 복사, 지갑/타이머/전략배열, 첫 지역 소유권, 실제 타입 ss 검색과 동맹이 있다. 마지막 동맹은 `SetAlliance(ai,mask)` 뒤 `allies[ai][local]`이 비제로일 때만 `AddAlly(local,ai)`를 더하는 방향별 규칙이며, 아직 wrapper가 대신 실행하지 않는다. 실제 생성자를 구현할 때 기존 참여 모듈을 재사용해야 한다.

`00414710`은 설정을 다시600바이트 복사하고 미션이면 name/defaultAiName을 Player에 쓴다. `RawPlayerWorld::Technology/Create`에 해당하는 실제 기술 부모를 조회/생성하고 `004915c0`으로 기술 내용을 추가한 뒤 flags4의 contained들을 순회한다. 실제 TYPE의 속성으로 builder 목록/원소별 개수/비용/테마/최저가 및 분류 자료를 만든다. 이를 빈 초기 갱신으로 대체하면 이후 전략·피해 통지가 실제 brain을 읽을 때 원본과 달라진다. 전략 프레임 `00417d30/00417de0` 및 호출된 주요 `00414af0/00416eb0/004139f0/00417c70`도 후속이다. **aiName 또는 flags20만으로 Outcome 참여를 켜지 않는다.**

현재 재사용 가능한 실제 하위는 Player 기술 부모의 `RawPlayerWorld::{Technology,Create}`, 현재9기록의 `Records`, 섬 신전 getter의 `RawIslandRuntime::Temple`, 분산 SP의 `ScrambledSpStore::Set`, 실제 사제 조회 계층이다. 후속 [Player 기술 단위](cpp-player-technology-20261015.md)에서 `004903b0`의 flags4 기술 제거와 `004915c0`의 문자열 타입해석→ensure00490420→contained 생성 사슬도 `RawPlayerTechnology`로 복원하고 원본158입력/집중2개를 확인했다. 실제 세션의 가상파괴/ContentForm 알림/현재 PlayerKnown 연결 전제는 여전히 필요하다. 단순 `MissionPlayers::Load`의 double SP/대칭동맹 사본을 이 실제 경계로 재사용하지 않는다.

## 원본 대조와 검사

읽기 전용 선택형 정밀 export10함수와 [manifest](../../tools/ghidra/mission-start-functions.json), [격리 관찰 도구](../../tools/decomp_mission_start.py), [SHA 보고서](../../cpppj/mission-start.json), [fixture](../../cpppj/tests/fixtures/mission-start-x86.tsv)를 추가했다. 같은 입력 재실행은 합산하지 않는다. 현재 **204고유입력**은 기본값20/단일설정64/능력32/실제Prepare8/Start48/생성wrapper32다. 기본값/단일설정/능력/Prepare는 전체600바이트, Start는 모든 변경DWORD/세double비트, wrapper는 해당 Player180바이트/반환값과 전체 효과 순서를 독립 기계어 출력에 대조한다.

Prepare는 실제 `004c2d40`의 스택 local600도 실행했다. 생성wrapper는 실제 `00417e50`/기존 `004911a0`를 실행하고 allocator/큰 생성자/초기갱신은 인자/순서 관찰 경계다. Config/CRT 조회·변환 및 SP/기술/Gump/간헐천/카메라 하위도 명시 외부 경계다. 원본 게임을 실행하거나 전략까지 원본과 같음을 검증한 관찰이 아니다. 엄격 허용 목록에 없는 코드 진입은 오류다. 처음 확인 중 실제 `__stricmp`004e65ed/부착의 owner getter0048f550을 엄격 guard가 중단했으며 실제 주소/기존 정밀 범위만 추가한 뒤204입력 생성/고유성/SHA 감사가 통과했다.

`MissionStartTests.cpp`에 원본204입력 재생과 누락 생성자/공유현재owner/시작표식 경계 검사2개를 추가했다. root가 공유 CMake에 등록한 뒤 **통합 게임/검사 Release 빌드 경고0·오류0, 집중2개 모두 통과**를 확인했다(`extracted/run43-45-build.log`, `extracted/mission-start-root-focused.log`). 이후 빈 마지막 trace를 `-` sentinel로 표현하도록 도구/fixture/검사를 교정하고 같은204입력의 원본 출력 재생성/고유성/SHA 감사를 통과했다. 의미상 production 변경은 없다. 포맷 교정 뒤에도 다시 **Release경고/오류0·집중2개 실패0**을 확인했다(`extracted/script-command-ui-build.log`, `extracted/mission-start-root-final-focused.log`). 최초 공백검사는 미추적 fixture를 포함하지 못했으며 최종 staged 공백감사를 통과한 뒤 같은단위 커밋을교정했다. 이 결과를 앞 단위의 전체1183개 검사에 포함된 것으로 합산하지 않는다. Session/GUI/LEFT/CMake/커밋은 이 하위 agent가 수정하지 않았다.
