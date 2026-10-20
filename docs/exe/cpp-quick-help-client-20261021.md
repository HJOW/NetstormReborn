# 전용 Help의 실제 Client 조립 — 단위257, 2026-10-21

시작 커밋은 `fa97b47f`다. 현재 PC와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-20**으로 같다. 새 디컴파일·원본/CD·AGENTS 변경과 원격 push는 없다.

이번에는 기존 `QuickHelpWindow`·`HtmlBodyGump`·파생/기본 HTML·링크·요청/제어/이력을 실제 `RawMenuControls`와 `RawButtonControls`의 공통 등록표, 안정된 파생 노드, 가상 Attach/Handle, 글꼴·StyleText·버퍼·스크롤, 돌 버튼, 배경·장식·ImageGump, 픽셀 수집/정렬, scalar/Release에 연결했다. 일반 Tell 창으로 대체하지 않는다. 단, 실제 화면과 사용자 플레이에서 동일하다는 판정은 아직 하지 않는다.

`UberGump`는 Tutorial의 사건 소비를 먼저 유지한 뒤 Help `02000017`/요청 `02000057`을 전용 창에 전달한다. 지식 카드의 정적 themeHelp, F1, 내부 링크와 Back, 스크립트 Tell 복귀, 모달 입력, 기존 브리핑 위의 출력과 Session 해제 전 정리를 연결했다. 현재 dialog 목록/serial 조회를 사용한다. 기존 Tell/브리핑 자체의 모달은 여전히 이전 Client 어댑터이므로 전용 창끼리의 전체 원본 모달 결합과 동일하다는 뜻은 아니다.

보호 PE를 읽어 창 VT514ec4의 Attach4c5d60/Handle4c6e00/scalar4c6de0, HTML VT50bd38의 Attach46cc30/기본 Help464990/scalar4d4c40을 확인했다. HTML의46ca40은 +3c 슬롯이며 Handle로 연결하지 않는다. F1의 실제4d8c47은 Help 명령/0/**F1Help**를 예약한다. 일반 빈 요청으로 바꾸지 않는다. 그림 제외 TYPE는 PE0054120c/5412dc의 **109/161**이다. 이 확인은 파일 바이트의 정적 검사이며 새 PE 함수 실행 관측은 없다.

원본 주소와 호스트 토큰의 차이도 처리했다. 문자열 고리는101부터 시작하므로 그대로 Help의 두 번째 인자에 넣으면 원본 signed TYPE<256 분기로 잘못 들어간다. 같은80칸 문자열 고리의 토큰에70000000 영역 별칭을 주고, 해석 시 원래 고리 수명을 검사한다. HTML 링크는 부모별62000000 영역을 사용하고 부모 scalar에서 같은 배열/보조 상태를 반납한다. 현재 링크 쓰기 cursor와 보존되는 이전 뒤쪽 바이트를 구별한다. 도움말 버튼의 Fit은 기존 공통 돌 버튼에 빠져 있던 실제 글꼴 측정을 같은 메뉴 측정기에 연결한다.

월드가 없는 메인 메뉴에서도 TYPE 링크가 열려야 하므로 `ReadAssetQuickHelpType`가 실제 전역 TYPE/AST의 range/hpPerSec/class/cost/flags와 기존 공용 요구 에너지 후처리를 공급한다. 설치한 월드에서는 기존 Session의 현재 값과 최대HP 처리기를 사용한다. 생성 때 사용한 파일 캐시/이력/PopupState를 유지하고 창 소멸4c6d40처럼 이력/팝업 명령만 지우며80바이트 인자를 보존한다.

앞 인수인계의 infoMode 설명은 정정한다. 원본565ec4는 정보 HTML/전체 본문 변환이 쓰는 같은 상태로 유지한다. 이번에 확인한 정밀 C의 참조는 네 곳의 쓰기뿐이며, 다중 줄의 제한 필드+d4는 별도의0 초기화다. 따라서 infoMode를+d4로 임의 복사하지 않는다.

내부 주제/`cmd:` 링크를 연결하고 원본 보호 help.english의 `cmd:Tell,TechSupport`도 같은 입력 큐를 사용한다. 외부 URL의 autoOpen/등록 프로토콜·OS 실행과492a30 기술 학습, Help 파일 기록 스트림은 아직 미복원이며 도달 시 명시 오류다. 링크 분해의 세부 토큰 규칙·중첩 모달 초기화 전역/대체 창·전체 Script 상태도 추가 원본 대조가 필요하다. 10.72에는10.78 전용 ImageGump를 설치하지 않는다.

새 조립 검사 세 개는 실제 보호 Help/TYPE/SHP/팔레트·글꼴을 공통 트리에 설치해 부모/자식·스크롤·창 재사용·내부 Back·돌 버튼 누름/뗌·창 밖 입력·지연 삭제·메인 메뉴 Sun Cannon 정보/그림과 픽셀 출력을 확인하도록 작성했다. **컴파일만 했고 실행하지 않았다.** 단위255/256의 독립 기대값/원문은 수정하지 않았다. 첫 빌드의 include/자산 프레임 필드 오류는 수정했다.

최종 전체 Release는 **경고0·오류0·종료0**,로그는 `extracted/cpp-quick-help-client-20261021-build5.log`다. 새 Client 정적 감사와 기존 HTML/창/제어/지식/미션 수명 감사가 종료0이고,기존 창/추가 경로·제어 최초 원문 보존 감사도 종료0이다. 현재 소스 핀 **32보고서·54항목**의 전체 JSON 역변환은 시작 커밋과 같다. 과거 근거·결과·원문·기대값·바이너리는 보존했으며 기존 RawTutorialLater·TYPE 요구 에너지 client/CMake의 역사 핀 문제를 새 통과로 세지 않는다. 이번 실제 세 바이너리 해시도 현재 보고서와 확인했다. 보고서는 `cpppj/quick-help-client-session.json`,재검사는 `python tools/cpp_quick_help_client_audit.py --verify`다.

게임/C++검사/CTest/전체 회귀와11미션 자연 완주는 이전 사용자 지시대로 후속 실제 통합 플레이에 이월한다. 현재 goal은 active이며 외관·음향·입력·전투·자연 완주 동일성은 완료 판정하지 않는다.
