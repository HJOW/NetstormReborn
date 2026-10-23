# 기존 Editor 실제 child와 원시 Contents의 Session 연결 제안

2026-10-24,현재/마지막 정밀 디컴파일 **HJOW-Athlon / 2026-10-23**이다. 이 작업은 신규 사본/patch/문서와 독립 reader만 작성했다. 기존 공유 Session·Client·CMake·LEFT·공유 build·GUI·git은 root가 소유한다.

기존 [child 제안](../../extracted/cpp-editor-session-children-20261024-proposed.patch) SHA256 **`801f9e73def03b27cf6567a4b1e9cd23ceaad23a974e31b912f8069129cbb45e`**는 Contents가 명시 throw인 최초 상태 그대로 보존했다. Money/Meta/TypeNames/Deck을 같은 실제 Session/Storage/Client 장치에 연결하며 기술UI0042f540은 별도 실제 장치다. 이 제안은 전체Editor 외곽 완료나 컴파일 성공을 주장하지 않는다.

후속 [Contents delta](../../extracted/cpp-editor-session-contents-20261024-proposed.patch) SHA256은 **`31c7b2f1417a6de55c0098039c3e6787e4fa9a8a289ac0075531a32a7763e62d`**다. 기존 제안의 include1곳/factory설명/meta.contents만 바꾸고 Territory/privateSnapshot/Editor 외곽의 문맥을 덮지 않는다. [생성기와 manifest](../../extracted/cpp-editor-session-contents-20261024-work/manifest.json)는 명시 이전 Session 사본 SHA를 확인한다. 그 사본을 대상으로 `git apply --check --whitespace=error-all` 종료0이다. 공유 제품에는 root가 두 제안을 적용했으며,이 작성자가 전체 Session을 별도 컴파일하거나 설치 검사를 실행하지 않았다.

## 실제 연결과 현재 값

- Technology 선택 직후 얻은 `TemplateWriteStream` 생성수명을 `State::technology`에 고정한다. TypeNames가 manager.current를 바꾼 뒤에도 그 노드를 `Storage.AccessSection`으로 동기 빌려 전체 `LoadFortContents`를 실행한다. 별도 stream/버퍼/pos 사본이나 parsed `FortContent.item.type`을 사용하지 않는다.
- 현재 변환토큰은 Client의 단일 `FortTypeConversionStorage::Current()` DWORD 참조다. Bytes(token)은 실제255BYTE 할당을 순수 조회하며 checking도 현재 Session 값의 비영 여부다. 변환 실패를 타입identity로 돌리지 않는다.
- 현재 두 TYPE 조회는 각각 `core.types[kind].flags1/contentListFlags`를 읽는다. 부모 fallback은 같은 고정 Pool 블록에 대응하는 SID의 **현재 Slot[10]**을 읽고 그 실제TYPE의 `containerListFlags`를 사용한다. contained에 저장한 원래type WORD나 과거 TYPE 사본은 사용하지 않는다. 설치 동안 Pool 할당 블록을 재확보하지 않는 기존 Session 수명을 따른다.
- `create`는 같은 실제 `RawContainedCreate::Create(parent,kind,quantity,listFlags,attachFlags,quality)`다. quantity는QA BYTE/QB signed WORD의원문DWORD,quality는0이며 나머지 flags/count를 재계산하거나 clamp하지 않는다. 하위가 같은 Pool인지 검사한다. 실제 shared primitive의 유효부모 계약(parent0 거부)은 변경하지 않았으며 원시 reader 자체가 반환SID0을 임의 필터하지 않는다.
- 원문 Technology wrapper는 reader의 false 반환을 무시하고 나머지를 계속한다. 예외는 전파하며 앞선 cursor/Create 효과와 부모의 기존 depth cleanup을 보존한다. reader는 loadingDepth를 증감하지 않는다.
- reader가 정상 반환한 직후 **현재 Current()!=0**을 `meta.hasTypeConversion`에 다시 동기화한다. callback이 표를 바꾸거나 없애도 해제 직전값을 사용한다. 그 다음 release hook은 현재 토큰을 다시 읽어 실제Release하고Current=0을 쓴다. Release/Storage cursor공개는 게임callback을 추가하지 않는 동기 장치이므로 이 재동기화와 부모의 bool관문 사이에 현재값이 달라질 효과가 없다. 예외에서는 해제관문에 진입하지 않는다.

`Storage.AccessSection`의 정상 `WithMemory→PublishCursor`와 예외 `PublishCursor`가 같은 Technology typed cursor를 현재 raw pos/end에 맞춘다. TypeNames는 각 선택 반환을 자체 registry에 두고 같은 AccessSection을 사용하며,실패한 본문에만 별도 Unwind를 한 번 적용한다. meta와Deck이 각자의 cleanup을 중복 적용하지 않는다.

## Deck과 남은 외곽

앞선 제안의 Deck은 같은 ProductionDeck 할당/실제Clear/Open/공유GameRandom/RawDeckRestore Full·Append를 사용한다. TYPE의현재group/flags2와 실제nugget/geyser전역을 연결하고,nonFull의현재conversion/해제/Refresh는RawFortDeckLoad가 소유한다. `LoadExistingEditorDeck()`는 원래World/Graph 뒤에 요청할 독립 단계이며 그 자체가 loaded를 확정하지 않는다.

신규 reader의 근거와 **668원문 관측/2격리검사·실패0/경고·오류0**은 [원시 Contents 복원](cpp-fort-contents-load-20261024.md)과 [검증 기록](../../extracted/cpp-fort-contents-load-20261024-isolated/verification.json)에 있다. 전체Editor 최초 설치의 Subscriber/World/Deck/private Snapshot/State/EditorPlayers/맵tail/loaded 순서는 root의 별도 조립과 공용 빌드·통합검사로 판정해야 한다. 실제 선택정보UI0042f540 장치도 해당 위치에서 필수로 남는다.
