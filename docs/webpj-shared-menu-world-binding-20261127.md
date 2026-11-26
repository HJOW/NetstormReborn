# 공유 메뉴·Chat 생성과 World 어댑터 연결 분리

- 단위: WB04-E1378, 2026-11-27, HJOW-Athlon.

createClientSharedProductionMenu는 실제 메뉴 작성기·공유 글자/그림 버퍼·스크롤·Chat을 World 없이 만든다. bindWorld는 현재 실제 Session을 받아 About·Player·객체·사제·Factory·지식 어댑터를 한 번 연결한다. 연결 전의 객체 효과와 중복/반납 후 연결은 오류로 처리한다. 기존 createClientProductionMenu는 같은 공유 생성과 실제 연결을 연속 호출하는 호환 입구다.

전체 생산 조립기의 입력도 Session 대신 실제 ProductionControlContext다. 같은 공유 메뉴·Chat을 보존하는 패널 bindWorld를 제공하며, 늦게 만들어진 지식 어댑터는 현재 getter로 읽는다. 실제 앱은 이번 단위에서 기존 Ready 위치를 유지한 채 Context로 조립하고 실제 Session을 별도 연결한다. 아직 State7 호출 위치를 바꾸지 않았다.

공유 메뉴 반납은 실제 Chat·숫자 창·경고·메뉴/스크롤을 정리한 뒤 World 어댑터 참조를 놓는다. 같은 반납의 중복 호출은 새 미션의 공유 글자 장치를 건드리지 않는다.

## 검증

Node26 집중8/8. 실제 자산·글꼴·팔레트·ClientGumpRoot·Chat/Console 본체로 World 없는 생성과 호스트 미조회, 실제 World 연결 뒤 동일 자식/Console과 초기9.7초 점멸 기한 보존, 실제 어댑터 연결·중복/반납 관문을 확인했다. 가짜 Session은 사용하지 않았다. 번들 재현1/1. 운영 번들은3,463,907바이트다. Chromium12/12가 통과했다. Chat10개(미션 재진입 로그 유지·F3·입력/제출·완성·클립보드·서식), 객체 메뉴1개와1-1 진입/대기1개의 화면3곳0픽셀·덤프3곳허용밖0을 확인했다. 공격·포획·희생·승리는 수행하지 않았다. [브라우저 원문](webpj-e1378-evidence/browser.log) · [검증값](webpj-e1378-evidence/verification.json).

## 후속

앱의 게임 메뉴 SP 추적과 장치 생성에서 eager Session 조회를 없애고, 미니맵·World 전용 메뉴는 현재 실제 World를 연결하도록 옮긴다. 그 뒤 HUD를 State7에 생성하고 Ready에서 같은 HUD를 도킹한다. Runtime 반납과 전체 State 부모 연결은 별도 수명 검증을 거친다.
