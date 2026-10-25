# 월드 다시 그리기와 미니맵 갱신의 두 DWORD 연결

2026-10-25 단위430. 기준10.78, 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**이며 마지막 전체 정밀본은2026-10-23이다. 이번에는 기존 정밀본과 구현을 대조했고 새 디컴파일/원본 실행은 하지 않았다. 이전 단위429는deaf73f8의집중63개 검증 제품이다. 마지막 전체 통합은 여전히33a98ea8의2870개·메뉴·자연 Early2다.

## 두 주소와 각 쓰기

원문005949d4는 실제 미니맵 Refresh가 읽고 마지막에0으로 소비하는 갱신 요청이다. Vortex·Fort 수신·HUD 도킹은 이를 증가시키고, 섬 소유자/지역 투표는1을 쓴다. 섬/지역은 별도로0059a8b0 월드 다시 그리기 DWORD를 증가시킨다. 영구 증가 버전이라는 이름이나 기존 `revision` 멤버명만으로 두 주소를 합치지 않는다.

최초13소스 source1에서 섬의 두 인자를 잘못 합친 것을 정밀 C/같은 보호PE 명령으로 찾아 **빌드1을 중단**했다. 부분 게임 실행 파일은 검증 제품으로 사용하지 않았고 집중 검사도 실행하지 않았다. source1/부분 빌드 로그는 보존하며, 아래 실제 명령에 맞춘 source2에서 다시 빌드한다.

| 부모/실제 쓰기 명령 위치 | 0059a8b0 WorldRedraw | 005949d4 MapRevision |
|---|---|---|
| Island Owner0046ef99/0046ef9f | 현재값+1 | 1 저장 |
| Region Ownership0045574d/00455753 | 지형 효과 복귀 뒤 현재값+1 | 1 저장 |
| Region Paint004709c2 | 현재값+1 | 쓰지 않음 |
| Vortex004dc6e8 | 쓰지 않음 | 현재값+1 |
| Fort 수신004b3e7b | 쓰지 않음 | 현재값+1 |
| HUD004c67f3 | 쓰지 않음 | 현재값+1 |
| MetaDirty0048136f/00481470/004814af·00481502 | 쓰지 않음 | 최초1/효과 뒤 재조회/종료0 |

`ClientGameplayGlobals::MapRevision()`이 최초 BSS0 DWORD를 소유하고 다음 기존 소비자가 직접 같은 참조를 빌린다.

- Session의 `RawIslandRuntime` 두 번째 갱신 인자, `RawRegionOwnership`의 MapRevision, `RawCampaignOwnership`/`RawVortexOwner`, 실제 Login의 `RawMissionClientPackets`.
- 같은 `UberGump`의 기존 `RawUberRootContext`/`HudDock` 참조. `HudRuntime`의 별도 fullRefresh는 제거했다.
- 최초 Button 등록표 생성 전 연결한 `MetadisplayDirtyState`와 실제 `RawMinimapControls`. 표시 생성·월드 바인딩·팔레트 변경·고유 큐 포화·전체/부분 Refresh·Report가 모두 같은 현재값을 사용한다.

`MetadisplayDirtyState`의 마지막 선택형 포인터와 `FullRefresh()` 참조 API는 기존 aggregate/독립 Native 관측 입력을 유지한다. 포인터가 없는 기존 호출만 원래 값형 fullRefresh를 사용한다. 포인터가 있으면 fallback을 조회·동기화·초기화하지 않는다. `RawButtonControls`의 새 마지막 인자는 기존 호출에서 기본 널이며, 실제 Client는 같은 MapRevision 주소를 최초 자식 생성 전 제공한다.

0059a8b0은 기존 `ClientKencloudGlobals::WorldRedraw()`를 빌린다. 실제 Client 서비스가 같은 owner를 전달하고, 섬의 첫 갱신 인자·지역 투표·도장이 이를 직접 증가시킨다. 장치 없는 독립 Session만 기존 BSS0 입력을 지역 fallback으로 유지한다. 지역 도장 자체의+1과 그 효과가 끝난 뒤 투표 wrapper의+1은 원문의 서로 다른 쓰기이므로 둘 다 남긴다. 기존 Region 상태의 값형 필드는 독립 aggregate 호환 입력이다.

일반/준비 대여와 반납은 이 DWORD를 초기화하지 않는다. 전체 Native cleanup의read/write adapter는 아직 연결하지 않았으므로 cleanup이 원문 순서로 증가시켰다는 의미는 아니다. 서버/boss의 광범위한 별칭과도 별개다.

## 검증과 범위

교정한 소스를 다시 동결하고 변경 소비자를 집중 검사한다. 기존 Native 미니맵1040·HUD2150·Vortex864 및 섬/지역 관측은 수정하지 않는다. 신규 검사는 실제 owner의 전체4096셀 소비/부분 큐/비활성 보존/재생성 callback 뒤 재조회, 실제 Button→미니맵의 팔레트 변경·bitmap 변화와0소비, 같은 Session의 실제0x2a 비동기 수신·unsigned 감김을 다룬다. 섬/지역에서는 World17→18과 Mapffffffff→1을 구별하고 효과 뒤 현재값 재조회도 확인한다. 기존 실제 Client의 준비/일반 대여 검사도 현재 DWORD 보존을 확인한다.

**source2 동결24소스의 빌드2가 종료0·컴파일 경고/오류0**으로 완료됐고 같은 제품에서 **집중83개·실패0**을 통과했다. 신규8검사는 실제 전체/부분/비활성/재생성 뒤 미니맵 소비, Button/팔레트 bitmap, 실제0x2a 수신, 섬·지역의 두 주소 구별과 효과 뒤 재조회를 검증했다. 기존 실제 Client 준비/일반 대여14검사와 Native 미니맵/HUD/Vortex/섬·지역/기존Editor 설치도 함께 통과했다.

[실행 보고서](../../cpppj/map-revision-runtime-20261025.json)는 source1 중단·source2의 기존1파일 수정/11파일 추가, 각 필터 로그/Native PE8쓰기 근거와 제품 SHA를 기록한다. 게임 SHA는 **e9303f11…**다. 보호1821파일/24소스/4실행 파일 SHA도 일치했다. 전체 CTest/긴 GUI는 이번 단위에 실행하지 않았으며 사용자 결정대로 다음3~5단위 또는 기능 묶음의 같은 최종 제품에 통합한다. 이전2870 통합을 이번 제품에 인용하지 않는다.

아직 Native cleanup/State·카메라64BYTE 초기화·일부 UI/신전 capture/debug 부모의005949d4 쓰기와 서버/boss 등의 연결은 남는다. 이번 공유 전역 연결을 실제 Edit 진입·사용자 편집/저장·모든 미션 완주의 완료로 해석하지 않는다.
