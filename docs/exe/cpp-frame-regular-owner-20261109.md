# 제품1219 — 충돌 anim·썬 디스크의 장기 Regular와 현재 TYPE

제품1219의 구현·제한된 검증을 마감했다. 전체 목표는 진행 중이다.

## 범위와 실제 본문

VT00509758/+5c0045a6e0와 VT0050b570/+5c00469540을 장기 Storage의 실제 Pool/프레임 표·Frame.Set/현재 가상 삭제에 연결했다. Native Session의 같은 두 진입도 이 본문을 직접 사용한다. 기존 typed 인스턴스는 원래 가상표 계약을 유지하고 같은 정적 핵심 본문을 호출한다. 생성/난수/카메라/Session을 임시로 구성하지 않는다.

anim은 현재 TYPE의 event별 글자 구간·signed count를 사용해 실제 프레임을 쓰거나 flags0 가상 삭제 후-1을 반환한다. 썬 디스크는 현재 TYPE의 방향/클러스터 수를 읽어 signed modulo 프레임을 지정하고 payload를 반환한다. 공통 RiftTypeRecord에 빠져 있던 +110 directions/+114 frameCount를 추가해 기존 원본 callback 해석기로 실제 Table 적재 시 초기화했다. 내장 TYPE는 원래0을 유지한다. Native는 Session의 별도 방향 표 사본을 읽지 않는다.

## 원문과 정수 산술

PATCH45a6e0/469540·CD47c810/437290과 실제 TYPE getter4ac550/공통 주소 조회49a840의 정밀 C/전체 함수 PE를 보존했다. 현재와 마지막 정밀 Client양판본2026-11-03·R전용2026-11-07은 HJOW-Athlon이며 새 디컴파일/원문 변경0이다.

469540은 +114를 +110으로 signed IDIV한 뒤 current+1의 DWORD 감김, 두 signed 나머지, SUB/ADD 감김을 실행한다. Native에는 이 순서의 산술을 사용하고 0분모/INT_MIN÷-1 하드웨어 오류만 호스트 domain_error로 나타낸다. 기존 typed NextFrame의 정상 범위 사전검사는 유지한다. 음수/최댓값을 정상값으로 고쳐 성공시키지 않는다.

새 `disc-regular-boundary-1219-x86.tsv`는 제품 산술 작성 전 실제 PATCH 명령·TYPE getter/주소 조회를 독립 격리한 **70입력: 정상47·IDIV오류23**의 최초 관측이다. Frame.Set는 요청 인자 관측 경계이며 실제 Renderer/불법 프레임 표시까지 검증한 자료가 아니다. 기존192입력 fixture를 수정/재생성하지 않았다. 보조 관측 도구의 옛 export 경로 부재·추가 실제 TYPE 주소 함수 누락·나눗셈 오류 VM 재사용 문제와 교정은 원시에 보존했다. 현재 정밀 범위만 사용하고 입력별 VM을 독립 준비했다.

## 검증과 남은 범위

신규 Host/프레임/현재 TYPE/만료 중 자기 Regular 삭제·오류 접두/70개 산술 golden, 기존 TYPE callback metadata22행씩과 충돌/디스크 fixture를 대조한다. 실제 client SID와 원문 억제 플래그·금지 네트워크 callback을 사용하며 새 MP/공중3종 전용/포획희생/특정 공격/승리 실행을 추가하지 않는다.

분배상 연결은39/87VT, 잔여48VT/23함수다. 큰 하위/모든 NoWorld caller·owner/실제 플레이 전체 완료 수가 아니다. 내장 Form 삭제3종·큰 비영 AI·다른 배치/Fort 상위 Finish·Source/listener/표/Poll 등 전수 감사·지연큐/JoinTestBattle·M7/M8·K/S/B의 상위16계획·등록34버그는 유지한다.

## 확정 결과

Release/x64 제품2·집중5대상은 단일 최종 빌드 종료0, 경고·오류0이며 소스/새자료15개를 고정했다. **비MP350/350**(기존328+신규11+추가 기존11)이 통과했다. 신규11은 장기 프레임/현재TYPE/실제 Host/자기 Regular 소멸/오류 접두/70산술 golden과 실제 Table의 원문 metadata22행씩이다. 기존11은 충돌6·디스크2·TYPE metadata/입력3이다.

최초 빌드 C2664는 새 golden 검사의 경로 인자 형식 불일치였다. 동기 읽기 함수에 c_str()로 전달하도록 교정했으며 제품 본문/원문 fixture/검사 기대값은 유지했다. 최초 집중349/350의 신규 만료 사례는 live 부모의 Hash 입력을 빠뜨려 원문 Squid.cpp:1629 found 단언이 검출했다. Host 부모3개의 실제 현재 프레임 저장 단계/Hash 등록 입력을 준비했으며 풀 반납·자기 Regular 소멸·깊이 등 기대는 유지했다.

같은 EXE에서 Tutorial4 정상 UI TYPE75/owner1 실제 생성·요청 증가→600프레임 대기와015 PNG를 확인했다. 같은 Client800×600 Edit TEST01↔Main2회/11관측도 종료0이다. 모든 충돌/발사의 실제 플레이·건물 완성/TestBattle·공중3종/포획희생/특정공격/승리/MP를 검증한 결과는 아니다. 모든 시작 빌드/집중/GUI 핸들이 종료했다. 기존 보호 원본/CD/fixture·AGENTS2800파일 불변, 새 fixture는별도 최초SHA로 고정했다. 원본 게임/MP 실행·push0이다.

- Netstorm.exe: 13949952 bytes, SHA256 `e58ed51bc5412b459d548fa545e221c353cf22aa1330095820ac8decf35d910c`.
- R.exe: 677376 bytes, SHA256 `701388e687878965d7823b8315112be185992b7a6c0a24cd6303d1e5432ed814`.

[현행 인계](../../LEFT_JOBS.md)·[최종 보고서](../../cpppj/frame-regular-owner-20261109.json), 원시 `extracted/cpp-resume-1219/`의 original-evidence/새관측/static-audit/source-frozen/build/tests/GUI/보호 결과를 따른다.
