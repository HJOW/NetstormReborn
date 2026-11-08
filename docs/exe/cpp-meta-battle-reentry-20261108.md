# 1156 — M2.5 인계 후 같은 전투 시작 수신의 재진입

2026-11-08 / HJOW-Athlon. 직전 제품1155/커밋dc2d85be. 표시·파일·제품 버전11.01.

M2.5의 인계 후1ah/1bh 재수신을 실제 Source·현재 World·같은 전체 복합 전송에 연결했다. M2를 마치고 M3으로 진행한다.

첫 State7이 `editor.metaTransport`를 비워도 같은 등록표의 1ah/1bh는 살아 있다. 이제 각 실제 수신이 전체 Source owner를 넘긴다. 1ah는 원문 수신 효과가 끝난 뒤 기존 next7에 약한 Source만 연결한다. 현재 World가 있어도 같은 외부 PollStarting을 한 번 선택하며, 소비한 약한 참조를 먼저 비워 재진입의 새 예약을 지우지 않는다. 새 상태 쓰기나 호스트 command를 추가하지 않는다. 1bh는 원래 위치의 동기 RunStarting에 Source를 전달하고 이전 World를 외곽 복귀까지 보유한다. current7/즉시8을 다음 프레임으로 미루지 않는다.

현재 World가 있으면 실제 복합 전송을 조회하고 TCP·UDP·두 표·전체 Meta owner가 Source와 동일한지 정리 전에 확인한다. 같은 전체 coordinator를 보유해 Native 정리가 남긴 내부 listener/양끝과 정상1..8·9·12를 잃지 않는다. Session 등록은 실제 Runtime 반납에서 먼저 detach한다. 같은 coordinator는 기존 TCP/UDP/내부 노드·버퍼·깊이를 유지하고 현재 World의 장치만 다시 연결한 뒤 새 Session을 대여한다. 필요한 client→host 내부 owner만 없을 때 늦게 만든다. Poll 도중 생긴 내부 owner는 첫 ID 수신 전에 외곽 대여를 시작하되 accept를 다시 실행하지 않는다.

현재 Meta 필드가 비어 있더라도 같은 현재 World 장치의 Source만 명시적으로 인정한다. 다른 World/Session 콜백 중 정리 금지와 반납 중 대여의 실행 관문은 그대로다. Meta의 1ah/1bh는 Session이 다시 등록하지 않은 전역 슬롯이므로 이 정상 재진입은 다른 Session 대여 실행을 허용하는 예외가 필요 없다. 불일치를 phase/known/sender 필터나 가짜 성공으로 가리지 않는다. 같은 두 수신 본체와 파일/observer 순서는1154/1155를 유지한다.

원문4b2df0/4b6dd0의 현재 장치 유지·정리 전 캡처·동기 State7/즉시8 순서, source pin/next7 단일 Poll 우선 선택, 실제 TCP/UDP/두 표/전체 Meta identity, 기존 내부 끝/버퍼/깊이 유지, World 등록 detach→같은 전송 hooks 재연결→새 World 대여, 다른 Session callback 보호/늦은 내부 Poll 대여를 정적으로 대조했다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0, 보호2800파일 불변이다. 새 C++ 단위/MP/게임/통신/전체/ZIP 실행0이며 정상/반복 접속의 동작 검증 결과가 아니다. 보조 C/ASM·빌드는 Git 제외 `extracted/cpp-resume-1156/`에 있다.

Netstorm.exe 13,169,664바이트/SHA-256 `76ff76230af03c4c189d9e7de5e5bd6adfaf3f6aaf1d3a27f6f55f12f2a0722d` / R.exe 670,208바이트/SHA-256 `4cfdfe3f46384090bbd9328959d3aebda4844b29556273bb0f8f4f1d7963c9be`

M2의 기록1151·복제/Zone1152·첫 State7/수명1153·1ah1154·1bh1155·재진입1156을 구현 마감한다. 3차 잔여는 M3~M8의6묶음이다. 다음 M3은 공통 Squid Pop 부모4b02d0와 입력 부모4dc220의 현재 checking/전체 부모·실제 호출자, Descend와 메타 후속 카메라/HTML 연결이다. 이 두 함수를 메타 전환 함수 자체로 오해하지 않는다. 이미 존재하는 RunSquidPopParent 등은 현재 코드와 원문을 먼저 대조하고 필요한 차이만 수정한다. M6에는 개발자 이름 제한4b8be0의 직접 Caller와 요청 DWORD 활성 writer를 남긴다.

마지막 전체 디컴파일2026-11-03/HJOW-Athlon, R전용2026-11-07/동일 호스트이며 생성 원문은 수정하지 않았다.
