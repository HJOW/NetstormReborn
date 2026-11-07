# 1099 — 실제 메타 Gump 장치와 공용 NSInfo 슬롯 정정

제품1099 / 표시·파일·제품버전11.01이다.1098(`39c89889 / 20261108-04 CP`)의 표시 본체를 실제 UI에 연결하면서 두 해석을 정정했다. **00593150은 기존 ClientNsInfoGlobals.current인 NSInfo 창이며, Gump 가상+18은 기본 가상표0050ac70에서00464560 Push다.**1098에서 CS에 더한 별도 playerList 슬롯/접근자를 제거하고, 모든 새 표시 본체의 이름/계약을 Push로 고쳤다. 실제 기계어는 트리 분리·void8 저장 뒤 가상+8(true) 재그리기를 호출하므로 Redraw(false)로 대체하지 않는다.

RawMenuControls.BindMetaDisplayGumps가 같은 공통 노드·트리 Push·다중 줄 ReplaceText/AppendText·이름 마크업·FitContent·RequestScroll·기존 파생 Attach·실제 등록ID 배열 조회를 연결한다. 문자열 끝 NUL도 기존 처리기에 전달한다. 객체/Renderer/현재 번역/진단 장치는 이 UI 어댑터가 빈 성공으로 채우지 않고 실제 상위가 공급해야 한다. 이 장치의 수명은 같은 RawMenuControls보다 짧아야 한다.

새 한 줄/다중 줄 메타 글자 공장은 기존 TextGump/ScrollingTextGumpWidgets·실제 버퍼/Slider·공통 등록표/serial을 사용한다. 같은 map의 실제 생성물을 사용하며 별도 이름표 복사본을 만들지 않는다. 메타 공장 소유 토큰은 별도 집합에 기록하고 공통 가상 scalar/Cleanup과 Release로 지운다. 부분 생성의 등록 노드도 Kill하고 기본 생성 전 미등록 노드는 같은 저장소에서 반납한다. 다른 NSInfo/Help/메뉴의 소유 집합은 포함하지 않는다. 기존 외부 공통 버튼/배경 생성기의 OS 할당 실패 전체를 새로 검증한 것은 아니다.

0047cab0은 같은 NSInfo.current를 Push하고 반환 뒤 현재 슬롯을 다시 읽어 visible/현재 float 위치/parent0·mode0으로 부착한다. 실제 EditorRuntime의 metaThird도 이 기존 NSInfo 장치에 연결했다. **전체0047dd60·CS 수신/Pool·가상객체/Renderer·HTML·NSInfo 메타 배치와1dh/28~33 등록은 여전히 후속**이다. UI 장치 연결을 전체 메타 화면 성공으로 세지 않는다. 메타 전용 정리의 상태 이탈 부모 접합도 실제 장면 수명 연결 때 이어간다.

첫 빌드의 NSInfo shared_ptr/raw포인터 비교 C2678 한 건은 .get() 비교로 교정했다. 첫 실패 로그/종료값도 원시 폴더에 보존했다. 최종 NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0이다. 기존 원문/전체PE 가상표·Push·NSInfo 재조회, 기존 실제 UI 처리기와 동일 저장소 배선을 정적으로 대조했다. 보호2800파일 불변·새fixture0·새C++/MP/게임/통신/새R·원본R/전체검사/34미션/ZIP 실행0이다. 새 공장/Push·UI 접합의 실제 MP 동작은 미검증이다. Netstorm.exe 12,405,760바이트/SHA-256 `dfeb790879dc775fe1abb7da9c2f611e7365644983336ce7ee9a016873718597`,새R.exe 669,696바이트/SHA-256 `91e3b569b435f5396801442fa1f204479ae472009f3d2dd8edbbce10922a5b2a`다.

다음은0047dd60 전체·CS 수신과 실제 객체/Renderer/HTML·NSInfo 메타 배치를 완성한 뒤1dh/28~33에 접합한다. LAN/Outpost/checking5→한국어후속→Edit시나리오UI 순서와 사용자 지정 검사 제외를 유지한다. 전체 목표는 미완료다. 보고서는`cpppj/meta-gump-devices-20261108.json`, 원문/PE·빌드/감사는`extracted/cpp-resume-1099/`(Git제외)다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 재생성/생성소스 변경0이다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
