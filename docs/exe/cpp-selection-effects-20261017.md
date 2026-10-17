# 선택 비트와 같은 Session의 마나·사정거리 표시

작업 단위143. 기준 `e95ed13`, 현재 PC 및 마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-16**이다. AGENTS와 두 LEFT를 다시 읽었다. 새 디컴파일·원본 게임 실행·전체화면 DirectDraw 수정은 없다.

## 실제 하위 주소와 명칭 교정

단위142에서 경계로 남긴 `4741d0`과 `474900`의 실제 하위를 확인했다. 각각 new(0xe0)→`4737f0`의 마나 표시 생성, new(0x200)→`474520`의 사정거리 표시 생성이다. 생산/운반 생성자가 아니다. `474240`도 이동 가능 판정이 아니라 group0/1/6/3 또는 bomb 비트의 **사정거리 표시 종류** 판정이다.

현재 `UserSelectionHooks` 명칭을 `production/prepareCarrier/movableType`에서 `showMana/showRange/rangeType`으로 고쳤다. 이전 작업의 원본 관찰·fixture와 과거 보고서는 보존한다. 이 명칭 교정은 생산·경제 복원이 완료됐다는 뜻이 아니다.

## 선택 비트와 표시 순서

보호 PE의83개 생성자 VT에서+7c는 두 몸체다. 일반45개는 `41c0b0`, Damageable38개는 `44b2b0`이다. 두 몸체 모두 인자 하위 bit0으로 raw extra bit2를 교체한다. Damageable은 **교체 전에 현재 가상+88을 호출**한다. 따라서 원본4d5c90이 그 뒤+88을 다시 부르면 Damageable은 변경 전과 변경 후 두 번 표시를 갱신한다.

[`RawSquidSelection`](../../cpppj/src/o/RawSquidSelection.h)은 현재 raw VT를 독립 생성한83개 표에서 찾아 이 두 몸체를 분배한다. 임의 VT를 기본 몸체로 대체하지 않는다. 표시 callback이 extra나 VT를 바꿔도 표시 후 현재 extra에서 bit2만 바꾼다. 원본과 같이 물리 범위 안 free/dead 슬롯에도 쓸 수 있도록 SidPool의 이 한 비트 쓰기 권한을 부여했다. 게임의 일반 할당 API를 느슨하게 바꾸지 않았다.

같은 모듈의 `SelectionUsesRange`는474240, `StartSelectionContentPreview`는426080을 복원한다. 후자는 로딩 중과 extra&9를 제외하고, TYPE<70 진단 후 현재 TYPE를 다시 읽으며, 실제491 이벤트·원문 float `3e4ccccd`(약0.2)의 Regular 생성을 요청한다. 할당/Regular 생성 본문은 기존 Host에 연결한다.

## 같은 세션 연결

`RawCampaignSession.MakeUserSelection`은 같은 현재 Pool/TYPE와 다음 기존 몸체들을 연결한다.

- 실제 선택+7c와 기존 SquidDisplay의 현재+88.
- 실제 ProcessForm→같은 Host/Kernel 프로세스의+1c 통지.
- 기존 CursorMana/Range 생성자와 현재50/51 종류.
- 실제 사정거리 종류, 현재 내용물·같은 위치 제단, 현재493 Regular 조회.
- 실제 로딩 깊이와 Host의491 Regular 생성, 같은 SoundList/현재 위치 소리.

UI의 현재 시계/입력 관문·강조·dump/진단은 caller 경계다. 반환 몸체의 훅은 같은 Session보다 먼저 해제해야 한다. 선택 비트 처리기는 반환 몸체가 수명을 소유하며 별도 Pool/가짜 프로세스를 만들지 않는다.

실제 Client/UberGump 선택 입구는 아직 교체하지 않았다. 특히491 Regular의 프레임 소비는 이번 생성 검증과 구분해야 한다. 자연 플레이 중의 선택·우클릭 완료로 확대하지 않는다.

## 독립 원본 검증

[`decomp_squid_selection.py`](../../tools/decomp_squid_selection.py)는 보호 PE의 두+7c,474240,426080 네 몸체를 실제 CPU로 실행한다. 정밀 함수표에 빠진41c0b0은 실제 ret4까지22바이트만 별도 허용한다. 표시+88·할당·Regular496e80·진단만 명시 입력이며 C++ 제품이나 테스트로 기대 분기를 만들지 않는다.

**5,600개 입력·11,200회 정상 반환·157,084명령·53/64비트 정밀도 차이0**이다. 모든83VT, 전체 extra BYTE, 인자 상위 비트, 표시가 바꾸는 현재 extra/VT, signed group·bomb 비트, 로딩·extra 관문, 진단의 TYPE 교정과 할당 실패를 포함한다. 원본이 실행하는 SEH의 격리FS0 복구와 보존GP/스택/canary/x87 계약도 확인했다.

초기 새 fixture는 VT 인덱스로 입력을 기록했다. 제품 코드에 의존하지 않는 입력을 더 명확하게 보존하려고 최종 fixture는 **실제 VT DWORD**를 기록한다. 같은 물리 입력을 다시 관찰했으며, 초기 도구/fixture/보고서는 `extracted/goal-squid-selection-20261017-indexed-input/`에 보존했다. 기존 프로젝트 fixture는 바꾸지 않았다.

## 검증 진행과 남은 연결

빌드1은 라이브러리와 실행 파일 컴파일에 성공했다. 공통 헤더로 라이브러리406/검사347파일이 넓게 다시 컴파일됐고, 현재 프로젝트는 파일 병렬 옵션을 쓰지 않았다. 로컬 논리CPU4개와 `cl /?`의 `/MP[n]`을 확인한 뒤, 응답 파일로 workspace/검사 프로젝트를 검증한 검사CL만 중단했다. 빌드1 끝의MSB6006(-1)은 의도적 중단이다. 기존CL 환경을 보존·복구하면서 `/MP4`로 빌드2를 재개했다. 소스나 검사 범위를 줄이지 않았다.

빌드2와 명칭 정리 후 증분 빌드3은 모두 종료0·경고0·오류0이다. 각 빌드의 집중 검사6개(UserSelection_4개·SquidSelection_2개)가 전부 통과했다. 같은 실제Session 사제의 bit2·기존 표시·491 Regular 생성과 실제50 프로세스의event3→flag94 통지를 확인했다. 이 검사는491 프레임을 실행하거나 실제Client 입력을 검증한 것이 아니다.

최종 전체 CTest는 **1,580개·실패0·종료0·191.60초**다. 새 독립 관찰의 C++ 재생2개, 기존 세 인자 선택3개와 같은Session 통합1개가 최종6개 집중 검사에서 모두 통과했다. 현재 원본 근거 감사는61종 중50종 성공·기존11종 실패·새 실패0이며 직전HEAD와 같던 현재 핀49보고서121개를 갱신/과거 보존했다. 로그 접두부는 `extracted/goal-squid-selection-20261017-`다.

CD10.72의 기존 정밀 대응은 사정거리 종류 `48b9d0`, 마나 생성 `48b950`,491 생성 입구 `4e36f0`이다. 두 선택 가상 통지와 사정거리 생성은 현재 정밀 대응이 없다. 이번 모듈과 CPU 관찰은10.78 전용이며 CD를 거부한다. 새 CD 복원 완료로 기록하지 않는다.

다음은 실제 Client의 같은 선택 저장소와 시계/강조/dump,491 프레임 소비, 실제49af80 설명·번역/정적 문자열 수명, VT별+a8/+ac와 원본 DWORD 소비자, 게임 시간을 정지하지 않는 우클릭이다. 로그인된 로컬 전송·생산/경제/AI 및 대상11스테이지 자연 완주·승패·저장·다음 미션·전체 동일성은 계속 미완료이며 전체 목표는 active다.


추가 정적 확인에서 `InstallContents`의 `carrierEffects.checkCarrier`가 원본426080 대신 기존 지면 검사 `priests->CheckCarrier`에 연결된 오류를 발견했다. 현재 단위의 실제 선택 factory와 구별한다. 다음 작업은42a750의491 내용물 표시·428700 표시 조각 프로세스를 복원하고, 이 기존 내용물 통지 연결을 같은426080 헬퍼로 교체하는 것이다. 생성만 연결하고 아직 없는 프레임 소비를 성공으로 처리하지 않는다.

새 헬퍼명은42a750의 실제491 내용물 표시를 반영해 `StartSelectionContentPreview`로 정정했다. `UserSelectionHooks.movementPreview`는 단위142의 호환 이름으로 남기며 실제 호출은 이 내용물 표시 헬퍼에 연결한다. 원본 관측/분기/기존 fixture는 바꾸지 않았다.


최종 인수인계와 연결 보고서는 `LEFT_JOBS.md` 및 `cpppj/campaign-selection-effects.json`이다. 보호 EXE2/TARC의SHA3, 과거 보고서 이력과 현재핀만의 변경, 기존CMake 바이트 접두, 새 파일의UTF-8/LF를 확인했다. 기존fixture·AGENTS·원본·dotnetpj는 수정하지 않았다. 실제Client입구가 변하지 않아 클론창 진입 검사는 반복하지 않았으며 단위141의 진입5회를 이번 선택의 자연 조작 검증으로 확대하지 않는다. 인계 후 `20261017-43 CP 선택 가상 통지와 같은 세션 마나 및 사정거리 연결`로 로컬커밋하고push하지 않는다.

기존 내용물 통지 연결 오류는 보호PE의 **42977f `call 426080`**으로도 확인했다. 해당 원본 호출과 현재426fc0 지면 검사를 동일한 처리로 인정하지 않는다. 정적 근거는 `extracted/goal-squid-selection-20261017-content-binding-call.json`에 보존했다. 이 오류와491 프레임 소비는 다음 작업의 우선 대상이다.
