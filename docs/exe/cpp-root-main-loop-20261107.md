# 1075 — R 메인 루프·파일 감시와 예약 리셋

`root::MainLoop`가00479760/00478fa0/00479340의 실제 반복/감시·예약 처리를 같은 시계/CCh·전송/파일/라우팅에 연결한다. Root00474bd0/00474d30의 현재 CCh 전달과41h 안내 작성기도 추가했다. 제품 기준1075 / 11.01, EXE 12,366,336바이트, SHA-256 `0bafe9f5a074902d7515db64401a5bb9c55f9a2b99b8c9906874add0c3388f8e`다. 직전 CP는 `7087ed4d / 20261107-127 CP`다. **전체 초기화·정리/서버 목록·보조 객체/41h 수신과 새R.exe 최종 owner/실제 호출은 아직 없다.** 컴파일·정적 대조를 실제 서버/주기·안내 전달 실행 검증으로 계산하지 않는다.

## 메인 cycle과 같은 현재 frame

Run은 LAN 인자 때 현재 localLaunch를1로 쓰고 실제 Windows 콘솔 제목/40ms 대기/최소화·스레드 우선순위/NETSTORMOOGABOOGA의500 통지를 수행하는 몸체다. **500 통지는 원문처럼 초기화 이전**이며 준비 성공을 합성하지 않는다. 표시 banner는 사용자 요구11.01로 쓴다. 최초initialize(true)→현재 모듈 폴더 진단→reset/stop이0인 동안 Step→정상shutdown→reset만0→stop이0이면initialize(false)의 순서다. 상위 실제 초기화/정리 장치가 비었으면 명시 오류이며 빈 함수로 성공 처리하지 않는다. 모듈 진단은 wide/UTF-8의 실제 긴 경로를 지원하고 CWD는 바꾸지 않는다.

Step은 실제 Clock.Tick→현재51개 CCh→같은 NetworkPump→파일 감시→예약 처리다. 대기값은60에서 시작한다. 각 CCh의 token snapshot을 한 번 읽고 **wait가 첫Tick 결과보다 작지 않으면 같은 CCh를 다시 Tick**한다. 원문 최소값 식의 중복 호출을 제거하지 않는다. 네트워크는 같은 ChallengeStore의 현재40표/수신·송신과 실제 TransceiverTransport를 쓰며 보조 객체의 실제 집합/Poll만 상위가 공급한다. reset/stop은 다음 frame 경계에서 다시 확인한다.

## 파일 감시의 분석 정정

00478fa0은 process 최초 flag1 때 마지막 시각을0으로 구성한다. 현재game-last가 **엄격히60초 초과**면 마지막 시각을 먼저 갱신하고 뉴스→서버 목록→blacklist 순서로 호출한다. 중간에 예외가 나도 앞선 시각/뉴스 반납을 되돌리지 않는다.

**중간 함수00471f20은 Config 설정 재로드가 아니라 rootservers.dat 목록 재로드다.**00471ec0이 파일을 찾고00471710이 현재 목록을 비운 뒤 읽는다. 이전 다음 작업의 '설정 갱신' 표현을 이 원문 근거로 정정한다. 현재 MainLoop는 실제 reloadRootServers 장치를 요구하며 목록/체크섬 본체는 아직 후속이다. 기존 SharedFiles의 실제 뉴스·blacklist mtime 장치를 사용하고 Config의 onChanged 재해석과 혼동하지 않는다.

## 예약 도달과 실제 Root 안내

scheduledAt0은 비활성이다. 예약시각<=현재game이면 예약을0으로 만든 뒤 goingDown에 따라 stop 또는 reset DWORD를 원문처럼 증가시킨다. 도달 전에는 남은 시간이300초 이하면 마지막 안내 후60초 초과,그 외에는300초 초과일 때 안내한다. remaining/60의 ftol64 하위DWORD+1 분을 `~r~3~EServer is resetting/going down in %d minutes.` 형식으로 작성한다.기계어47943e/47951d의 fdiv60/ftol을 대조했다. Root 전송→level29로그→현재game으로 마지막 안내 갱신 순서다.

Root 안내는 ff allocator의41h/초기헤더4·version3/byte4 미작성ff,byte5=1/sourceDWORD0/객체1BYTE0/수신subscriber0을 사용한다. 명시 길이 본문은 NUL 뒤도 복사하고 Text 경로는 첫NUL+종료NUL이다. 현재 zone1..50에 같은 packet을 보낸다.00474bd0처럼 Root 진단321→현재 zone/현재 token/flags1→descriptor=-1 진단325→현재 실제 receiver `(zone<<8,NULL,packet)` 호출이다. 프레임 검증/observer를 임의 추가하거나 가짜 연결을 만들지 않는다. 폐기7f/80도 제외한다. **41h 수신기가 아직 없으므로 실제 CCh/Client 안내 전달 완료를 주장하지 않는다.**

## 확인과 다음 작업

첫/직접 include 보완 뒤 Release/x64 빌드2회 모두 종료0·경고/오류0,보호2800파일 불변·새fixture0이다. **새 단위/MP/게임/통신·소켓/DNS·Run/Step/예약·LAN창 통지·새 시계/우선순위·파일 실행/원본R 검사는0**이며 전체 검사/34미션/ZIP도 반복하지 않았다. 실제 Windows창/실패/SEH·불법본문/포인터·모든 x87 중간 정밀도 동일성을 검증한 것은 아니다.

다음은 rootservers.dat 검색·목록/체크섬과 실제 보조 객체·나머지 Root/CS 수신/41h/전체 초기화·정리,새R.exe 최종 owner·entry와 같은 현재 상태/장치의 실제 Run 호출이다. 이어 다른 LAN hostname/0F존47·3BYTE/0F·1d 수신·메타/OS launch를 연결한다. 전체 목표 미완료다. 원문/PE·전체 함수 기계어·정적 감사/빌드 로그는 Git 제외 `extracted/cpp-resume-1075/`, 추적 보고서는 `cpppj/root-main-loop-20261107.json`이다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**, R 전용은 **2026-11-07 / HJOW-Athlon**이며 같은 호스트 기존 산출물만 읽었다. 재디컴파일/기존 산출물 변경0이다.
