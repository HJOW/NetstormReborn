# 실제 NSInfo 전체 생성과 공통 UI — 단위399 / 2026-10-24

시작은 `53a1ebf9 / 20261024-21 CP`다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 새 전체 디컴파일과 원본 게임 직접 실행은0이다. 기존 NSInfo 두 소스와 검사 자료는 이전 중단 커밋에 있었지만 CMake에 미등록이었다. 이번에 실제 제품 라이브러리와 검사 실행 파일에 연결했다. 전체 목표는 미완료다.

## 실제 소유자와 생성 경계

[ClientNsInfoGlobals](../../cpppj/src/client/ClientNsInfoGlobals.h)는 current/zone/선택 서버와 이름80BYTE를 Client 수명에 보관한다. 선택 서버는 BSS0에서 시작하고 원문 Interpret Options의 `curServer` ReadInt가 현재값에서 갱신한다. 정상 보호 시작 서버 파일의 두 행을 읽기만 한다. 현재 이름 조회는 LAN 번역 시 strcpy의 끝 이후 보존, 유효 행에서80BYTE 초기화 후 이름 추출, 무효 번호에서 기존 버퍼 보존을 구별한다. 전체 목록 관리·갱신·저장/긴 line-reader·파일 부재 시 원문 초기 저장 부모는 미완료다.

[RawNsInfoContext](../../cpppj/src/client/RawNsInfoContext.h)가 실제 gameplay/playerKnown·월드 모드와 같은80칸 형식 문자열 고리, 번역, 현재 배경/색/TYPE를 빌린다. `HudNsInfoModeView`는 기존 DWORD Native 검사와 실제 bool 모드를 소비 시점에 직접 읽으며 별도 동기화 미러를 만들지 않는다. 같은 배경 전역은 RawButtonControls의 기존 생산 전역에서 조회한다.

[RawMenuControls](../../cpppj/src/client/RawMenuControls.cpp)는 실제 `HudNsInfoCreate`의 NSInfo/PlayerInfo8/BattleInfo/특수 버튼 생성과 일반 Native Attach를 공통 Gump·StyleText·Buffer·Slider·버튼 계층에 연결한다. 별도 등록표나 빈 자식으로 대신하지 않는다. 상위 생성의 직접 할당50노드는 실제 하위 Widget을 포함하면 **Gump130개**, 버퍼/Slider는 각12개다. 한 줄 문자는9개, 특수 버튼8개, Start 돌 버튼8개이며 형식 문자열 고리는1회 사용한다.

실제 Client는 기존 월드 UI 설치 경계에서 숨김 NSInfo를 생성하고 원문 Uber가 쓰는 전체 화면 크기/플래그/부착을 적용한다. 기존 미션 안내 재구성에서는 유지하고 월드/페이지 이탈 시 정리한다. 이 경로를 새 Uber 전체 생성·교체 완료라고 판정하지 않는다. Session 이전의 전체 실제 생성은 별도 통합 검사로 확인했다.

최초 통합 검사에서 번역 실패 시 부모 필드 대입/부착 전의 PlayerInfo·문자·버튼이 남는 문제가 있었다. 생성자가 직접 할당한 실제 토큰을 추적해 등록된 노드는 같은 자식 우선 Kill/Cleanup으로 반납한다. new 성공 뒤 기본 생성자 전의 미등록 저장소는 등록표에 없음을 확인한 뒤 호스트 소유 기록을 반납한다. id0으로 루트를 Kill하지 않는다. 이는 명시 Close의 회복 경계이며 원본 전체 EH unwind 복원의 증거는 아니다. 원문처럼 죽은 current 전역은 임의0으로 쓰지 않으며 생존 getter와 구별한다.

## 검증

- build1(82459)은 제품/검사의 NSInfo6기호 링크 누락으로 종료1이었다. 미등록 두 모듈을 등록했다. build2(32665)는 신규 검사에서 전방 선언만 보던 헤더 누락으로 종료1이었다. 실제 정의를 포함한 build3/4/5(33379/50101/29838)는 모두 종료0·관측 컴파일 경고오류0이다.
- 최초 집중1은 기존 Native4 통과 뒤 실제 부분 정리3check가 실패했다. 위 제품 정리 수정 뒤 최종 집중2는 **신규8+기존71=79검사/실패0**이다. 전체130노드/12실제 버퍼·Slider/공통 등록·파싱된 제목·숨김 픽셀 동일/두 root 동시 생성·교체/페이지 정리/번역 중단과 메타맵 관문/실제 Client 옵션·서버80BYTE를 검사했다.
- 현재 호스트/PE/정밀 소스 SHA를 확인한 별도 읽기 전용 Unicorn 재생1은 **생성170+일반 부착310=480행**이 보존 자료와 원시 BYTE·명령 수까지 전부 같았다. 이전 VM 고정 main/쓰기 경로는 호출하지 않았고 기존 fixture는 변경0이다. 상위 생성은 공통/문자/버튼/서버 조회 등의 외부 장치를 지정한 관측이다. C++의 실제130노드 조합 및 모든 하위 본문의 독립 Native 검증과 구별한다.
- 실제 GUI1(50503)은 **195.13초/종료0**이다. Early2 A~I, 신전1/워크샾1, 발사기4→3, 회수1·환불75·SP10000→3075, 자연 완료→Tutorial3 안내·실제SP2000이 통과했다. 상태 주입0/기존완료1→1이다. NSInfo78행 모두 root1/Player8/Battle1/특수 버튼8/한 줄9/스크롤12이상이며 실제 현재 토큰은50→748이었다. 전역/물리78행도 같은 실제 저장소이고 물리 대여1→2다. goal/nextBriefing BMP를 직접 확인했다. 다음 예산 표시1700은 단계 갱신 중이며 실제SP2000과 구별한다.
- 전체 CTest1(82169)은 **2710검사/실패0·종료0·324.78초(wrapper)**다. 집중/GUI/전체의 최종 바이너리가 같고 원본1394개는 같은 기존 기준과 일치한다. AGENTS/originals/CD/Native 추적 변경0이다. 배포 스크립트는 d 전체를 포함하며 새로 읽는 !rootservers.dat는 제외 대상이 아님을 확인했다.

최종 제품 SHA256: `43c6be89acd6efb844bb6182ee86f17c740efb837df7139e4d1739eb24a0b80a`

최종 검사 SHA256: `f6ec0d12a7b5cae47fb74e50ced9e59676cddffcd812886d4dd208aea1c0d030`

[제품 보고서](../../cpppj/nsinfo-client-runtime-20261024.json)에 변경 소스15/공통 참조11/보존 Native11/원시122파일 SHA, 최초 실패/수정 뒤 관측/새 Native 재생과 한계를 기록한다. 원시는 `extracted/cpp-nsinfo-399-*`다.

## 남은 연결

전용 메타맵 배치·입력·특수 버튼 Draw, 부모/하위 전체 EH와 서버 목록 관리 전체는 미완료다. 이번 정상 숨김 생성의 통과를 그 증거로 확대하지 않는다. 다음은 **ChatWindow/Console/TextEntry/공유 로그 버퍼·callback과 HTML 전체 공장**, 그 뒤 실제 Client의 새 Uber 교체·State/Core/Pool 순서 접합이다. ChatWindow/TextEntry 준비 자료의 CMake 등록 여부부터 실제로 확인해야 한다. 모든 Early/캠페인/Demo 사용자 완주와 Edit/언어별 시나리오 전체·완전 동일성,393 Wide SaveOptions 원인은 남으며 /goal은 활성 상태다.
