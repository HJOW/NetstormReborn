# 일본어 Tell 신기술·경고·팁40개 — 단위1351

2026-11-26, HJOW-Athlon. 마지막 디컴파일 PC도 HJOW-Athlon이며 디컴파일 변경은 없다.

NewTech~TipList의 고유 표시 줄86개를 번역했다. 신기술/제단·사제/연결 경고·데모 안내·팁40개를 포함한다. UTF-8 BOM/CRLF이며 명령·치환·시간 조건·링크·스타일과 tip0~39 키·tip39 내부 순번 초기화 명령을 보존했다. 오래된 판매/지원 문구와 URL/전화번호는 원문 자료로만 번역하며 접속하지 않았다.

한국어/원본 영어42파일 감사와 일본어9/9검사가 통과했다. 전체41/43 완료·Tell 부분 번역 상태를 유지하고 다음 구간은 AllianceRequested다.

팁0~39를 실제 다음 버튼으로 모두 표시했다. 팁3에서 이전→2·다음→3도 확인했고 마지막 tip39를 표시하면서 원문의 tipNumber=0이 실행되는 것을 보고로 검증했다. 긴 팁의 줄바꿈·버튼·메뉴 용어와 마지막 안내도 확인했다. [긴 팁](../screens/cpp-japanese-tell-1351-20261126/tip19.png) · [메뉴 용어](../screens/cpp-japanese-tell-1351-20261126/tip38.png) · [마지막 팁](../screens/cpp-japanese-tell-1351-20261126/tip39.png).

신기술 창의 global.newTechName/global.newTechPic에는 표시 전용 썬 캐논 이름/그림을 공급하여 동적 그림1개를 확인했다. 실제 AcquireNewTechnology·기술 추가·희생은 실행하지 않았다. [신기술 그림](../screens/cpp-japanese-tell-1351-20261126/newtech.png). 최초 검사에서 표시 BYTE를 받는 Config UI 명령에 UTF-8 문자열을 넣어 이름 깨짐/그림 누락이 보였다. 실제 제품 configure가 쓰는 UTF-8 설정 경계와 같은 시작 설정으로 공급하도록 검사만 교정한 뒤 전체 이름·그림·라벨·버튼이 온전했다. 실제 제품 경로를 바꾼 오류 수정으로 세지 않는다.

제단 업그레이드·진행 중·연결/옛 DirectDraw/데모 안내6종도 표시만 확인했다. 최종GUI2회 종료0이며 최초 표시 입력 실행도 별도로 보존했다. 제품 C++ 변경·재빌드는 없다. EXE SHA-256 `458f23e09021c5c8d7230459c1499034d8fddfb69a471da60af5c39e4741b918`, 보호 원본/CD·AGENTS/README와 설치 사본3,212파일은 전후 동일하다. [검증 JSON](../../cpppj/japanese-tell-1351-20261126.json), 원시 자료는 `extracted/cpp-followups-1328/*1351*`에 있다.

다음1352는 Tell AllianceRequested~END다. 동맹/희생/DirectDraw/종료 공통 창을 번역하고 Tell 전체 감사 후 부분 번역 상태를 해제한다.
