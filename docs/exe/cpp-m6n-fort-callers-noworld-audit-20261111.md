# M6-n 잔여: Fort 선택 호출자 수명·NoWorld 도달성 감사 — 작업1272

2026-11-11 / HJOW-Athlon. 마지막 Client 디컴파일 2026-11-03/HJOW-Athlon과 현재 호스트가 같고 새 디컴파일은 없다. **M6-n의 Fort 선택 호출자 두 곳과 Player 기록 대여 수명 결함을 교정하고, NoWorld 최초 후처리의 도달 가능한 객체 종류를 호출자 근거로 확정했다.** MP·사제 포획/희생·특정 적 공격·승리 실행은 0이다. M7.2·M8과 전체 목표는 미완료다.

## 코드 교정

1. **`RequestFortEditState`/`FortSelectionEvent`의 Battle 판정** — 원문 `004b3560`은 전역 `00594fbc`를 읽는다. 구현은 `world_ ? world_->Session().BattleActive() : 0`이라 Session이 없으면 0이었다(Session의 `assetState.inBattle`은 설치 때의 사본). Native는 `ownerMode.battle`(Client 장기 Authority)을 읽는 `UberGump::BattleGate()`로 바꿨다. 독립 typed/CD는 기존 Session 값을 유지한다.
2. **`FortEditEffects.ownedIslands`** — 원문 `0046f430`(소유 섬 개수)은 전역 섬 목록을 순회한다. 구현은 Session이 없으면 "편집 복귀의 실제 섬 목록 없음"으로 던졌다. Native의 섬 목록과 Fort 모드는 `ClientConstructionGlobals`(장기)에 있으므로 `RawIslandQueries(Islands(),FinishOwnerState()).CountOwned(owner)`를 Session 없이 실행한다.
3. **Player 기록 대여 수명(실제 크래시)** — 장기 `RawPriestWorldState::hitPoints.playerRecords`가 이전 Session이 대여한 Player9 span을 다음 Session까지 쥐고 있었다. 다음 Session의 `ClientPlayerRecords::Initialize`가 새 `std::vector` 블록(다른 주소)을 만들면 `BindPlayerRecords`의 "동일 주소" 검사가 `invalid_argument("사제 HP의 같은 Player9 고정 기록 필요")`로 실패한다. 직전 단위들은 힙이 같은 주소를 재사용해 우연히 통과했을 뿐이며, 1269의 새 오류 로그가 일반 Edit 두 번째 진입의 이 예외를 정확히 잡아냈다(`logs61111021601.log`). `~RawPriestWorldBindings`가 Session 종료 때 대여를 반납하도록 고쳤다. MP/재접속에서도 같은 위험이 있던 수명 결함이다.

## NoWorld 도달성 감사 (Session 없이 Pop되는 객체 종류)

Session 밖 장기 Storage에서 객체를 만드는 호출자를 전수 검색했다(`CreateAsset`/`PopAsset` 호출자): `ClientMetaBattleObjects`(메타 전투), `NativeMetaParticipantCreate`(참가자), `NativeMetaScene`(글리프), `ClientRendererFrame`(메타 표시 move/create), 그리고 Host 정기 효과 안의 Balloon/Carrier/Dude/Fence/Flare/Ice/Lifetime/Mana/Gem/RandomPlacement/건설 바인딩. 앞의 네 경로가 쓰는 TYPE은 클라이언트 전역 `participantType=108`(`00541208`), `battleType=109`(`0054120c`), `glyphType=161`(`005412dc`)이며 보호 PE 초기값을 읽어 확인했다. 해당 VT는 `0050c9f0`(TYPE108/109)/`0050cc08`(TYPE161)이고 1267에서 공통 Pop 허용에 연결했다. 나머지 경로는 미션/Editor의 Session 안에서만 객체를 만든다. 따라서 `DefaultResourcePostPop`에서 아직 던지는 Session 소유 VT(Crab·Teleport·Rune·Altar·Dais·Aviary·Flyer·Gun 4종·Blocker·Player·Priest·Residence·Temple)는 Session 없는 Pop으로 도달하지 않으며, 만약 도달하면 빈 성공이 아니라 명시 오류가 난다. 패킷이 TYPE을 고르는 경로는 확인되지 않았다(메타 생성 TYPE은 위 고정 전역이다).

## 검증

Release/x64 빌드 176.6초/종료0(이후 수명 교정 증분 53.9초), 경고·오류 0. 일반 Edit 두 번 왕복(1266 스크립트)이 교정 전에는 둘째 진입에서 위 예외로 종료했고, 교정 후 두 번 모두 통과(`dataUnchanged`, 종료 0, 로그 파일 없음). `netstorm_menu_session_tests`(6건)·`netstorm_main_popup_tests`(3건)의 실패 이름은 1271 직후와 동일하다. 정상 Tutorial4 생산/건설/600프레임(13.8→24.5초) 종료 코드 0. 보호 2,808파일 불변.

## 남은 M6-n

- 큰 AI 장기 소유자(비영 brain의 `RawAiBrainMemory`/생성·소멸 장부 분리)는 NoWorld 정상 도달이 확인되지 않았고 구현하지 않았다. 일반 Descend(`RequestVortexDeparture`)의 Session 의존, 배치 호출자 전수, Fort 상위 Finish는 별도 감사가 남는다. 이 감사는 위 호출자 근거를 넘어 "모든 process"를 전수했다고 주장하지 않는다.
