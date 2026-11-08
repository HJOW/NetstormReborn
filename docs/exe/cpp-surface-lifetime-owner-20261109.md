# cpppj 1189 — 장기 표면 수명과 가상 삭제

제품은 **1189 / 11.01**이다. Native World의 `RawSquidDestroyDispatch`와 `RawSurfaceLifecycle`을 같은 `RawSurfaceStorage` 장기 owner로 옮겼다. Native World는 같은 주소를 차용하고 독립 typed World는 기존 지역 소유권을 유지한다. 같은 Pool·BaseDestroy·Neighbors·Graph·CommonDeletion·Host를 사용하며 새 World/Acquire/난수/값 snapshot은 만들지 않았다. 전체 M6 완료가 아니다.

가상10 삭제는 같은 현재 authority/boss와 실제 editor DWORD 주소를 읽는다. 다리 삭제 전처리는 각 후보에서 현재 연결 TYPE5412c4 전체 DWORD를 읽는다. 기존 typed 값 인자 계약은 유지하고 Native 부모에 현재 주소를 별도로 연결했다. 첫 연결 삭제 효과가 타입 전역이나 다음 후보를 바꾸면 다음 비교가 그 변경을 반영한다. BYTE 타입과 낮은8비트만 같은 DWORD는 같은 타입으로 처리하지 않는다.

지연 낙하 예약은 같은 Host/Pool/TYPE/현재 checking에 직접 연결한다. 큰 이동 유닛 낙하/파편·소리는 현재 World가 소유한 기존 실제 runtime 콜백을 Storage가 weak로 빌린다. 선택 호출 동안 현재 binding과 실제 외곽 Runtime pin을 유지한다. 실제 Host 반납/최종 Drain/World 소멸 뒤 세 지점에서 큰 종단 binding도 분리한다. 장기 Storage는 World/Session this를 강하게 보관하지 않는다. 연결 객체 가상 삭제는 기존 실제 자원 삭제 종단을 사용한다. 아직 없는 큰 NoWorld 하위는 선택 위치에서 명시 실패하며 성공 상수나 새 World로 바꾸지 않았다.

첫 빌드는 새 파일에서 RawBridgeFallParent.h 선언 헤더를 빠뜨려 실패했다. 첫 빌드/고정 소스 원시를 initial 이름으로 보존했고 실제 빌드 종료 뒤 누락 헤더만 추가했다. 이후 최종 제품으로 아래 검증을 수행했다.

기존 표면 통합 장면7개는 최근 Pop의 명시 전송 요청 장치 계약을 연결하지 않아 첫 검사에서 중단됐다. before-pop-observer 원시를 보존하고 typed 기하 장면에 현재 요청을 기록하는 관측기를 연결했다. 원래 flags와 공간 효과를 우회하거나 제품 전송을 바꾸지 않았으며 패킷 작성/송수신은 검사하지 않는다.

원문 `004220f0`, `004221b0`, `00422300`, `00442240`, `00442640`, `00421530`의 전체 C/PE 범위를 보존했다. 현재와 마지막 완성 디컴파일 호스트는 HJOW-Athlon이다(Client2026-11-03, R2026-11-07). 디컴파일 소스 변경과 원본 게임 실행은 없다.

NetstormCpp/NetstormRoot/표면 집중 대상 Release/x64 빌드는 종료0·컴파일/링크 경고오류0이다. 빌드 중12소스를 고정했다. 비MP26검사가 실패0이며 신규4검사는 Native PATCH Pre840행의 독립 PE 재생, 앞선 삭제 뒤 현재 TYPE 재조회, 전체 DWORD 비교, 초기 TYPE 사본이 다른 상태에서 현재 주소를 사용하는 네 실제 Graph/Host/낙하/끝 칸/연결 정리 시나리오다. 기존 섬·다리·가상 삭제의 독립 기계어/통합 검사도 재생했다. MP/패킷 검사 대상을 실행하지 않았다.

최종 제품에서 격리 Tutorial4 생산/건설 요청→실제 TYPE75 객체 생성·600프레임 대기와 같은 Client의800×600 Edit→TEST01→메인 복귀2회를 확인하고 정상 종료했다. 보호2800·설치 자료·EXE·고정12소스는 불변이다. stderr의 기존 인트로 자료/캐시 부재 문구는 보고서에 보존했다. 전체 솔루션·34경로·자연 완주·전체 NoWorld 검증으로 확대하지 않는다.

다음은 ConstructionRuntime의 넓은 RawWorldCore 의존을 실제 소비13필드 뷰로 좁히고 같은 장기 건설 owner를 연결하는 작업이다. 장기 건설 이전 시 Place/PlayerBounds/LocalGeyser의 현재 Player base getter도 함께 연결해야 한다. 큰 파편/낙하·사제/Dude/받침·비영 AI/피해 Regular·Carrier 보충·다른 Form/일반 가상 삭제의 전체 NoWorld owner와 Source/listener/표/Poll·잔여63F·큐·Snapshot/JoinTestBattle는 남는다. 태그25/활성88/무태그63과 delay queue 비활성은 그대로다. 최신 ZIP1092는 현 제품 ZIP이 아니며 M8에서 마감한다.

새MP 단위/패킷/게임/통신·원본 실행·사제 포획/희생·특정 적 공격·승리·공중3종 전용 실행·Test Battle은0이다. [단위 보고서](../../cpppj/surface-lifetime-owner-20261109.json) · [현행 인계](../../LEFT_JOBS.md). 원시는 `extracted/cpp-resume-1189/`이다.
