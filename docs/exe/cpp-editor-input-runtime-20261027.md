# 단위468 편집 작업본 재진입·클립보드와 화면 회귀 수정

2026-10-27 / HJOW-Athlon(192.168.0.94). 마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이며 정밀 export를 변경하지 않았다. 사용자는 현재 작업과 서브 AI 작업 마무리 후 인수인계·로컬 커밋·목표 일시중지를 요청했다. 전체 미션·편집기·원본 동일성 목표는 미완료다.

## 적용한 동작

`EditorCatalogWorkspace`를 실제 목록과 Editor의 두 lazy 설치, 현재 이름 변경과 Save as 경로에 공유했다. 보호 원본의 loose Fort를 기존 사용자 작업본과 readonly 속성을 보존하며 한 번 이관하고, 이후 실제 work 폴더의 OS 열거만 원문 목록에 전달한다. 첫점 이름 변환·중복 물리 항목·디렉터리·명시 부재 한 경로·수락50개는 기존 원문 처리기가 담당한다. 새 이름의 우선 표시나 목록 확장은 없다.

완료된 이관 키가 없어지면 원본 disk/TAFF로 되살리지 않는다. 장부는 work 바깥의 `<work>-state`에 Pending/Completed·source/work identity를 기록한다. 존재 장부의 읽기 실패·손상·공유 잠금·중단 Pending을 새 빈 장부로 취급하지 않는다. 기존 `BaseFileSystem::TryRead` 의미는 보존하고, 미이관 archive 읽기에 별도 const `TryReadArchiveOnly`를 사용한다. 절대 source/work 경로는 전체 상대 경로로 매핑하며 외부 경로를 basename으로 추측하지 않는다.

실제 H: exFAT GUI에서 장부 `MoveFileExW`가 Windows 오류5로 실패했다. 같은 atomic 교체만 제한적으로 재시도하고, 계속 거부되면 old 장부와 새 pending 원문을 보존한다. 실제 외부 reader가 반납되는 경우와 영구 공유 거부를 별도 C++ 검사로 확인했다. 직접 덮어쓰기·old 장부 선삭제·권한 우회는 추가하지 않았다.

CF_TEXT 원문 read/write와 실제 Win32 장치를 최초 HUD Chat·후속 Save as의 같은 TextEntry 문맥에 연결했다. Native880관측과 최초 문맥의 now/Console 보존을 검사했다. 실제 OS 클립보드 복사·붙여넣기는 미검증이다. [클립보드 상세](cpp-text-clipboard-20261026.md)를 본다.

## 사용자가 신고한 화면 회귀

Edit 중앙에 titleMenu GIF가 남던 원인은 State1의 원문 `004b7016` 첫BYTE 초기화 누락이었다. `PrepareNativeFortState(editing=true)`에서 cleanup(0)/HUD 재생성 전에 `00565e70`의 첫BYTE만 지운다. State3이 먼저 선택한 titleMenu/victory/defeat는 `editing=false`에서 보존한다.

메인 메뉴는 host canvas에 버튼 표면을 먼저 그린 뒤 Native GIF가 덮고, RenderText 글자만 마지막에 그리는 순서였다. 원문 MainMenu container -80의 자식 stone -83·배경 -82를 `AddDepth`로 맞추고, 두 실제 노드를 GIF와 같은 Collect/Sort/Draw에 한 번 제출한다. 공통 그리기는 명시한 메인 버튼 가족에만 적용한다. 기존 독립 메뉴의 출력 범위와 외부 HUD 등록 카운터는 보존했다.

canvas의 `DrawPlan`이 부수적으로 채우던 글자 치수 의존도 제거했다. 실제 생성 직후 같은 원문 글꼴 측정기로 폭/높이를 저장한다. 새 픽셀 검사는 보호 titleMenu/A01 자산의 글자 밖 질감, 눌림 bevel, clip/pitch padding, 현재 Font0의 가운데 글리프 전체 표면을 확인한다. 실제 Main→Edit→Main→Edit, Tips, Options 캡처도 검토했다. 앞선 상태·저장 검사만으로 이 시각 회귀를 놓쳤으며 전체 외관 동일성을 증명한 것으로 취급하지 않는다.

## 실패와 검증 구분

최초 source-first/append·논리 dedup 설계, 신규 Directory의 성공한 빈행 기대, CRLF patch 적용, 없는 검사 필터명, private Measure 호출, 장부 오류5, 메인 버튼 공통 그리기의 독립 메뉴 침범 실패를 원시 로그에 보존했다. 기존 C++ 검사 본문과 Native fixture 기대값을 약화하지 않았다. 신규7개 설계 검사는 원문 의미에 맞는11개로 교체했고, 외부 reader2개와 메인 픽셀1개를 추가했다.

최종 빌드·집중·전체·GUI·보호 입력 및 기존 등록 보존 결과는 [실행 보고서](../../cpppj/editor-input-runtime-integration-20261027.json)에 기록한다. Source4/6의 이전 전체 통과와 Source9의 시각 수정 중 독립 메뉴 실패는 최종 제품 결과와 구별한다. 검사 중 별도 webpj 작업의 `8fafa298`·`4067e507` 커밋이 AGENTS.md와 LEFT_JOBS.webpj.md를 변경했다. 변경은 webpj의 Edit 제외이며 cpppj의 Edit 범위는 유지된다. 초기 raw guard 거부와 시작/최신 커밋·현재 파일 SHA를 별도 감사로 보존했다. 현재 AGENTS 본문은 해당 최신 커밋과 같고 이 cpppj 작업에서는 AGENTS나 webpj 인계 문서를 쓰거나 스테이징하지 않는다.

## 재개점

미적용 서브 AI 준비본은 [보관 안내](prepared/README.md)의 다섯 묶음으로 인계한다. NewFort는 Subscriber/공유 이름표·빈 Image를 먼저 조합해 C++ 빌드·검사한 뒤 실제 NewEditor 설치/외곽·tail·State 확정·AddIsland 큐를 연결해야 한다. Add Island는 전용 MetaImage/Bitmap/목록·창 수명 구현이 남았다. 임시 창이나 가짜 빈 Fort로 경계를 통과시키지 않는다.

Native3 승패·Intro39·기존 저장상태 복구, 전체 State7, Test Battle/편집 복귀·소유자/지형 도구·언어별 시나리오, Early3~6·Priest9·Campaign16·Demo3의 자연 완주 및 원본 표시·음향 대조는 계속 남아 있다. Early3는 미연결 다리를 수정한 뒤 채취/귀환과 C까지 관찰했으며, 전체 실행·완주 성공으로 기록하지 않는다. 다음 단계는 실제 Workshop 메뉴의 Sun Disc Thrower71과 Wind Generator75 생산 추가다.
