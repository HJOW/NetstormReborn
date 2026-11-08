# cpppj 1187 — 장기 Damageable와 간헐천 수명

제품은 **1187 / 11.01**이다. Native Session의 Damageable 전처리·내용물 해방·후처리와 Geyser 수명을 같은 `RawSurfaceStorage` 소유자로 옮겼다. 같은 Pool·TYPE·spot·내용물 검색 전역·HP/공통 Pre/Post·Host·Dude population을 사용하며 독립 typed Session은 기존 지역 소유권을 유지한다. 새 World·SP·난수·seed를 만들지 않았다. 전체 M6 완료가 아니다.

현재 사제 TYPE와 후처리 battle/population/피해 사건 DWORD를 원문의 소비 위치에서 읽는다. Native Geyser는 현재 가상78 getter→물리 frame DWORD 저장→표시→현재 extra→AI→현재 signed 개수/목록 압축→Damageable 순서다. 실제 현재 목록의 중복을 모두 제거하고 사용하지 않는 꼬리는 보존한다. 음수 개수는 목록 물리를 읽지 않는다. typed의 기존 사전 검사 계약은 유지한다.

원문 `0044b4b0`, `0044b9e0`, `0044b830`, `00485580`, `00485540`, `0040ea00`의 전체 C/PE 범위를 보존했다. Geyser 가상표 `0050da88+78`의 실제 PE 진입은 `004acfe0`이며 현재 TYPE+128을 읽는20바이트 leaf다. Ghidra 함수 metadata가 없어 C를 새로 만들지 않고 실제 포인터 바이트·RET 경계·역어셈블·SHA-256을 별도 보존했다. 마지막 완성 디컴파일 호스트는 현재와 같은 HJOW-Athlon이다(Client2026-11-03, R2026-11-07). 디컴파일 소스 변경은 없다.

큰 파편/Impact·사제 생성·Dude 드롭·받침 정리·금 간 다리 가상 삭제는 선택 위치에서 현재 Session/전체 Runtime을 pin해 기존 실제 모듈 종단을 호출한다. 비영 AI/피해 Regular도 기존 실제 종단이다. 이 큰 하위의 전체 NoWorld owner, ConstructionRuntime의 넓은 RawWorldCore 의존/섬 정리, Carrier dependentFound→보충과 다른 Form·일반 가상 삭제 분배는 남는다. 없는 큰 하위를 빈 성공이나 새 World로 대신하지 않았다. 태그25/활성88/무태그63 및 delay queue 비활성은 그대로다.

NetstormCpp/NetstormRoot/집중 대상 Release/x64 빌드는 종료0·컴파일/링크 경고오류0이며 빌드 중16소스를 고정했다. 첫 빌드 종료 뒤 새 Post 검사의 잘못된 기대만 정정해 재빌드했다. battle=false에도 공통 Post가 항상 실행되는 실제 계약을 반영했으며 제품 동작을 바꾸지 않았다. 첫 빌드와 고정 해시는 initial 이름으로 보존했다. 비MP20검사가 실패0이고 신규7검사와 Native 전처리1632행·Geyser2016행의 기존 독립 기계어 재생을 포함한다.

최종 제품으로 격리 Tutorial4 생산·건설 요청 후 실제 TYPE75/owner1 객체 생성과600프레임 대기를 확인했다. 같은 Client의800×600 Edit→TEST01→메인 복귀를2회 확인하고 정상 종료했다. 설치 자료·EXE·고정16소스와 AGENTS 포함 보호2800파일이 불변이다. 실행 stderr의 기존 시작 자료/캐시 부재 문구는 보고서에 보존했으며 게임/검사 실패와 구별했다. 전체 솔루션·34경로·자연 완주·전체 NoWorld 검증으로 합산하지 않는다.

새MP 단위/패킷/게임/통신·원본 실행·포획/희생·특정 공격·승리·공중3종 전용 실행과 Test Battle은0이다. 다음은 나머지 NoWorld 파생/큰 owner와 Source/listener/표/Poll·63잔여F·큐·Snapshot/JoinTestBattle다. 최신 ZIP1092는 이번 제품 ZIP이 아니며 M8에서 마감한다.

[단위 보고서](../../cpppj/damageable-lifetime-owner-20261109.json) · [현행 인계](../../LEFT_JOBS.md). 원시는 `extracted/cpp-resume-1187/`이다.
