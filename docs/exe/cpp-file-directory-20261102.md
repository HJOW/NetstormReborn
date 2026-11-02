# cpppj 단위734 — 디렉터리 getter와 무인자 경로 고리 인계

2026-11-02 / HJOW-Athlon / Windows 10. 마지막 전체 정밀 디컴파일은 HJOW-Athlon·2026-10-23이며 새 디컴파일은 없다. 사용자 AGENTS.md의 5차 전체 자동 생성물/reader·일반 권한 설치와 Windows10/11 AMD64·Windows11 ARM 고려는733에서 반영한 기준을 유지한다. 5차 이관 구현과 ARM 검증은 아직 미착수다.

## 원문과 실제 연결

`RawFileName::Directory`는 **459990 전체**의 고리 선증가/256BYTE clear·UNC 서버 건너뛰기·직전 dot 하나 이동·전체(-1)/단계별 경로·고정 역슬래시 연결·첫NUL 뒤 부분 버퍼를 복원한다. 전체 경로 모드의 역방향 탐색은 offset0을 검사하지 않는다. 따라서 `d\file`이나 `C:\Netstorm.exe`의 결과를 일반적인 parent_path로 바꾸지 않는다. 구분자 없는 원문의 NULL 읽기는 호스트에서 현재 칸을 보존한 invalid_argument로 거부하며, 원문 C++ 예외였다고 해석하지 않는다. 256BYTE 밖의 미정 쓰기도 기존 작성기와 같이 length_error다.

`ModuleDirectory`는 실제435cd0 무인자 분기의 **FileSpec→Directory→Device→Format**을 같은 고리의 세 칸에 연결하고 전역 목적지처럼 즉시 소유 복사한다. 실제 무인자 main은 실행 위치 탐색 전 공유 소유자를 만들고 PortableGameDirectory→ClientOptions→BaseFileSystem→설정/ClientStaticStrings로 넘긴다. 명시 `--run`은 기존처럼 새 고리에서 시작한다. BaseFileSystem의 기존 호출자도 외부 소유자를 넘기지 않으면 독립 BSS0을 갖는다.

원본과 다른 Windows10/11 휴대 배포 정책은 유지한다. 자기 EXE 옆의 game/netstorm.tarc를 우선하고 옆 자료를 다음으로 찾으며 다른 CWD의 자료를 고르지 않는다. 드라이브 최상위 EXE를 디렉터리로 오인하는 원문 결과는 고리 세 칸에 그대로 남기되 OS의 실제 부모 경로를 선택한다. UTF-8 경로가256BYTE 이상이면 이 모듈 경로 계산에서 고정 고리를 우회하고 OS 부모를 보존한다. 이는 경로 **선택** 검사이며 다른 VFS 소비자까지 무제한 긴 경로를 지원한다는 근거는 아니다. 무인자 경로는 native filesystem::path로 전달한다. 기존 manifest의 UTF-8 activeCodePage는 이미 있었으며 이전판의 Unicode 실행이 깨졌다고 주장하지 않는다. 호환 차이와 이유는 코드에 한국어 주석을 남겼다.

## 원문 관측 범위

[도구](../../tools/decomp_file_directory.py)는 실제459990/CRT와435cd0/FileSpec/장치/작성기를 CPU로 실행한다. 부모의 GetModuleHandleA/GetModuleFileNameA 두 OS 경계만 호출 인자를 검사하는 명시 응답이다. 16입력×두 separator×레벨 -2/-1/0/1/2/7의 **192행** 중 정상162행과 원문 읽기 실패30행을 구별했다. 각각 두 FPCW에서 관측했다. 모듈 경로는 무인자5행(역슬래시/슬래시/UNC/드라이브 루트/dot 디렉터리)과 명시 인자1행이다. 무인자 소모는3칸, 명시 인자는 복사만 하며0칸이다. 총 **396관측**이며 최초 fixture를 덮지 않는 재관측도 일치했다.

각 정상 반환은 EIP/ESP·비영 보존 레지스터·FPCW·입력 불변·고리 번호/전체 바이트를 확인했다. 부모의 SEH 전역도 복구됐다. 새 fixture는 디렉터리 현재256BYTE와 부모10×256BYTE를 보관한다. 모듈 Win32 실기 자체나 전체Startup을 이 관측에 포함하지 않는다.10.78 기준이며10.72 대응 구간을 새로 관측하지 않았다.

## 검증과 후속

Release 빌드 경고/오류0·새5개와 기존13개 **집중18개/실패0**이다. 원문 디렉터리192행·무인자 부모5행·이전 작성기352행/임시 문자열204행/Startup24행의 **777행**을 비교하고 실제 Unicode 패키지·다른CWD·인접 자료·누락 자료·긴 Unicode 선택·실제 Client의 공유 고리/설정/VFS와 기존 Startup/저장/재적재를 확인했다. 실행 파일 설정CLI는1024·종료0이며 게임 프레임0이다. EXE SHA-256은 `65352cd6dfe04a8072d3167e49cae901f26e0a2f85b024d4570c4da3a964eb00`다. [통합 보고서](../../cpppj/file-directory-20261102.json)를 따른다.

보호 기준2647개(AGENTS1·원본1394·CD425·기존fixture827)는 모두 불변이다. 새fixture2개를 더한 현재 보호 대상은2649개/fixture829개다. AGENTS는 읽기만 했으며 변경하지 않았다.

전체34/전체 C++/새 ZIP·새 GUI·포획/희생·특정 적 공격/승리·공중3종 전용·새 MP/통신 검사는 추가하지 않는다. 현재 작업은 Startup441790의 선행6후보 탐색/CWD/TARC등록 전체를 마쳤다는 뜻이 아니다. Full getter·FileSpec FindNext/VFS 검색 수명·Template·일반 FILE/TAFF/보조 실패와 UI/호환성 잔여를 이어간다. 전체1~8차 목표는 진행 중이다. 원시는 `extracted/cpp-file-directory-734/`다.
