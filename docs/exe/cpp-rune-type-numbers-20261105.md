# 단위870 — Rune·다이스의 현재 제단/다이스 번호와 참조 수명

Rune·다이스와 참여자·AI·메뉴의 제단/다이스 타입 번호를 보상과 같은 Client 장기 owner에 연결했다. 의식 효과나 실제 희생 재현을 실행하지 않는다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**, 현재 호스트도 일치하며 산출물은 변경하지 않았다.

## 연결과 순서

`RunePostPopState`가 `005412e8/005412ec`를 선택적으로 빌리고, `AltarType()/DaisType()`은 현재 **const 참조**를 반환한다. 독립/CD 입력은 포인터가 없으면 기존 `altarType/daisType` 필드를 쓴다. 이 형태는 값 getter의 임시 객체를 `PriestParticipationState`, `AiBuildChoiceGlobals`, `AiDropTargetGlobals`, `AiPriestActionGlobals`, `SalvageCommandState`의 참조 멤버에 보관하는 오류를 막는다. 실제 Session은 869의 보상과 같은 `RawGameAuxState` 주소를 연결한다.

Rune finder `00448a20`은 후보 반복 안에서 현재 다이스 번호를 비교한다. 시간 helper `00448340`은 typed finder 인자로 제단 번호를 한 번 저장하고, 현재 위치/TYPE 부류·원소유자 WORD·억제/Player BYTE로7/10/25를 선택한다. 기존 구현의 읽기 위치를 유지하며 번호 원천만 바꿨다. 최초 예약/생성이 돌아온 뒤의 현재 번호도 같은 getter를 읽는다.

다이스/제단의 typed 조회·수명 분기·가상 분류와 실제 Factory 생성 인자, 참여자·AI·메뉴/저장/선택의 기존 번호 읽기도 같은 getter로 교체했다. 참조형 소비자는 원래 owner의 주소를 보관하고, `const auto type`처럼 값으로 저장하던 원문 경계는 값으로 유지한다. 기존 가상표/TYPE 사전 계약과 전체 파생 Pop 복원 잔여를 이번 번호 연결 완료로 확대하지 않는다.

실제 `RitualChoiceInputs().daisType`은 getter 생성 뒤 바뀐 현재 번호를 읽는다. 선택 제단 SID나 너겟 종류와 혼동하지 않으며 입력 명령/패킷 송신은 검사하지 않는다. `RawCampaignRitual.inc`의 너겟 참조와 다른 운반/내용물·채집/건설/덱 소비자는 후속 잔여다.

## 검증

제품/두 집중 대상 Release 빌드는 **종료0·168.081초·경고/오류0**이다. 첫8소스와 최종 SHA-256이 같으며 제품/검사 사후 교정은0이다. 집중 검사는 **고유30개/6묶음(새4), 첫 실행 실패0**이다. 새 순수3개는 참조형 참여자/AI의 원래 owner 유지·실제 finder/시간 선택·예약 뒤 현재 번호를, 실제 Session1개는 Load 뒤 선택 입력의 getter 생성 후 현재 번호 읽기를 확인했다. 원래 번호는 Session 해제 전에 복원했다.

기존 buried/Rune 현재 부품18개·양판본 Rune 원문1920행 재생1개·869 실제 Session 현재 번호1개·다이스 읽기/진단 부모6개도 통과했다. 실제 의식 효과·포획/희생/전투/성공 이동은 실행하지 않는다. 다이스/제단 수명 효과의 번호 치환은 정적 연결·컴파일 근거이며 해당 효과 본체의 신규 동작 검증으로 확대하지 않는다.

새 Native/fixture·기존 기대값 변경0, 보호2761개/fixture941개 전부 불변이다. 제품870/11.01, EXE 10592768바이트, SHA-256 `0eb96ad0ea895eb02058733f76abc01fa47ce4213e5e6bd1232769eb73676f6d`다. checking10(부모4/부분3/MP3)·완료부모38/부분3은 유지하며868 공중 전용 실행 검사는 여전히 생략 상태다.

세부 해시는 [JSON](../../cpppj/rune-type-numbers-20261105.json), 원시·첫 소스·보호/실행 목록은 Git 제외 `extracted/cpp-resume-870/`에 보존한다. 다음871은 다른 너겟 번호 소비자다. 정적 목록의154 중 구조체 바이트 오프셋/고정 색표는 TYPE 번호로 바꾸지 않는다.

실제 게임/GUI·포획/희생/특정 적 공격/승리·공중3종 전용·신규 MP/통신·물리/PCM/CD 재시도와 전체71/CTest/34미션/ZIP 반복은 제외한다. 전체 Pop/Factory/군도·메뉴/Edit/State/파일·Windows 및 후속 단계는 여전히 미완료다.
