# UserUpdated0054da8c 시작 writer 분류 — 673 정적 감사

2026-11-01, HJOW-Athlon. AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 다시 읽었다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 현재 hostname과 일치한다. 원문10.78 정밀 C·보호 PE·기존 TAFF 추출 텍스트만 읽었다. 제품/도구/원본 수정, Native·게임·빌드·검사·커밋은 모두0이며 이 ignored 문서만 작성했다.

**판정:** 기본 무인자 시작의 UserUpdated0과 UserNotUpdated 안내는 원문 동작이다. 이 안내 때문에 옛 스모크의 Options 첫 입력이 막히는 현상을 UserUpdated=1 합성으로 해결하면 안 된다. 실제 OK 버튼을 눌러 진행하는 입력 교정이 맞다. 유일 writer는 공통 WinMain의 비기본 명령줄/security.dat 검증 경로라 호출 위치 자체는 MP 전용이 아니지만, 의미는 **구 Auto Updater의 버전 검증**이고 추가 허용 효과는 옛 인터넷 멀티플레이 쪽이다. 싱글플레이·LAN을 위한 새1/2차 필수 결함으로 승격할 근거는 없다. 비기본 런처 입력 호환이 아직 복원되지 않았다는 사실과 구 인터넷MP 소비 경계는 별도로 유지한다.

## 실제 의미와 안내 버튼

기존 추출 `extracted/tarc/d/tell.english:295~301`의 UserNotUpdated는 제목이 Not Validated다. Auto Updater를 실행하지 않았거나 현재 버전 검증에 실패했다는 안내이며 **싱글플레이와 LAN은 허용**하고 인터넷 multiplayer 제한을 설명한다. 버튼은 원문 **`$Button=OK,DoNothing,0`**이다. 따라서 사용자 이름·프로필·캠페인 완료값의 검증이라고 부르거나, Config의 동명 키로 대체할 근거가 없다.

## 유일 writer의 도달 조건과 순서

| 근거 | 원문 동작 |
| --- | --- |
| WinMain `00438dc0`, Netstorm.c:46822~46834 | 원문 lpCmdLine의 첫 BYTE가 비어 있으면 아래 전체 블록을 생략한다. security.dat가 있어도 이 경로로 읽지 않는다. |
| :46835~46848, PE00438e88/00438eb4 | 비어 있지 않은 원문 명령줄을 지역 버퍼에 복사하고004e3c40으로 가공한 뒤 현재 작업 디렉터리의 `security.dat`를 입력 스트림으로 연다. MP/미션/Player 관문은 이 앞에 없다. |
| :46849~46860, PE00438f12/00438f27/**00438f32** | 열기/스트림 상태가 정상일 때 토큰을00437da0으로 추출하고004e3720의 반환DWORD를0054da8c에 저장한다. 직접 writer는 이 한 곳으로 확인됐다. |
| :46861~46872 | 스트림 Close 뒤 `DeleteFileA("security.dat")`를 요청한다. 해당 블록 밖에서 원문 lpCmdLine 첫BYTE를0으로 지운다. 파일 열기 실패도 명령줄 비우기를 생략하지 않는다. |

보호 PE의0054da8c는 `.data`의 raw 범위 밖 zero-fill 부분이며 메모리 매핑 초기4BYTE는00000000이다. 그러므로 **원문 명령줄 없음 또는 파일 부재 시 BSS0 유지**가 기본 계약이다. 이 값을 무조건1로 설정하는 초기화는 원문이 아니다.

가공 함수004e3c40(:179306)은 명령줄 바이트를 별도 버퍼에 넣고004e39b0(:179156)을 호출한다. 결과는 this+4/+8의 별도 바이트열이며 평문 명령줄/이름 문자열을 곧바로 비교하는 함수가 아니다.004e3720(:178987)은 security.dat에서 읽은 NUL 종료 토큰을 이 가공 결과의 앞부분과 순서대로 비교한다. 토큰이 끝나면1을 반환하므로 완전 길이 일치 검사라고 바꾸면 안 된다. 스트림 토큰 추출00437da0(:45742)의 빈 입력/EOF/오류와 부모가 추출 뒤 상태를 다시 확인하지 않는 위치도 후속 구현 시 별도 원문 계약으로 다뤄야 한다. 이번에는 검증 토큰을 생성하거나 함수/PE를 실행하지 않았다.

## 실제 소비자와 우선순위

1. **일반 State3 첫 안내** — `004b6dd0`, :145745~145756 / PE004b72b6. WelcomeGiven이 비영이고 startup 확인이0이며 첫 메뉴 안내가 남아 있을 때 UserUpdated0이면 UserNotUpdated를 연다. TellTips도 그 뒤 현재값으로 다시 읽는다. 검증 상태는 일반 메뉴에서 보이지만 안내 이후 싱글플레이를 거부하는 관문은 아니다.
2. **멀티플레이 설정 UI** — `004cc970`, :161977~162079 / PE004ccab7·004ccb8f. `lastMultiplayerFort`, Find Local Server, TCP/IP/SPX/IPX, 서버 선택/버튼 상태와 함께 UserUpdated를 읽는다. 이 함수는00515c90[6]의 가상 처리기이며 일반 캠페인/튜토리얼 진행 writer가 아니다. 인터넷MP 제한 의미는 실제 UserNotUpdated 문구와도 일치한다. 이 값을0으로 둔다고 LAN 전체를 금지하는 구현을 추가하면 안 된다.

확인한 직접 참조는 writer1곳과 위 읽기3곳이다. 간접 alias 전체 부재의 증명으로 확대하지 않는다.00000000 기본값은 현재 보유 자료의 일반 실행에 맞으며, 외부 Auto Updater가 만든 유효 전달 자료/런처가 없는 상태에서 항상1을 기대할 이유는 없다.

## 현행 연결과 실제 입력 자료

- `ClientLocalStateGlobals.h:22~23,92`가 원문0054da8c의 Client 수명DWORD를 BSS0으로 보관한다. Config 동명 키를 사용하지 않으며 실제 시작 검증 writer는 아직 없다.
- `UberEditor.inc:122`가 같은 현재 UserUpdated getter를 NativeMainMenuState에 연결하고, `NativeMainMenuState.cpp:43`이 원문 조건으로 UserNotUpdated를 연다. 안내를 열어 스모크 첫 입력이 가려지는 것은 이 소비자의 정상 결과다.
- `src/app/main.cpp:848~860`의 무인자 GUI/명시 검사CLI와 `ClientMain.cpp:267~275`의 `options_.settings` 설정 파싱은 구 WinMain의 lpCmdLine/security.dat 전달 계약과 구별한다. 현재 검사CLI의 `--run`/`--set` 인자를 원문 보안 입력으로 가공해서는 안 된다.
- `originals/security.dat`와 `originalCD/security.dat`는 현재 없다. 두 원본 트리의 파일 목록에서도 해당 이름을 찾지 못했다. 원본 EXE/DLL/텍스트 중 `security.dat` 문자열은 Netstorm.exe에서만 확인됐다. 구 Auto Updater 실행 파일이나 유효 전달 자료가 제공됐다고 주장하지 않는다. 원본 파일 생성/수정/삭제는 하지 않았다.

## 최소 후속 제안

현재672 스모크는 실제 UserNotUpdated의 OK를 누른 뒤 Options로 진행한다. 제품 전역/설정/원본 파일을 바꾸지 않는다. 안내 제목/버튼/DoNothing 효과와 다른 첫 안내(Tips/Welcome)의 겹침을 실제 현재 노드로 구별한다.

비기본 구 런처 시작 호환을 후속으로 복원할 때는 원문 lpCmdLine을 검사CLI 설정 문자열과 분리하고, 가공/토큰 대조·파일 열기/추출/Close/삭제 요청·원문 명령줄 비우기의 순서를 명시 경계로 보존한다. 현재 목표의 TCP/IP LAN 구현에 인터넷 검증1을 필수로 요구하지 않는다. 공식 인터넷 서버/Auto Updater 복구를 새 목표로 가정하지 않는다. 원문 DeleteFile 요청은 정적 근거일 뿐이며 보호된 originals/originalCD에 실제 적용할 권한으로 해석하지 않는다.
