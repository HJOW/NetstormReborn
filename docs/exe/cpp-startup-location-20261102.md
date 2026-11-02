# cpppj 단위735 — 여섯 setup 경로 탐색과 실제 파일 루트

2026-11-02 / HJOW-Athlon / Windows 10. 마지막 전체 정밀 디컴파일은 HJOW-Athlon·2026-10-23이며 새 디컴파일은 없다. 실행 제품은 **735 / 표시11.01**이다. 5차 전체 자동 파일/reader·일반 권한/읽기 전용 설치와 Windows10/11 AMD64·Windows11 ARM 고려 요구는733의 현행 기준을 유지한다. 5차 구현과 ARM 검증은 미착수다.

## 실제 변경

이전 실제 시작 코드는 `d\`를 고정했다. 원문441790처럼 실행 위치에서 **`.\`, `.\d\`, `..\d\`, `..\..\d\`, `..\`, `..\..\`** 순서로 setup.cfg를 찾도록 연결했다. 각 후보는459c60의 지역 복사→459520 정규화→실제41ab10 Probe이며 첫 성공 뒤 다른 후보를 보지 않는다. 성공한 원래 고리 뷰로 FileSpec→Directory(-1)→Device→Format을 수행하고 dataPrefix를 소유 복사한다. 전부 실패하면 원문195행 assertion을 호스트 logic_error로 보고한다. 원문 C++ throw였다고 해석하지 않는다.

실제10.78 Client::Run은 같은 파일 시스템과 경로 소유자로 이 탐색을 실행한 뒤 CD 검색→dataPrefix/.. 경로 작성→기본/보조 아카이브 등록→설정 Load를 진행한다. dataPrefix·InstallDir·CDDir과 이후 자산/저장/편집의 기본 루트를 같은 선택 결과에 연결했다. VFS 객체의 주소와 공유 경로 소유자는 보존하면서 생성자에서 미리 읽은 옛 목록을 새 최초 Startup에 넘기지 않는다. 기본과 CD의 두 목록은 기존 Win32 열거 순서로 각각 등록한다. 기존 ConfigInterface reader가 참조한 VFS도 갱신 후의 루트를 사용한다.

원본과 다른 Windows10/11 호환 처리는 한국어 주석으로 명시했다. 상대 `--run`은 호출자 CWD를 기준으로 절대 경로를 한 번 정해 기본 루트를 중복해서 붙이지 않는다. 원문의 프로세스 전체 chdir/getcwd 대신 Unicode 경로로 같은 부모 자료 루트를 선택하고 **프로세스 CWD는 유지**한다. 10.72는 이번10.78 근거로 확장하지 않고 기존 시작 경로를 보존했다. 분석 CLI의 독립 StartConfiguration은 기존 명시 자료 루트 계약을 유지한다.

## 원문 근거

[관측 도구](../../tools/decomp_startup_location.py)는 **실제441790~4418bd(CD검색 직전), 실제Probe/정규화/FileSpec/Directory/Device/Format/CRT**를 CPU로 실행했다. fopen/fclose·두 빈 TAFF 목록·없는 보조 디스크만 명시 경계다. 빈/점/상대 Game/드라이브 경로/끝 구분자/UNC의6루트×첫 성공0~5 또는 전부 실패×초기번호0/9의 **84행**을 두 FPCW로 관측한 **168회**와 재관측이 일치했다. 정상72행과 원문 assertion12행을 구별했다. 각 방문 인자·dataPrefix·현재 번호·전체10×256BYTE를 고정했고 입력 루트/FPCW 불변을 확인했다.

부분 부모의 관측이므로 전체 함수 반환 ABI, CD/CWD 장치, TARC 열거·등록, 뒤 설정 Load의 새 Native 전체 실행으로 세지 않는다. 실제 디스크를 쓰는 호스트 검사는 별도로 여섯 후보 각각의 첫 성공과 미방문을 확인했다. [Native 보고서](../../cpppj/startup-location-native-20261102.json)를 따른다.

## 검사와 실제 실행

새4개·기존13개 **집중17개/실패0**이다. 독립 원문은 새84행과 이전 디렉터리192/무인자 부모5/Startup24/등록27의 **332행**이다. 실제 Unicode 설치에서는 자식 launcher의 옛 TAFF를 버리고 부모 자료와 별도 CD의 두 목록을 등록하며, 기존 reader의 설정 Load·InstallDir/CDDir·같은 고리 소유권/CWD 불변을 확인했다. setup 전체 누락은 CD 검색 전에 중단하고 기존 VFS를 보존한다.

첫 집중 실행은 새 통합 입력의 **1검사/2단언 실패**였다. 설정 파일 서명과 같은 줄에 SCREENW를 넣어 원문 규칙상 죽은 키가 됐고, 경로 동일성은 이미 통과했지만 문자열 기대값에는 혼합 구분자를 썼다. 입력을 서명 다음 줄로 옮기고 선택한 정규화 문자열이 그대로 설정에 기록되는지 비교했다. 제품 소스는 이 교정에서 바꾸지 않았으며 최초 로그를 보존했다. Release 최초/검사 재빌드 모두 경고/오류0이다.

[짧은 실행 도구](../../tools/cpp_startup_location_smoke.py)로 보호 originals의 한글😀 작업 사본을 만들고, setup/TAFF가 없는 하위 launcher735를 `--run` 위치로 지정했다. 시작 리소스용 원본 EXE만 하위 폴더에 복사했다. 같은735제품 **1프로세스·30프레임 제한·1.956초·종료0/timeout없음**으로 메인 메뉴를 열었다. 25프레임의 실제 app presented bitmap을 검토했다. 부모 d/setup.cfg·TAFF를 사용하고 저장한 두 디렉터리가 부모 자료 폴더와 실제 같은 경로임을 독립 XOR/BOM 해독으로 확인했다. 하위 폴더에는 options를 만들지 않았다. 물리 화면·입력·PCM/포커스나 시나리오 진입/완주 검사는 아니다.

EXE SHA-256은 `202e58e48e871009a21a734610f793ac616d780be0a8c45938c3974597abaf4c`다. 보호2649개(AGENTS1·원본1394·CD425·기존fixture829)는 모두 불변이며 새fixture1개를 더한 현재 대상은2650개/fixture830개다. GUI 전후에는 새fixture까지 포함해 비교했다. [통합 보고서](../../cpppj/startup-location-20261102.json), 원시는 `extracted/cpp-startup-location-735/`다.

## 남은 범위

Full getter·FileSpec FindNext/TAFF wildcard의 같은 고리 소모/검색 수명은 아직 연결하지 않았다. 생성자의 선행 TAFF 읽기 자체도 남아 전체 최초 I/O 순서 복원으로 세지 않는다. 가상 부모 선택의 실제 성공은 확인했지만 chdir/getcwd 실패 종류별 Native/호스트 대조·일반 FILE/TAFF/보조 실패·Template·기타 UI/호환성은 후속이다. 기존10.72 경로·분석 CLI의 고정 d 정책도 이번10.78 복원 범위 밖이다. 전체34/전체 C++/새 ZIP·포획/희생·특정 적 공격/승리·공중3종 전용·MP/통신 검사는 추가하지 않았다. 전체1~8차 목표는 진행 중이다.
