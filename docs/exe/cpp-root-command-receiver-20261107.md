# 1086 — R87h 전체 명령 수신

87h의15상위명령/관리자6세부분기와 실제 길드·파일·CCh/전송을 연결하고 기존 등록 부모에 추가했다. 제품1086/11.01, EXE 12,366,336바이트, SHA-256 `d2ebb6a21d6785863cc666668ca468ed79cdfbe6c2da3692a835e9472019a725`다. 직전 CP는 `46ca7472 / 20261107-152 CP`다. **원문18수신중 폐기80을 제외한17개 몸체의 작성/컴파일은 마쳤다. 새R.exe 최종owner/실제 Bind·Install·Run 호출과 LAN 동작 검증은 아직 없다.** 전체 서버 완성이 아니다.

## 실제 수신과 입력

InstallPacketObservers가87h의 빈443e40 observer/버전1·정책0·고정274와 nullable 현재 수신을 등록한다. 기존 ReceiveBattleControl의 exit0→CHAL진단1365→signed zone/현재 CCh/lowBYTE 참가자→Command다. 원문은 `00452f50-004550a7;00455e19-00455e2b` 합8555BYTE이며 모든 실제 범위를 대조했다.

먼저 템플릿3파일을 현재 장치에서 읽고 operation+22h/first+72h/second+c2h/section+4 순서로 네 C문자열을 복사한다. 입력 자체는 바꾸지 않는다. 논리30/80필드 안의 NUL을 새로 강제하지 않으며 실제 패킷 물리 경계만 검사한다.15개 상위명령과 원문 철자는 JSON의 commands, 관리자6개는 adminCommands를 따른다.

## 페이지와 길드

GET은 section, GUILD는 first 필드의 페이지다. 알 수 없는 상위명령은 응답 없이 반환한다. 대부분 빈페이지 송신을 생략하지만 ADMIN GETPLAYER는 빈페이지도 보낸다. GUILDSUBMIT/GUILDSETPASS는 원문처럼 공통TIME/존 치환을 호출하지 않는다. 기존 CommandPages와 실제 확장 CCh 메시지를 공유한다.

길드13명령은 같은 SharedFiles의 현재모집/비밀번호/생성·탈퇴/해산/로그인/가입요청·승인과 실제 저장을 사용한다. GUILDREQAPROVE는 현재소속의 길드 이름/second를atoi한 구독자의 첫 요청을 승인하고 응답하지 않는다. GUILDSETPENDING 상태0/1/2/3/기타는 Error3/Error1/Error4/Error2/Ok다. GUILDNEW 상태2/3/기타는 ErrorCode2/ErrorCode3/Code1이며 기타 상태는 그뒤 현재 subscriber의88h 이름 방송도 수행한다.

해산은 삭제 전에 길드/회원 사본을 만들고 삭제·저장 뒤 사본 회원마다 같은 CCh에 빈88h를 보낸다. 탈퇴는 회원제거→QuitComplete 조건부 송신→그뒤 현재 참가자의 subscriber를 다시 읽어 빈이름을 방송한다.88h는 고정98이며 첫 이름BYTE만0, 나머지 미작성 지역은 현재 CRT 입력으로 유지한다.

길드 요청 목록은 GuildSingleRequest 섹션/공통치환을 한 번 하고 같은 길드 요청만1부터 센다. 승인 여부 필터 없이 현재ID/PLAYERNAME/SUBID·요청범위를 치환하며 끝까지 읽는다. 모집 목록은 GuildView/모집1만 세고 GUILDNAME/MEMBERS/SUBID를 치환하며 범위뒤 첫초과에서 멈춘다.52e10/52ee0의 현재카운트와6개 페이지 이동·signed DWORD/tag 순서도 유지한다.

## 관리자 명단과 제재

현재 참가자+75의 비영만 권한으로 쓴다. 대문자X가 없으면 count0으로 반환하며, 있으면 빈항목0/첫두atoi를 사용하고 뒤X는 무시한다. GETPLAYER는 PlayerList/Player/NEXTPLAYER를 먼저 추출→기본/next 공통치환→현재시작부터6개 실제행/13tag·색→현재페이지/next메뉴 삽입 순서다. 원문 MAXPLAYERS50을 수정하지 않았다.

대상존은 현재51칸의 실제 표, 참가자는 DWORD곱/덧셈 뒤 전체CCh 실제물리를 조회한다. 개별 Read/Put/Text가 각 접근을 검사하며 다른 정상부모의40칸 논리API로 대체하거나 clamp하지 않는다. 물리밖 읽기는 명시 오류다.

SHUTUP은 실제 현재SID의 chatlist 삽입/첫삭제→대상+74쓰기→전체저장이다. ADMIN은 대상+75의 비영toggle만 한다. KICK은 실제 현재 하위소켓 닫힘뿐이다. FORTBAN은 현재SID 삽입/blacklist저장→하위소켓닫힘, IPBAN은 현재IP의 기존정규화/축소strlen비교·중복제외 삽입/전체저장→하위소켓닫힘이다. 노드 Terminate/즉시 참가자삭제를 추가하지 않았다. 현재 대상이 없거나 무효여도 원문 고정 Tell 새로고침을 응답한다.

chatlist/blacklist는 같은 SaveSubscribers의 signed DWORD/endl, 주소는 현재정규화16BYTE의 첫NUL 문자열/endl이다. 주소 writer에서 다시 정규화하지 않는다. 자동쓰기는 기존 WritableFile/사용자Saves/Unicode이고 reader도 Saves우선이다. 원본/설치 파일을 쓰지 않았다.

## 확인과 잔여

Release/x64 NetstormCpp 빌드 종료0·경고/오류0이다. 새단위/MP/게임/통신·명령/페이지/제재·파일왕복/패킷/원본R 실행검사0, 전체검사/34미션/ZIP 반복0이다. 보호2800파일 불변·새fixture0이다. 원문/보호PE·기계어와 현재 장치 접합만 정적으로 대조했다.

실제 네트워크실패/callback수명·파일오류/왕복, 구형 StringList의 delimiterCount 할당/힙넘침·SEH/스택·미작성물리 자체, 현대CRT의 범위밖atoi/locale 전체 일치는 검증하지 않았다. 사전구현을 동작/안정성 검증으로 계산하지 않는다.

다음은 Root 보조객체/집합·Poll, 전체전역패킷등록/477e10초기화·478af0정리, 새R.exe 최종owner/entry·실제UDP번호발행/Announce/Bind/Install/Run이다. 세 .server 콘텐츠/최종배포, LAN hostname/0F존47·3BYTE/0F·1d/메타/OSlaunch·LAN/Outpost/checking5·한국어후속/Edit시나리오UI도 남아 전체 목표는 미완료다.

원시/기계어·정적 감사/빌드 로그는 Git제외 `extracted/cpp-resume-1086/`, 추적 보고서는 `cpppj/root-command-receiver-20261107.json`이다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 같은 호스트 기존 산출물만 읽었다. 재디컴파일/생성소스변경0이다.
