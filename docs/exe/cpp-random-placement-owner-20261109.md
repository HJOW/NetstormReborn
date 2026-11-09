# 단일 난수 배치와 너겟 배치 부모의 장기 연결 — 1203

실제 Client의 단일 난수 배치 `0040db90`과 너겟 배치 부모 `0040e050`을 `ClientConstructionGlobals`에 연결했다. 기존 `RawFortRandomPlacement`·`RawFortFinishOwner::PlaceRandom`·`PlaceBattleNuggets` 본문을 그대로 조립하며, 같은 장기 Pool/hash/spot/TYPE/프레임·Client 난수·Factory/HP/가상 owner/Pop을 사용한다. 새 World나 별도 Player 배열·전역 seed를 만들지 않는다.

Native Session의 너겟 배치 호출은 이 부모를 직접 사용한다. 단일 배치 상태·완료 모드·보상 TYPE 상태도 장기 owner로 옮겼고, 기존 Native 지형/Farm·섬·완료·보상/변환 Caller가 같은 상태를 참조한다. 독립 typed Session은 기존 지역 상태와 경로를 유지한다. 현재 지면/edgeFarm TYPE는 같은 GameAux 주소를 읽으며, ownerOverride와 오벨리스크 TYPE는 재결합에서 초기화하지 않는다.

다른 기존 Fort·초기 사제·AddRandomObjects Caller의 조립본과 `RawFortFinishOwner::Finish`의 상위 연결은 아직 Session에 있다. 이들이 같은 상태를 빌리는 것과 모든 활성 Caller의 장기 연결 완료를 구별한다. 이번 배치 부모가 사용하는 반복 본문은 `PlaceRandom`이며, 전체 Fort 후처리의 마감을 뜻하지 않는다.

단일 배치는 기존 spot→edgeFarm→전체 MayPlace→지역→프레임→Create/HP→owner→Pop 순서다. 지역 좌표 난수와 전역 프레임 난수도 구별한다. 배치 부모는 `!inSingle` 진단 뒤 현재 boss를 읽고, boss이면 현재 벽시각의 TimeSeed, 그 외에는 같은 ChunkMap 지역0 첫 기록의 byte3×103을 사용한다. 시계/진단 효과 뒤의 현재 오벨리스크 TYPE를 반복 배치에 전달한다. 배치 반환을 새 성공 관문으로 해석하지 않는다.

섬 owner/Bounds 조회는 `RawIslandRuntime`의 기존 읽기 본문을 World 없는 공통 getter로 분리했다. owner는 부재 진단 뒤 존재와 현재 DWORD를 다시 읽고, Bounds는 진단 뒤에도 현재 물리 기록의 네 float 비트를 읽는다. CD의 owner 부재 진단 생략과 Bounds 진단 계약은 기존과 같다. Native 연결은 원문의 `Islandlist.cpp` 문구·행을 실제 Storage 진단에 전달한다.

Patch `0040e050/0040db90/0040ded0/0046f320/0046f130/004ac560`과 CD 대응5개, 총11함수의 정밀 C 및 전체 범위 PE를 읽었다. 지면97·edgeFarm121·오벨리스크112의 실제 초기 DWORD도 확인했다. 단일 배치 C에는 in_stack/unaff_EBP 조각 주의가 있으므로 C만으로 복원 정확성을 판단하지 않는다. 기존 원문 fixture·양판본 비MP 지형의 실제 생성/HP/owner/Pop 연결 검사를 함께 사용한다. 최초 CD 주소 누락 관측과 실제 헤더의 대응 주소 `0042c930`으로 교정한 원시도 보존했다.

새 실제 생성 검사의 최초 기대는 “전용 PostPop까지 도달한 뒤 실패”였지만, 실제 생성 뒤 금지한 Client 효과에서 먼저 멈췄다. 원문 `004ac560`은 전송을 PostPop보다 먼저 호출하므로 검사 기대 순서가 틀렸다. 비서버 입력도 조사했으나 해당 할당 입력은 원문 진단 대상이었다. 최초 실패·추가 진단·해당 소스를 모두 보존하고, 원문의 서버 입력으로 복귀했다. 최종 검사는 실제 생성·현재 VT/좌표/owner/hash·표시가 선행 Client 효과 실패에도 남는지 확인하며, 송신 성공·패킷 내용·후처리 통과를 기대하지 않는다. 실제 대상이 보이는 검사 카메라도 지정했다. 이 교정으로 제품의 서버/패킷/후처리 본문을 변경하지 않았다.

검증한 현재 소스·제품 SHA, 빌드 종료, 집중 검사와 허용 GUI 결과는 [보고서](../../cpppj/random-placement-owner-20261109.json)가 기준이다. 원본/CD/fixture와 AGENTS.md는 변경하지 않았고, 새 MP/패킷/통신 실행 검사도 하지 않는다. 허용 GUI는 정상 생산·건설 요청과 대기, 같은 Client의 Edit 왕복이며 공격·승리·포획/희생·공중3종·Test Battle·전체 회귀·최종 ZIP 검증을 포함하지 않는다.

**남은 범위:** 보충의 실제 배치 선택은 아직 기존 weak Runtime 대여·전체 pin을 거친다. 장기 배치 부모와 단일 본체를 연결한 것만으로 전체 NoWorld 보충을 닫지 않는다. 오벨리스크의 보상 선택·두 Regular 확보/생성·전용 PostPop과 정기 사건은 아직 Session 하위가 있다. 미연결 Client/전용 효과를 성공으로 넘기거나 실패 전 실제 생성/HP/공간의 부분 효과를 숨기지 않는다. 이 하위를 장기로 연결한 뒤 보충 부모의 잔여 Runtime 선택을 제거해야 한다.

기존57개 Pre/Post 부모와 전용26개 잔여·전용 통지6개 잔여 수는 이번 연결로 바뀌지 않았다. 다른 Form/Regular·큰 AI와 M6 전수 Caller/owner, M6-l/m·M7/M8/K/S/B도 남는다. [현행 인계](../../LEFT_JOBS.md)의 우선순위를 계속 따른다.

마지막 정밀 디컴파일은 Client 양판본2026-11-03·R전용2026-11-07, 모두 **HJOW-Athlon**이며 현재 호스트와 같다. 정밀 디컴파일 소스는 수정하지 않았다.
