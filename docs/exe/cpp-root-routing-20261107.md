# 1070 — Root 내부 라우팅과 원격 참가자 요약

`root::Routing`이 원문00474b00/00474ce0/00476070/00456f80을 같은 현재 연결/패킷 등록표/CCh 저장소에 연결한다. 제품 기준은1070 / 11.01, 게임 EXE 12,366,336바이트, SHA-256 `8a1274af6f685cf985019160b446f65740f9e0cac31f61a354040219e8dc36b6`다. 직전 CP는 `5d41eca5 / 20261107-113 CP`다. **새 R.exe의 최종 owner·Install/Relay 실제 호출, 상위 공유 표/파일·주소 장치와 나머지 수신은 아직 남는다.** 이 단위는 컴파일/정적 대조이며 실제 서버/접속/플레이 동작 검증이 아니다.

## 같은 수신기의 NULL 직접 호출

00474b00은 Chalmode4 진단247 뒤 비Root면 현재 일반1번에 보내고, Rootmode2면 현재 type1..137/등록된 실제 receiver를 찾아 `(sender0, connectionNULL, packet)`로 바로 호출한다. 첫 인자 challenge는 원문처럼 사용하지 않는다. 기계어00474b6d/6e/70/72의 push packet/0/0/call로 확인했다. 일반 프레임 분배를 호출하거나 가짜 PacketConnection을 만들지 않는다.

`RawPacketDispatch::RegisterDirect`가 기존 같은 PacketRegistration 슬롯의 일반 수신과 nullable 수신을 함께 등록한다. 두 입구는 shared callable 하나를 사용하므로 mutable 상태가 갈라지지 않는다. 모든 함수/공유 할당을 먼저 준비하고 Touch 뒤 noexcept 이동으로 교체한다. 기존 Register도 이전 direct 수신기를 제거하며 Session 대여 snapshot/복원은 같은 전체 등록 구조를 사용한다. InvokeDirect는 owner pin/호출 사본을 유지하고 프레임 검증·observer·기록·시각·연결 깊이/닫힘을 추가하지 않는다. 일반 수신기만 등록됐는데 NULL이 필요하면 호스트 물리 오류를 명시한다.

Root2h 종료·16h 목록 요청·17h 요약·1Ch 로그인 요청을 같은 nullable 형태로 등록했다.1Ch는 원문 실제 연결 읽기가 필요하므로 NULL을 명시적으로 거부한다. 폐기 외부 클라이언트 검증7f/80h는 이 내부 경로에서도 제외하며 별도 외부 통신을 추가하지 않는다.

## Root17h와 참가자 생성자

00476070은 현재 종료 플래그면 실제 exit0, 음수 sender면 level9 로그 후 반환한다. 그 외에는 packet+5 signed존/현재 존 수·실제 CCh token을 검사한다. CCh flags1이 꺼졌을 때만 원격 요약을 반영한 뒤 Rootmode2면00474ce0을 호출한다. **00474ce0은 Root모드 진단358 후 반환하는 몸체**이므로 임의 전체 방송/재전송을 추가하지 않았다.

00456f80의 퇴장은 활성 여부와 무관하게0..39 첫 subscriber 일치의188BYTE를 지운다. 가입은 중복 관문 없이1..39 첫 비활성 슬롯에 현재 process 기본 identity6/core10/position8, NULL이름, flags0을 넣고 levelBYTE를 복사한다. 소켓 생성/참가자 종료/전투 삭제 경로로 바꾸지 않았다.

`InitializeParticipant`로0044c7b0을 공유해 기존 로컬 로그인과 원격 요약이 같은 지정 폭/문자 제거·CRT시계/+70·74/identity·core/flags mask·위치·소속0을 쓴다. NULL이름은 +26 선두NUL만 쓰고 tail/기타 미작성 필드를 보존한다. 원격 기본값은 별도 mutable process 바이트이며 로컬 grid로 대체하지 않는다. PE 로더 초기0과 실제 CRT 초기화004ba0c0/004ba570/004ba7b0도 대조했다. 모든 CRT clock 환경과 원본 미작성 메모리/불법 접근 자체의 동일성을 주장하지 않는다.

## 확인 범위와 재개

첫/relay 보완 후 Release/x64 빌드2회 종료0을 직접 관측했다. 마지막 등록 교체 보완 빌드는 최종 EXE 링크 로그·소스보다 최신 산출물·관련 빌드 프로세스 없음을 확인했다. 마지막 도구 응답이 유실돼 그 종료코드는 직접 관측하지 못했으며 보고서에는 null로 남겼다. 세 로그의 경고/오류는0이다. 보호2800파일 불변/새fixture0이다. **새 단위·MP·게임·통신·소켓/DNS·원본R 실행·새 런타임 파일 쓰기 검사는0**이며 전체 검사/34미션/ZIP도 반복하지 않았다.

다음은 blacklist/marked/IP/filter·guild/guildreq/news 실제 공유 로더와 사용자 홈 저장, 주소 서식/해석·시계/기록/보조 객체·나머지 Root/CS 수신/입력, 새R.exe 전체 초기화/최종 owner와 실제 호출이다. 이어 클라이언트0F 존47/서버3BYTE 차이·0F/1d 수신·메타/OS launch를 연결한다. 빈 R.exe/기존 보호 R 실행으로 완성 처리하지 않는다. 원문/기계어·정적 감사/빌드 로그는 Git 제외 `extracted/cpp-resume-1070/`, 추적 보고서는 `cpppj/root-routing-20261107.json`이다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**, R 전용은 **2026-11-07 / HJOW-Athlon**이며 재디컴파일/기존 산출물 변경은0이다.
