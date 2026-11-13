# webpj 실제 건물 표시·가상 선택 연결 (WB04-E409, 2026-11-14)

5-A `RawCampaignFactoryDisplay.cpp/.h`와 `ObjectHitVtables.inc`의 두 단위를 이식했다. 같은 실제 SID 풀/공간 해시/자산/프레임 표/Kernel/현재 시계와 소유자 색 저장소를 빌린다. 실제 GameWorld/Session 생성 및 앱 수명 연결을 완료한 것으로 세지 않는다.

## 근거와 기준선 뒤 변경 판정

기준선 `1f00a25a`의 표시기 전체와 83VT 표, `CampaignSessionTests.cpp`의 Factory 표시·픽셀 선택 시험을 읽었다. 표시기 두 파일의 기준선 뒤 차이는 cpp +10/−6, h +3/−1이며 마지막 변경은 `a0342fae9`다. 대응표 base는 정확한8자리 `a0342fae`다. 가상 표와 선행 RawObjectHit는 기준선 뒤 변경이 없다.

3.6절 **② 지금 반영**: 메타 섬의 실제 추가Draw(478d10)와 선택200픽셀 버퍼를 Client 장치로 연결하는 변경이다. 단일 플레이의 실제 표시/선택 보정이므로 반영한다. `bindMetaPixels`는 null을 거부하고 설치 자체에서는 출력하지 않는다. 일반 Draw의 메타 콜백과 Hit의 실제200×200 버퍼/전체 clip을 각각 전달하며, 도달한 장치가 없으면 실패한다. 실제 Client의 상위 MetaWorld 장치 대여는 ClientRendererFrame/앱 연결 단위에서 한다.

## 이식 내용

- 83개 보호10.78 VT의 +78/+a4/+b0 주소를 읽기 전용 표로 유지하고, 확인되지 않은 표는 예외로 거부한다. 호스트 포인터로 실행하지 않는다. 기본 getter·다리·Factory·썬 애비어리·썬더 캐논의 다섯 실제 +78을 분배한다.
- Factory, 주문/도움말 관문, 마나, 마나볼트, 간헐천, 다이스, 신전, 아처, 메타 배율/추상색 등의 이미 검증된 추가Draw 몸체를 공유한다. 일반 화면과 픽셀 선택이 같은 현재 raw/실제 가상 표를 읽는다.
- 원본 수집기의5000항목 한계·실제 깊이·그림자 비트·SID 신원을 유지한다. SpriteDraw에는 같은 현재 HP getter, 실제 FindEvent/내용물 조회, Kernel의 상태57 목록과 기한, 현재 소유자 표/배너·진단 DWORD를 연결한다.
- 실제 파일의 명명색과 고정 lookup 입력을 구별한다. 외부9색표의4/6/7/0행을 복사하며 독립 표시 입력은 기존 실제 색 계산을 쓴다. 같은 Client 공통 OwnerColorTableState를 빌린 뒤에는 지역 표를 버리고 현재 토큰/옵션을 계속 읽는다.
- JS의 프레임 표는 FrameCodeTable 자체 또는 RawWorldCore의 `{frames}` 레코드를 받아 같은188칸 번호로 연결한다. 자산 정의의 실제 flat baseFrame을 사용한다.

`Handles`의 원문 범위도 유지한다. 예를 들어 마나103의 통합 Draw/+a4는 복원하지만 기존 독립 Sprites 수집 대상에는 추가하지 않는다. 원문에 없는 지원 범위를 추정하여 늘리지 않는다.

## 검사와 최초 실패

새 `raw-campaign-factory-display.test.mjs` **7/7 통과**. 실제 자산의 TypeTable/SHP, SquidFactory, SidPool, SquidHash, SquidUnpop, RawSquidDestroy, Kernel, SquidProcessHost/State, 실제 프레임 표와 소유자 색표를 조립했다. 별도 게임 계산 모델/빈 성공 게임 장치는 만들지 않았다.

1. 비공중 일곱 종류(75/86/103/105/122/123/131)의 기본/추가 그림과 대상색 그림자를 독립 SHP Decode 합성과 전체640×480 바이트로 대조한다.
2. 실제 공간 수집의 본체/그림자 두 항목·순서/깊이·같은 SID, legacy/통합 Draw 일치, 현재 프레임 변경과 dirty clip 밖 보존, dead 그림자 생략을 확인한다.
3. CPP의 실제 선택 시험과 같은 비공중 다섯 종류(사제·윈드 타워·크리스탈·워크샾·윈드 신전)를 두 카메라에서 대조한다. 실제 opacity/색255/opaque/200밖의 **1440점**과 raw 불변을 검사한다. 윈드 신전은 실제 drawWindThunderTemple을 공급한다.
4. 현재 Player 색 번호/같은 OwnerColorTableState 토큰·표 수명과 실제 HP getter의 세 번 조회를 출력 중 확인한다.
5. 메타 장치 누락·설치 무출력·일반 화면 콜백과 실제200픽셀 선택 장치의 SID/상대점/clip/버퍼 쓰기를 확인한다. 이 항목만 메타 VT를 명시 입력으로 둔 경계 검사이며 실제 MetaWorld를 생성했다고 주장하지 않는다.
6. 실제 BombEffectTimerProcess를 로컬0x50 플래그로 호스트/Kernel에 부착하고 현재 멤버/기한/시계 변경에 따른 상태 막대 출력을 검사한다. 게임 진행/송신을 대신하는 성공 콜백은 없다.
7. 팔레트 미설치·미확인 VT·도달한 다리/신전 장치 누락과 opaque 조기 반환을 구별한다.

첫 초안3/6 실패는 검사 입력/기대값 오류였다. 마나를 Handles에 포함한다고 가정했고, 최대HP 조회를1회로 가정했으며, 추가Draw 플래그 없는 다리에 추가 장치 오류를 기대했다. 원문 범위/3회 조회/extra2 입력으로 바로잡았다. 다음 마나 픽셀 기대값은 소스색 치환으로 그림자를 계산하여 실패했으므로 실제 대상색 치환으로 수정했다. 상태 막대 검사 추가 시 서버 슬롯 권한과 실제 writer 요구에 각각 도달했다. 서버 권한 풀과 원문 로컬0x50 부착 플래그를 명시하여 실제 호스트 부착을 사용했다. 이 과정에서 제품 분배 몸체의 수정은 없었다.

전체 Node v26.13.0·v22.23.3 각각 **3915/3915 통과**, 실패/취소/건너뜀0(335.9초/378.8초). 대응표 오류0, port660·partial58·todo178이다. 83VT의 전 DWORD를 기준선 .inc와 별도 정적 대조하여 모두 일치했다.

## 남은 경계

실제 공중3종의 생성/생산/선택/행동 전용 검사를 새로 추가하지 않았다. 원본 자료 변경·게임/브라우저 실행은 없다. `GameWorld`/`RawCampaignSession`의 자연 생성·건설·수명/실제 입력 부모 시험은 해당 이식 단위에서 수행해야 한다. 현재 연결은 실제 하위 장치의 부품 조립 검사이며 실제 미션은 아직 플레이할 수 없다. ClientRendererFrame은 RawMenuControls·GameWorld·ClientGameplayGlobals를 더 요구한다.
