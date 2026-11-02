# cpppj 단위736 — 시작 아카이브의 같은 FileSpec과 검색 수명

2026-11-02 / HJOW-Athlon / Windows 10. 마지막 전체 정밀 디컴파일은 HJOW-Athlon·2026-10-23이며 새 디컴파일은 없다. 733의5차 전체 자동 파일/reader·일반 권한 설치와 Windows10/11 AMD64·Windows11 ARM 고려 요구는 유지한다. 이관 구현과 ARM 검증은 미착수다.

## 실제 연결

`RawFileName::Full`은 원문459890처럼 같은10칸을 선증가/clear한 뒤 전체 입력 또는 **조건 없이 입력+2**에서 첫NUL까지 복사한다. flag0은 `.\`를 확인하는 경로 정규화가 아니다. 물리 입력의 첫NUL 뒤+2에 문자가 있어도 그 문자를 복사한다. 256BYTE 밖 쓰기와 +2의 물리 입력 자체가 없는 host view는 현재 칸 clear 뒤 거부하며 원문 C++ throw라고 해석하지 않는다.

FileSpecQuery에 원문459610 재대입·4597c0 Next·+104 유효 표식을 연결했다. 재대입은 새 문자열을 먼저 정규화한 뒤 이전 비NULL 핸들(실패 핸들 포함)을 닫는다. 검색은 원래 입력으로 시작하고, Next가 실패해도 마지막 문자열은 유지한다. 핸들0/이미 종료한 검색은 OS로 넘기지 않는다. 실제 Win32 W 장치는 원본 A 장치와 다른 Windows10/11 Unicode 호환이라는 한국어 주석을 남겼다. 기존 단일 조회의 명시 find/close 장치는 Next를 호출하기 전까지 next 콜백을 요구하지 않는다.

`BeginStartupArchives`는 원문441965~441a44 순서대로 기본 pattern 작성→같은 FileSpec의 Full/등록/Next→CD pattern 작성→**그 뒤 첫 핸들 닫기/재대입**→보조 Full/등록/Next→보조 루트 설정을 실행한다. 정렬하거나 등록 결과로 후보를 건너뛰지 않는다. 반환한 FileSpec이 마지막 검색 핸들을 계속 소유한다.

실제10.78 PrepareStartupFiles는 이 부모를 사용한다. Client::Run은 그 반환 객체를 ConfigInterface::Startup의 Load/InstallDir/CDDir Set 뒤까지 보관하고 원문 위치에서 반납한다. 등록 목록·기본/보조 reader·경로 소유자는735의 선택을 이어받는다. 기존 독립 RegisterArchives API와 생성자의 선행 읽기는 값형 계약을 유지하며 이번 Native 고리 부모에 합산하지 않는다. 전체 최초 I/O 순서 복원은 여전히 남는다.

## 독립 원문 근거

[관측 도구](../../tools/decomp_startup_archives.py)는 실제 **441965~441a44, Full, FileSpec 생성/재대입/Next, CRT**와 뒤의 별도 실제 소멸자를 CPU로 실행한다. FindFirst/Next/Close·등록/보조루트 설정만 명시 경계다. CWD를 확정한 명시 입력에서 시작하며 전체Startup의 앞 탐색/뒤 설정 Load/부모 반환 ABI를 실행한 근거로 세지 않는다. 설정 경계 marker 뒤 소멸자 호출은 원문 소스의 Load/두 Set 뒤 반납 위치와 구별해 기록한다.

기본루트3표기×CD없음/드라이브/UNC×파일0/1/3개×초기번호0/9의 **54행**, Full의 물리 입력8종×두flag의 **16행**이다. 두 FPCW의 **140관측**과 최초 fixture를 덮지 않는 재관측이 일치했다. 실제 등록 인자/순서·각 Next/Close와 실패 핸들·마지막 문자열·전체10×256BYTE 고리를 고정했다. Full은 정상 반환/비영 보존 레지스터/ESP/FPCW·입력 불변도 확인했다. 전체264BYTE FileSpec의 ABI/끝NUL 뒤 미정 배열을 복원했다는 뜻은 아니다.

## 검증과 남은 범위

Release 빌드 경고/오류0·새5개와 기존20개 **집중25개/실패0**이다. EXE SHA-256은 `5f694a28e066a608e52fbb2413f7853a761ce94fae4198706ca0532007e3a95f`이며 [통합 보고서](../../cpppj/startup-archives-20261102.json)를 따른다. 설정CLI1024/종료0·게임프레임0이다. 보호2650개(AGENTS1·원본1394·CD425·기존fixture830)는 모두 불변이며 새fixture2개를 더한 현재2652개/fixture832개다. 두목록/Full 새70행에 ConfigLoad12·setup탐색84·디렉터리192/모듈5·Startup24·등록27을 더한 원문414행을 대조한다. 등록 콜백 실패의 활성 핸들 반납, 일반/검색/종료/실패 핸들의 전환, 기존 실제 Unicode VFS/설정/기본·CD 등록 회귀를 포함한다.

735의 실제 같은 자료 루트/메인 메뉴 성공은 별도 제품 근거이며 이번에 새 GUI/전체34/전체 C++/ZIP은 반복하지 않는다. 생성자의 선행 TAFF 읽기·전체 최초 I/O 순서, 다른 FileSpec 소비자/실패, 일반 FILE/TAFF/보조/Template·UI/호환성 잔여는 남는다. 포획/희생·특정 적 공격/승리·공중3종 전용·새 MP/통신 검사는 추가하지 않는다. 전체1~8차 목표는 진행 중이다. 원시는 `extracted/cpp-startup-archives-736/`다.
