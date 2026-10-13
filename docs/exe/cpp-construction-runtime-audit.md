# cpppj 실제 건설 경로 감사

2026-10-13, 현재 PC 및 마지막 정밀 디컴파일 PC는 **VM-W11-CODEX**다. `AGENTS.md`와 두 인수인계 문서를 읽고 현재 소스를 정적으로 조사했다. 원본/클론 GUI 실행, 보호 파일 변경, 커밋/푸시는 하지 않았다. 이 문서는 구현 모듈의 존재와 실제 실행 연결을 구별한다.

## 썬 캐논의 실제 분기

10.78의 `.type`를 `tools/shp.py::load_type_texts`로 아카이브에서 읽었다. 썬 캐논은 `createsisland emplacement shadow`, HP 600, 비용 400, 발자국 3×3, `constructionRate=10`, `group=cannon`이다. `RiftTypeTable.cpp`의 플래그 표/후처리에 따라 `flags1`에는 받침 `0x400`·HP `0x10`이 있고, `flags2`에는 emplacement `0x40000`·cannon `0x08000000`·dropBlocking `0x10`이 있다. **후처리 `0049b0d0`는 `(flags2&0x70000)&&!(flags2&factory0x4000)`인 타입에 `flags1 0x8000`을 추가한다.** 파일에 명시되지 않아도 썬 캐논의 최종 타입은 건설 운반 객체가 필요한 타입이다.

따라서 `RawConstructionLocal::Place`는 예측 배치 뒤 `moveBuilder(localPlayer,first,0,0)`를 호출하고, `RawConstructionConfirm::Confirm`은 **작업장/그래프/기술 기준점**이 있어야 확정한다. `RawConstructionNotice::Handle`은 abstract로 놓은 뒤 needsBuilder 분기로 간다. 로컬 플레이어는 앞선 Local의 운반을 사용하며 `moveLocalBuilder`가 꺼져 있으면 통지에서 중복 생성하지 않는다. 로컬 사제 자체를 썬 캐논으로 보내는 경로가 아니다.

`RawConstructionBuilder::Move` (`00443000`)는 기준점에서 첫 **teleportEffect(134)** 객체를 client flags2로 생성하고 가상 Pop(0x10)·실제 경로 시작 `00489080`을 호출한다. 실제 PE 전역 `00541270=134`, `005411fc=105`(manaBolt), `00532750=0.05f`다. 공유 Mana 공급원을 선택/정렬/소비하며 각 공급원에서 manaBolt를 생성·경로 이동시킨다. 경로 목표는 건물 발자국의 위쪽 가운데를 반올림한 점이다.

**발견한 실제 연결 오류의 교정 완료**: 원본 `00496e80`은 두 번째 인자를 Regular `+0x18 event`, 네 번째 인자를 `+0x1c payload`로 저장한다. Builder의 `(this,0,carrier,00532750)`는 **event0/payload0.05f**다. 기존 `timerEvent`/Host adapter는 이 값을 event로 쓰고 payload0을 넘겼다. 기존 builder oracle은 생성자 몸체를 대체했고 C++ 검사가 세 인자를 `Bits(payload),sid,event`로 잘못 해석해 기록하여 원시 인자 순서만 일치했다. 2026-10-13 `timerPayload` float→event0/currentPayload로 수정하고, 실제 생성자 몸체를 포함한 두 PE의 새192개 입력과 실제 C++ Host 필드를 대조하는 검사를 추가했다. 기존1110개 재실행/재라벨은 새 입력에 더하지 않는다. 세부 근거는 [교정 기록](cpp-construction-builder-reconstruction.md)을 따른다.

첫 객체의 도착 판정 `00444080`은 같은 칸의 abstract·needsBuilder 자산을 찾아 **서버 SID에만 `00443fe0(target,0.6f)`**를 호출한다(`005325c0=0.6000000238418579f`). 예측 객체만 보이면 0x3ea로 대기하며, 확정 자산에 시작했거나 기다릴 대상이 없으면 첫 객체를 삭제하고 0x3e9를 반환한다. 이 도착/재시도 몸체와 PathProcess 전체는 현재 `RawPathAnimation::Stop`에 들어 있지 않다. 파일의 `constructionRate=10`, Start 인자0의 기본10초, 정상 teleportEffect 도착의 실제0.6초를 서로 바꾸어 쓰면 원본 동작이 달라진다.

사제 접근/이동이 필요한 통지 분기는 워크샾 `0x4000`, 보텍스 `0x200`, 데이스 `0x400000`이다. 첫 미션의 사제/신전/워크샾·이후 의식에는 그 경로도 필요하지만, 첫 썬 캐논의 teleportEffect 건설에 임의로 사제를 보내면 원본 동작과 달라진다.

## 시작부터 완료까지의 호출 순서

| 끝점 | 실제 API/원본 | 공급하거나 연결해야 하는 자료 |
| --- | --- | --- |
| 사제 우클릭 → Construct 항목 | 현재 `UberGump::OpenObjectMenu`는 Construct를 비활성 표시 | 원본 선택/기술/건설 메뉴 몸체와 raw Menu/GumpForm 필요. 현재 버튼 연결만으로 이 항목은 실행되지 않음 |
| 썬 캐논 선택 → 미리보기 확보 | `RawCursorDrop`는 이미 존재하는 preview Form을 받음 | 타입 WORD +18, 플래그 WORD +20, 품질 BYTE +30, 연관 SID 가상 +0. 선택/preview 생성·취소·활성화/잠금 해제 몸체는 Drop 구현에 없음 |
| 마우스 위치 → 합법 미리보기 | `RawCanonPlacementPipeline::MayPlace` (`0049b510`) | 월드 좌표·방향·argument·로컬 owner, 같은 hash/spot/섬 지도/지역 제한, SHP/프레임/hotspot, graphReady·동맹·작업장 기술 목록 |
| 합법 위치 왼쪽 내려놓기 | `RawCursorDrop::Drop` (`004473e0`) | 실제 queryCursor/prepareCursor/previewSource/공유 삭제, 현재 타입/좌표·비용 플래그. 월드 분기에서 사운드/기록/로컬 배치/비용/잠금/팝업 닫기 순서 |
| 로컬 예측 배치 | `RawConstructionLocal::Place` (`004433b0`) | 미리보기 삭제 → 생산 칸 반환(플래그8일 때) → client SID(flags2) → 필요 시 noIsland 9칸 → 본체 abstract/HP0 → 건설용 teleportEffect/공급 manaBolt 경로 → 서버9 요청. localPlayer·patternTypes·spot은 현재 월드와 공유 |
| 서버 요청 | `RawConstructionRequest::Request` (`00444760`) | 전체 MayPlace → 같은 abstract 충돌 검사 → genus 필요 시 밀어내기 → 받침 확정 → 본 확정 → 실패 받침 정리. LocalMessage의 BYTE/WORD/double 시각을 원본대로 요청 인자로 변환하는 실제 메시지 분배 필요 |
| 서버 확정 | `RawConstructionConfirm::Confirm` (`00444590`) | decoder 순서의 server SID(flags0) → 통지0x3f broadcast → 동일 Notice 직접 호출 → 기록기. 로컬 예측 SID 재사용으로 대체하면 안 됨 |
| 확정 통지 | `RawConstructionNotice::Handle` (`004441b0`) | 먼저 예측 Clear → abstract/이력 → Place → needsBuilder 분기(기본 로컬 운반 중복 생성 생략) → refresh. Clear는 같은 pool/hash의 현재 후보를 삭제해야 함 |
| 실제 공간 등록 | `RawConstructionPlace::Place` (`00442c80`) | abstract BYTE+40 bit0, HP DWORD+26, frame DWORD+36, WORD+12, owner BYTE+34, 가상 Pop(abstract면2, 실체면0x41). SHP 크기·same hash/spot·가상 postPop 필요 |
| 운반 도착 → 건설 시작 | 첫 teleportEffect 도착 `00444080`→`RawConstructionProcess::Start` (`00443fe0`→`00443c40`) | 서버 대상 확인 후 duration0.6f → reserve → 실제 visual 생성 → 현재 game time에서 start/end → type30 ProcessForm(0x10) → duration>1일 때만 jimBuild 소리 프로세스 → refresh. 기본10초는 인자0의 별도 경로. host를 주면 Kernel 소유권을 실제 사용 |
| HP 진행 | `ConstructionProcess::RunFrame`→`RawConstructionProcess::Run` (`004428c0`) | 매 프레임 같은 now, 실제 최대HP, HP 쓰기 후 display update. authority/quickbuild 등이 현재 전역을 읽음 |
| 최대HP 완료 | 동일 Process의 Kill → 공유 Form 삭제 → `OnFormDestroy`→`FormDestroyed`→`Complete` (`00444030`→`00443e20`) | 소리 contained 삭제 → 갱신/완료 소리 → 서버 SID·권한·HP 최대·abstract일 때 가상 Unpop → abstract 해제 → 현재 위치 Repop(flags9) → 필요한 genus 밀어내기 → HP 후처리 |
| 표시 종료 | Build process type17와 render registry | RawConstructionFrame은 abstract 해제 뒤 Kill. Kernel/Form 삭제와 RawRenderRegistry 제거·source/output/particles 해제를 같은 수명에 연결 |
| 건설된 썬 캐논 공격 | 건설 완료의 범위 밖 | 실제 자동 각도/사정거리/발사/피해/소리·애니메이션을 별도로 연결. 건설 완료 검사만으로 캠페인 진행 가능 또는 동일성을 주장할 수 없음 |

## 같은 월드 저장소를 구성하는 순서

`RawSurfaceWorld::Impl`은 이미 pool/hash/spot/types/frames/shapes/RawGraph/SquidDisplay/SquidPop/Unpop/Factory/Owner/ProcessHost/FormDispatch를 공유한다. 그러나 현재 Load는 terrain/noIsland/bridge만 허용하고, 공개 API로 pool이나 건설/커서를 제공하지 않는다. `GameWorld`의 일반 사제/건물 값형 목록을 별도 건설 pool에 복제하는 방식은 raw 충돌·장부·삭제·선택/렌더 identity가 달라지므로 연결이 아니다.

실제 추가 시 다음 수명/순환 참조를 지킨다.

1. 모든 타입/SHP/프레임/hotspot 자료와 동일 pool/hash/spot/섬 지도/지역/시계/Player·SP·기술·장부를 준비한다. `SquidPostPopState`의 ownerFactories와 별도 추가 목록은 `RawPlayerPlacementAnchor`가 직접 참조한다. `ContainedFinderState`도 해당 기술 Form 저장소와 같다.
2. Pop/Unpop/가상 postPop, owner, 공통 자산 삭제와 type1/2/3/6·ProcessForm 분배기를 만든다. Content/Gump preview를 실제 Form 수명에 등록한다. 일반 자산/표면·사제 재정의를 같은 삭제 호출에 둔다.
3. `RawCanonPlacementPipeline`을 만든다. 이 생성자는 anchor→permission→surrounding→relations→terrain→geometry/shape→collision→preview→접두를 **이미 모두 내부 조립**한다. 모듈별 임의 `true` callback으로 대체할 필요가 없다. `MayPlace`는 preview editor/동맹과 anchor graphReady를 permission의 현재 값으로 동기화한다.
4. Place/Clear/Effects를 구성하고 공유 가상 Pop/삭제를 연결한다. Process와 Visual/Sound System을 구성한 뒤 Notice→Confirm→Request를 만든다. Confirm.handle은 Notice를 참조하므로 Notice가 먼저 살아 있어야 한다.
5. Local의 실제 factory/place, send→Request 메시지 처리를 연결한다. Cursor Drop/ReturnPiece와 선택/질의/취소 경계를 마지막에 연결한다. 생성 중에는 Pop/Start/GUI 사건을 실행하지 않는다.
6. 한 프레임 now를 ProcessState·ConstructionState·VisualFrameState·BuilderState 등에 같은 값으로 넣고 실제 Kernel을 실행한다. 정지 중 logical frame/입력/표시 계약은 원본 상위 루프를 따른다.

## 각 callback의 정상 썬 캐논 경로 도달 여부

| 경계 | 정상 첫 썬 캐논에서 호출 | 현재 구현/연결 상태 |
| --- | --- | --- |
| cursor query/prepare/highlight/activate/lock 해제/closeGump | 예 | Drop 외 실제 게임 커서 전역 몸체 미연결. `Cursor` 클래스는 OS 모양 처리이며 게임 preview 몸체가 아님 |
| cursor GUI detach/attach/receive·returnPiece | 월드 Drop에는 없음. production Form 플래그8이면 Local Return 호출 | RawCursorReturnPiece/RawContainedCreate 몸체와 실제 Regular adapter 있음. 선택 UI의 source/flags 생성 필요 |
| MayPlace 모양/충돌/표면 관계/anchor | 예 | Pipeline 내부 실제 몸체 있음. 외부 상태가 핵심: 그래프/표면 소유자/기술 Form이 현재 세계와 연결돼야 함 |
| ConstructionBuilder Move/speed/source/Mana | 썬 캐논은 후처리 flags1 0x8000으로 **호출** | RawConstructionBuilder 몸체와 Regular event0/payload 교정·actual ctor 대조는 있음. 실제 경로 시작/속도/Mana 선택·소비와 이번 RawConstructionArrival의 전체 경로 등록은 후속 |
| Request.displace | 썬 캐논 genus와 displace mask가 겹치는 실제 설정에서 호출 | Effects 몸체 있음. 후보 clearPath/unpop/pop·동맹이 현재 월드에 연결되어야 함 |
| noIsland 생성/Pop/owner/Graph | 지면 없는 배치면 예 | 기존 raw 표면 몸체 있음. local과 server 확정이 같은 spot을 갱신하고 예측 Clear 후 서버 위치를 등록해야 함 |
| Place.cost/Refund/Reject 메시지 | 로컬 정상 배치 비용은 Cursor가 차감. 비로컬/실패 경로는 별도 | Place+SquidReward+ScrambledSpStore adapter 있음. 실제 Player/SP 표시는 현재 시작값 표시와 구별 |
| Notice 소유 Priest lookup/contained 삭제/approach/tryMove | 썬 캐논에는 미호출 | 해당 type(mask0x404200) 정상 건설에는 필수. `00489080` 실제 이동 시작/PathProcess 전체는 현재 RawPathAnimation만으로 대체할 수 없음 |
| Process.effect·reserve·refresh·maxHP·updateDisplay | 예. 정상0.6초는 진행 jimBuild 소리 없음 | 각각 RawConstructionVisual/실제 SoundProcess adapter/Reward 몸체 있음. reserve와 actual renderer 저장소 연결 필요 |
| Process.removeContained·unpop/repop·push | 완료에서 예/조건부 | 동일 ProcessHost/FormDispatch·asset virtual 공간 삭제가 필수. Hook만 채우고 Kernel/Form 제거를 생략하면 완료 아님 |
| build visual capture/gradient/palette/registry/Run/Draw/free | 예 | RawConstructionVisual/Frame/Lifetime/ConstructionVisualSystem·DrawVisibleScene 있음. SHP capture/팔레트·현재 Renderer blit/actual collection과 게임 월드에서 조립 필요 |
| ConstructionData0x37 Write/Receive | 동기화 계층이 요청할 때 | 실제 host constructor/지정 form 갱신 있음. 요청/Notice0x3f 전체 wire 분배와 상위 command queue는 현재 client에서 없음 |

## 다음 구현에 필요한 검증

동일한 실제 미션 초기 pool에서 사제 메뉴→썬 캐논 선택→합법/불법 위치→회전/취소→local/server 두 번호·noIsland 예측 정리→비용400 한 번 차감→abstract HP0→teleportEffect/manaBolt 실제 경로·도착/서버 대상 재시도→duration0.6초 HP600→ProcessForm 및 소리/표시의 공통 삭제→실체 공간/Graph/장부·픽셀→다음 공격까지 확인한다. 정상 시작뿐 아니라 비용 부족·배치 실패·공급원/사제 없음·건설 중 대상 삭제·미션 종료 재진입을 별도로 확인한다. 현재 각 모듈 단위 fixture와 helper 통합은 그 종단 실행을 증명하지 않는다.
