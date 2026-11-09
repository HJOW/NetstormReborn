# 오벨리스크 보상·후처리·정기 사건의 장기 연결 — 1204

Native 오벨리스크의 보상 선택/변환 본체, 전용 PostPop, 사건0/1을 같은 Client 수명에 연결했다. `RawGemReward`, `RawBuriedPostPop`, `RawBuriedRegular`의 기존 원문 본문을 사용한다. 같은 Pool·현재 TYPE/프레임·난수·제외 TYPE 목록·boss/로딩/Bridge TYPE·Host/공통 후처리를 읽으며 새 World나 Player 배열을 만들지 않는다.

`ClientConstructionGlobals`가 Gem 본체와 현재 제외 목록을 보유한다. 실제 Native Session의 보상·건설·메뉴·변환 Caller도 같은 객체를 참조한다. Deck의 미작성 초기 영역은 기존 `CurrentAllocationBytes`를 현재 Client 장치로 공급한다. 실제 선택 위치에서 현재 fill/시계 장치를 복사해 호출하며, 초기 결합 때 할당·추첨·초기화를 하지 않는다. Gem/Deck 진단은 원문의 문구·소스·행을 같은 Storage 진단으로 전달한다. 중앙 assertion 통합은 M7.5에 남는다.

Storage는 같은 Client 난수를 alias 소유권으로 보유하고 보상 owner를 weak로 빌린다. 선택 호출 동안만 실제 owner를 유지해 Storage→건설 owner→Storage의 강한 순환을 피한다. Native Session의 PostPop/Regular 바인딩은 Storage의 같은 본체를 사용한다. World 반납 뒤 실제 VT0050a748을 선택하는 기본 PostPop/Host도 이 본체로 이어진다. 독립 typed Session의 기존 지역 경로와 구별한다.

오벨리스크 PostPop의 두 Regular는 기존 “항상 확보 성공” 경로 대신 실제 `AllocateResourceRegular`와 같은 확보 객체의 재구성·Host 부착을 사용한다. 두 확보는 각각 성공/실패를 판정한다. 보상 QA WORD 쓰기 뒤 현재 Bridge TYPE 비교·통지, 두 예약과 공통 PostPop의 순서도 기존 본문이 보존한다. 정기 사건0의 현재 프레임/난수/지연과 사건1의 원문 payload 반환도 같은 Native Host가 실행한다.

보충의 모드/Bounds 부모가 이제 장기 너겟 배치 부모를 직접 호출한다. `replenishPlacement`의 Session 대여와 Runtime pin은 제거했다. 이 대여가 필요하다는 옛 검사는 실제 Runtime이 없거나 반납된 뒤에도 같은 보충 본체가 동작하는 검사로 교체했다. Bounds/여백/전달 사각형의 기존 원문 fixture는 유지했다. 남은 건설 큰 효과16개는 여전히 현재 Runtime의 weak 대여·실제 pin 계약을 사용한다.

또한 실제 `0041f910`의5BYTE 점프가 `004b0950`으로 향하는 것을 확인해 네 VT의 공통 Pre 부모를 연결했다. Pre/Post 부모 분배는 **57→61개**, 전용 잔여는 **26→22개**다. 통지는 기존77개 빈 부모/6개 전용 잔여를 유지한다. 이 수치를 큰 선택 효과 전체의 NoWorld 완료 수로 세지 않는다.

원문 근거는 Patch8개와 CD 대응7개, 총15함수의 C와 전체 정밀 범위 PE다. Gem의 실제 진단 문자열 `0`/`type`, Deck 추가/항목/추첨 진단과 실제 오벨리스크 VT의+10/+14/+18/+20/+5c도 대조했다. 마지막 정밀 디컴파일은 Client 양판본2026-11-03·R전용2026-11-07, 모두 **HJOW-Athlon**이며 현재 호스트와 같다. 디컴파일 소스·보호 원본/CD·fixture·AGENTS.md는 변경하지 않았다.

집중 검사는 기존 Gem/오벨리스크 원문 fixture, 현재 장치/제외 목록 공유·진단 복귀, 같은 실제 Pool/Host의 두 Regular·공통 깊이·QA·프레임/난수와 owner 반납을 확인한다. Native Pop의 직접 검사는 원문의 송신 억제 flags10을 사용하며 AI/선택/표시 장치의 호출을 관측한다. 패킷·송신 성공·다중 peer나 큰 AI 본문의 통과를 검증한 것이 아니다. 허용 GUI는 Tutorial4 정상 생산·건설 요청과 대기, 같은 Client의 TEST01 에디터 왕복이다. 실제 결과·최초 실패와 교정·제품 SHA·고정 소스는 [보고서](../../cpppj/buried-resource-owner-20261109.json)가 기준이다.

**남은 범위:** 다른 배치 Caller/Fort 상위 Finish, 전용22개 삭제 부모·6개 통지·다른 Form/Regular·큰 AI와 M6 전수 Caller/owner 감사, 지연큐·JoinTestBattle가 남는다. 네트워크 전체 복원이나 모든 NoWorld 효과 완료로 판정하지 않는다. M7/M8/K/S/B의 우선순위와 사용자 제외 검사는 [현행 인계](../../LEFT_JOBS.md)를 따른다. 전체34경로·전체 솔루션 실행·최종 ZIP은 이번 단위에서 반복하지 않는다.
