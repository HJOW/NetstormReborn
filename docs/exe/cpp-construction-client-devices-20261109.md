# cpppj 1198 — 실제 Client 장치와 NoWorld 섬 조회

제품은 **1198 /11.01**이다. 건설의 설정·벽시계·Renderer 갱신·수량/Bounds 진단·문구 서식·Tell 등 실제 Client 장치7개를 Session 대여에서 분리했다. 첫 메뉴/Session 전에 같은 Storage/Gameplay/Replay owner와 장치 함수를 결합한다. 현재 Renderer와 메뉴는 호출 시 다시 선택하며 새 World/Player/시계나 성공값을 만들지 않는다. 기존 typed 서비스의 설정/시계/표시/진단도 같은 함수 몸체를 사용한다. 큰 게임 효과17개는1197의 실제 Runtime 대여/pin을 유지한다.

현재 장치 집합과 함수 사본은 선택한 호출이 끝날 때까지 보유한다. 콜백 중 장치가 바뀌어도 그 호출의 함수/예외를 보존하고 다음 호출부터 새 집합을 선택한다. 필수 함수가 빠진 재결합은 기존 집합을 바꾸기 전에 거부한다. Client 최종 소멸은 실제 Menu/Session/Host 정리 뒤 장치 함수를 끊고 Renderer 등을 반납한다. 반납 뒤 Session 설정 콜백으로 우회하지 않는다. Tell은 기존 표시 바이트 변환과 `InspectTell→Tell`의 실제 창/튜토리얼 정지 경로를 그대로 사용하며 Meta Tell로 치환하지 않았다.

섬 존재 조회는 같은3204BYTE 기록을 읽는 `RawIslandExists`로 분리했다. 기존 RawIslandRuntime도 같은 함수를 사용한다. 원본 PATCH0046ee30/46ee50와 CD004bd480/4bd4c0/4bd4f0의 전체 C/PE를 대조해 signed count 관문과 비정규 존재DWORD 반환을 보존했다. 건설 어댑터에서만 기존처럼 bool로 변환한다. 물리100기록 밖 접근은 기존 호스트 경계처럼 거부하며 조회는 값을 쓰거나 Pool/Player/모드/효과를 요구하지 않는다.

Release/x64 NetstormCpp·NetstormRoot·세 집중 대상은 종료0/경고오류0이고 최종 고정11소스가 불변이다. 최초 빌드는 시작부에서 존재하지 않는 `buildReplay_`를 참조해 실패했다. 기존 서비스와 같은 `trace_->buildReplay`로 교정했다. 그 성공 빌드 뒤 제품8파일은 불변이다. 최초 집중 검사에서는 새 설정 검사가 `ConfigInterface::Set`의 반환값을 성공 여부로 잘못 해석해1건 실패했다. 원문4414d0/CD4a9930·기존 헤더/검사에 있는 “이전 키 존재 여부” 계약에 따라 최초 삽입 기대값만 false로 고쳤다. 최초 컴파일 오류와 검사 실패 원시를 보존했다.

최종 **비MP27+35+명시17=79검사 실패0/신규3개**다. 기존 양판본 IslandRuntime8검사를 집중 대상에 포함했다. 새 검사는 같은 실제 ConfigInterface/초기화한 비활성 Player9 기록으로 World/Runtime 없는 생성 입구의 현재 설정/fallback·시계 선택·전역 난수/본체/상태 보존, 장치 교체 중 수명/예외·반납 뒤 우회 금지, 섬 DWORD/signed count/물리 경계를 확인한다. 기존 Runtime pin 검사는 큰 MayPlace 효과로 유지하며 실제 생성/패킷 이전에 중단한다. 결합 Notice/패킷2개는 제외했고 신규 MP 실행은 없다. 정적18조건도 통과했다.

최종 클론 GUI는 Tutorial4 정상 생산·건설 요청→실제TYPE75/owner1 생성·600frame 대기, 같은Client800×600 Edit TEST01→메인 왕복2회를 확인했고 모두 정상 종료0이다. 원본/CD/fixture·AGENTS 포함 보호2800파일과 격리 설치 자료/실행 파일은 불변이다. NoWorld 전체 삭제·고유 Tell/진단 분기·건물 완성·Test Battle·MP·포획/희생·특정 공격·승리·공중3종·사용자 수동 지정 검사는 재현하지 않았다.

다음은 `RawSurfaceStorage::DefaultProcessAssetHooks/ProcessFormDispatch`의 실제 자산 가상 삭제/Pre/Post·다른 Form/전용 Regular, 큰 AI/CarrierPreDestroy→ReplenishNuggets/실제 너겟 배치다. Source/listener/등록표/Poll·63F·Snapshot/debugBurst·지연큐/JoinTestBattle와M6/M7/M8/K/S/B도 미완료다. 태그25/활성88/무태그63F·큐OFF·최신ZIP1092를 유지한다.

마지막 정밀 디컴파일은 Client2026-11-03/R2026-11-07/HJOW-Athlon으로 현재 호스트와 같고 정밀 소스 변경은 없다. [검증 보고서](../../cpppj/construction-client-devices-20261109.json), [1197 장기 본체](cpp-construction-body-owner-20261109.md), [1195 삭제 bool](cpp-construction-destroy-runtime-20261109.md), [현행 인계](../../LEFT_JOBS.md)를 참조한다. C/PE 전체 범위·빌드/최초 오류/최종 검사/GUI/보호 해시는 `extracted/cpp-resume-1198/`에 있다.
