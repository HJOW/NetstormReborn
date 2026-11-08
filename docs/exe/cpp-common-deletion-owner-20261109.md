# cpppj 1186 — 장기 공통 삭제와 현재 UI·전역

제품은 **1186 / 11.01**이다. Native World/기존 Session caller가 `RawSurfaceStorage`의 같은 `SquidDestroyLifecycle`을 빌린다. 같은 공통 삭제 깊이·현재 TYPE·장부·Graph와1185의 같은 HP/보상을 사용한다. 독립 typed World의 지역 처리기와 기존 Bind 거부 계약은 유지한다. 전체 M6 완료가 아니다.

공통 Pre의 현재 로딩 깊이 DWORD/fort bool·레인 바리케이드 TYPE, 현재 전체 DWORD localOwner·metamap 억제와 같은 noIsland/fakeSurface TYPE를 실제 주소에서 조회한다. 선택된 abstract/buried 객체는 좌표 DWORD 원시 비트를 실제 `RawUserSelection::Remember`에 전달하고, 그 helper가 TYPE flags2&30000 관문과 실제 UI rememberedPoint/mouseType 기록을 수행한 뒤 선택을 해제한다. 지역 placement 사본/pendingPlacement를 Native 성공으로 쓰지 않는다. 일반 fresh 대여 뒤에도 현재 전역 주소만 다시 결합하며 실제 값을 초기화하지 않는다.

현재 Session의 파생 종속 삭제는 World가 소유한 짧은 binding을 Storage가 weak로 빌린다. 선택 호출 동안 실제 전체 Runtime pin을 보유한다. 실제 Host의 State 반납/최종 정리가 끝난 뒤 binding도 분리한다. 장기 AI 통지는 같은 현재 Player/brain의 기존 Client helper를 사용하며 Session this를 장기 공통 처리기에 저장하지 않는다. 선택/전송은 실제 장치를 사용한다. NoWorld의 아직 없는 파생 Release/Destroy와 비영 brain 전체 owner는 다음 범위로 남기며 빈 성공으로 바꾸지 않는다.

같은 Client에서 Edit 진입·복귀를 두 번 반복하는 추가 검사에서 두 번째 진입이 `postPop 현재 목록 연결의 소유/장치 오류`로 실패했다. 기존 목록의 getter는 같은 장기 ClientPlayerRecords의 현재 +0/+4/+8을 조회하여 재할당 이후에도 유효했으나, 재등록 guard가 이를 거부했다. 실패 실행/빌드/검사 원시는 `live-editor-fixed`, `build-before-rebind.*`, `tests-before-rebind.*`로 보존했다. Native 경로에만 실제 같은 C++ 저장소·행의 binding을 명시 재사용하는 `ReuseOrBindFactory`를 연결했다. 다른 Client/행과 독립 데이터는 계속 거부하며 일반 BindFactory의 중복 거부는 바꾸지 않았다. Player의 실제 초기화/재할당과 현재 데이터 쓰기를 생략하거나 복사하지 않는다.

원문 `004b0950`, `004b0840`, `004166e0`, `004d5c60`, `004d5440`, `004637b0`의 전체 C/PE 범위를 보존했다. 현재/마지막 완성 디컴파일 호스트는 HJOW-Athlon이다(Client2026-11-03, R2026-11-07). 디컴파일 소스 변경은 없다.

보완 후 NetstormCpp/NetstormRoot/공통 삭제 집중 대상의 최종 Release/x64 빌드가 종료0·컴파일/링크 경고오류0이다. 비MP24검사(공통 삭제10·Graph8·PlayerRecords6)가 실패0이며, 이 중 새5검사는 현재 억제/owner·UI 기억점 원시 비트/순서·실제 동일 source 재사용/다른 source 거부를 확인한다. 기존 삭제·Graph의 독립 기계어 재생도 포함한다. 최종 제품으로 격리 Tutorial4 생산/건설 요청·600frame 대기와 같은 Client의 Edit→TEST01→메인 복귀를2회 확인하고 정상 종료했다. 보호 파일 중 원본/CD/fixture2799는 작업 시작부터 불변이다. AGENTS.md는 작업 중 개발자 커밋551d27ae가 요약 대상에 webpj를 추가했으며, 내용을 다시 읽고 기준 해시만 반영했다. 에이전트는 이 파일을 수정하지 않았다. 그 기준 뒤2800·설치·EXE·고정17소스 불변이다. 전체 솔루션/34경로/완주/전체 NoWorld 검증으로 합산하지 않는다.

빌드 중 제품 소스는 고정했다. Host 반납 보완 및 Edit 실패 뒤 Player binding 보완은 각각 직전 빌드/실행 종료 후 적용했다. 새MP 단위/패킷/게임/통신·원본 게임·포획/희생·특정 공격·승리·공중3종 전용 실행은0이며 delay queue도 비활성 유지한다. 다음은 NoWorld 자원/파생 삭제·ConstructionRuntime·큰 AI/Carrier owner, Source/listener/표/Poll·잔여 F/큐·Snapshot/JoinTestBattle다. 최신 ZIP1092는 이번 제품의 ZIP이 아니며 M8에서 마감한다.

[단위 보고서](../../cpppj/common-deletion-owner-20261109.json) · [현행 인계](../../LEFT_JOBS.md). 원시는 `extracted/cpp-resume-1186/`이다.
