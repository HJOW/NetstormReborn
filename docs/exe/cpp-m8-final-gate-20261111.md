# M8 최종 관문 1차 통과: 배포 ZIP 바이트·압축 해제 실행·외부 접속 재감사 (제품 1278, 2026-11-11)

마지막 디컴파일 PC 호스트명: HJOW-Athlon(변경 없음, 재디컴파일 없음). 이 문서는 M6-n/M7 완료 직후(순서 지시: M6-n → M7 → **M8** → K2~K5 → S → B → L)의 M8 관문 결과이며, 뒤 단위(K/S/B/L)가 코드를 바꾸므로 **같은 도구로 최종 출시 직전에 다시 실행**해야 한다.

## 1. 배포 ZIP 전체 바이트 정합성

`powershell -File cpppj\package-cpppj.ps1 -NoBuild <zip>`으로 실제 ZIP을 만들고(1430항목, 압축 193.9MB, 40초) 새 도구 [tools/verify_cpp_package.py](../../tools/verify_cpp_package.py)로 모든 항목을 원천과 SHA-256 대조했다.

| 항목 | 결과 |
| --- | --- |
| 원천과 해시 일치 | **1423개**(Netstorm.exe·R.exe·NetstormSmacker.dll·`game/`(originals + originalCD MOVIE + 서버 페이지)·`fonts/`·`lang/`·`third-party/`·LICENSE) |
| 생성 텍스트 비어 있지 않음 | 4개(Netstorm.bat, Netstorm-SavedMode.bat, README.txt, BUILD_INFO.txt) |
| VC 런타임 DLL(PE 시그니처) | 3개(msvcp140, vcruntime140, vcruntime140_1) |
| 필수 항목 누락 | 0 (exe/R/글꼴 라이선스/한국어 번역표/배치 파일/게임 자료) |
| 금지 항목 | 0 (`d/options.cfg`, `fullscreenStateFile.dat`, `*.php` 잔재 제외 확인) |
| exe 안 글꼴 시그니처 | 0 (`ttcf`/`wOF2`/`OTTO` 없음) — 글꼴은 `fonts\`의 D2Coding 파일에서만 적재 |
| BUILD_INFO의 SHA-256 | Netstorm.exe `2bacfa00…d196`, R.exe `fc1d0d46…6aad`가 ZIP 실제 바이트와 일치 |
| 압축 해제 | 1430파일/242,045,169바이트가 ZIP과 같음 |

## 2. 압축을 푼 배포본의 실제 실행

푼 폴더의 `Netstorm.exe --run game --mcp --window --no-audio`(HJOW-Athlon, 격리 사용자 폴더)로:

- **영어:** 메인 → Edit(432맵, 50행씩 `Next 50 >` 페이지 이동) → TEST01 → 편집 화면 → Edit 메뉴 `Scenario...` 패널 열기/ESC 닫기 → 종료 코드 0, 로그 파일 없음, 게임 폴더 파일 불변.
- **한국어:** `currentLanguage=korean` 메인 메뉴 8버튼이 `fonts\D2Coding...ttc`로 한글 표시(캡처 확인), 편집 화면까지 진입.
- **발견한 한계(결함 아님):** 설치 경로가 약 200자를 넘으면 원문 경로 작성기의 256BYTE 한계(`원문 경로 작성기의256BYTE 물리 범위 초과`)로 시작 직후 종료한다(오류 로그가 원인을 지목). 원문 경로 버퍼의 한계를 그대로 보존한 것이며 일반 설치 경로에서는 도달하지 않는다. 배포 안내(README)에 짧은 경로 설치를 권하는 문구는 K5/S4 점검 때 함께 검토한다.
- 읽기 전용 설치 위치 실행은 S4 범위다(이번에 하지 않음).

## 3. 외부 접속·클라이언트 검증 비활성화 재감사

- **PE import:** `Netstorm.exe`에 WinINet/WinHTTP/URLMon import 0. `ws2_32.dll`은 서수 import(LAN TCP/UDP, M6), `ShellExecuteW`는 도움말 링크 실행(사용자 선택 링크), `R.exe`는 `ws2_32`(서수)·`GetAdaptersAddresses`(로컬 어댑터 주소)뿐이다. 새 R/LAN 경계는 기존 [외부 접속 목록](cpp-external-services-20261101.md)의 정책(루프백/RFC1918/링크로컬만 연결, 폐기 공식 도메인·7f/80 온라인 검증 비활성)을 그대로 따른다.
- **문자열:** exe에서 `master.`/`security.dat`/`Not Validated`/`PIRATE`/`bugs@`/`setup.exe`/`nsLaunch` 0건. `titanic`은 CD 표시 파일 이름 `cd.netstorm.titanic.activision` 1건뿐이다. URL은 매니페스트 스키마와 FSR 셰이더 주석의 출처 링크뿐이며 접속하지 않는다. `UserNotUpdated`는 설정 키 이름이다.
- **이번 단위들(오류 로그·MCP·K1·S1)이 새 외부 접속을 만들지 않음:** 오류 로그는 사용자 홈 파일에만 쓰고, MCP는 표준 입출력이며, 한국어/시나리오는 로컬 파일 읽기/쓰기뿐이다.

## 4. checking 목록

기존 [1157 마감 대조](cpp-checking-parent-closure-20261108.md)의 "checking 목록 0개"를 유지한다. 이후 변경은 checking 소비자(Pop/Unpop/Mana/Factory/Graph/Building 진단, `RawCheckingFlag`)를 건드리지 않았고, 소스의 해당 진단 연결(`RawCampaignSession.cpp` 1108~1114 등)은 모두 실제 assertion 장치로 연결된다.

## 5. 남은 M8 일

- 일본어 등 다른 언어 글꼴(NotoSans/NotoSansJP)의 외부 배치·라이선스·ZIP 포함은 L 단계에서 같은 도구로 확인한다(`fonts/`에는 이미 파일이 있으나 지금 ZIP은 한국어 글꼴만 넣는다).
- K/S/B/L이 코드를 바꾸므로 **최종 출시 직전에** `package-cpppj.ps1` + `tools/verify_cpp_package.py --extract`를 다시 실행한다.
- 읽기 전용 설치 위치 실행(S4), 다른 모니터/DPI·ARM 실기는 별도다.
