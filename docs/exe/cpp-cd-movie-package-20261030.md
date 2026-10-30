# CD 경로 및 인트로 영상 배포 — 단위595

2026-10-30 / HJOW-Athlon / Windows10 19045. 기준 커밋 f5b45f3d, 마지막 전체 정밀 디컴파일은 같은 HJOW-Athlon/2026-10-23이다. 보호 기준은 원본1394/CD425/기존fixture741/AGENTS.md/정밀 functions.tsv의2562파일이다. 보호 파일은 수정하지 않는다.

`CdDirectory`는 시작 설정이 CDDir로 사용하던 원문 CD 탐색을 복원한다. 설치 경로를 CD로 가정하던 Client 경계 대신 실제 CD-ROM을 열거하고 표시 파일이 있는 디렉터리를 사용한다. CD가 없으면 ConfigInterface의 기존 InstallDir fallback을 유지한다.

| 10.78 | 10.72 대응 | 복원 범위 |
| --- | --- | --- |
|00440d40|004a8a60|CD 디렉터리 첫 발견/목적지 반환|
|004dde40|004bf740|GetLogicalDriveStrings 순서/종류5 CD-ROM만 선택|
|004dd340|004bdea0|표시 이름/OS 항목 순서/깊이2 재귀/핸들 수명|

표시 파일은 `cd.netstorm.titanic.activision`이다. 모든 드라이브 종류를 먼저 조사한 뒤 CD들만 순서대로 탐색한다. 파일들을 정렬하지 않으며 하위 디렉터리가 먼저 나열되면 루트의 나중 표시 파일보다 먼저 재귀한다. 점으로 시작하는 디렉터리는 재귀하지 않는다. 깊이2까지 내려가고 표시 이름은 ASCII 대소문자를 무시한다. 발견한 목적지는 원문 strncpy/NUL 처리로 쓴다. 원문의 cancel 가지는 현재 핸들을 닫지 않고-1을 반환하며 상위는 비영 성공으로 처리한다. 이 특이한 부분효과도 관측/검사에서 보존한다. 실제 시작 장치의 cancel은 아직 입력 처리기 설치 전이므로 false다.

Windows10/11에서는 ANSI 대신 W 파일/드라이브 API를 쓴다. UTF-8 설정 경로가 잘리지 않도록 실제 출력 버퍼도 긴 Unicode 경로에 맞게 확장했다. 원본과 다른 장치 경계/이유를 한국어 코드 주석에 기록했다. 원문의 경로 탐색 판단은 그대로이며 CD 캐시 검사00440c60이나 설치 레지스트리 탐색 전체를 복원한 것은 아니다.

## 원문 관측

[독립 도구](../../tools/decomp_cd_directory.py)는 보호 PE의 위 세 몸체를 Unicorn으로 실행한다. Win32의 드라이브/Find API와 ASCII CRT stricmp/strncpy만 명시 장치다. 실제 게임/창/실제 CD 드라이브는 실행하지 않는다. 최초15관측과 독립 재관측2회/30개가 전체 바이트로 일치한다. 목적지256BYTE 전체/마지막 guard, OS 호출 순서와 현재 열린 핸들을 대조한다. 부재/고정식 제외/첫째·둘째 CD/대소문자/루트보다 먼저인 하위/깊이2와3/점 디렉터리/일반 파일/중단/잘림/길이1/열거 실패를 포함한다.

전체 빌드 대기 중 같은 제품/검사 소스를 별도 임시 진입점으로 컴파일해 새2검사/실패0을 확인했다(quick-cd/build-final.log, tests.log). 첫 보조 빌드의 cmd 인용 실패는 build.log에 남겼으며 제품/CMake 검사 결과로 세지 않는다. 정식 Release 검사 실행 파일에서도 최종 집중24/실패0·실제 등록3487개를 확인했다.

C++ 검사는 이15개와 긴 한국어 UTF-8 경로/빈 목적지 거부를 같은 제품 본체에서 실행한다. 실제 물리 드라이브는 C/H 고정식뿐이며 CD-ROM은0개다. 실제 CD가 삽입된 W 장치 경로는 아직 검증하지 못했다.

## 배포 변경

`package-cpppj.ps1`은 명시 MovieSource, 게임 MOVIE, 저장소 originalCD/MOVIE 순으로 영상 원천을 선택한다. 실제 SMK만 game/MOVIE에 원문 바이트로 동봉하며 README/BUILD_INFO에 출처와 각 SHA256을 적는다. 원본 CD의 ANG/실행 파일은 추가하지 않는다. 명시 원천이 없거나 비어 있으면 오류이며 자동 원천이 전부 없으면 영상 미동봉을 경고한다. 원본의 InstallDir fallback과 introMovie/currentLanguage 치환을 그대로 사용하므로 영상 경로를 강제로 덮어쓰는 게임 분기는 추가하지 않는다. 원본 CD 전용 자료를 설치 폴더에도 넣는 배포 편의는 주석에 구별한다.

검증 도구의 fresh-install은 options.cfg 부재를 요구하고 intro/introMovie를 CLI에서 덮어쓰지 않는다. 실제 MovieGump가 OpenFile에 전달한 movie_source도 읽기 전용으로 보고해 언어별 기본 파일을 확인한다. 노드의 재생 판단/수명을 바꾸는 검사용 분기는 없다.

자동/명시 원천의 NoBuild DryRun1376항목과 잘못된 명시 원천의 의도한 종료1을 확인했다. 기본 배포 스크립트는 NoBuild 없이 실제 CMake 빌드와 ZIP 생성을 완료했다. 마지막 두 대상은 같은4개 병렬/UseMultiToolTask 설정으로 빌드했고 경고/오류0이다. 처음 직렬 빌드와 배포 스크립트의 추적 옵션을 맞추지 않아 재컴파일이 추가된 사실도 보존했다. 앞으로는 같은 빌드 옵션을 사용한다.

최종 집중은 CdDirectory2/ConfigInterface10/Movie7/NativeIntro2/ClientOptionsFile3의24개이며 실패0, 등록3487이다. 최초 char fill의 C4244 경고는 char 값으로 정리했다. 전체 C++ 검사는 실행하지 않았다.

새 ZIP은1376항목/게임1358파일이며 원본 영문·독문 SMK2개, VC 런타임3개/디코더 DLL1개, 대응 소스·라이선스를 포함한다. 모든 게임 파일/SMK와 대응 소스·라이선스를 원천과 바이트 단위로 비교했다. 개인 options/fullscreen 파일은 제외했다. BUILD_INFO의 실행 파일 SHA도 실제 ZIP 내용과 일치한다. 최초 검증기의 SHA 문자열 대소문자 비교 실패를 보존하고 대문자/소문자와 무관한 정확한 필드별 비교로 확인했다.

- 게임 SHA256: `f3c9de83f85769a567292852ce35703b3d770da855d0c02b53fa07bb3aa45d14`
- ZIP SHA256: `bdfbdfbe5347680fe093e7ac6f57df8094580c1572dd154d44a10969f733e47e`
- 로컬 ZIP: `extracted/cpp-cd-movie-package-595-20261030/NetstormReborn-cpppj-595.zip`

## 실제 배포본 검증

ZIP을 서로 다른 한글 경로 두 곳에 새로 풀었다. 사용자 설정 파일 없이 intro/introMovie CLI override를 주지 않고 같은 배포본의 실제 실행 파일을 시작했다. 영문 기본값과 currentLanguage=german의 실제 movie_source가 각각 동봉 파일을 가리키는지 확인했다. 비검정 영상 전체 RGB는 독립 FFmpeg와 일치했고 두 실제 창 DC도 논리 화면의 전체 RGB와 일치했다. ESC/Space 종료 후 Native40→41→3→4, 노드/두 정지 반납과 완료1 저장·재시작 메인을 확인했다.

영문 본 실행27.879923초/독문27.961037초·재시작2회, 모두 종료0이다. 독문 tell.german에는 UserNotUpdated가 없어 실제로 해당 TELL 부재 로그가 나온다. 보호 원문에도 부재함을 확인했고 임의 영문 fallback은 추가하지 않았다. 메뉴 복귀는 정상이다. 보호 독문 본문의 Hauptmenü 라벨도 조건부 클릭 후보에 넣었으며 실제 조작부가 있을 때만 입력을 보낸다. 이 범위를 독문 전체 UI/캠페인 완료로 확대하지 않는다.

별도 한글 자료 사본에서 MOVIE/개인 설정을 제외한 새 설치도 실행했다. 실제 파일 열기 실패→부분 노드 공통 지연 반납→두 정지0/메인 복귀/intro1 저장을3.729565초/종료0으로 확인했다. 영상 부재를 가짜 재생 성공으로 보고하지 않는다.

최종 실제 창 실행은 영문/독문 본 실행·재시작4회와 영상 부재1회의 총5회다. 실제 시나리오 플레이는 하지 않았다. 보호2562파일은 최종 변경0이며 현재 fixture는742개다. 최종 [집계](../../cpppj/cd-movie-package-runtime-20261030.json), 원시는 `extracted/cpp-cd-movie-package-595-20261030/`를 따른다.

자체11소스의22 checkout 필터(core.autocrlf true/input)는 실행 바이트와 일치한다. PowerShell 스크립트는 기존 text eol=crlf 정책에 따라 Git blob은LF/실행·checkout은CRLF다. 줄바꿈만 다른 두 SHA를 집계에 따로 기록하고 원문 fixture/Native 도구는 blob도 그대로인 것을 확인했다.

## 남은 범위

594의 집중51·실제 논리/수명5쌍·창 모드 물리 RGB 근거는 유지하나 이 변경 제품 검증을 대신하지 않는다. 실제 PCM 장치0개/전경NULL인 세션의 활성 전체화면 출력, Win11 실기, 현재제품 전체34/전체C++/자연 플레이 완주·전체 동일성은 미확인이다. 포획/희생/특정 적 공격/시나리오 승리·공중3종 전용·신규MP 검사는 하지 않는다. 전체 목표는 미완료이며 싱글플레이/UI·호환성 뒤 MP 무시험 사전 구현과4~7차 순서를 유지한다.
