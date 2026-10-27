# 크리스탈·스펠 비석의 실제 우클릭 — 단위515

단위513과 같은 제품으로 스톰 크리스탈과 비석의 우클릭→About→해당 Quick Help→바깥 클릭 반납을 확인했다. 최종 **1024×768·1280×720 각각 두 대상, 4사례 통과**다. 제품 동작을 바꾸지 않았으며 새 도구 `tools/cpp_special_object_menus_smoke.py`와 [보고서](../../cpppj/special-object-menus-runtime-20261028.json)를 남겼다.

| 대상 | 실행 입력 | 실제 메뉴와 확인 내용 |
| --- | --- | --- |
| 스톰 크리스탈154 | TEST02 사본의 geyser/nugget TypeNames 해시를 서로 교환한 명시 변형 | 실제 nugget SID15024의 Form 참조, Storm Crystal 제목, About(TYPE154)·Player, 회수/편집/Dig Up 제외, 크리스탈 Help 본문·창 반납 |
| 비석112 | 원본 dissolvedalliance에 원래 배치된 buried SID15026, cell88,119 | 방향키 누름/뗌으로 덱 뒤 대상을 화면 안에 이동, 실제 Form 참조, Obelisk 제목·Class Spell, About(TYPE112)만, 비석 Help 본문·반납 |

실제 활성1·비정지0과 메뉴 루트/대상 SID, Help 부모/본문을 검사했다. 바깥 클릭 뒤 Help가 없어지고 메뉴 instance/index/menu도 모두0이다. 캡처에서 비석 메뉴와 크리스탈 Help를 직접 검토했다. 크리스탈은 정보의 Owner가 Nobody여도 raw 소유자1 때문에 Player 하위 메뉴가 존재할 수 있다. 정보 작성기의 크리스탈 owner0 처리와 공통 Object의 raw 소유자 처리는 원문에서 별개다.

비석의 일반 플레이 우클릭에 임의의 Learn 항목을 만들지 않았다. 원본462390은 Editor일 때 Turn To Spell!, special일 때 CHEAT Dig Up을 포함하며 나머지는 Object 기본 메뉴다. 실제 스펠 학습은 별도 운반/도착 경로이며 이번 짧은 우클릭 검사에 합산하지 않는다. 크리스탈의 공통 Damageable 작성에서도 nugget 회수 제외 조건을 유지했다.

최초 탐색의 방향키 release 누락으로 비석이 클릭 전에 화면을 벗어났다. 키 뗌을 복원한 뒤 메뉴가 열렸다. 또 검사 도구의 ‘ESC로 Help가 없어져야 한다’는 가정이 실패했다. QuickHelpControl은 이 입력을 공통 부모에 위임하며 실제 Help는 남았다. 닫기 증거를 원문이 명시하는 바깥 왼쪽 클릭의 Kill/반납으로 수정했다. 제품의 ESC나 카메라 동작은 바꾸지 않았다. 이 두 중간 관측을 최종 성공에 합산하지 않는다.

탐색/도구 실패를 포함한 **전체8게임 실행은 종료0**이고 최종 수락은 위4사례다. 원본/CD/AGENTS/기존 검사와 fixture는 동일하며 실행 사본·설정은 복구했다. 빌드와 C++ 검사는 추가하지 않았다. 게임 SHA는 `74548abb1600396188d4365464a808d555418a3bb080211b97e78f8cdfb2c362`, 등록 검사는3,340개다. 기존513 집중39개나510 전체3,337개를 이번 실행 수와 합치지 않는다.

마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이고 현재도 같은 PC다. 디컴파일 산출물 변경은 없다. 훨리기그·맨오워·더스트 데빌은514의 기존 소스 정적 대조만 유지하며 이번 도구의 생산/생성/선택 대상에 넣지 않았다. 전체화면·Windows11·자연 완주·전체 타입/소유자/상태 동일성은 후속이다. 다리3상태·Dig Up·Spell/Obelisk 전환·Meltdown/Detonate의 명령 실행과 제단/일반 건물형 잔여를 이어간다. 연녹색 직사각형 분석 보류도 유지한다. 원시는 `extracted/cpp-object-menus-515/`다.
