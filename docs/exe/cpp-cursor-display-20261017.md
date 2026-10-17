# 커서 공유 전역과 수집·표시 연결 — 2026-10-17

현재 PC와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-16**이다. 새 디컴파일 내보내기나 원본·클론 게임 실행은 없다. 기존10.78의446060 수집과446270 표시를 이미 복원한 위치·수명·내려놓기 상태에 연결하는 작업이다.

## 연결 방식

`CursorPreviewSource`는 같은 실제 풀과 Drop/Lifecycle/Placement/Bridge/Canon 격자를 참조한다. `PreviewCollection::BindCursor`와 `PreviewDraw::BindCursor`로 같은 참조를 전달한다. 다른 풀과 CD 연결은 거부한다. 기존 독립 상태 인터페이스를 유지하며 양쪽 입력 모두 같은 계산 본체를 실행한다. 콜백 뒤의 SID/개수·격자·부모·판정·플레이어 조회는 현재 저장소를 읽는다. 고정된 초기 사본을 새 전역으로 사용하지 않는다.

| 원본 전역 | 실제 저장소와 용도 |
| --- | --- |
| 5325c8 / 55a280 / 55a278 | Drop.lockDepth / Lifecycle.abstractType / Lifecycle.currentGump |
| 55a1b8 / 55a1d8 | Lifecycle.previewCount / previews 배열30 |
| **55a290** | Placement.placementAllowed: 수집과 표시의 허용값. Drop.legal(55a28c)과 별도 |
| 55a300/304 / 540c70 | Drop.x/y / localPlayer |
| 5411d0/1d4 / 5411a0 / 5412c4 | Bridge의 island/stalag / bridge / connector 타입 |
| 59a91c/920 / 5325e0 / 55a2b0..bc | Placement.screenOrigin / labelSpacing / bounds |
| **55a2e0** / 55a2dc | Drop.costFlags(20/40 호출 비트) / Placement.placementFlags(배치 경고) |
| 55a294/298/29c | Placement.descriptionAllowed / spaceAllowed / sourceAllowed |
| 55a270 / 55a19c | Drop.cursorWidth / Lifecycle.costTextWidth: 금액 전체 폭과 아이콘 앞 글자 폭 |
| 비용 그림·치수 / 경고 치수 | Lifecycle.costImage·costHotspotX/Y / warningWidths/Heights |
| 미리보기 격자 | CanonPlacementPipelineState.preview.blocked의 실제144바이트 |

`MakeCursorPreviewAssetShapes`는 Take의 실제 자산 어댑터가 저장한 block+1 토큰을 **각 비용 그림 호출 때** 읽는다. 일반 타입은 같은 GameAssets SHP를 읽는다. 빈/범위 밖 비용 토큰은 거부한다. 옵션·기호 타입·팔레트 표와 텍스트 콜백은 기존 출력 계층의 책임을 유지한다.

## 검증 범위

- 기존 PATCH 수집520개/표시477개를 실제 참조에 공급하고 이전 중복 입력은 다른 값으로 바꾼다. 기대 수집 바이트·피드백·명령열은 기존 독립 x86 fixture에서만 읽으며 재생성하지 않는다.
- SP 경계 뒤 실제 목록/개수 변경과 저장한 XY/깊이 유지, 격자 출력 뒤 실제 격자/부모 변경, 본체 출력 뒤 허용/플레이어 변경을 확인한다. 표시 허용과 내려놓기 허용은 반대로 공급한다. 이 재진입 검사는 호스트 연결 계약이며 신규 독립 원문 관측으로 합산하지 않는다.
- 공통 수집→정렬→kind4 분배→SHP 해제/투명 픽셀 합성을 기존 입력과 실제 공유 참조에서 모두 실행한다. 실제 보호 자산의 비용 이미지 토큰 변경도 확인한다.
- 기존 세 판본 수집1,560/표시1,431의 SHA·입력/반환/경계 집계를 감사했다. 원문을 새로 표본 실행한 수는 수집63/표시42, 합계105개·두 정밀도210정상 반환이다. 원본 게임 또는 OS 호출은 없다.
- 최종 Release 빌드2는 종료0·경고0·오류0이다. 집중21개/전체 **1,485개 모두 통과**, 실패0·전체231.63초·CTest종료0이다. 이전1,480개에 새5개를 더했다. 실제 공유 참조와 기존 입력의 공통 픽셀 검사를 함께 실행했다.
- 영구 로그는 `extracted/goal-cursor-display-20261017-` 접두사다. 전체 내부 출력은 `ctest-output.log`, 최종 집중은 `collection2.log`/`draw2.log`다. 새 보고서는 의존24파일을 고정하며 실제 변경8개는 스테이지 바이트와 일치한다. 기본 시스템 커서는 이번에 통과했으나 이전 간헐 실패 원인은 미확정이다.

## 남은 실제 플레이 연결

실제 Client의 Session/현재 전역과 메뉴/명령/Tell/GumpForm·생성·일반 건설 확정/배치가 남아 **Construct는 여전히 비활성**이다. 실제 Take→위치→수집/표시·내려놓기를 같은 세션에 조립하고 GUI에서 확인해야 한다. 텍스트/팔레트 공급, 내려놓기 위치426a70, 생산·경제·AI 전략·승패/저장/다음 미션 및 사용자 자연 완주는 별도다. 이번 검사의 통과를 실제 GUI 활성이나 Early Missions/캠페인/데모의 완주 증거로 사용하지 않는다. 전체 목표는 active이며 DirectDraw는 이번 범위에서 제외한다.
