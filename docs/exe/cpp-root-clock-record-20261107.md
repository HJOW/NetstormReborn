# 1073 — R FullTime과 실제 패킷 기록

`root::Clock`/`ClockState`/`PacketRecord`가 R의 현재 시계·중첩 정지/프레임 통계와 실제 사용자 홈 기록 장치를 연결한다. 제품 기준1073 / 11.01, EXE 12,366,336바이트, SHA-256 `84cfbabce3d2cce2caeb5994f6485da1bf19146e073e18e50e8b1ac6649f295f`다. 직전 CP는 `ddf2ff64 / 20261107-124 CP`다. **새R.exe 최종 owner/시계 최초 초기화·Tick/실제 기록 호출은 아직 없다.** 컴파일·정적 대조를 서버/시간·파일 실행 검증으로 계산하지 않는다.

## 실제 R의 시각과 현재 프레임

004737d0은 `baseSeconds*1000 + (timeGetTime-origin[-synchronizationMilliseconds])`를 **uint32 모듈러 연산**한 뒤 몫/나머지를 사용한다. 나머지/1000+몫의 double 변환 순서를 유지하며 Client GameClock의 별도 식으로 바꾸지 않는다.004739b0의 새 wall과00473880의 게임 시각을 구분한다. 게임 시각은 실제 보정DWORD/누적 정지·저장 게임 시각을 사용하고 보정과 누적 정지가 함께 비영이면 원문 fulltime.cpp77 진단을 요청한다.

00473a60/ac0은 최초 정지만 캡처하고 마지막 정지가 풀릴 때만 누적 정지 시간을 갱신한다.473a30의 signed 양수 판정과473b30의 누적값만0도 구분한다.00473b80은 이전 game→새game→별도 새wall→firstGame0일 때 쓰기→delta→10개 history 최초 초기화/현재 wall 저장→instantaneous/averageFPS→frameDWORD 증가다. average는 실제9간격 history에 numerator10을 사용하는 원문 그대로다. FPS의 비양수 구간은-1이며 averageFrameSeconds는1/averageFPS다. signed index가 음수인 원문 불법 배열 접근은 명시 물리 오류다.

00473e40의 실제 Windows 초기화도 작성했다. HIGH_PRIORITY_CLASS로 올린 뒤 UTC 초 경계 변경을 기다리고 timeGetTime 원점과 time32-0x32c85780 기준을 저장하고 Tick한 뒤 기존 우선순위를 복원한다. 호스트 예외에서도 우선순위 대여를 반납한다. **이 함수는 기존 pause/history/frame을 초기화하지 않는다.** PE004d1af8의96BYTE 로더 초기0과0/1000/10/1/-1 상수,FullTime 기본 생성자의0을 대조했다. 원본 x87 control word/모든 중간 반올림·구형 CRT/실제 OS 시간 실패 전체 동일성은 증명하지 않는다.

Clock.Bind는 현재wall의 ftol64 하위DWORD인 now와 별도 새wall 캡처 captureNow를 같은 전송에 연결한다. 원문00483870의 받은 패킷 timestamp는004d1b28 wall이다. 기존 새노드의 두 별도 지역 캡처를 프레임 현재값으로 합치지 않는다. invalid ftol64의 signed64 indefinite도 하위DWORD0으로 옮긴다.

## 현재 시계와 같은 실제 기록 파일

00482720/826d0의 먼저 닫기→enabled1→w+b/닫기→enabled0 순서를 복원했다. 실제 wide FILE을 사용하며 요청 파일명은 기존 UserDataRoot의 **Saved Games/Netstorm/Diagnostics/** 아래에 저장한다. 기존 절대/상대 설치 경로로 쓰는 fallback은 없다. 열기 실패 뒤enabled1/NULL FILE은 원문대로 구별하고, 그 상태의 실제 쓰기는 명시 물리 오류다.

00482770은 방향1BYTE+현재wall double8+현재frameDWORD4+현재IPv4DWORD4+현재senderDWORD4의 **21BYTE 접두** 뒤 실제 패킷을 쓴다. 개별6번 fwrite 뒤 fflush이며 반환값으로 이미 쓴 내용을 되돌리지 않는다. 본문 길이는 접두를 쓴 뒤 현재 packet 헤더에서 다시 읽는다. 송신 기계어48515a/15e/165/171/172→174에서 length/packet/sender/IPv4/direction의5인자를 확인했다. **writer는 전달된5번째 length를 사용하지 않는다.** 기존 transport hook의 길이 값을 그대로 믿고 본문을 쓰지 않는다.

PacketRecord.Bind는 실제 Configuration.openRecord와 Transceiver.record를 같은 FILE/ClockState에 연결한다. 기록과 연결의 필드를 임시 독립 시계/주소로 대체하지 않는다. 최종 R owner가 이 상태와 callback 수명을 유지하고 실제 초기화/프레임/전송을 호출해야 한다.

## 확인과 다음 작업

Release/x64 제품 빌드 종료0·경고/오류0, 보호2800파일 불변·새fixture0이다. **새 단위/MP/게임/통신·소켓/DNS·Clock Initialize/Tick·우선순위 변경·새 런타임 기록 읽기/쓰기·원본R 실행 검사는0**이며 전체 검사/34미션/ZIP도 반복하지 않았다. 다음은 보조 객체·나머지 Root/CS 수신/입력·실제 CCh주기와 메인루프/리셋·종료,새R.exe 전체 초기화/최종 owner·주소/시계/기록/파일 watcher 실제 연결,다른 LAN hostname 해석/0F·1d 수신·메타/OS launch다. 전체 목표 미완료다.

원문/PE·기계어·정적 감사/빌드 로그는 Git 제외 `extracted/cpp-resume-1073/`, 추적 보고서는 `cpppj/root-clock-record-20261107.json`이다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**, R 전용은 **2026-11-07 / HJOW-Athlon**이며 같은 호스트 기존 산출물만 읽었다. 재디컴파일/기존 산출물 변경0이다.
