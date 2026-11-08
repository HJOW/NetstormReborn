# 1163 — M5.1 레지스트리와 LAN hostname 환경 계약

2026-11-08 / HJOW-Athlon. 이전1162/2a309a87, 표시·파일·제품11.01이다.

실제 Client와 R의 ConfigRegistry.registryValue에 공용 legacy Windows 조회를 연결했다. 원문은 RegOpenKeyA 후 RegQueryValueA(80BYTE)로 열린 키 아래 `#` 뒤 하위키의 기본값을 읽는다. named value용 RegQueryValueExA로 바꾸지 않았다. 여섯 HKEY의 prefix/순서·첫 구분자·1024BYTE 경로·잘못된 root의 진단 후 NULL HKEY 조회를 유지한다. 내부UTF-8/실제ACP는 무손실 변환하고 원문의 키 반납 누락은 RAII로 보완했다. 원문 밖 버퍼/NUL/손실 바이트는 명시 오류다.

클라이언트004dda10/CD004bebe0/R00481600의 C·PE를 대조했다. 10.78/CD의 실제 진단 파일/줄 차이도 owner에 연결했다. 현재 Client의 초기 Config는 SurfaceStorage 이전이라 기존clog 복귀 진단이며 이후 현재AssertDisplay다. 이 초기 경계도 M7.5의 중앙 진단 장치 복원에 포함한다. R은 실제 trace 구성 뒤 Load하는 같은 owner다. x64 프로세스의 기본registry view를 사용하며 원문32bitview를 강제하지 않는다.

LAN 이름은 원문Client004e0cd0/R00486870처럼 실제gethostbyname의 첫IPv4만 선택한다. 뒤의 사설주소를 찾아 순서를 바꾸지 않는다. 첫결과가loopback/RFC1918/link-local일 때만 접속한다. 이름조회는 자기WSAStartup/cleanup참조를 균형 반납하고 생성자에서는 수행하지 않는다. Client의 기존 연결 조기재사용→이름조회 순서·기존숫자parser·실패false/lastError와 공인IP차단을 유지한다. R의inet_addr/한점IPX문법·조회실패IP0·같은port/flags쓰기와 자기어댑터조회는 유지한다. 이름은 원문의char바이트이며 별도IDN확장은 없다.

명시hostname의 OS조회는 설정된DNS/hosts/LLMNR 등에 질의할 수 있다. DNS를 전혀 사용하지 않는다는 과거 현재값은 이번 단위로 대체했다. 실제TCP접속의 LAN범위 제한과 폐기7f/80·클라이언트검증 비활성은 유지한다. 외부접속 정책 문서/JSON에도 이 차이를 기록했다.

두 Release/x64 빌드 종료0·경고/오류0이며 양쪽PE의 RegOpenKeyA/RegQueryValueA/RegCloseKey/gethostbyname import를 확인했다. 보호2800파일 불변이다. 새C++단위/레지스트리조회/DNS/MP패킷·게임·통신/ZIP실행0이다. 컴파일/정적접합을 실제LAN동작검증으로 쓰지 않는다. 마지막 클라이언트정밀2026-11-03/HJOW-Athlon·R2026-11-07/같은호스트, 생성원문변경0이다.

M5.1의 두환경계약을 마쳤고 다음M5.2의 Admin.server/Update.server/Guild.server 콘텐츠/실제reader·배포는 미완료다. 원본문이 제공되지 않아 같은본문 확보로 주장하지 않고 원문section/명령/tag호환 콘텐츠를 준비 중이다. 원시근거 extracted/cpp-resume-1163/(Git제외), 보고서는 cpppj/lan-environment-contracts-20261108.json이다.

Netstorm.exe 13,179,904바이트/SHA-256 `b6b918135e07a77aca52bc11f4495cb6a88e6191525332a0c5cbd2760957542f` / R.exe 675,840바이트/SHA-256 `bf4e46769bd9ff11b11d745355f44140971f2c26299d633c4dc8e1449887e14d`
