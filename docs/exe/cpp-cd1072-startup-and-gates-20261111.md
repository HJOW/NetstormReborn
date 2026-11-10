# CD 10.72: Windows 10 구동 관찰·해결 방안과 판본 게이트 목록 (C1·C2, 2026-11-11)

마지막 디컴파일 PC 호스트명: HJOW-Athlon(변경 없음). [60/120프레임·CD 10.72 계획](cpp-post-plan-fps-cd1072-20261111.md)의 C1(구동 원인·해결 방안 문서화)과 C2(판본 게이트 목록)다. AGENTS는 10.72 복원을 **차후 작업**으로 두고 "문서화 시 10.72 관련 내용과 Windows 10 구동 해결 방안을 기록"하도록 했으므로, 이 문서가 현재 요구의 마감이고 C3~C5(실제 복원)는 아직 하지 않았다.

## 1. 10.72 CD판 구성(원본 설치 CD 복사본 `originalCD`)

| 파일 | 역할(관찰) |
| --- | --- |
| `NETSTORM.EXE`(1,647,616바이트) | 10.72 본체. 10.78의 `Netstorm.exe`(1,552,384)보다 크다. PE import는 `KERNEL32/USER32/WINMM/WSOCK32/smackw32`이며 **`ddraw.dll`을 정적으로 쓰지 않고 `LoadLibrary("ddraw.dll")`+`DirectDrawCreate`로 동적 적재**한다(문자열 `ddraw.dll`/`DirectDrawCreate`, `Illegal screen mode: DirectDraw without being installed`/`no DirectDraw and no DIB`). 10.78도 같은 방식이다. |
| `NETRUN.EXE`, `SETUP.EXE`, `SETUP.BIN/DAT`, `AUTORUN.INF`, `AUTOPLAY.ICO`, `PRODUCT.INI`, `DSETUP*.DLL`, `3DCARD.DLL`, `SYSINV.DLL`, `GETINFO.DLL`, `VALIDATE.EXE`, `NETSTORM.VAL`, `TESTCONN.EXE`, `SPLASHAPP.EXE`, `nsutil.exe` | 설치/실행기/하드웨어 조회/검증 도구. 게임 실행에는 `NETSTORM.EXE`와 데이터(`NETSTORM.TARC`, `D`, `SOUND`, `MUSIC`, `MOVIE`, `NSENGLISHRES.DLL`, `SMACKW32.DLL`)가 필요하다. |
| `CD.NETSTORM.TITANIC.ACTIVISION` | CD 확인용 표시 파일(10.78은 `hd.netstorm.titanic.activision`). 본체에 `InsertCD` 문구가 있다. |
| `R.EXE` | 10.71 서버(R). 클론은 새 R을 따로 구현했다. |

## 2. Windows 10에서의 실제 관찰(HJOW-Athlon, Windows 10 build 19045)

원본 CD 복사본(`extracted/cd-run`, 보호 디렉터리를 쓰지 않음)에서 `NETSTORM.EXE`를 20초 실행하고 내가 시작한 프로세스만 종료했다.

- 창 `NetstormClient`("Activision and Titanic Entertainment Present: NetStorm", 656×519)가 **뜨고 응답한다**(`IsHungAppWindow` 거짓, 오류 대화상자 없음). 시작 중 폰트 캐시(`D\!Arial.*.chfnt`)를 만들었으므로 UI 초기화까지 간다.
- 창 내용은 PrintWindow로 검게만 잡히고 화면 캡처는 이 환경에서 불가해 **무엇이 그려지는지는 확인하지 못했다**(Smacker 인트로 재생 중이거나 DirectDraw 표면이 캡처되지 않는 것으로 보인다).
- 사용자가 겪는 "아예 구동 안 됨"은 이 환경에서 재현되지 않았다. 원인 후보는 ① 기본 전체화면 DirectDraw 모드 전환(10.72의 `screen mode` 기본값), ② CD 확인(`InsertCD`, `GetDriveTypeA`) 실패, ③ 설치되지 않은 DirectX 6 `DSETUP`/구형 `3DCARD.DLL` 의존, ④ 호환 모드 부재다. ①②는 같은 호스트의 사용자 환경(전체화면·실제 CD 드라이브)에 따라 달라진다.

## 3. 해결 방안(클론 복원 때 적용)

1. **전체화면 DirectDraw 대신 창 모드 DIB 호환 장치**: 10.78 클론이 이미 쓰는 Windows 10/11 borderless/DIB `Screen` 경로를 10.72에도 재사용한다(`Illegal screen mode` 분기는 이 장치가 항상 DIB이므로 도달하지 않는다).
2. **CD 확인을 설정으로 대체**: `GetDriveTypeA`/표시 파일 조회를 클론 설정(`CDDir` 지정 + 표시 파일 존재만 확인)으로 바꾸고 CD 삽입 대기(`InsertCD`)는 하지 않는다. 폐기된 온라인 유효성 검증(`VALIDATE.EXE`, `NETSTORM.VAL`)은 10.78과 같이 복구하지 않는다.
3. **설치/실행기 불필요**: `SETUP`/`NETRUN`/`DSETUP*`/`3DCARD`/`SYSINV`/`GETINFO`는 복원 대상이 아니다. 배포는 클론 단독 exe + 데이터(`game/`)로 한다.
4. **Smacker 인트로**: `SMACKW32.DLL` 대신 클론의 `NetstormSmacker.dll`/영상 재생 장치를 쓴다(10.78 방식과 동일).
5. **외부 접속·검증 비활성**: 10.72의 서버 주소/클라이언트 검증 호출도 [외부 접속 문서](cpp-external-services-20261101.md) 정책대로 찾아 문서화·비활성한다(미착수).

## 4. 판본 게이트 목록(C2)

클론 소스에서 `Patch1078`이 아니면 거부(`throw`)하는 몸체는 **241개 파일 306곳**(`o/` 240개, `client/` 1개)이다. 반대로 `Cd1072` 분기를 가진 파일은 40개 55곳(자산·타입 표·지형·섬 배치·일부 전투 계산)이다.

- **대화형 실행 불가:** `Netstorm.exe --run <CD 자료> --cd`는 `Unknown --run option: --cd`로 거부된다. `--cd`는 분석 CLI(`--inspect-*`, `--dump-*`, `--read-*`)에만 있고 일반 클라이언트(메뉴·미션·월드)는 Patch1078 전용이다. 따라서 C3(복원)은 먼저 이 게이트들을 판본별로 분기하는 일이다.
- **복원 순서(제안):** ① 자산/타입/폰트 경로(대부분 분기 있음) → ② 시작/메뉴/HUD → ③ 미션/튜토리얼 시작 → ④ 전투 몸체(Gun·Carrier·Ai·Pop 계열의 `Patch1078` 단언을 CD 대응 주소(소스 주석의 `↔ CD 004a9460` 등)로 채움) → ⑤ LAN.
- **디컴파일 대응:** 10.72 5,823함수 중 10.78과 대응하는 2,496쌍이 있어(강 1,738) 소스 주석에 이미 연결돼 있다. 나머지 3,300여 함수는 10.72 전용이거나 대응 미확정이다.

## 5. 남은 일

C3~C5(실제 복원·CD 설정 대체·CD 자료 실행 검증)는 복원 범위(어느 모드까지)와 CD 확인 정책의 사용자 결정 뒤에 착수한다. 사용자 환경의 구동 실패 로그/오류 문구가 있으면 §2의 원인 후보를 좁힐 수 있다.
