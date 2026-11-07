# 1066 — R 시작 설정과 현재 옵션

**1069 정정:** 00477570이 호출하는0045cda0/0045da40은 각각 `guild.txt`/`guildreq.txt`를 읽는다. 아래의 두 채팅 표라는 표현은 길드/길드 요청 표로 정정한다. 실제 파일 로더 연결은 여전히 후속이다.

`root::Configuration`이 R004700b0/00470620→00477570→00477950의 시작·재해석 부모를 기존 ConfigInterface와 같은 R 전역에 연결한다. 제품 기준은1066 / 11.01, 게임 EXE 12,365,824바이트, SHA-256 `02e9c176b6906193a85430b7d48e412c54649aa328c38042d9448d3158d24b12`다. 직전 CP는 `c9b2eec8 / 20261107-100 CP`다. **새 R.exe/실제 프로세스 초기화 호출은 아직 없고**, moderation/filter·기록 장치와 CCh 관리자는 실제 caller가 제공해야 한다. 빈 callback으로 서버 초기화 성공을 합성하지 않는다.

## 탐색과 읽기 순서

R PE004cd760의6후보 `.\`, `.\d\`, `..\d\`, `..\..\d\`, `..\`, `..\..\`에서 첫 setup.cfg 성공을 선택한다. 같은 RawFileName의 고리와 FileSpec Directory/Device/Format을 쓰며 고리 재사용 전에 지역 값을 복사한다. 모두 실패해195 진단이 복귀하면 지역 기본 `d\`를 쓴다. 실제 cwd를 바꾸지 않고 호출자의 가상 root/파일 장치를 사용한다.

IDENTITY 뒤 USER를 선택하고 identity.cfg를 작성한다. options/user/dev/setup/demo/guild 순서로 경로를 작성하지만 demo는 읽지 않는다. NULL commandline에 대응하는 빈 인자→원문 내부10.71 버전 문자열→@→identity/user/options/dev/guild/setup을 **같은 ConfigInterface::Load**에 보낸다. R에는 클라이언트 Startup의 InstallDir/CDDir Set이 없으므로 그 함수 전체를 호출하지 않는다. 내부 wire 값과 향후 실행 파일의 제품 표시11.01을 구별한다.

변경 callback을 Load 직전에 등록하고 Load 뒤 현재 dataPrefix를 쓴다. 명시 Interpret는00477950의 위치에서 한 번 실행하며 Load 자체에 추가 callback을 만들지 않는다. Config Set 이후에는 현재 Interpret를 실행하고 최초 전용 포트/개수/모드 전체를 다시 읽지 않는다. 전용 Config/고리/현재 전역은 Configuration보다 오래 살아야 하며 소멸자는 자기 전용 Config 변경 등록을 비운다.

## 같은 현재 전역과 초기값

Options는 PE data의 trace29/throwAssert1/telnet6801/스팸2·3/최저major6/multline6/diag90/keepAliveFreq15/autoBan1/기록용LimitPirates1과 나머지 BSS0을 구별한다. 기본 TCP/UDP 포트와 타임아웃을 임의 수치로 채우지 않는다. 세 타임아웃과 LAN 빈 서버 종료 기한은 같은 NetworkState에, 설정 개수/최종 증가·모드 OR는 기존 ConnectionState에 쓴다. RootDirectoryGlobals는 현재 groupByLevel/maxPlayers/checksum 참조를 빌리므로 재해석 뒤 사본이 남지 않는다. checksum 자체는 옵션으로 읽지 않는다.

00477570 선두의 실제 blacklist/ipban/chatlist/두 채팅 표/filter 로드는 명시 reloadModeration 장치를 요구한다. 그 뒤 재해석26옵션을 원문 순서대로 읽는다. clientadmin의 지역 NULL 포인터에 비어 있지 않은 값이 할당되지 않으면 원문의 NULL strcpy를 호스트 예외로 막는다. 최초 부모는33옵션(기존 InterpretMode의 마지막3개 포함)을 같은 순서로 읽고 지정된 로그/기록 파일 열기 위치를 보존한다. 두 함수의 **옵션 키 순서는 정적 대조에서26/33 모두 일치**했다. 이는 새 MP 단위/프로토콜 실행 검사가 아니다.

ReadInt는 원문 유효성462 진단과 현재 키 조회/atoi DWORD 감김을 쓰고 변경될 때만 현재 값을 쓴다. 문자열판은473 진단/첫NUL·512BYTE 지역 경계를 사용한다. 할당 문자열은 빈 키 값이면 기존 NULL/값을 보존하고 `_stricmp`상 같은 문자열은 기존 대소문자도 보존하며 변경일 때만 소유 값을 교체한다. maxPlayers는 재해석 끝에0..39로 제한한다. 원문 포인터/배열 ABI와 clientadmin 지역 할당 누수 자체를 재현하지 않고 C++ 소유 문자열을 쓴다. 기록 이름은80BYTE 전역 범위를 검사한다.

`legacyLimitPirates`는 과거 옵션을 기록하는 값만 유지하며 폐기된 외부 등록/클라이언트 검증 소비자는 연결하지 않는다. 공개 서버/외부 접속도 추가하지 않았다. 실제 설정 우선 읽기/저장·기록 쓰기는 향후 R의 VFS/사용자 홈 장치에 연결해야 한다. 원본 설치 위치로 쓰는 fallback은 작성하지 않았다.

## 확인 범위와 후속

Release/x64 NetstormCpp 빌드는 종료0·경고/오류0이며 보호2800파일 불변·새fixture0이다. 새 단위/MP/게임/통신/소켓 열기/원본R 실행 검사는0이고 전체 검사/34미션/ZIP도 반복하지 않았다. 실제 Config 시작·파일 읽기/기록 열기·callback 재해석은 이번에 실행하지 않았다. 원문의512BYTE 초과/NULL strcpy 등 잘못된 물리 접근은 호스트 예외로 거부하며 불법 메모리/SEH 배치 자체 일치를 주장하지 않는다.

다음은 CCh 생성·관리/Root·CS나머지 수신·시계/기록/보조 객체와 새R.exe의 실제 초기화/호출이다. 이어0F 존47/3BYTE 차이와 클라이언트0F/1d 수신/메타·OS launch를 연결한다. 원시 함수/PE 기본값/옵션 순서·빌드 로그는 Git 제외 `extracted/cpp-resume-1066/`, 보고서는 `cpppj/root-configuration-20261107.json`이다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**, R 전용은 **2026-11-07 / HJOW-Athlon**이며 재디컴파일/기존 산출물 수정은0이다.
