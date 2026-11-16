# 원본 10.78 최초 실행 상태와 세 클론의 적용 기준

2026-11-16, **HJOW-Athlon**. 사용자 요청에 따라 메인 메뉴 전 안내창, Struggle For Freedom의 최초 잠금 상태, Options 기본값만 분석했다. **원본 분석과 인계는 완료했으며 cpppj·dotnetpj·webpj 제품 코드의 수정·검증은 후속 작업이다.** cpppj부터 반영하고, 확정된 동작을 dotnetpj와 webpj에도 적용한다.

**후속 갱신:** 같은 날 [cpppj1314 최초 상태 반영·검증](cpp-first-run-1314-20261116.md)을 완료했다. 아래는 원본 분석 시점의 근거와 인계 기록이며, cpppj 기본값5·5·동일 설정 해석·GUI13회 결과는 후속 문서가 기준이다. DN/WB 반영과 analyzeManager 수정은 여전히 미완료다.

## 1. 기준 상태와 관찰 조건

- 사용자 커밋 `4040c1029`는 `originals/d/options.cfg`와 `originals/fullscreenStateFile.dat`를 제거했다. 현재 두 파일이 없는 상태가 정상적인 처음 설치 상태다. 누락 오류로 취급하거나 이전 파일을 복원하지 않는다. 게임이 실행 후 설정을 새로 만드는 것은 정상이다.
- `originals/d/setup.cfg`는 그대로 존재한다. 최초 실행은 이 기본 설정과 실행 파일 초기값을 사용하며, 이전 사용자의 환영창 확인·음량 변경·`Done*` 진행 기록을 물려받지 않는다. TAFF에도 `options.cfg` 엔트리는 없었다.
- 현재 PC는 기존 전체 디컴파일 기록과 같은 **HJOW-Athlon**이다. 이번에는 REA/Ghidra로 원본 PE의 환영창 분기를 추가 정적 대조했다. 기존 `extracted/refined` 산출물은 수정하지 않았으며 마지막 전체 디컴파일 호스트도 변경하지 않는다.
- 무옵션 독립 복사본의 첫 실행에서 설정 파일 자동 생성을 확인했다. 기본 전체화면과 창 모드 모두 Windows Graphics Capture가 시간 초과하여, 화면 관찰은 게임 전용 analyzeManager의 `screen` 캡처로 진행했다.
- analyzeManager는 없는 `options.cfg`를 필수로 열어 **복사본 준비 단계에서 실패했다**. 보호 원본에는 파일을 만들지 않았다. `extracted/original-first-run-20261116/capture-source/originals`라는 독립 관찰 소스에 게임이 자동 생성한 설정과 창 모드 설정만 두고 재시도했다. 이 설정에는 `welcomeGiven`·`Done*`·음량 변경이 없었다. 도구가 관찰 세션의 `InstallDir`·`startInFullScreen=0`·`SCREENW=1024`·`SCREENH=768`을 설정하고, 앞선 관찰 사본에는 `screenModeFlags=10`이 추가되어 있었다. **화면 모드·해상도는 기본값 검증으로 세지 않는다.**
- 실제 관찰 세션: `20261116T132116403Z-d03ee04ede0d`. 아래 PNG는 이 세션의 1024×768 클라이언트 영역을 보존한 것이다. 미션은 시작하거나 클리어하지 않았다. 세션 종료 결과는 `closed=true`, 자동 녹화는 `stopped`다.

원본 실행 파일 SHA-256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`, `netstorm.tarc`는 `f2cafc1a0242ffe59b9b2fbfd6555907a03585cd6053d4bb4b0599a2445ea2e6`, `d/setup.cfg`는 `bd79e34635ba284036638461d9f0532de99e38c48887c1791abfa08c6fd052b3`이다. 실행 전후 **originals 1,392파일·originalCD 425파일, 총 1,817파일의 이름·SHA-256이 모두 동일**했고 두 삭제 파일도 계속 없는 상태다.

## 2. 메인 메뉴 전 안내창

실제 화면에서 다음 순서를 확인했다. 첫 두 창이 열려 있는 동안 배경 타이틀은 보이지만 메인 메뉴의 8개 버튼은 아직 표시되지 않는다.

1. `welcomeGiven=0`이면 Tell **Welcome**을 연다. 제목은 `NetStorm`, 내용은 Nimbus의 공중 섬 세계와 적 사제 포획·희생에 대한 소개다. 버튼은 **OK** 하나다. [첫 안내창](../screens/original-first-run-20261116/welcome.png).
2. OK를 누르면 Tell **Welcome2**를 연다. 제목은 `NetStorm`, 내용은 Early Missions로 기본 개념을 먼저 익히라는 안내다. 버튼은 **Play Early Missions**와 **Main Menu**다. [두 번째 안내창](../screens/original-first-run-20261116/welcome2.png).
3. Welcome2 본문의 `<$config,welcomeGiven=1>`은 **창이 열릴 때** 실행된다. Main Menu 버튼을 눌러야 비로소 메인 메뉴 버튼들이 표시된다. Play Early Missions의 명령은 `MissionBegin,Tutorial1`이며 이번에는 실행하지 않았다.

정적 근거는 `originals/netstorm.tarc`의 `\d\tell.english` `[WELCOME]`·`[WELCOME2]`, PE `0x4b7275`의 `WelcomeGiven` 조회와 부모 `FUN_004b6dd0`다. REA Evidence는 `ev_b0524efe35f4e063f135210ea92bbd0d580158a9dff6fb370b3cc041f0daed33`·`ev_6d11386c97945e72a85b43bf8e380881d198a8383282ef163e6b0639928b896c`이며 로컬 원시는 `extracted/original-first-run-20261116/rea-evidence.json`에 있다.

`welcomeGiven=1`인 이후 실행은 첫 메뉴에서 `tellTips=1`일 때 Tips를 여는 기존 분기다. `tipNumber` 기본값은 0이다. 이 이후 실행은 이번에 새로 관찰하지 않았고 [기존 시작창 분석](../webpj-startup-dialogs-20261030.md)과 `cpppj/src/client/NativeMainMenuState.cpp`를 근거로 구별한다. 폐기된 클라이언트 검증·Not Validated 안내는 클론에서 계속 비활성화한다.

## 3. Struggle For Freedom의 잠금과 클리어 표시

Campaign → Struggle For Freedom에서 **1번만 밝은 글자로 선택 가능, 2~6번은 어두운 글자로 비활성, 여섯 행 모두 클리어 체크 없음**을 실제 확인했다. 2번 행 클릭 전후 PNG는 SHA-256 `b8a8e36e8fc352769b9346535dc69630db3a00f2cc25c4b63355a32b1f8eae09`로 같고 `changedRatio=0`이었다. [최초 캠페인 목록](../screens/original-first-run-20261116/struggle-locked.png).

원본 TAFF의 `\d\offical3.english`는 다음 조건을 사용한다. 키는 대소문자를 구분하지 않는다. 없는 `Done*` 값은 0으로 판정한다.

| 행 | 미션 키 | 클리어 체크 조건 | 선택 가능 조건 |
| --- | --- | --- | --- |
| 1 The War Begins! | `TheWarBegins` | `DoneTheWarBegins` | 항상 가능(`1`) |
| 2 Master of Whirligigs | `MasterOfWhirligigs` | `DoneMasterOfWhirligigs` | `DoneTheWarBegins` |
| 3 Save the Island! | `SaveTheIsland` | `DoneSaveTheIsland` | `DoneMasterOfWhirligigs` |
| 4 Fragile Fortune | `FragileFortune` | `DoneFragileFortune` | `DoneSaveTheIsland` |
| 5 Thundering Power! | `ThunderingPower` | `DoneThunderingPower` | `DoneFragileFortune` |
| 6 Dissolved Alliance | `DissolvedAlliance` | `DoneDissolvedAlliance` | `DoneThunderingPower` |

`$Checked`의 4번째 인자는 **해당 행의 클리어 체크**, 5번째는 **선택 가능 여부**다. 두 값을 합치지 않는다. 예를 들어 `DoneTheWarBegins=1`만 저장되면 1번은 체크되고, 2번은 체크 없이 선택 가능해지며, 3~6번은 계속 잠긴다. `\d\thewarbegins.english`의 승리 안내에는 `<$Config,Done{mission.fileName}=1>`이 있다. 이 해금 규칙은 원본 스크립트와 [기존 Checked 분석](cpp-html-checked-client-20261025.md)의 정적 대조이며 **실제 클리어 후 화면을 새로 관찰했다는 의미는 아니다.**

`screenShots`의 기존 Struggle For Freedom 화면은 6개가 이미 클리어된 사용자 저장 상태다([이전 비교](../webpj-campaign-lists-20261029.md) 2절). 그 화면과 옛 `options.cfg`의 `Done*` 값을 최초 설치 기본값으로 사용하지 않는다. 다른 캠페인·Priest Training의 해금 조건을 이번 한 묶음의 규칙으로 일반화하지 않는다.

## 4. Options 초기값

[Options 전체](../screens/original-first-run-20261116/options.png), [효과음 음량](../screens/original-first-run-20261116/sound-volume.png), [음악 음량](../screens/original-first-run-20261116/music-volume.png)을 실제 열어 확인했다. 토글·음량은 바꾸지 않았다.

| 항목 | 최초 기준 | 근거와 한계 |
| --- | --- | --- |
| Sound On / Play Music / Wind Noise | 모두 켜짐 | 실제 메뉴와 `setup.cfg`의 `sound=1`·`music=1`·`playWind=1` |
| Speaker Swap L/R | 꺼짐 | 실제 메뉴, 스피커 교환 기본 0 |
| Sound Effect Volume | **5 / 5** | 실제 하위 메뉴의 Volume 5 체크; setup에 해당 키가 없어 실행 파일 초기값 사용 |
| Music Volume | **5 / 5** | 실제 하위 메뉴의 Volume 5 체크; setup에 해당 키가 없어 실행 파일 초기값 사용 |
| Edge Scroll in Fullscreen | 켜짐 | 실제 메뉴와 `edgeScroll=1`; 가장자리 이동 자체는 이번에 검증하지 않음 |
| Auto-Demo | 켜짐 | 실제 메뉴와 `autoDemo=1`; 자동 데모 진입은 이번 범위 밖 |
| Tell Tips at Startup | 켜짐 | 실제 메뉴와 `tellTips=1`; 최초에는 Tips보다 Welcome 분기가 먼저 |
| 해상도 | **800×600** | 수정하지 않은 setup의 `SCREENW=800`·`SCREENH=600`. 관찰 세션의 1024×768은 도구 설정이므로 기본값이 아님 |
| 전체화면 시작 요청 | 켜짐(`startInFullScreen=1`) | setup의 `workingFullScreenFlags=11`·`windowScreenFlags=10`. 원본 실제 전체화면 출력·재실행 정상 여부는 미확인; 클론은 AGENTS의 전체화면 재실행 오류 방지 요구를 계속 충족해야 함 |

**분석 당시 클론과 확인된 차이:** 삭제 직전 원본 옵션(`git show 4040c1029^:originals/d/options.cfg`)에는 `welcomeGiven=1`·1024×768·`soundVolume=3`·`musicVolume=2`가 저장되어 있었다. 이는 이전 사용자의 저장값이다. 실제 최초 메뉴는 **둘 다5**이며, 원본 PE의 `DAT_00531900`(파일 오프셋 `0x131900`)과 `DAT_00531904`(`0x131904`)의 초기 DWORD도 각각 `05 00 00 00`이다. 이 전역을 `FUN_00435220`이 해당 설정 키가 있을 때만 덮어쓴다. 당시 `cpppj/src/client/ClientAudio.h`의 `ClientAudioOptions`는3·2, `webpj/www/platform/settings.js`의 fallback도3·2였다. **cpppj는1314에서5·5로 교정했고 DN/WB 반영은 남았다.** 저장된3·2를 가진 기존 사용자는 그대로3·2를 유지해야 한다.

언어는 원본 자동 생성 설정에서 `english`였다. 11.01에 추가된 Difficulty·Frame Rate·Language·AA/FSR 등의 정책은 기존 요구를 유지한다(예: 난이도 보통, AA/FSR 꺼짐). 원본에 없는 항목의 기본값을 이번 관찰로 검증했다고 기록하지 않는다. Server Diagnostic 항목의 원본 온라인 기능도 복구 대상에 넣지 않는다.

## 5. 구현 인계와 완료 기준

이번 단위는 문서와 증거만 바꿨다. 기존에 환영창·Checked·설정 저장 부품이 있다는 사실과, **설정 없는 제품 실행 전체가 검증됐다는 것**은 구별한다.

- **cpppj 우선(다음 제품 단위 1314):** 기존 Saved Games와 설치 데이터에 영향이 없는 빈 `NETSTORM_USER_DATA` 및 이전 `options.cfg`가 없는 별도 데이터 사본으로 일반 시작을 비교한다. Welcome → Welcome2 → Main Menu, 잠긴 행 무반응, 6개 체크 없음, 위 Options 기본값을 맞춘다. 이미 일치하는 부품은 보존한다. 설정 자동 생성·재실행 유지와 완료 값에 따른 체크/다음 행 해금도 확인한다. 숨은 `userCheat`·실행 인자·오라클용 설정이 정상 시작을 우회하지 않게 한다.
- **저장·배포 공통:** 사용자 저장값이 존재하면 그대로 우선하며, 이번 요구를 기존 사용자의 진행을 초기화하는 것으로 구현하지 않는다. 배포·자산 준비·분석 사본에 옛 개인 `welcomeGiven`·`Done*`·음량·해상도 값을 섞지 않는다. `assets/game-data/d/options.cfg`는 현재 설명 주석만 있는 템플릿이므로 이를 옛 진행 파일과 혼동하지 않는다. 옵션 파일이 없는 입력도 정상 처리해야 한다.
- **dotnetpj:** cpppj 확정 커밋 뒤 새 경로의 `HostStartupScene`·Tell/Checked·설정 저장 수명에 같은 기준을 반영한다. 빈 사용자 루트로 시작·재실행을 비교하고, 설치 데이터 폴더에 쓰는 현행 `HostStartupScene.SaveOptions`는 기존 사용자 홈 정책(D1)과 함께 보정한다. 기존 메뉴 L4a 12장면은 깨끗한 최초 상태 전체를 보증하지 않는다. R4 부품 이식과 제품 플레이 미완료 상태는 그대로다.
- **webpj:** `netstorm.settings`·`netstorm.progress`가 모두 없는 독립 브라우저 저장소에서 확인한다. 클릭하여 시작 → Welcome → Welcome2 → Main Menu, Struggle 최초 잠금과 완료 체크·다음 행 해금, 토글·음량이 대상이다. 기존 브라우저 저장값은 지우지 않는다. 해상도·전체화면·Edit·멀티플레이 등의 웹 제외는 유지한다. 시작창/진행 저장 부품은 존재하지만 실제 미션 완료 연결은 아직 잔여다.
- **analyzeManager 공통 잔여:** `SessionStore.CreateAsync`가 `options.cfg`와 `setup.cfg`를 같은 필수 파일로 읽는 가정을 수정한다. options가 없으면 관찰 사본에 표시/경로용 설정만 생성하고, 원본 setup 기본값·환영/진행 상태를 보존해야 한다. 보호 원본에 옵션 파일을 다시 만들어 우회하지 않는다. 이번의 원본 그대로 `start_session` 실패 세션은 `20261116T132030115Z-1c0c78cc8e58`이다.

최초 실행 분석·PNG 보존·세 LEFT_JOBS 갱신만 완료 처리한다. 제품 수정·새 배포 관문·실제 미션 클리어는 완료 처리하지 않는다. cpppj의 CD10.72 보류 및 기존 DN/WB 미완료·중단 인계도 유지한다.

## 6. 보존한 화면과 로컬 원시

| 화면 | PNG SHA-256 |
| --- | --- |
| Welcome | `9271038a3c56557e361ab60d9cb65e87cd082705b0bf226e2ff8572bdda677cb` |
| Welcome2 | `ddcc7684ba9f1c67d2b73e1d5dcda3beadc3f1aedefb956d6418d6b473f76883` |
| Struggle 최초 목록 | `b8a8e36e8fc352769b9346535dc69630db3a00f2cc25c4b63355a32b1f8eae09` |
| Options | `c1e4167fff4a7493cd53a0676896c9734b4db0fd2cf8456d9b1e0a25d0e7514c` |
| Sound Effect Volume | `1ed9f5e2b3dbc8795f32813759fba2436af1ba1af235bc51dd13872dc0c4b791` |
| Music Volume | `af37b841b242fa911d613ecfec8a7b5148e76eafe6e6567b8120eed5d6ed9a2a` |

로컬 원시는 `extracted/original-first-run-20261116/`의 `protected-before.json`·`protected-check.json`·`setup-decoded.txt`·`options-after-unmodified-launch.txt`·`rea-evidence.json`과 `capture-source/extracted/analyzeManager/<세션 ID>/`의 이벤트·보고서·녹화다. 로컬 원시 경로와 세션이 없는 PC에서도 이 문서와 추적된 PNG 6장으로 관찰 결과를 검토할 수 있다. 전체 코드 회귀·클론 GUI 검사는 이번 문서 단위에서 실행하지 않았다.
