# cpppj 1190 — 건설 참조와 현재 Player 선택

제품은 **1190 / 11.01**이다. 건설 연결이 실제 사용하는13개 참조를 `RawConstructionCore`로 분리하고 전체 RawWorldCore의 Kernel/Form/World 지역 부품 의존을 제거했다. Native는 같은 Storage의 Pool·Hash·spot·TYPE·프레임·장부·모드·Factory·Owner·Building·가상 삭제·표면 수명·연결 전역을 직접 빌린다. typed는 기존 World의 같은13참조를 선택한다. 새 World/Acquire/Kernel/Player 사본/난수 seed는 만들지 않았다.

Native Session은 같은 ClientPlayerRecords shared owner의 현재 Records 조회기를 건설 Runtime에 연결한다. Place·PlayerBounds·LocalGeyser는 결합 시 레코드를 읽거나 초기화하지 않고 실제 원문 선택 위치에서 조회한다. 전체 건설 owner/상태와 create/pop/destroy·표시/경제 장치는 아직 기존 Session/World 종단을 사용하며, 이번 단위로 장기 Construction 또는 전체 NoWorld 완료를 판정하지 않는다.

원문의 포인터 선택 시점은 소비자마다 다르다.442b50 비용 안내는 진입에서 고른 Player 포인터를 flags 쓰기와 SP 효과 뒤 이름 읽기까지 유지한다.48f5c0 경계 member도 진입의 this를 유지하고 inactive 진단 뒤 같은 this의 region을 읽는다. record6BYTE는 후속 PlayerId/배치 진단 앞에서 복사한다.486360 생성과4857d0 제외 순회는 다음 Player 차례마다 현재 global base를 다시 읽되, 그 차례 선택한 this는 regionExists/경계 호출까지 유지한다. 새 playerBoundsRecord 경계는 앞선 Caller가 선택한 같은 레코드를 실제 BoundsRecord에 전달한다. 효과 뒤 무조건 다른 base를 다시 고르거나 이름/레코드 값을 미리 복사하는 동작을 넣지 않았다.

선택 span의 물리는 caller 수명 계약을 따른다. 실제 Session의 Player Use는 사용 중 재초기화를 거부한다. 독립 검사에서 base를 교체할 때는 이전 할당도 살아 있게 보관해 같은 this와 다음 base의 차이를 관측한다. 같은 레코드 내용이 효과 중 바뀌면 후행 필드 조회는 그 변경을 읽는다. 기존 typed span/owner callback 계약은 유지한다.

원문 `00442b50`, `0048f5c0`, `00486360`, `004857d0`, `00432430`, `00432050`의 전체 C/PE 범위를 보존했다. 현재와 마지막 완성 디컴파일 호스트는 HJOW-Athlon이다(Client2026-11-03, R2026-11-07). 디컴파일 소스 변경과 원본 실행은 없다.

NetstormCpp/NetstormRoot/건설 Player 집중 대상 Release/x64 빌드는 종료0·컴파일/링크 경고오류0이며 빌드 중13소스를 고정했다. 비MP27검사는 실패0이다. 신규6검사는 현재base의 PlayerBounds439행/LocalGeyser221행 독립 PE 재생, Bounds 진단 뒤 같은 this/다음 호출의 새 base, Generate 다음 Player의 새 base, Exclusions 지역 효과 뒤 같은 this, Charge SP 효과 뒤 같은 레코드의 현재 이름과 다음 호출의 새 base를 확인한다. 기존 건설 몸체/기하의 독립 기계어 관찰도 재생했다. 새MP 프로토콜·패킷·통신 검사는 실행하지 않았다.

최종 제품에서 격리 Tutorial4 생산/건설 요청→실제 TYPE75 객체 생성·600프레임 대기와 같은 Client의800×600 Edit→TEST01→메인 복귀2회를 확인하고 정상 종료했다. 보호2800·설치 자료·EXE·고정13소스는 불변이다. stderr의 기존 인트로 자료/캐시 부재 문구는 보고서에 보존했다. 전체 솔루션·34경로·자연 완주·전체 NoWorld 검증으로 확대하지 않는다.

다음은 같은 장기 건설 owner/실제 장치와 현재 Player가 아직 없는 단계의 지연 구성·선택 검증이다. 큰 NoWorld 파편/낙하·사제/Dude/받침·비영 AI/피해 Regular·Carrier 보충·다른 Form/일반 가상 삭제와 Source/listener/표/Poll·잔여63F·큐·Snapshot/JoinTestBattle는 남는다. 태그25/활성88/무태그63과 delay queue 비활성은 그대로다. 최신 ZIP1092는 현 제품 ZIP이 아니며 M8에서 마감한다.

새MP 단위/패킷/게임/통신·원본 실행·포획/희생·특정 공격·승리·공중3종 전용 실행·Test Battle은0이다. [단위 보고서](../../cpppj/construction-player-view-20261109.json) · [현행 인계](../../LEFT_JOBS.md). 원시는 `extracted/cpp-resume-1190/`이다.
