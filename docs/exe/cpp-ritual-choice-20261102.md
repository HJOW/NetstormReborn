# 의식 보상 선택 입력·6b 수신·비미션 창 — 단위707

2026-11-02, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 PC의2026-10-23 자료다. 705~706의 기술 목록에 이어 선택 후 요청 경로를 연결했다. **이번 단위는 디컴파일 기반 사전 구현이며 동작 검증 완료가 아니다.** 비미션 의식 UI와 연결되므로 사용자 지시대로 새 단위·에뮬레이터·게임·통신 검사를 하지 않았다.

## 원문에서 확인한 경로

004d82ed/004d8351/004d83b7은 각각 GetNewTech/RequestStormpower/UpgradeAltar다. 공통0040af50 생성자는 다음 헤더를 만든다. 706 인계의 “헤더 BYTE1”은 첫 BYTE의 버전값1을 뜻하며 메시지 번호와 구별해야 한다.

| 오프셋 | 폭 | 값/의미 |
| --- | --- | --- |
| 0 | BYTE | 프로토콜 버전1 (`0052ee65 & 7f`) |
| 1 | BYTE | 메시지6b (`0052ee64`) |
| 2 | WORD | 선언 길이10 (`0052ee68`) |
| 4 | WORD | 선택한 제단 SID |
| 6 | DWORD | 기술/보상 종류 |

GetNewTech020000d5는 같은 현재005453dc의 제단과 일반 입력의 두 번째 인자를 쓴다. RequestStormpower020000f9는 같은 제단과 현재 nugget TYPE를 쓴다. 두 분기는 헤더 생성→대상0 진단→현재 제단 재조회 순서다. UpgradeAltar020000d4는 입력 인자의 하위 WORD와 현재 dais TYPE를 쓴다. 세 분기 모두004e2dc0(9,packet)으로 요청한다.

등록 표는6b의 빈 observer0040b380, 버전1/불일치 정책0, 수신00448e90이다. 수신은 부모의 raw+11 mask1이면 반환하고, 같은 contained finder를 순회한다. 후보+20의 mask4가 있으면+22를1로 쓴 뒤 가상 삭제(flags0)를 호출한다. 마지막에 현재 packet+6의 DWORD를 읽고 같은 부모에서004afa70(type,0,4,0,1)을 실행한다. 원문에 없는 sender/owner/TYPE 관문이나 직접 기술 지급·희생 완료는 넣지 않았다.

004cfc10은 missionPost가0일 때만 가상+80을 두 번 읽는다. 첫 값의 signed `<3`을 세 번째 인자로, 두 번째 값을 두 번째 인자로 사용해 `SacrificeMenu(target,level,canUpgrade)`를 같은 Tell로 연다. 기존 Carrier.cpp의 실제005453dc 설정과 요청/확인 음향 순서는 유지했다.

## 실제 연결

- `RitualChoiceInput`은 세 명령의 뗌을 장치 조회 없이 소비하고 실제 폭의10BYTE 요청을 작성한다. `GameMenuCommands`의 이름/주소/허용 표와 `UberGump::Event`를 같은 입력에 연결했다.
- `RawCampaignSession::RitualChoiceInputs`는 기존 `CarrierInputState::dais`, 실제 종류 전역, 같은 로컬9번 송신 큐를 제공한다. 즉시 GetTechnology0200009f 호출로 바꾸지 않았다.
- `ReceiveRitualChoice`는 실제 Pool·contained finder·가상 삭제·contained Create를 쓴다.6b는 기존 로컬 로그인과 Editor 공통 등록 경로에 추가했고, Editor의 등록 소유자 확인/수신 반납 목록도17종으로 함께 갱신했다.
- 기존 비미션 `showSacrifice`의 빈 콜백은 같은 Session/원문 미션 관문/Tell에 연결했다. 미션에서는 대상 레벨이나 창을 조회하기 전에 반환한다.

[정적 추출 도구](../../tools/inspect_ritual_choice_source.py)는 보호 PE를 읽고 검토한 바이트 범위의 디스어셈블 및 같은 PC의 정밀 C를 추출한다. **기계어나 제품을 실행하지 않는다.** [정적 보고서](../../cpppj/ritual-choice-static-20261102.json)는 실제 헤더/명령 초기값·원문 해시를 기록하며 원문 실행 fixture나 검사 통과 근거가 아니다. 원시 자료는 `extracted/cpp-ritual-choice-707/static/`다. CD10.72의 대응0040b130/00483dc0은 후속 비교 대상으로만 남긴다.

## 빌드·보호·한계

Release `NetstormCpp` 빌드 경고·오류0, 표시11.01 제품 SHA-256은 `79de8bd3262ce0d3e7129470eb7522202ab581f88b2b869c9f152463178330ab`다. [제품 보고서](../../cpppj/ritual-choice-20261102.json)와 원시 `extracted/cpp-ritual-choice-707/build-v1.log`를 따른다. 기존 AGENTS·원본1394·CD425·fixture805의2625개 변경0이며 새 fixture0이다.

신규 검사/검사 실행·x86 에뮬레이션·게임/GUI·통신은 모두0이다. 마지막 집중 실행은706의6개 실패0이고, 마지막 실제 GUI는704 제품이다. 새로운 입력·선택 Form·비미션 창이 원본과 동일하게 동작한다고 검증한 것은 아니다. heap/SEH/손상 포인터·전체 ABI 동일성을 주장하지 않는다.

조기 희생 안내, 전체 보유 뒤 실제 랭크 상승 효과, 후반 상태/사망 안내, TCP/IP·Outpust는 남는다. 다음 일반 UI 작업은 HTML Input004d3ed0/Zsend와 Edit/시나리오·수명 경로다. 전체 C++/34/ZIP을 반복하지 않았으며 포획/희생·특정 적 공격·승리·공중3종 전용 행동도 재현하지 않았다. 전체 목표는 미완료다.
