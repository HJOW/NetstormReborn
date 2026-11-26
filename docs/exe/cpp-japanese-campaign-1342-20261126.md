# 일본어 캠페인1 1-1~1-3 — 단위1342

2026-11-26, HJOW-Athlon. 마지막 디컴파일 PC도 HJOW-Athlon이며 디컴파일 변경은 없다.

`thewarbegins`·`masterofwhirligigs`·`savetheisland`의 제목·브리핑·성공/실패 문장·버튼 라벨을 번역했다. UTF-8 BOM/CRLF이며 캠페인 목록과 같은 제목을 쓴다. 공식27/43파일 구조 감사가 통과했다. 이 세 본문에는 안내 그림 참조가 없다.

감사 도구에 `--original-archive`를 추가했다. 선택한 TAFF를 읽기만 하여 원본 영어43파일을 찾고, 번역된27파일의 실행 명령·AI/SP·기술·섹션/진행 조건·치환·그림 순서를 한국어 번들과 독립적으로 대조했다. 두 비교 모두 차이0이다. 원본의 특이한 역사학자 인용문도 내용 자체를 고치지 않고 옮겼다. 사용 예: `python tools/audit_cpp_japanese_bodies.py --original-archive originals/netstorm.tarc --json <보고경로>`.

실제 미션 진입3회가 모두 종료0이었다. 각 일본어 시작 버튼을 눌러 phase4·비정지 상태로 넘어가고 실패 본문도 표시되는 것을 확인했다. 실패 본문은 실제 패배 재현 없이 Tell 표시만 요청했다. 진행 완료값은 설치 사본에서 물려받은1-1=1·1-2=1·1-3=0을 세 보고 모두 보존했다. 이번 실행의 승리 증거로 세지 않는다. [1-1](../screens/cpp-japanese-campaign-1342-20261126/thewarbegins.png) · [1-2](../screens/cpp-japanese-campaign-1342-20261126/masterofwhirligigs.png) · [1-3](../screens/cpp-japanese-campaign-1342-20261126/savetheisland.png).

최초 검증 장치의 `split(frame\t)`는 raw_renderer_frame/raw_camera_frame도 나눴고, 초기 진행0 가정도 설치 사본과 달랐다. 원시 보고를 확인하여 줄 첫머리 frame으로만 나누고 초기 진행값 보존을 단언하도록 교정했다. 제품 변경은 없으며 최초 오류 설명을 `validation-1342-first.txt`에 남겼다.

제품 재빌드는 하지 않았다. 직전1340 일본어9/9검사 뒤 제품 C++ 변경은 없다. EXE SHA-256 `116d44b3e08ca27fbc69a3b43aedf83f03e2b9ebaf1b64dcb87672cb02917564`, 보호 원본/CD·AGENTS/README와 설치 사본3,212파일 지문은 전후 동일하다. [검증 JSON](../../cpppj/japanese-campaign-1342-20261126.json), 원시 자료는 `extracted/cpp-followups-1328/*1342*`에 있다.

공격·포획·희생·승리·성공 안내/완료 키·MP는 실행하지 않았다. 본문 표시 확인이므로 기존 자동 플레이 승리 검사를 반복하지 않았다. 다음1343은 캠페인1 1-4~1-6이다.
