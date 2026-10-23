# 팔레트 clear 장치의 좁은 제안과 Kencloud 수명 잔여

단위384 준비 자료다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**으로 일치한다. 공유 제품·CMake·LEFT·git·공용 build/lib·GUI는 변경/실행하지 않았다. 기존 동결 팔레트7파일과 최초 fixture/report는 그대로 보존했다.

## 명칭 정정: 구름 캐시와 ColorSort는 서로 다른 소유자다

이전 동결 API의 `colorSortName`/`cache`/`buildColorSort`/`stepColorSort`와 함수명은 주소·제어 순서는 맞지만 **00471040/00470b20의 의미를 ColorSort라고 부른 것은 잘못**이다. 이들은 `Kencloud.cpp`의512×512 구름 캐시 생성/갱신이다. 동결7파일을 다시 쓰지 않고 이 후속 문서에서 정정한다. 이 API에 `BuildGumpShadeMaps`나 빈 성공을 연결하면 원문과 달라진다.

실제 색검색 ColorSort는 `0043b430`의 `paletteLoaded(00555e08)` 관문과 두 할당 `00555d88`/`00555dd8` 계열이다(정밀 C48685행 이후). 현재 `PaletteShade`/`PreviewColorMaps`/`SpriteColorMaps`/`ProductionColorMaps`/`OwnerColorMaps`의 정규 RGB·검색 계산은 이 색표 결과를 이미 재사용한다. 새 구름 RNG나 별도 ColorSort 전체 객체를 만들어 이를 대신할 이유는 없다.

## 실제 LoadPalette(path,1)의 지우기 순서

`004a4850`은 유효한0x308 RGB/0x400 BGRX 파일을 현재 저장 팔레트로 읽은 뒤 `004a4923`에서 clear를 검사하고, **00555e08=1**을 쓴다. clear0 또는 현재flags0은 장치를 자연 생략한다. 둘 다 비영이면 다음 순서다.

| 주소 | 효과·현재 재조회 | 현행 재사용/필수 차이 |
|---|---|---|
| `004a493b→004a14e0` | 현재 잠금깊이 증가, 첫 깊이에서 현재 drawSurf 잠금 | 기존 `Screen::Lock` |
| `004a4940..4961→004a2060` | **Lock 복귀 뒤** 현재 width/height를 읽어 `(0,0,w,h,0)`; 현재clip 적용 | 기존 `FillRect`의 bit80 경로 |
| `004a4966..498d` | 현재깊이--,0일 때 **현재 drawSurf** Unlock; 복귀 뒤 buffer/pitch/VFX 포인터0 | 기존 `Screen::Unlock` |
| `004a4993→004a1fb0` | 현재flags bit1이0이면 자연return; 비영이면 DirectDraw COM GetVerticalBlankStatus(+44) 반복 | 현GDI는 자연 생략; 실제DD장치 미복원 |
| `004a4998→004a46d0` | 현재clip 캡처, viewport 전체 `004a1800` Update, 이후clip/VFX pane 복구; DD+BackDib이면 표면 Lock/Unlock 추가 | GDI는 기존 `Update(full viewport)` 앞뒤 clip 보존 필요 |
| `004a499d→004a16d0` | 현재flags bit4가0이면 자연return; 비영이면 parallax 합성/Flip | 실제DD/flip 미복원 |
| `004a49a2..49ab` | `SetPalette(0,256,NULL,1)` 진입 | 기존 `Screen::SetPalette`, 기본/날씨 색표 계산 재사용 |

`004a46d0`은 단순 invalidation이 아니다. 현재 DIB의 지워진 내용을 **팔레트를 바꾸기 전에** 실제 창 표면으로 복사한다. 기존 `Screen::Clear`는 Lock/Fill/Unlock만 있어 이 효과가 빠져 있다. 기존 `Update`는 clip을 전체화면으로 바꾸므로 전후 같은 clip을 복구해야 한다. 현재 Screen의 viewport는 SetMode가 정한 `(0,0,width,height)`이며 별도 viewport setter는 없다. 원문 SetMode도 `005c7a54..60`을0,0,w,h로 쓴다(정밀 C130840~130861행).

**별도 fillBlt 잔여:** 원문 `004a2060`은 lowflags bit80이 없으면 Unlock→`ScreenSurface::fillBlt(004a0fb0)`→Lock 경로다. 현 `Screen::FillRect`는 bit80이 없으면 return한다. 원본과 현 SetMode의 DirectDraw불가 fallback10은 bit80이 없으므로 이 경로를 빈 성공으로 처리하면 안 된다. 제안은 일반GDI8a의 실제 지우기/표시를 연결하고, bit80없는 도달을 **Lock 뒤 명시 미복원 예외**로 남긴다. fallback10까지 완료하려면 Surface 종류1의 직접 bits 채움과 원문 Unlock/재Lock을 별도로 복원·관측해야 한다. DirectDraw/flip 역시 예외를 내며 성공을 주장하지 않는다.

## 사본 patch와 수명

[제안 patch](../../extracted/cpp-screen-palette-device-20261024-proposed.patch)는 네 기존 파일의 **신규 사본**과 새 header/검사만 포함한다. 공용 적용은 하지 않았다.

- `Screen::LoadPalette(palette,edition,clear=0)`를 추가한다. 기존 호출은clear0의 동작을 유지한다. 파일 읽기 뒤 새 header의 작은 순서 helper가 loaded1과 필수 장치 순서를 소유한다. 일반GDI에서 기존 Lock/Fill/Unlock/Update를 실제 호출하고, 미복원DD/flip/fillBlt는 도달 시 예외로 남긴다.
- `paletteLoaded` 실제 DWORD는 **Client가 Screen보다 먼저 소유**하고 최초/해상도 변경 Screen 모두 같은 주소를 빌린다. Screen 안의 독립 bool/매번0 사본은 쓰지 않는다. Screen은 원래 해상도 변경 때 재생성되지만 원문의00555e08은 Client 전역이므로 이 차이를 보존한다. Client없는 기존 독립 Screen만 명시 local0 fallback을 쓴다. Client 슬롯은 Screen보다 뒤에 파괴되어 참조가 유효하다.
- `Client::LoadPaletteFile(path,clear)`는 완성된 **GamePalSpec 경로를 재결합하지 않고** 현재 파일/Screen에 보낸다. `RefreshPaletteConsumers()`는 기존 AudioTints/소프트웨어커서/`UberGump::PaletteChanged` 또는 Renderer invalidation을 재사용한다. 기존 `LoadScenePalette(name)`은 DataDir 경로→LoadPaletteFile(path,0)→같은 소비자 갱신의 기존 순서를 유지한다.
- 신규 전환 부모의 load hook은 LoadPaletteFile(path,1), 후속 maps/소비자 hook은 기존 표 계산과 RefreshPaletteConsumers에 합성할 수 있다. **RefreshPaletteConsumers 하나가 원문0043c010 전체라는 뜻은 아니다.** 현 Screen::LoadPalette는 기존처럼 ShadeMaps까지 미리 계산한다. 색표 계산 시점/원문 파일 캐시·추가 표의 물리 소유자는 별도 통합 대조 대상이며, 새 helper 검사가 이를 증명하지 않는다.
- PathSpec 반환 문자열은 완료된 호출까지 같은 Client 소유 물리에 NUL을 포함해 유지해야 한다. 임시 `std::string`의 `c_str()`을 span으로 반환하거나 DataDir를 다시 붙이면 안 된다. 이름/구름 캐시/렌더러 현재DWORD/dirty의 장기 소유자는 이 좁은 patch에서 새로 만들지 않는다.

## Kencloud 최소 후속 경계

정밀 C92501~92548/92551~92576/92863~92923행과 실제 PE를 읽은 결과다. 이 절의 하위 전체는 **정적 분석**이며 신규 기계어 실행 fixture로 검증한 것으로 표시하지 않는다.

| 실제 전역 | PE 최초값/형태 | 필요한 수명·효과 |
|---|---|---|
| `00569ffc` | BSS0 pointer | Client 장기512×512 BYTE 할당/해제. `00470bb0` CloudFileSpec→유효512×512 GIF, 없거나 크기 다르면free 뒤 fallback |
| `00569ff0/f8/f4` | BSS0 | 같은 할당의 end/+512/+0x3fe00 뷰. 소유권 사본이나 별도배열 금지 |
| `0056a000`, `005697f0` | 각512 DWORD 표 | 현재 dirty/처리행. step과 draw/update가 공유 |
| `0056a800` | seed pointer표, 현재count400 | 같은 캐시의 내부 주소400개를 두 난수 호출 뒤 현재base로 계산 |
| `0056b7a0` | BSS 이름, 다음설정은+0x54 | 구름 이름. 다음0056b7f4/f8/fc/800/804와 연속 물리를 고려해야 하며 임의256자 독립 안전문자열 의미를 부여하지 않음 |
| `00540a34/38` | 228/245 | 구름 색 범위. fallback new0x40000 전체는 현재34의 lowBYTE로 채움 |
| `00540a7c/80/84/88/8c/90` | 72/2/10/32/99999/400 | cloudiness/cloudSpeed/cloudFury/cloudSpacer/cloudRecenter/현재seed count. InterpretOptions가 같은DWORD에 씀 |
| `00540a98[16]` | 0,8,4,12,1,11,5,15,3,10,2,7,14,9,6,13 | 현재phase0056b804를+1&15한 뒤 이 표에서 시작행 선택 |
| `00532710` | 원본 단일 GameRandom 상태 | `004558c0`의 같은 게임 난수. CRT holdrand와 전혀 별개 |

`00471040`은 cache가 이미 비영이면 dirty정리/전체표시 플래그만 쓴다. 처음이면 CloudFileSpec을 시도하고 fallback만 전체 색 채움과 signed `10000/cloudFury` 회의 warm-up을 수행한다. **GIF 로드 성공도** seed400×두GameRandom(510,512)과 spacerGameRandom(256)+5를 소비한다. `00470b20`은 phase를 진행하고 첫BYTE를end[-1]에 기록한 뒤0x3dd stride의 겹치는 `memmove`와512행 dirty표를 갱신한다. 기존 `UberGump::clouds_`는 DecodeGif→정적 Tile 배경이며 이 부모의 변화/할당/난수 소비를 실행하지 않는다.

현재 소스의 ClientMain/UberGump/RawCampaignSession 공개에는 GameRandom 소유자나 getter가 없다. `RawCampaignSession::Impl::random`(322행)이 같은 seed/SP/지형 등의 실제 상태이며 `GameWorld::Session()`만 공개되어 있다. `UberGump::HasWorld()`는 bool이고 current random의 getter가 아니다. 설치 중 `world_=make_unique<GameWorld>`가 아직 끝나지 않을 수 있으므로 메뉴를 통해 새 Session을 찾거나 public loaded관문을 우회했다고 가정하면 안 된다.

현재 최소안은 **새 RNG 생성 없이**, 이미 생성된 설치중 Impl.random을 private/설치전용 필수getter로 빌려 설치부모가 Kencloud `random(limit)` hook에 직접 연결하는 것이다. dialog의 설치분리 설계도 ctor에서 Session/Impl/random을 만든 뒤 Editor 파일/전체설치를 별도로 실행한다. 따라서 palette는 loaded=false에서 그 현재 Session.random에 직접 접근해야 한다. 장기 메뉴/월드 없는 구름 프레임까지 동일00532710을 요구하는 후속에는 Client의 기존 실제 GameRandom을 단일 소유로 일반화할 근거·현재값 보존이 추가로 필요하다. **새GameRandom, 별도시드, CRT치환, 호출뒤state사본동기화는 제안하지 않는다.** 기존 GameRandom/미션수명 변경과 Kencloud 전체 복원은 이 Screen patch에 포함하지 않는다.

## 선관측·격리 검증 범위

[독립 경로](../../extracted/cpp-palette-device-20261024-proposed/README.md)에 최초 fixture/report·사본·검사·로그·최종SHA를 보존한다.

- 최초 원문은 파일 읽기가 끝난 **004a4923**에서 정확한 문맥(EDI0,stackclear+38)에 진입해 **SetPalette004a2640 진입 checkpoint**까지 실행했다. 실제 inline unlock,004a1fb0/004a46d0/004a16d0의 GDI 자연 분기와clip복구를 실행했다. Lock/FillRect/Surface.Unlock/Update는 명시 장치이고 실제GDI/OS/파일팔레트본문은 실행하지 않았다. DD/flip은 실제 부모 진입에서 명시 중단했다.
- 초기 계측 실행은 허용 범위가004a1fdc의ret를 제외해 fixture 생성 전에 중단했다. Capstone으로ret를 확인해 끝제외값을004a1fdd로 교정한 뒤 최초 자료를 생성했다. 제품/fixture를 맞추기 위한 변경이 아니며 생성 뒤 덮어쓰기는 거부한다.
- **83입력×027f/037f=166관측/7948명령**이다. checkpoint122/DDvblank18/DDflip18/lock·fill·unlock·update중단각2이다. 두 정밀도/전체13필드/64BYTE물리/trace를 새 사본 helper의 두 검사로 모두 대조해 실패0이다. 새 검사에는 자연 생략/도달한 필수콜백 누락도 포함한다.
- 사본 `Screen.cpp`/`ClientMain.cpp` MSVC `/Zs /W4` 문법검사는 종료0이다. 기존 `getenv/fopen`의 C4996 경고4개가 있으며 신규 변경 줄의 경고/오류는 없다. 최초 syntax1 뒤 loaded DWORD를 Client 수명으로 교정한 syntax2와, root의 owner→Subscriber→State/UI접합 현문맥으로 다시 작성한 **syntax3**를 구별한다. syntax3가 최종 사본이다. helper 검사 빌드/실행은2검사0실패/종료0이다. 공유 lib는 링크하지 않았다.
- 실제 Screen/Client/GDI 객체를 생성해 표시한 검사, 전체Editor loaded 성공, 전체Kencloud 기계어 대조를 수행한 것이 아니다. 후속 실제GDI 사용자 확인은 일반창GDI8a에서 이름변경→구clip지우기→viewport복사→팔레트변경/전체표시, 같은이름의자연생략,두다른팔레트반복,해상도재생성후같은loaded주소를 관측해야 한다. bit80없는fallback과DD는 요구경계를 복원한 뒤 별도 검증한다.
