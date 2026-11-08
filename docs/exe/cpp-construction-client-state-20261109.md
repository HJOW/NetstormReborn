# cpppj 1194 — Client 건설 상태와 실제 backing 수명

제품은 **1194 / 11.01**이다. 별도 ClientConstructionGlobals가 같은 실제 Storage/Gameplay/Replay와 건설 State를 보유하고 Native Session은 같은 상태를 빌린다. Source/Authority/GameAux/owner 정체성은 World 대여 전에 확인한다. 상태 결합은 World/Player/SP/난수나 누적 상태를 초기화하지 않는다. 독립 typed Session은 기존 지역 상태를 유지한다.

Storage의 실제 자원 장치는 Gameplay alias를 보유하므로 Gameplay 자체에 State를 넣으면 pattern getter→Storage→Gameplay의 참조 순환이 생긴다. 별도 Client owner가 backing을 보유하고 State를 먼저 반납한다. 새 검사는 이 실제 역방향 보유 관계를 포함해 같은 상태 재결합, 호출자 backing 반납 뒤 조회, owner 반납 뒤 전부 해제됨을 확인한다.

Native8TYPE getter는 Session this 없이 장기 Storage/GameAux만 참조한다. 새 네 슬롯의 초기값00541204=107/00541264=131/0054125c=129/00541290=142를 보호 PE에서 확인했으며 fresh helper가 덮지 않는다. 관련 Gun/Missile 소비자도 같은 주소를 읽는다. GunMode의 const 참조 수명도 유지한다. 모든 역할 소비자를 전수 복원한 단계는 아니다. NativeBounds mission은 실제 장기00594fa0이며 local Fort 로더의 편의 writer를 alias해 준비 prefix를 바꾸지 않는다.

ValidateState/ConfirmSupport/Support는 준비 사본 대신 같은 현재 getter를 읽는다. 두 Collision은 후보별 현재 server를 읽으므로 앞선 지형 효과가 바꾼 값이 다음 후보에 반영된다. 기존 지원 범위/필수 효과/정상 거부 계약은 유지한다.

Release/x64 NetstormCpp·NetstormRoot·세 집중 대상 종료0/경고오류0/고정20소스 불변이다. 비MP35+명시17+새대상11, 합계63검사 실패0이며 새4개다. 새대상에는 기존 순수 충돌 PE 세판본/geometry7개가 포함된다. current대상19개 중 결합 Notice/패킷2개는 제외했다. 상위 current검증은 typed 합성 실제 부품과 금지 효과로, 상태owner는 실제 Source/Native marker/장기 backing으로 검사했다. 전체 Native 건설 body/새 MP 검사로 확대 해석하지 않는다. 정적25조건/C·PE10함수/초기4DWORD를 대조했다.

최초 빌드에서는 새 검사의 PopulateGeysers 세 호출에 필수 네 번째 옵션 인자가 누락돼 집중 대상 컴파일이 실패했다. 검사에 명시 옵션{}을 추가했으며 제품 소스19개는 최초 빌드 뒤 변경하지 않았다. 최초 실패 로그와 최종 성공을 보고서 verificationHistory에 구분했다.

최종 GUI는 Tutorial4의 정상 생산/건설 요청→실제TYPE75/owner1 생성과600frame 대기, 같은Client800×600 Edit TEST01 진입/메인 복귀2회/정상 종료0이다. 보호2800파일·설치자료·EXE는 불변이다. 원본 게임·새MP/패킷/통신·포획/희생·특정 공격·승리·공중3종·Test Battle·사용자 수동 지정 검사는 실행하지 않았다. 기존 부재 파일 stderr는 보고서에 보존했다.

**전체 건설 Bindings body는 아직 Session 소유다.** 다음은 별도 Client owner의 body/실제 장치와 선택된 Runtime pin·실제 destroy bool·큰 NoWorld/AI/Carrier·다른 Form/일반 삭제다. Source/listener/표/Poll·63F·지연큐/JoinTestBattle와M6-l/m/n·M7/M8/K/S/B도 남는다. 태그25/활성88/무태그63·큐OFF와최신ZIP1092를 유지한다.

마지막 디컴파일은Client2026-11-03/R2026-11-07/HJOW-Athlon이며 현재와 같고 원문 소스 변경은 없다. C/PE10함수444590/486130/486360/49b510/48f5c0/432050/425830/425c20/46b830/482340의 전체 분리 범위·해시·현재 전역 참조와 실행 원시는extracted/cpp-resume-1194/에 있다. [보고서](../../cpppj/construction-client-state-20261109.json), [1193 현재 전역](cpp-construction-current-globals-20261109.md).

Netstorm.exe 13629952바이트/SHA-256 `2f0e5dfa506c576202475757f30f8224fd8d698feb56b06975039335b6877a27`. R.exe 677376바이트/SHA-256 `65cd7deeba04f7557193623288f16deda40c07db858e6a460537b082b8425676`.
