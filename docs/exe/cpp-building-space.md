# 건물 Pop/Unpop 옆면의 실제 raw 공간 효과

2026-10-13, 마지막 정밀 디컴파일 PC **VM-W11-CODEX**. 현재 PC와 같음을 확인하고 작업했다. 원본 게임 프로세스·GUI·OS 진입점은 실행하지 않았으며 두 PE를 읽기 전용으로 격리 에뮬레이션한다.

`SquidPop`(004b02d0/CD 004ad490)과 `SquidUnpop`(004afe50/CD 004ad0b0)은 건물 genus mask `0x50444200`의 묻히지 않은 발자국에서 spot 처리 직후 옆면 표면을 갱신한다. 옆면은 `x==left || x==right`이고 위·아래 모서리를 제외한다. 모든 칸의 y/x 순서와 패치 overlap 조기 반환을 유지해야 앞선 칸에서 이미 쓴 표면 단어도 보존된다.

`RawBuildingSpace`는 같은 `SidPool`·`SquidHash`·타입 표를 받는다. 표면 WORD 지도는 해시의 실제 0단계 배열이며 별도 복사를 만들지 않는다. Pop/Unpop 모두 선택 연결을 요구하므로 외부 연결 없는 건물 호출은 기존처럼 변경 전에 거부한다. `RawSurfaceWorld`도 이 몸체를 생성해 실제 Pop/Unpop과 표면 진단 호출에 연결했다. 기존 일반 건물은 아직 GameWorld의 값형 목록에 있으므로 이것만으로 건물 월드 이관이나 1-1 완주를 주장하지 않는다.

Pop은 유효 SID의 타입 flags1 `0x800`을 확인한다. **건물 this 자체**의 현재 float 좌표를 0 방향으로 절삭하고 그 칸의 지면 타입이 현재 islandType이면 WORD의 bit2를 지우고 bit1을 켠다. 후보 표면의 좌표를 넘기는 것이 아니다. 섬이 아니면 낮은 BYTE의 bit1/2를 지운다. 높은 BYTE와 나머지 비트는 유지한다. Unpop은 표면 플래그·섬 확인을 하지 않고 유효 SID의 낮은 BYTE bit1/2를 지운다. 두 경로는 후보의 free/dead/void/contained 상태를 검사하지 않으므로 일반 `AllocatedBytes`의 free 필터를 적용하지 않는 좁은 전용 접근을 사용한다.

섬 확인 0046f680/CD 004bdd20은 **boolean**이다. 영역 번호·지면 소유자 조회와 다르다. 좌표 float wrapper는 0046f6e0/CD 004bdd80이다. 표면 통지 004214a0은 패치 debug가 켜지고 묻히지 않은 실제 다리의 수명 nibble이 7을 넘을 때만 assert한다. CD 00448c10은 raw 포인터만 반환하며 호출자는 반환값을 버린다. 두 판본 모두 정상 경로에 추가 상태 쓰기는 없다. 호스트는 패치 debug 오류만 예외로 보고한다.

독립 입력은 `tools/decomp_buildingspace_oracle.py`가 기존 Pop oracle과 현재 PC의 정밀 함수 범위를 읽는다. 타입·SHP·슬롯·지면 지도는 합성 입력이며, 실제 함수/후처리를 대체하지 않는다. 표시/postPop 억제와 비전투 null dirty에 범위를 한정한다. 각 호출 뒤 앞192슬롯의 모든 바이트를 직접 대조하고 전체 풀·네 해시·spot은 Adler32로 비교한다. 후보에는 free/비표면/내부/모서리/섬·비섬 좌표가 포함된다. `BuildingSpaceTests`는 같은 실제 Pop/Unpop 순서로 fixture를 재생한다. 두 PE **251개** 원본/C++ 대조와 새 **2개 검사**, 전체 **904개/실패0** 통과. 이번 단위 GUI 실행은 없다.

초기 합성 SHP의 공유 버퍼 준비 순서 때문에 건물 크기가1로 덮인 것(679 CHECK)과 후보 좌표를 섬 확인에 넘긴 구현 오류(후속80 CHECK)를 실제 바이트 대조로 구별해 교정했다. 최초·중간 로그는 `extracted/buildingspace-cpp-initial.log`, `buildingspace-cpp-shape-corrected.log` 및 `buildingspace-cpp-diagnostic.log`에 보존했다. 실제 호출자 this 좌표를 확인하고 새 입력의 건물 지면도 섬/비섬으로 변형해 양 분기를 대조했다. 최종은 `buildingspace-cpp-final.log`/`buildingspace-verify-final.log`이며 원본 프로세스·보호 파일 변경은 없다.
