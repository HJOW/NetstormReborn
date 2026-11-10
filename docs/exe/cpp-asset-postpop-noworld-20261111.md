# World 없는 자산 전용 후처리 연결 — 작업1267

2026-11-11 / HJOW-Athlon. **보호막·Generator·장식 Gun·Arc·Acid와 메타/일반 Sprite 두 VT의 NoWorld 후처리를 같은 장기 본문에 연결했다. 큰 NoWorld·M6-n·M6·전체 목표는 미완료이며 새 MP 실행은 0이다.** 마지막 Client 디컴파일은 2026-11-03(R 전용 2026-11-07), 둘 다 현재와 같은 HJOW-Athlon이다. 새 디컴파일/원문 변경은 없다.

| 원본 VT | 실제 +20 | 연결 내용 |
| --- | --- | --- |
| 00507770 보호막 | 00448d20 | flags1에서 독립 new40을 두 번 시도해 event1(0.08)·event2(0.01) 예약 → 공통 PostPopBase 한 번 |
| 00501ea0 Generator | 0041c150 | 최초에만 현재 theme 조회 → 공통 Damageable 부모 |
| 0050b570 장식 Gun | 004694b0 | 최초에만 event0(0.05) 예약 → 공통 PostPopBase |
| 0050e668 Arc | 0048c080 | extra&9==0이고 최초일 때 event0(1.0) → Damageable |
| 0050e738 Acid | 0048c7a0 | 같은 조건에서 event0(0.3) → Damageable |
| 0050c9f0 / 0050cc08 메타·일반 Sprite | 004b0d30 | PE의 +7c/+80/+84가 41c0b0/4ad0f0/4ad110인 공통 쌍임을 확인해 `SquidPop::Supports` 허용에 추가, 표시(+88)는 기존 `SupportsDisplay` |

보호막은 `RawSurfaceStorage::ForcefieldPostPop()`이 `AllocateResourceRegular`/`ConstructResourceRegular`로 같은 확보 메모리/Host를 사용한다(실패해도 두 번째 시도 유지). 나머지 넷은 기존 장기 `resourceGeyser_`를 `ResourceAssetPostPop()`으로 이름 붙여 공유한다. Session의 보호막 본문(`RawPriestWorldBindings`)은 바꾸지 않았다. Session 반납 뒤 가상 분배가 `HandlesForcefield` 거짓이 되어 이 장기 본문으로 내려간다. 메타 Sprite는 `ClientMetaBattleObjects`가 `SurfaceStorage()->PopAsset`을 Session 없이 직접 호출하는 실제 경로다.

빌드: 최초 Release/x64 NetstormCpp·NetstormRoot·집중 대상 **484.7초/종료0**(헤더 변경), Sprite 허용 추가 증분 545.5초/종료0, 복원 후 최종 573.0초/종료0, 경고·오류 0. `netstorm_construction_owner_tests` **641건 중 새 7건 통과**, 실패 2건은 기존이다: `ClientBalloon_NonFrameEventRequiresActualCarrierRuntime`는 기대 문구 "장기 벌룬 현재 Carrier Regular 미연결"이 현재 소스 어디에도 없는 낡은 검사이고, `rawRunePostPop_real_pop_construction_complete_and_304`는 이전 단위(1260~1265)의 실행 목록에서 계속 제외된 검사다. 이 두 검사를 이번 변경 전 기준선에서 직접 다시 빌드·실행하지는 않았다. `netstorm_pop_context_tests`는 **내 변경을 `git stash`로 뺀 기준선과 변경 후의 실패 목록이 25건으로 완전히 같고(46 check 동일)**, `SquidPop_Vtables_RejectsUnrestoredPostPopOverrides`는 통과한다.

최종 EXE의 정상 Tutorial4 진입 → 생산/건설 요청(TYPE75/owner1, 건설 요청 0→1) → 600프레임 대기(게임 시각 13.8→24.0초) → 종료 코드 0을 확인했다. 포획/희생·특정 적 공격·승리·공중3종·MP/Join/Test Battle 실행은 0이다. 보호 원본/CD/fixture/AGENTS 2,808파일은 불변이다.

남은 NoWorld 최초 후처리 행(이 문서 기준): Crab 2, Teleport(flags1), Rune, Altar, Dais, Aviary 3, Flyer 3, Gun Angle/First/Archer/Sun 5, Blocker 3, Player, Priest, Residence(flags1), Temple 4. 이들은 Session 소유 장치(`gunBlocker`, `ownership`, `building`, 사제/의식 큰 후단 등)가 필요해 각각 장기 owner 이전이 필요하다. 자세한 내용은 [인계](../../LEFT_JOBS.md)와 [보고서](../../cpppj/asset-postpop-noworld-20261111.json)를 따른다.
