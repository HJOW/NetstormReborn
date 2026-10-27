# 개별 다리 우클릭의 품질 변경 — 단위516

기존 제품에서 Editor의 **Set as Cracked 선택이 미연결 오류로 종료1**인 것을 실제 재현했다. 세 우클릭 명령02000125/126/127을 일반 입력에서 현재 Session의 개별 다리 변경에 연결했다. 메뉴가 반납된 뒤에도 실제 사건의 y=SID와 고정 품질0/1/2를 사용하며 release는 소비만 한다.

원본 일반 입력의 호출은004d841f/004d844f/004d847f이고 하위는00421f20이다. 개별 변경은 Editor→현재 TYPE가 bridge인지 확인한 뒤 기존00421db0/00421e60/00421eb0을 실행한다. 전체 변경00422a70과 달리 **missionPost만 켠 상태에서는 개별 변경이 일어나지 않는다.** `RawBridgeEditor::SetOne`은 이 차이와 예약/free/dead SID를 별도 제외하지 않는 원문 순서를 유지한다. 물리 범위 밖의 호스트 접근은 오류로 진단하고, 비Editor는 SID를 읽기 전에 반환한다.

## 독립 원문 근거

새 `tools/decomp_editor_bridge_one.py`는 기존 전체 다리 관측기의 명시 하위 기계어와 새421f20 진입을 사용한다. Editor0/1/80000000, 품질0/1/2/3/ffffffff, 예약1·일반5·dead6·후보 부재9·비다리10·free11·서버15000·0번 SID, seed2종의 **고유240입력 정상 반환**을 확보했다. 기대값은 C++에서 계산하지 않았다. 원본 게임/창/OS는 실행하지 않았으며 공격형 공중3종은 대상이 아니다.

관심9슬롯 전체50BYTE·가상+88 순서·실제 난수·후보 버퍼 확보/반납을 기록했다. 가상 갱신과 new/free만 외부 경계로 대체하고 품질·프레임 후보·난수는 원문이다. fixture는 `editor-bridge-one-x86.tsv`, [원문 보고서](../../cpppj/editor-bridge-one-native-20261028.json)다. 기존 전체 다리30행 fixture는 바꾸지 않았다. 새 C++ 재생은 나머지23991슬롯의 보존까지 비교하며 별도 실패 검사에서 Editor의 조기 거부와 수명 쓰기 후 미연결 효과의 부분 변경을 확인한다.

## 실제 Edit

원본50개 목록 상한을 위해 loose Fort는 TEST02 하나인 별도 자산 복사를 사용했다. 원본 TEST02 바이트와 원본/CD 파일은 바꾸지 않았으며, 저장은 게임이 관리하는 형제 Editor 작업본에만 수행했다. 상태나 변경 함수를 직접 주입하지 않았다.

최종 같은 게임의 **1280×720·1024×768 두 실행**에서 Edit→TEST02→다리119,146 우클릭→세 품질→Main Menu/Yes 저장→Edit/TEST02 재진입을 통과했다. 실제 대상 SID15954는 J11/frame37(Cracked)→J01/frame35(Normal)→J20/frame40(Hard)이며, 매 단계 나머지660개 다리의 프레임은 그대로였다. 저장 후 재진입한 같은 위치의 다리도 Hard/frame40이다. 저장 파일5282BYTE는 독립 Fort 판독기에서35섹션·남은 바이트0·다리661개로 적재됐다. 활성1과 메뉴/Form 참조·실제 명령/인자도 확인했다.

첫 수정 제품 실행은 제품의 변경/저장/재진입까지 성공했으나 도구가 object행의 walking 열을 frame으로 읽어 실패했다. 실제 보고서의 frame37/35/40을 확인해 열7로 고쳤으며 이 실행을 최종 두 사례에 합산하지 않는다. 새 도구는 `tools/cpp_bridge_quality_menu_smoke.py`다. 캡처에서 변경 뒤/재진입한 Editor 표시를 직접 검토했다. 같은 최종 제품의 일반 플레이 짧은 검사에서도 편집 전용 세 항목이 표시되지 않았고 우클릭/ESC 반납을 통과했다. 최종 수락은 Editor 두 사례와 일반 모드 한 사례다. 전체6게임실행은 종료0 다섯 번/종료1 한 번이며 기존 제품의 명령 실패와 수정 제품의 도구 실패를 따로 보존했다.

## 마감 상태와 잔여

Release 빌드는 경고/오류0이고 집중 검사는 **51고유/52관측·실패0**이다. RawBridgeEditor4(새2개 포함)·ObjectMenu20·UserSelection13·EditorSaveGoMain2·EditorTestBattle3·GameMenu10을 실행했다. EditorTestBattle_UsesActualGameMenuNumbersAndInputQueue가 마지막 두 필터에 겹치므로52개 고유로 세지 않는다. 실제 등록은3,342개이며 전체 반복은 하지 않았다. 마지막 전체3,337/실패0은510의 이전508 제품 기록이다.

최종 [실행/보호 보고서](../../cpppj/bridge-quality-menu-runtime-20261028.json)에서 원본/CD/AGENTS와 기존 fixture가 동일함을 확인했다. 기존 검사 파일 변경은 새 재생/등록을 더한 RawBridgeEditorTests.cpp·tests/CMakeLists.txt 두 개이며 다른 기존 검사는 동일하다. 게임 SHA는 `7cc3b2eb5dd6d1b1f043fe82c6a70b3503c8aec9da1b1219628846cb01bea100`, 검사 SHA는 `952f5e1a29d78a72ed0119ebc9f90424b62e8b3628417295b83bbde904aba6bb`다. 실행 사본은 각 실행 뒤 복구했고 형제 작업본만 저장했다.

이번 단위는 개별 다리3명령이며 Dig Up·Spell/Obelisk 전환·Meltdown/Detonate와 제단/일반 건물형 전체 조건, 원본 전체 선택기004d5840 등은 남는다. 전체화면·Windows11·자연 완주·원본 전체 동일성의 증거는 아니다. 세 공격형 공중 유닛은 기존 디컴파일 정적 분석만 유지하고, 연녹색 직사각형 분석도 보류한다.

마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이고 현재 PC도 같다. 디컴파일 산출물은 수정하지 않았다. 원시는 `extracted/cpp-bridge-menu-516/`에 보존한다.
