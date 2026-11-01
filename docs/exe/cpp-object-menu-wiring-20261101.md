# 객체 메뉴·특수 명령 연결의 원본 정적 대조 — 단위681

2026-11-01, **HJOW-Athlon**. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23** 유지다. 기존 디컴파일·보호 파일·제품 코드는 변경하지 않았다. [보고서](../../cpppj/object-menu-wiring-20261101.json)와 [읽기 전용 도구](../../tools/cpp_object_menu_wiring_audit.py)를 남긴다.

보호10.78 PE의 실제 생성자 VT83개에서 `+a8/+ac` DWORD를 읽어 현재 `ObjectMenuSlots.inc`의83행과 전부 대조했다. 작성 입구13개 모두 현재 기본/Factory/Priest/Carrier/Damageable 또는8개 파생 작성기에 연결되어 있다. 열기 입구는 **총7개**이며, 비멀티5개는 현재 Client에 연결되고 나머지2개는 MP 메타맵 전용으로 이월한다.

| 원본 열기 입구 | 현재 경로 또는 이월 이유 | VT 수 |
| --- | --- | --- |
| 004adb30 | 공통 Object Open | 55 |
| 0041e660 | 주문형 선택→공통 Open | 16 |
| 004268c0 | Carrier 선택→공통 Open | 7 |
| 00492080 | Priest 선택→공통 Open | 1 |
| 004754b0 | Factory Open | 2 |
| 004789b0 | Metadisplay Battle/Challenge 참가·설명 메뉴,3차 MP 무시험 사전 구현 | 1 |
| 00479390 | Metadisplay Challenge Area/Descend 메뉴,3차 MP 무시험 사전 구현 | 1 |

최초 도구는 현재 Client의 열기5개를 전체로 가정해 자체 대조가 실패했다. 실제 PE/기존 생성 자료에는 처음부터7개가 있었으며 두 MP 입구를 기존 디컴파일에서 다시 분류했다. 기존 제품의 새 실패나, MP 실행 검증으로 세지 않는다.

특수 메뉴가 작성하는 명령 전역9개도 실제 PE의 DWORD와 현재 상수표가 같다. 현재 소스의 소비 지점/줄/해시를 함께 기록했다.

| 명령 | 현재 연결 |
| --- | --- |
| Meltdown | CampaignObjectMenuClient→Session CommandObject→RawMeltdown |
| Cracked/Normal/Hard | 일반 입력→Session SetBridgeQuality, 품질0/1/2 |
| Turn To Spell/Obelisk | CampaignObjectMenuClient→Session→RawObjectConversion |
| CHEAT Dig Up | 일반 입력→같은 Session GemReward.Reveal |
| Detonate | DispatchDetonateInput→Session CommandObject→RawObjectConversion |
| About/Help | 일반 입력→현재 RawMenuControls.OpenHelp |
| View Netstorm Knowledge | 일반 입력→현재 Session 지식 메뉴 |

따라서 이 명령들의 일반 입력 연결 자체를 미구현으로 다시 만들 필요는 없다. 특히 Dig Up의 실제 special 메뉴 클릭은 여전히 미검증이며, 노출 조건을 강제로 바꾸지 않았다. 크리스탈의 제목/owner 표시와 공통 메뉴의 raw owner, 제단/다이스의 실제 가상 count·회수·소유자 변경은 기존512~520 및 후속 근거를 유지한다. 이번 정적 대조를 그 효과/모든 소유자·레벨·손상·포획 상태의 추가 실행 근거로 합산하지 않는다.

이번 단위는 **제품 변경0, 새 C++ 검사0, 원문 본문 실행0, 게임 실행0**이다. 공격형 공중3종도 정적 표 참조만 했고, 신규 MP 검사·통신·사제 포획/희생·특정 적 공격·승리 재현은0이다. CD 메뉴 몸체/전체 선택기/간접 alias 부재/픽셀 일치는 대조하지 않았다. 제품과 EXE SHA는680 그대로이고 마지막 전체34/ZIP도645의644제품 그대로다. 원시 정적 로그는 `extracted/cpp-object-menu-wiring-681/audit-v2.log`다.

다음은 잔여 목록의 포괄적 ‘메뉴 미연결’ 대신 실제 비멀티 수명·Edit 도구의 선택된 하위를 확인한다. MP 두 열기 입구와 신전 checking 전용 AI 디버그00413dd0은 일반 싱글플레이 메뉴 근거와 분리한다.
