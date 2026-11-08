# cpppj 1197 — 장기 건설 본체와 선택 효과 수명

제품은 **1197 /11.01**이다. 전체 Native 건설 본체를 별도 ClientConstructionGlobals에 소유하고 Session은 같은 본체를 빌린다. SP/HP/보상/Canon/Chunks/최근·전체 목록/Bounds의 모드·방향·map/piece는 같은 실제 장기 owner를 사용한다. Player는 span을 고정하지 않고 원문 선택 위치에서 현재 base를 조회한다. 구성 자체는 World/Acquire·Player 할당·SP 초기화·난수 재시드를 하지 않는다. 독립 typed Session은 기존 지역 본체/Player/방송 계약을 유지한다.

장기 본체에 Session이 만든 효과 함수를 그대로 옮기지 않았다. 현재 효과25개는 Session이 소유한 별도 대여를 weak로 조회한다. 각 호출은 대여 lock→실제 전체 Runtime pin→함수 사본→실제 호출 순서이며 예외 복귀까지 대여와 pin을 보유한다. 기록기2개는 장기 Replay의 현재 객체를 직접 조회한다. body는 State/backing보다 먼저 해제하고 Storage→Gameplay alias와 참조 순환을 만들지 않는다.

실제 Runtime 공개는 기존 Gameplay의 동일 경계에서 Session의 World와 건설 효과에 같은 pin을 전달한다. body 설치 전에 공개된 pin도 이후 대여에 전달한다. 생성/직접 로드 중에는 실제 호출자가 보유한 수명을 사용하고 공개 후에는 실제 전체 owner를 pin한다. 명시 State 반납은 실제 World/Host 반납이 정상 복귀한 뒤 큰 건설 효과 대여만 끝낸다. 일반 최종 소멸도 실제 Host drain 뒤 대여를 반납하며 본체/State는 Client에 남긴다. 중복된 살아 있는 효과 대여나 다른 backing 재결합은 거부한다.

Release/x64 NetstormCpp·NetstormRoot·세 집중 대상은 종료0/경고오류0이고 최종 고정8소스가 불변이다. 최초 빌드 후 제품7파일은 바꾸지 않았다. 첫 검사에서 새3개는 World 없는 Native 환경의 현재 카메라/지도 view 장치가 누락돼 실패했다. 검사에 명시 view 입력만 추가했고 실제 출력·큰 효과·전송 금지는 유지했다. 최초 빌드/3실패와 최종 결과를 각각 보존했다.

최종 **비MP16+35+명시17=68검사 실패0/신규3개**다. 새 검사는 World·Player 구성 전 본체, 같은 backing/주소와 상태 보존, 선택한 효과가 외부 대여/전체 owner를 반납하고 예외를 내도 복귀까지 수명 유지, 다음 대여 선택, 최종 무순환 해제를 확인한다. 설정 효과에서 중단해 실제 Player 생성/패킷 처리로 들어가지 않는다. 결합 Notice/패킷2개는 제외했다. 정적18조건과 기존 원문C/PE10함수를 대조했다. 소스 계약 확인·부품 검사·GUI의 범위를 구별한다.

최종 클론 GUI는 Tutorial4 정상 생산·건설 요청→실제TYPE75/owner1 생성·600frame 대기, 같은Client800×600 Edit TEST01→메인 복귀2회를 확인했고 모두 정상 종료0이다. 원본/CD/fixture·AGENTS 포함 보호2800파일과 격리 설치 자료/실행 파일은 불변이다. 원본 게임·신규MP/패킷/통신·포획/희생·특정 공격·승리·공중3종·Test Battle·사용자 수동 지정 검사는 실행하지 않았다. 건물 완성이나 모든 삭제 분기/전체 동일성의 검증으로 확대하지 않는다.

**전체 NoWorld 기능을 완료한 단위는 아니다.** 현재 효과25개는 실제 Session 대여가 없으면 명시 미연결이다. Session이 필요 없는 Client 설정/표시/진단 장치의 NoWorld 소비, 큰 AI/CarrierPreDestroy→ReplenishNuggets/실제 너겟 배치·다른 Form/일반 삭제·전용 Regular, Source/listener/등록표/Poll·63F·Snapshot/debugBurst·지연큐/JoinTestBattle는 남는다. M6/M7/M8·K/S/B도 미완료이며 태그25/활성88/무태그63F·큐OFF·최신ZIP1092를 유지한다.

마지막 정밀 디컴파일은 Client2026-11-03/R2026-11-07/HJOW-Athlon으로 현재 호스트와 같으며 정밀 소스 변경은 없다. [검증 보고서](../../cpppj/construction-body-owner-20261109.json), [1195 삭제 bool/Runtime](cpp-construction-destroy-runtime-20261109.md), [1194 상태 owner](cpp-construction-client-state-20261109.md), [현행 인계](../../LEFT_JOBS.md)를 참조한다. 원문 함수 전체 분리 범위·빌드/최초 실패/최종 검사/GUI/소스 고정/보호 해시는 `extracted/cpp-resume-1197/`에 있다.
