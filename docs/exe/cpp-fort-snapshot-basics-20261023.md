# 실제 TypeNames·Deck snapshot과 명시 stream 수명

시작 HEAD는 `9eb899e4 / 20261023-27 CP`다. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-23**으로 일치한다. 기존 정밀 디컴파일과 보호 PE만 읽었으며 새 디컴파일/원본 게임 실행/보호 원문 변경은 없다. 단위361의 REA Ghidra H 드라이브 admission 오류는 같은 요청으로 반복하지 않았다.

## 원본 계약

`extracted/refined/originals/Netstorm.c:151050`의004bd6f0은 manager 존재와 isMine을 진단한 뒤 새 TypeNames stream을 만든다. 그 다음 State.Get을 호출한다. 실제 manager.current는 State로 바뀌지만 이미 받은 TypeNames stream에 계속 쓴다. State+17이0이면 새 TypeNames는 비어 있다.

State+17이 비영이면 현재 타입 수의 낮은 BYTE를 일반 MemStream 쓰기로 저장한다. 이후 현재 signed 타입 수를 조건마다 다시 읽어 이름을 조회하고, signed char를 위치%4×8만큼 밀어 더한 DWORD hash를 인라인으로 쓴다. 이름은 첫 NUL까지만 포함한다. 이름/진단 콜백이 바꾼 타입 수를 다음 반복에 반영한다. 256개 입력은 count BYTE가0이어도256개 hash를 기록한다.

같은 원문152272의004befd0은 고정 this 슬롯으로 owner를 정한다. 슬롯1..8은 같은 owner, 그 외는0이다. 생산 Refresh → 현재 Deck.Get → 현재 manager로 새 Deck stream 생성 → 현재 count의 낮은 BYTE 저장 → 현재 signed count 반복 순서다. 각5BYTE 항목의 앞4BYTE만, 각 BYTE 조회 뒤 그 쓰기의 용량 진단을 거쳐 기록한다. runtime 다섯째 BYTE는 저장하지 않는다.

두 writer의 인라인 쓰기는 용량 진단이 복귀하면 현재 data/pos를 다시 읽는다. 현재 pos/end를 진행하며 이미 쓴 자료를 rollback하지 않는다. 원본 진단 위치는 TypeNames manager586/isMine587, Deck manager1220, 인라인 용량36이다.

정밀 원문의10.72 대응 후보는 TypeNames00427ce0, Deck004294b0이다. 이번 본체 관측/실제 Session 검증은10.78이며, 이 후보만으로10.72의 동일성을 검증했다고 계산하지 않는다.

## 실제 연결

`RawFortSnapshotBasics`가 위 부모의 hash/반복/폭별 쓰기를 구현한다. 일반 count BYTE 쓰기는 기존 MemStream 경계에 맡긴다. 명시 stream의 필드/실제 버퍼 접근은 구현상 빌림이며 원본 외부 효과로 계산하지 않는다.

`CampaignTemplateStorage`는 기존 State의 `CreateDataManagerStream` 장치를 표준35개 섹션에 재사용한다. 기존 노드 제거/버퍼·노드 반납 →16384BYTE 생성 →목록 끝 부착 순서를 유지한다. `TemplateWriteStream`은 manager·섹션·생성 수명을 보유한다. current 변경은 유효성을 바꾸지 않지만 같은 이름 재생성/manager 해제 뒤의 옛 참조는 거부한다. 원시 섹션 벡터만 교체하거나 current에 쓰는 것으로 대신하지 않는다.

`WriteSection`은 같은 명시 노드에 일반 MemStream 쓰기를 수행한다. `AccessSection`은 원본 인라인 쓰기에16BYTE 필드와 실제 물리 버퍼를 빌리고 완료/호스트 예외 뒤 논리 끝만 raw 섹션에 공개한다. 기존 State 생성과 최소 확장도 같은 생성 수명에 참여한다. 새 Money/Badges/Technology 노드에는 옛 typed cursor를 재사용하지 않는다.

`RawCampaignSession::SnapshotFortTypeNames/Deck`은 같은 실제 TemplateStorage/State/TYPE/Player/생산 자료를 사용한다. Deck은 기존 `RawCampaignProductionBindings::Refresh`와 `RawProductionDeck::Get/Bytes`를 재사용한다. 이 명시 호출은 전체 snapshot 부모의 호출 위치나 Notify/파일 저장을 앞당기지 않는다.

## 독립 PE 관측

`tools/decomp_fort_snapshot_basics.py`는 보호 PE SHA256 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`의 두 writer 본체를 실행한다. Create/StateGet/타입 조회/생산 Refresh·GetDeck/최초 count Write/진단만 명시 경계다. hash와 인라인 메모리 쓰기, count 재조회는 실제 PE가 수행한다.

**1,030입력 × 두 x87 제어어 = 2,060관측**, 정밀도 차이0, 실제 본체994,328명령이다. owner1..8/0·9·15 clamp, signed 이름/NUL, 타입 수256의 count BYTE 감김, 콜백의 manager/mine/State/타입 수/Deck count 변경, 비영 cursor와 부분 쓰기를 포함한다.

정상은 completed1/stackdelta0이다. assert-stop은 첫 인라인 용량 진단 진입에서 멈춘다. TypeNames는 header06/pos1, Deck은0303/pos2까지 보존하며 completed0이다. 이 관측은 실제 예외 unwind가 아니므로 음수 스택 차이를 호스트 C++ 예외 ABI와 같다고 주장하지 않는다.

새 fixture `cpppj/tests/fixtures/fort-snapshot-basics-x86.tsv`는 앞12열 입력/뒤12열 관측이다. `extracted/cpp-fort-snapshot-basics-20261023-native.json`에 경계/범위/해시/원문 실행 수를 보존한다. 기존 fixture는 수정하지 않는다.

## 검증과 잔여

독립 전체2,060행의 현재 전역/자료/pos/end/trace를 대조한다. 별도 검사는 current 변경 뒤 명시 stream 쓰기, 같은 이름 재생성·빌림·manager 해제의 수명, 호스트 중단 뒤 논리 끝/부분 자료, 선택하지 않은 장치 생략을 확인한다.

실제 Session 검사는 TheWarBegins를 적재하고 새 WorldSnapshot 작업본을 만든 뒤 원시 TypeNames/Deck을 오염시킨다. 현재 실제 TYPE로 이름을 복원하고 실제 TotalMade의 허용133 변경에 따라 생산 재계산·Deck snapshot이 바뀌는지 확인한다. 같은 파일 저장 부모로 쓴 뒤 다시 파싱해 두 섹션을 확인하고 보호 입력 파일이 그대로인지 대조한다. 실제 원본 입력은 `originals/netstorm.tarc`의 `\d\thewarbegins.fort`이며2546BYTE, 원래 TypeNames는171개/685BYTE, Deck는38개/153BYTE, State+17은2다. 현재 TYPE188개와 금지 후 Deck0개는 이 과거 파일의 섹션을 그대로 다시 쓴 결과가 아니다.

최종 Release 빌드3은 종료0·경고/오류0이고 관련4검사가 모두 실패check0이다. 초과 쓰기의 물리 패딩 노출을 감사에서 발견해 일반 쓰기도 AccessSection의 예외 정리를 공유하도록 수정했다. 빌드 도중 수정과 오브젝트 생성 시각이 겹쳐 중간 빌드2가 옛 오브젝트를 재사용했고, 첫 관련 검사에서 패딩 보존 검사가 실패했다. 해당 소스의 명시 재컴파일을 확인한 빌드3과 같은 검사의 재실행으로 수정 적용/정상 동작을 확인했다. 최종 전체 CTest1은 **2,356검사·실패check0·종료0/271.61초**이며 LastTest.log의 내부 집계도 확인했다. 최종 검증 뒤 제품/검사 C++는 바꾸지 않았다.

로그는 `extracted/cpp-fort-snapshot-basics-20261023-*`다. UTF-8/추가 한국어 주석/diff/보호 경로를 확인하고 인수인계를 마감해 **20261023-29 CP**로 로컬 커밋한다. 작업 중 별도 세션의 `4ff49495 / 20261023-28 DN`이 들어왔으며 해당 변경은 섞지 않는다. 원격 push는 없다.

전체004bf9a0 snapshot에는 Technology 내용물 writer, Objects/Terr·Chaff writer, Territory의 현재 지역 생성/120BYTE 저장, Money의 공유 난수Next(0x3f2)+333.0f, Mission/Badges writer와 Notify가 더 필요하다. 표준35개 섹션 밖 임시 섹션/이름 교체도 Objects 구현 때 확장해야 한다. generic Fort 설치/Editor Player/새 맵/실제 입력·State1/6 및 전체 사용자 완주는 계속 미완료다. GUI에서 Editor가 가능해진 것으로 계산하지 않는다.

다음 읽기 전용 조사에서 Technology는 기존 RawContainedFinder::Begin/Next와 ContentFormHost::Robust(가상+60)를 재사용할 수 있음을 확인했다. 남은4bcea0 직렬화를 같은 Pool/CurrentTechnology/명시 stream에 묶는 방향이다. Money는 Session의 private Impl::random을 사용해야 하며 별도 난수 사본/API를 만들지 않는다. Mission의 실제 sequence/flags는 UberGump::MissionRuntime::state, 이름은 MissionScript/Init 저장소에 있다. guided Tutorial에만 현재를 반환하는 기존 Guide getter로 저장 유무를 판정하면 일반 Battle 미션을 누락한다. Badges는 로더의28×220BYTE 외부 span 형식은 있으나 조사한 Session 경로에서 live 장부 소유가 발견되지 않아 원시 Fort의 Badges로 대체하지 않는다.
