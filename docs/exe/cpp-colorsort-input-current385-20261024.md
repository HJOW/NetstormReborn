# ColorSort 고정 입력의 현재385 호환성과386 선택 목록

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 마지막 수행2026-10-23**이다. 단위386 준비이며 공유 제품·CMake·LEFT·git·공용 build/lib·게임·GUI를 변경하거나 실행하지 않았다. 최초 Native 도구/fixture/report와 기존 동결 제안50파일은 모두 그대로다.

## 정확한 적용 자료와 현재 기준

실제 제안 경로는 [extracted/cpp-colorsort-input-20261024-proposed.patch](../../extracted/cpp-colorsort-input-20261024-proposed.patch), SHA256 **68ccbf770a9fab9e364804c82a1c57276db03b539a67d59e9f06d23787262c7c**다. 기존15파일을 수정하는 패치이며 아래 신규7파일의 추가/등록은 별도다. 같은 이름의 새 패치를 만들어 최초 자료를 대체하지 않았다.

현재385의15원본 파일은 최초 proposal의 baseline SHA와 **15개 모두 동일**하다. Session의 checkpoint/CoreWrite와 ConfigInterface 후속 변경은 이15개 밖이다. 따라서 이번 재검토 시점에는 코드 수정 없이 같은 패치를 사용할 수 있다.

[현재385 격리 사본](../../extracted/cpp-colorsort-input-20261024-current385)은 현재 원문을 `baseline/`에 복사하고 패치의36개 hunk를 **문맥 완화 없이 정확히 재생**한 `copy/`를 보존한다. 15결과 모두 최초 proposal 사본과 같고 충돌0이다. [replay.json](../../extracted/cpp-colorsort-input-20261024-current385/replay.json), SHA256 `306a833496ef6128fc0d481b87130fe114535cbc9a07fc74f5da249421ba08bd`에 각 기준/결과 SHA, 현재 Session/ConfigInterface 문맥 SHA, 최초50파일 불변과 신규 선택을 기록한다. 이것은 패치 사본의 정적 호환 검사이며 이번에 컴파일·테스트·실제 Client 실행을 다시 수행한 기록은 아니다.

## 기존15파일의 현재 원본 SHA256

| 파일 | SHA256 |
|---|---|
| `cpppj/src/client/ClientMain.h` | `659f01a36f46a78cd2bde396e2466155e6ebde98e82584f7195d5ce044ee58a1` |
| `cpppj/src/client/ClientMain.cpp` | `54240c618fec0a0729e0e12919a8d17b4f651bdb19ce5e8c803a16c9619a87b8` |
| `cpppj/src/client/Screen.h` | `2ff3382a41fa53e0a7d3dcee890c569d0789544d77aa3635f1092de7b73320f8` |
| `cpppj/src/client/Screen.cpp` | `7a7df6ff43ea48cba984e329c0a4b77cad51650852d6ae8c7d52eac121f7b533` |
| `cpppj/src/client/RawCampaignFactoryDisplay.h` | `672b36dcf4fd5b3841e8a0c915f3d260e5c52129868d9a1a5d80523f7ea59531` |
| `cpppj/src/client/RawCampaignFactoryDisplay.cpp` | `8568f48c774dea5a6879bd6055d233c64af610c8274693403c4081e4b623959d` |
| `cpppj/src/client/RawCampaignCursorDisplay.h` | `e30e775cf87189bfa1dfeb8a26075decd485dea5e2d366056df8579dcc2f5b73` |
| `cpppj/src/client/RawCampaignCursorDisplay.cpp` | `8e3f58a74524806c727a1ea81ed725495a22fc41270a77fd6457342925a80117` |
| `cpppj/src/client/RawProductionControls.h` | `8826c67edd4bc027f1d060bcd1e0fe5aa4638c4683b05fd6d2c57e931c90b346` |
| `cpppj/src/client/RawProductionControls.cpp` | `caa8cddcba694d141e4bdcd0c7642b041a6078ec170108f0ec5fe60b9b1fae6a` |
| `cpppj/src/client/RawCampaignCursor.h` | `6351acc11edfaa4537d8f71230897ca8a35f19294c07b8260d0c053d45901373` |
| `cpppj/src/client/RawCampaignCursor.cpp` | `ca10fce916135c4b3f7a5cfb619490ddc613e0c7864487b465eb3e414e47a9a2` |
| `cpppj/src/client/GameWorld.h` | `da7e796b9cf9f479fca568e2f47edd1e8b730caa7cb20625a1ceb1f07591b0c4` |
| `cpppj/src/client/GameWorld.cpp` | `b58fd39af2a812cc9f89154fde5e4b95f9de99ad1eb0aec97353b977bb025ef0` |
| `cpppj/src/client/UberGump.cpp` | `b1031ba955f852537780fc342254ee8215462afccf81625c8ede68dc277a67e8` |

## 신규7파일의386 선택 목록

아래7개는 첫 작업에서 새로 만든 제품2·검사1·원문 도구1·fixture1·보고서1·문서1이다. 이 목록은 패치가 이미 담은 기존15파일과 구별한다. 이번 후속 호환 문서/사본 기록은 추가 인수인계 자료이며 이7개를 다시 작성하지 않는다.

| 파일 | SHA256 |
|---|---|
| `cpppj/src/client/ClientColorSortInput.h` | `3664d63a1e068242e6670a350887a35d0f428f22bea57943c5b6e244c39f49df` |
| `cpppj/src/client/ClientColorSortInput.cpp` | `2495412cb2e2c241484a653941caed13d303b0de27ae559382e9f9e686e166c5` |
| `cpppj/tests/ColorSortFixedInputTests.cpp` | `e03195d11245e0ffb372cbfdb7ae2e3a2ef68224531dc3bea33f856aa2373d1b` |
| `tools/decomp_colorsort_fixed_input.py` | `d4c967824796b97388a7cf68e69bb2010902da4d6ba2d95e44ef6effca55b81b` |
| `cpppj/tests/fixtures/colorsort-fixed-input-x86.tsv` | `70684bfd514b29e981d67faa4d2abc6cababcc5ed275b22f495f58e41d65813f` |
| `cpppj/colorsort-fixed-input-native.json` | `c7c623362b079f382f074ae467211e73dd2865e8359a686c7bd24c6251cb0de7` |
| `docs/exe/cpp-colorsort-fixed-input-next-20261024.md` | `8241cd211022db88806f931cddacd9f6be391ecf5eef474fcbb20b860ad3e73c` |

공용 등록은 root가 제품 cpp1·검사 cpp1과 fixture 경로를 추가한다. 원문6관측과 독립2검사는 최초 기록의 범위 그대로이며 전체 실행의 팔레트 교체/해상도/미션 수명 검증을 대신하지 않는다.

## 다른386 제안과 충돌

- dialog의 예정 `extracted/cpp-client-chat-core-owners-20261024-proposed.patch`는 **ClientMain.h/.cpp 두 경로가 겹친다.** include·전방선언·멤버/getter·생성자에 새 Chat/Core 소유자를 추가하므로 인접 hunk 문맥이 달라질 수 있다. ColorSort Run/팔레트 본문은 건드리지 않는다고 담당자가 확인했다. 이 후속 제안은 아직 동결 전이므로 SHA를 추측하지 않으며 root는 실제 적용 순서에서 두 생성자 목록과 선언 순서를 다시 조합한다.
- SavedStateRead, StateWorldCleanup, DotCommandRegister는 신규 독립 파일 범위라 이15파일과 경로가 겹치지 않는다고 각 담당자가 확인했다.
- ConfigInterface 장기 global/fort 수정은 이15개 밖이다. Native Config/ColorSort 몸체를 그 이유로 바꾸지 않는다.

## 실제 PE 및 잔여 접합 범위

PE의43b430은439de0의 첫 입력으로 현재 논리RGB를 캡처하고, 이후4b6100의43c010은 같은 ColorSort를 사용한다. 동결 수명 oracle은 같은 source 주소/값에서 화면 입력을 두 번 바꾸는6관측으로 그 구별을 고정했다. 제안은 같은 Client Input을 두 Screen과 World/Factory/Cursor/생산 소비자에게 빌리며 기존 표시RGB/명명색은 현재 팔레트를 따른다. 장기 owner는 menu/world와 Screen보다 늦게 파괴되고 Screen 교체는 getter만 다시 연결한다. 동료가 이 수명/입력 분리를 읽기 검토해 명백한 문제를 찾지 못했다.

15파일 사본을 전체 ColorSort 접합 완료로 확대하지 않는다. 특히 현재 `RawCampaignSession.cpp`의 `ConnectBuilding` 안 `buildingCaptureState.ownerColors`는 전용5종 표를 lazy 생성할 때 여전히 `assets.game.Palette()`의 COL을 직접 읽는다. `ConnectCursor` 최초 SetPalette도 같은 COL 입력이다. 뒤의 GameWorld SetPalette가 커서 표시기를 갱신하더라도 Session의 건설/파편 캡처용 전용표 저장소까지 같은 초기 Client Input을 빌리게 하지는 않는다. 실제 초기값이 우연히 같다는 이유로 이 경계를 완료 처리하지 않는다. 기존15를 임의16파일로 확대하지 않았으며 별도 Session 서비스/getter와 그 수명 접합이 필요하다.

raw0x3800/0x5008 검색 객체·qsort와 초기 `0043c4e0`의 `!color.dat` 캐시 부모도 여전히 별도다. 최초43c010 계산을 현재 Screen RGB로 다시 만드는 경로나 누락 장치를 빈 성공으로 채우는 경로는 추가하지 않았다. 다음 작은 부모는 별도 정적 근거/Native 선관측 뒤 준비한다.
