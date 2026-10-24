# Client의 실제 월드 물리와 Session 없는 미니맵 — 2026-10-24

실제 Client가 자산 초기화 뒤 `RawSurfaceStorage`를 소유하고 CampaignClientServices→Session→RawSurfaceWorld가 같은 Pool·Hash·spot·현재 TYPE·프레임/셰이프·소유자 모드를 사용하도록 연결했다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 시작 HEAD는 `b0e6bb2b`다. 새 디컴파일·Native 도구 실행0, 실제 새 게임 실행1회다.

월드 모듈은 물리를 단일 대여한다. 중복 대여와 잘못된 판본/배열 크기/타입은 기존 물리 초기화 전에 거부한다. 대여는 모든 모듈보다 먼저 확보하고 모듈/독립 Kernel 반납 뒤 해제한다. 실제 Session은 기존 Client Kernel을 계속 사용한다. 다음 월드는 기존 SID Reset(0), Hash Reset, spot 초기화를 사용해 배열 주소와 Pool 삭제 기록을 유지한다. 현재 TYPE의 수정도 초기 파일 사본으로 덮어쓰지 않는다. Session의 자산 입력/일부 기존 모듈 캐시를 모두 없앴다고 주장하지 않는다.

미니맵은 장기 물리의 강한 소유권과 짧은 Pop/Unpop 통지를 구별한다. Impl은 Session이나 전체 Core 사본을 보관하지 않는다. 같은 실제 저장소에서 Session 없이 전체 미니맵을 생성·등록·그리기할 수 있고, 같은 Pool/Hash/TYPE/spot/모드의 실제 월드에만 통지를 연결한다. 중복/다른 물리 바인딩은 거부하고 월드 반납 전에 UnbindWorld로 통지를 해제한다. 실제 기존 Client의 미니맵도 이 경로를 사용한다.

신규8검사는 두 판본의 실제 자산과 같은 배열 주소, 다음 대여의 Pool/공간 초기화 및 명시 TYPE 입력 보존, 중복/잘못된 월드의 부분 효과, 소유권 유지, 두 실제 Session 연속 적재, 부분 생성 실패/외부 Kernel 프로세스 보존, 다른 archive 정체성 거부, Session 이전 미니맵과 실제 Pop/Unpop/bitmap·공통 픽셀·월드 교체를 확인한다. 부분 실패는 deferExistingEditorFile의 잘못된 캠페인 지정이 첫 객체 전에 실패하는 경로이며 모든 중간/후반 실패를 증명하지 않는다. CD10.72의 검사는 물리 대여 범위이며 CD 게임 실행은 아니다.

build1~3은 모두 종료0·관측 경고오류0이다. 첫 집중 검사는 신규 물리4/Session3이 통과했으나 미니맵1검증이 실패했다. TYPE94도 일반 지면 색으로 표시될 것이라는 검사 가정이 틀렸다. 원문 RawMinimapRaster는 일반 지면을 isle97/noIsland157로 제한한다. 실제 표시 대상157로 검사 입력만 교정했고 **제품 SHA는 그대로**다. 최초 실패와 수정 전 검사 도구를 보존했다. 최종 집중 검사는 **신규8+기존24=32개·실패0**이다. 기존 미니맵/dirty Native fixture4개도 수정하지 않았다.

같은 제품으로 실제 Early2를 **195.78초** 진행했다. A~I, 신전·워크샾, 발사기4→3, 회수1회/환불75, SP10000→3075, 자연 완료와 Tutorial3 첫 안내/실제 SP2000이 통과했다. 상태 주입0·기존 완료값1→1이며 영구 진행 저장의 증거로 합산하지 않는다. 캡처의 다음 예산 숫자는 원문 단계 갱신 중인 표시이며 실제 SP와 구별한다.

새 읽기 전용 `shared_surface_storage` 행 **78개 모두** Pool/Hash/TYPE/spot 정체성이1이고 실제 대여가 활성이다. Early2에서는 대여1, Tutorial3에서는 대여2로 전환됐다. 목표/다음 안내 BMP를 직접 확인해 새 지형·사제·워크샾과 미니맵 출력도 확인했다. 이 캡처 확인을 원본 전체 픽셀 동일성 검증으로 세지 않는다.

전체 CTest는 **2695개·실패0·종료0·322.46초**다. 집중/실제 게임/전체 검사 전후 같은 제품·검사 SHA이며 제품은 `d61c46ca9cb8800c75c0c4e9e9c3f3c72273908cbf6881563f97dd03fa065a7b`, 검사는 `5cf5c8d91d292e2cc3fffa9504fcc58f9182c78b9d515f50490571a45bff7473`다. 원본1394파일은 모두 기존 기준과 같고 AGENTS/원본/CD/기존 Native fixture 추적 변경도0이다.

[제품 보고서](../../cpppj/surface-storage-runtime-20261024.json)에 소스18·기존 Native4·실제 도구7·최초 실패/실제 실행/전체 검사 원시 근거의 SHA를 보관했다. 원시는 `extracted/cpp-surface-storage-397-*`다.

이번 Pool 초기화는 현재 호스트의 새 월드 설치 경계이며 원문 NativeState의 새 Uber→Core 선택→Pool Reset 등 전체 순서의 연결을 대신하지 않는다. 새 Uber는 여전히 실제 Client 미연결이다. Runtime/Form/Graph/장부 등 나머지 소유자, 생산 패널의 Session 의존, 전체 HTML/NSInfo/Chat 공장, Edit/커스텀 시나리오와 모든 미션의 사용자 완주·동일성은 남았다.393 원본 경로 Wide 옵션 문제도 미해결이며 목표는 활성 상태다.
