# CD 10.72 미션 Start·AI 설정 순수 몸체 (제품 1310, 2026-11-14)

마지막 디컴파일 PC: **HJOW-Athlon**. 기존 `extracted/refined/originalCD/NETSTORM.c`와 원본 PE의 정적 x86 명령/문자열 표를 대조했다. 재디컴파일이나 원문 변경은 없다. CD 원본 SHA-256은 `613500a394b3062e914604eb7d09896d678192be24545d0e575082a87c851233`이다.

이 패치는 기존 `RawMissionStart`·`RawMissionAiSettings`에 **판본별 순수 몸체만** 더한다. 실제 CD Session/Guide/AI 생성자/플레이에는 연결하지 않는다. 기존 PATCH fixture와 호출자는 보존한다.

## 1. CD 미션 시작 `0040f840`

실제 원문 범위는 `0040f840~0040fac9`(650바이트)다. PATCH 대응은 `004c2b20`이지만 다음 동작은 다르다.

1. `+c0/+c4`를0으로 하고 `denySalvage`→`denyAscend`→`allowAnyCapture`→`moreGeysers`를 조회/저장한다. `moreGeysers`는0일 때 ordinaryGeysers=1이다.
2. `demoRestartTimer`가0이 아니면 정수 값을 현재 double 게임 시각에 더해 `+90`에 쓴다.
3. 실제 접두 `00511da0="@"`로 섹션 이름을 얻고 `tellAtList`에 쓴다. 현재 `+a0` 인덱스의 항목 첫 글자 뒤를 atoi로 읽어 `+a8`의 double 기한을 만든다. 빈 항목이면0이다.
4. `00480ce0`은 **flags+54 하위 bit 제거 → sequence+58 증가** 순서다.
5. `myStartMoney`를 정수→float로 좁혀 **`00414650(float)` 한 번** 호출한다. CD에는 PATCH의 editor=194162 치환, PlayerKnown 관문, owner별 scrambled SP 호출, 별도 PATCH RefreshSP 호출이 없다.
6. 현재 owner로 `00407550` 기술 제거 → `myTech` 조회 → **그 뒤 현재 owner 재조회** → `00407720` 기술 추가다. PATCH의 myTech 조회 전 owner 캡처를 사용하지 않는다. 기본 myTech 문자열 `00511d44`는 비어 있다.
7. owner1~8을 순회하며 매 위치의 현재 로컬 owner와 demo를 읽고 사제가 있는 참가자만 AI 생성을 요청한다. 실제 경로는 기본 설정 `004d6e60` → 공통 Apply `0040fad0(-1)` → owner Apply → `004d7720`이다. 본 순수 Start에서는 기존 `createAi(owner)` 장치가 이 실제 사슬을 담당해야 한다.
8. 명령 Gump 갱신 `004b1930` → `randGeysers`가 비영일 때 `00415a30` → 카메라 `00488f50` → 현재 double 게임 시각 캡처 → **started+88=1 → startSeconds+70 저장**이다.

새 `MissionStartHooks::setCdSp(float)`는 CD 전용 필수 경계다. 원문 `00414650`은 단일 float `005120d4`를 쓰고, `00518904`가 비영이면100000으로 바꾸며, `004ee9d0`의 실제 현재 SP 표시 갱신을 호출한다. 이 패치는 그 하위를 원본 owner별SP 함수로 대신하지 않고 **전체 CD 하위 몸체를 연결할 후속 장치**로 남긴다. CD `BindCurrentInputs`에는 localOwner만 요구하며 PATCH는 기존 세 getter 요구를 유지한다.

CD C문자열은 첫 NUL에서 끝난다. 섹션 목록의256바이트 지역 버퍼가 넘치는 입력은 명시 길이 오류로 거부하며, PATCH의 기존 입력 계약을 바꾸지 않는다. 성공/부분효과의 실제 게임 연결은 후속이다.

## 2. CD AI 설정 초기화 `004d6e60`

실제 범위 `004d6e60~004d6f33`(212바이트), 설정 크기는600바이트다. 원문이 쓰지 않는 caller 바이트를 지우지 않는다.

| 필드 | CD 원문 값 |
| --- | --- |
| +0 | `00540c78`의 빈 이름: 첫 BYTE0만 기록 |
| +1e | `00540c7c`의 기본 기술 문자열, 마지막 NUL 포함 |
| +220/+228 | float 6.0/3.0 |
| +230 | 3 |
| +224 | signed DWORD `0052e8dc`(PE 기본3)를 FILD→float로 변환 |
| +22c | DWORD `005120d0` 그대로(PE 기본6500) |
| +23c/+240/+244/+24c/+250/+254 | 0 |
| +238/+234/+248 | 4/3/0xffff |

기본 기술은 `sunWalker;windBattery;sunCannon;sunBalloon;sunBlocker;sunArcher;windArcher`다. 실제 쓰기 순서는 코드와 정적 명령 파일에 보존했다.

CD의6500과 PATCH 기본2500+4000은 우연히 같은 결과지만 입력 계약이 다르다. `MissionAiSettingsState::cdStartingSp{6500}`를 별도로 두어 CD 현재 DWORD를 받아 **+4000 없이** 저장한다. 기존 `startingSp`와 PATCH 기본값/fixture는 그대로다.

## 3. CD Apply `0040fad0`와 능력 `004d6f60`

CD Apply를 끝까지 정적으로 대조한 결과 기존 PATCH의 필드/조회 순서와 같다. 같은 구현을 공유하되 CD C문자열 종료와 능력 진단 차이를 분기한다.

- owner<=0은 `aiName` 등의 공통 키, owner>0은 `ai<번호>Name` 등의 키다.
- Name→Tech→BridgeDrawRate(+220)→StuffRefreshRate(+224)→TimeBetweenMoves(+228)→StartMoney(+22c)→GeyserAttachments(+230)→MaxInHand(+238)→Collectors(+234)→Enemy(+240)→NoTemple(+244)→Off(+254)→Color(+250) 순서다. 빈 값은 기존 바이트를 유지한다.
- Color는 슬래시 목록에서 양수 index만 기록한다. 0/none과 미등록 값은 기존 색을 유지한다.
- AllyList는 인덱스0~9마다 현재 설정을 다시 읽고 양수 번호의 하위5비트로 선택한 bit를 기존+23c에 OR한다.
- Ability도 인덱스0~9마다 다시 읽는다. 첫 `!`만 제거해 bit를 끄고 나머지는 켠다. 원문의256바이트 능력 이름 지역 복사 범위를 넘는 CD 입력은 명시 오류다.
- CD `004d6f60`은 미등록 이름이면 checking 여부와 관계없이 로그만 한 번 남긴 뒤 **index13 bit**를 수정한다. PATCH의 checking assertion은 CD에서 호출하지 않는다. 빈 이름은 아무 효과도 없다.

CD 색 표는 포인터 `00511c94`→문자열 `00511c98`의 `/none/blue/red/white/green/purple/yellow/lightblue/orange/`다. CD 능력 포인터 표 `00540cc8`의 실제 순서는 다음13개이며14번째 포인터는NULL이다.

1. FOCUS_ON_THINGS_THAT_ATTACK_BUILDINGS
2. BUILD_BLOCKERS_WHEN_BUILDINGS_HARMED
3. ATTACH_TO_ENEMY_ISLAND
4. STOP_COLLECTING_FROM_DANGEROUS_GEYSERS
5. COLLECT_STORM_POWER
6. ENCIRCLE_ENEMY_ISLAND
7. CONCENTRATE_FIRE
8. REPEAT_SUCCESSFUL_DROPS
9. CAPTURE_ENEMY_PRIEST
10. BUILD_DAIS
11. SACRIFICE_ENEMY_PRIEST
12. USE_PRIEST_TO_COLLECT
13. BUILD_BRIDGES

이 세 목록은 CD PE에서 직접 읽어 PATCH 표와 같은 것을 확인했다. `evidence.json`에 각 능력의 실제 포인터·문구, 색 표 포인터, 기본 기술/기본 이름/접두/기본 자금/refresh rate를 보존한다.

## 4. 검사와 한계

같은 `MissionStartTests.cpp`에 비MP 순수 검사5건을 더한다.

- `MissionStart_CdSingleSpAndCurrentOwnerSequence`: SP 단정도 변환과 get/owner/효과의 전체 순서, get(myTech) 뒤 owner 변경, double 기한, 마지막 시작 필드.
- `MissionStart_CdMissingSingleSpKeepsEarlierEffects`: 전용 장치가 없으면 PATCH 성공으로 우회하지 않고 앞선 flags/설정 효과만 남김.
- `MissionAiSettings_CdInitializeDirectMoneyAndAllBytes`: 실제600바이트와 양쪽4바이트 canary·미작성 영역·양수/음수 직접 자금DWORD.
- `MissionAiSettings_CdApplyByteOverridesAndLookupOrder`: 공통/owner 덮어쓰기·문자열 꼬리·빈 값·가산 동맹·unknown 능력 로그·66번 실제 키 조회 순서.
- `MissionAiSettings_CdAbilityAndColorTables`:13개 bit와9개 색의 고정 기대값 전수 비교.

**Release NetstormCpp 및 netstorm_cd_mission_start_tests 빌드가 종료 코드0으로 완료됐고, 같은 파일의 집중10건(기존5+새CD5)이 모두 통과했다.** 사용자 CD 복원 보류 요청 시 이미 진행 중이던 이 단위의 빌드/검사 결과만 정리했다. CD 게임·MP·패킷·통신 실행과 추가 복원은 하지 않았다. 실제 AI 생성·미션 플레이·공중3종 생산은 호출하거나 연결하지 않았다. 기대값은 원문 정적 대조와 PE 표에서 고정한 값이며 CD 독립 기계어 fixture 실행 결과로 표시하지 않는다. 원문 함수/준비본 SHA와 정적 명령은 같은 준비 디렉터리에 있다.

남은 라이브 연결: CD Session과 단일SP 소유자/표시 장치, Guide의 Start 선택, CD AI 생성자/사제/기술/간헐천/카메라 장치. 이 패치 적용만으로 CD 튜토리얼 진입 완료가 되지는 않는다.

## 5. 사용자 요청에 따른 보류

2026-11-14 사용자가 CD판(10.72) 복원을 미루도록 요청했다. 이 단위는 이미 구현한 순수 몸체를 검증·커밋하여 보존하는 경계이며, CD Session/Guide/SP/AI 생성자에 새로 연결하지 않는다. CD 메뉴까지는1309에서 확인됐고 튜토리얼의 첫 차단은 RawMenuHtml.inc의 PATCH 전용 HTML 검사다. HTML 정적 조사와 재개 순서는 [보류 인계](cpp-cd1072-deferred-20261114.md)에 남긴다. 보류 뒤에는 CD 작업 대신 M8 최종 배포 관문을 진행한다.
