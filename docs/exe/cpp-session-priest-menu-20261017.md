# 실제 Session의 사제 메뉴·건설 선택과 부모 내용물 통지 — 2026-10-17

현재 PC와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-16**이다. 새 디컴파일 내보내기·원본/클론 게임 실행은 없다. 기존에 독립 복원한 사제 메뉴를 실제 GameWorld가 소유하는 같은 Session의 저장소와 실제 내용물 생성에 연결한다.

## 같은 Session의 메뉴와 선택

`GameWorld::Session()`은 표시/선택이 사용하는 실제 캠페인 수명을 빌린다. 값형 옛 어댑터에서는 명시 거부한다. `RawCampaignSession::MakePriestMenu`는 같은 Pool/TYPE/프레임/9Player, 사제 상태, Host의493/25d 사건, 현재 Player 기술, 실제 contained 조회·생성, 현재 TotalMade 허용, 비용 및 분산 SP를 연결한다. UI/번역·기본 Carrier 메뉴·Tell·실제 커서 취득만 caller가 공급한다. 반환 메뉴는 Session보다 먼저 해제한다.

`RawPriestMenu`의 기존 독립 입력을 유지하면서 실제 전역 reader를 추가했다. 로컬 owner는 같은 PlayerWorld의00540c70, Workshop 조건은 core.ownerMode.battle의00594fbc, 편집은 로더005c85a4, 미션은 globals.missionPost의00594fa4를 **각 원본 접근 시점에** 읽는다. 오래된 논리 필드 값을 공급해도 실제 reader가 우선한다. 0055a480 특수 모드를 challenge00594fc8과 같은 값으로 추정하지 않는다.

0052f7e0의 TYPE 제외 목록은 Session의 별도 장기 저장소로 두고 공간 앵커의 추가 SID 목록과 섞지 않는다. 메뉴·선택은 기술 체인/허용/SP를 복사하거나 별도 가상 미리보기로 대체하지 않는다. 원본 관문 뒤 실제 Form6이 사제 부모에 부착되고 그 SID/160 플래그가 실제 커서 소비자 경계로 전달된다. **이 검사에서 RawCursorLifecycle.Take 자체를 실행한 것은 아니다.** 원본처럼 선택 관문만으로 SP를 차감하지 않는다.

## 실제 생성에서 드러난 Carrier 부모 통지

첫 통합 검사는 실제 사제에게 Form을 부착할 때 미연결 부모 통지에서 실패했다. `RawCarrierContentNotice`에 **PATCH429560 전체**를 복원하고 실제 VT+24가 이 진입점인 Walker/Carrier/사제만 분배했다. reason4를 통째로 무시하는 우회는 사용하지 않는다.

원본 순서대로 savedGame/로딩→폭탄의 번개/음향/지역 숫자 표시→포획 소리→현재 kind/상태/로딩의 Carrier 검사→사제 해방의 genus/타입 조회와 소리를 처리한다. 외부 효과 뒤 바뀐 내용물/부모·전역은 다시 읽고, TYPE+6c를 읽어 보존한 값은 new/생성자 뒤 재조회하지 않는다. new0과 생성 결과0은 별도로 처리한다.

실제 41cc50→497220은 **월드 float의16/11배+0.5→CRT signed64 절삭의 하위32비트→현재59a91c/920 원점 차감**이다. 번개 인자는 화면 정수이며 시작y는0이다. Source에서 빠진 타입/인자를 실제 PE 명령으로 확인해 반영했다. 같은 GameWorld의 SetParticleView가 현재 화면 원점을 공급한다. 005412c8의 실제 초기값은 **rainWalker156**이며 altar164가 아니다. 같은 현재 이동 타입 전역을 참조한다.

Session은 현재 중심·Carrier 검사·타입 공간 조회를 실제 기존 몸체에 연결한다. 새 미션의 savedGame=false와 PE 초기 bombBlocked0을 보관한다. 폭탄/포획의 표시·음향·TYPE 추가DWORD·할당/생성/타이머·genus 조회는 실제 서비스가 선택될 때 공급해야 한다. 이 미완료 경계를 빈 성공으로 바꾸지 않는다. 일반 신전 건설 Form의 원본 무효과 분기는 실제 전체 body를 거쳐 정상 반환한다.

## 독립 관찰·실패와 검증

- 새 원문3은 **1,731고유 입력 / 두 정밀도3,462정상 반환 / 명령·경계 관찰404,266회 / 차이0**다.429560·TYPE lookup/wrapper·41cc50/497220/CRT를 실제 실행한다. 중심·번개·음향/new/생성자/타이머/공간/Carrier 검사는 인수 관찰 경계다. 두 raw50바이트·현재 전역9DWORD·전체 효과 인수/순서·ABI/x87/SEH/허용 실행·쓰기를 대조한다. 모든 효과 종류의 실제 진입도 확인했다.
- 초기 원문1은 **제가 관찰 도구에서005c8464 풀 주소를 초기화하지 않아** 다른 주소를 읽은 무효 관측이었다. 무효 fixture/report는 extracted의native1-invalid로 따로 보존하고 정상 근거/합계에서 제외했다. 풀 주소 assertion과 SEH DWORD 외NULL 페이지 읽기 거부를 추가했다. 원문2는 실제 투영 helper497220 미허용에서 중단했고 정상 근거에 넣지 않는다. 기대값을 복원 C++ 출력으로 만들지 않았다.
- 기존 사제 메뉴905개는 과거 부분 export2개가 현재 PC에 없어 old --verify가 실패했다. 실행기는 원래부터 현재 정밀 함수표만 사용하므로, 쓰이지 않는 과거 export SHA는 **이력으로 보존**하고 기존 fixture를 덮어쓰지 않는 --compare를 추가했다. 현재 PC에서905개 원문을 새로 실행해 기존 모든 행과 일치했고 실제260,365명령이었다. 기존 fixture는 그대로다.
- 새 집중5개(실제 Session3/부모 통지2)와 기존 사제 메뉴4개가 통과했다. Session 전체 집중24개도 통과했다. 실제 보호thewarbegins 미션의 GameWorld와 같은 Pool·부족 SP 거부·실제 Form6 생성/부모 체인/160 인자·차감 없음·기술/TYPE 제외 변경을 확인한다. GUI/기본 Carrier 메뉴·번역·Tell·커서 취득은 이 검사의 명시 외부 경계다.
- 최종 **Release 빌드3 종료0·경고0·오류0**, 전체 내부 **1,494개 모두 통과·실패0·227.26초·CTest종료0**이다. 기존1,489개에 새5개를 더했다. 검사 뒤 설명의 숫자/범위만 교정했고 실행 토큰은 바꾸지 않았다. 기본 시스템 커서는 통과했으나 이전 간헐 실패 원인은 미확정이다.
- 영구 로그 접두사는 `extracted/goal-session-priest-menu-20261017-`다. 전체 내부 출력은ctest-output.log, 최종 집중은focused3.log/notice-focused3.log, Session 전체는all-session-focused3.log다. 현재 의존31개를 새 보고서로 고정하며 기존8보고서11핀·도구1핀·현재 원문 재검증 뒤 과거 검사 소스1핀을 전후 이력과 갱신했다. 이 작업 이전부터 HEAD와 달랐던 다른 과거 핀은 보존했다.

## 남은 실제 플레이

UberGump의 우클릭 메뉴는 여전히 임시 메뉴이고 Construct는 비활성이다. 실제 Client의 controlled 메뉴/명령 DWORD·원본 GumpForm·커서 Take/위치/수집/표시/내려놓기·일반 건설 확정을 같은 Session에 조립해야 한다. 현재 API는 그때 같은 게임 저장소를 빌리는 연결이다. TYPE 제외 목록의 미션/명령 생산자, 새 Session의 추가 FX 소비자와 실제 커서 수명도 남아 있다. 목표 Early Missions/캠페인1-1~1-2/데모3의 사용자 자연 완주·승패·저장/다음 미션과 전체 동일성은 미완료다. 전체 목표는 active이며 DirectDraw는 이번 범위에서 제외한다.
