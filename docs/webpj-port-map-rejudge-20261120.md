# 대응표 todo·partial 재판정 (WB04-E1098)

사용자가 고른 작업이다: `PORT_MAP.tsv`의 todo 행 중 웹이 이미 다른 모듈로 구현한 것을 증거로 찾아 상태와 웹 모듈 칸을 바로잡고, 남은 단위·줄 수를 다시 센다. 분류(class)는 `tools/port-map.mjs`·`rebaseline-225553df.json`이 정하므로 바꾸지 않고 status·web·base·메모만 고쳤다.

## 방법

1. cpppj 단위 파일이 쓰는 원본 본체 이름(`o::Raw…`)을 모은다.
2. 웹의 `export class/function` 표에서 같은 이름의 모듈을 찾고, 그 본체를 `new 이름(`/호출로 만드는 웹 모듈(Session 조립)을 찾는다.
3. 본체 이름이 없는 단위는 파일 내용을 읽어 같은 일을 하는 웹 모듈을 수동으로 짝짓는다.

## 결과

- **RawSurface 29행(todo→partial).** cpppj의 `RawSurface*.cpp`는 기준선 뒤 `14fd017c`(+413줄)에서 자산 Regular/PostPop/Pop/Unpop·전송·표시 장기 소유자 연결을 `RawSurfaceStorage`에 모은 Native 소유 구조다. 웹은 같은 원본 본체(예: `RawBalloonBody`는 `raw-campaign-session-types.js`가 훅 8개[frames·create·pop·unpop·destroy·check·allocateRegular·constructRegular]까지 같게 연결하고 `raw-campaign-session-regular.js`가 `cloudRegular`로 분배한다)를 Session 조립 모듈(`raw-campaign-session-*`, `raw-campaign-asset-bindings.js`, `raw-surface-world.js`, `raw-priest-world-bindings.js` 등)에서 만들고 연결하며 데모 3종과 모든 캠페인 미션의 월드 진행이 그 위에서 돈다. 그래서 todo(웹 코드 없음)는 사실이 아니다. 다만 cpppj의 `Bind*Runtime` 대여·세션 사이 장기 수명·NoWorld/Native 경로는 옮기지 않았으므로 partial로 올렸고 남은 것은 4D-8g 재대조에 남긴다.
- **Dialog 2행(todo→partial).** `DialogScript`(Tell 본문 치환 뒤 1차 HTML의 조건·명령·지시문과 2차 입력)는 `ui/html-text.js`의 `prepareDialogContent`·`tell-parameters.js`·`tell-window.js`가, `DialogBodyLayout`(본문 줄 배치·스크롤)은 `html-body-gump.js`·`tell-window.js`가 맡는다.
- **HelpLinkPolicy(partial→port).** 옛 메모의 "바깥 링크 새 탭/창 열기는 미구현"이 낡았다. WB03-AD에서 구현했고(`ui/external-link.js`·`platform/external-opener.js`는 웹 전용 모듈이라 행에 쓰지 않고 메모에 둔다) `external-access.test.mjs`·`e2e/external-opener.spec.js`가 확인한다.

### 재판정한 RawSurface·Dialog 행의 웹 대응

| cpppj 단위 | 웹 모듈(증거) | 상태 |
| --- | --- | --- |
| `DialogBodyLayout.cpp` | ui/html-body-gump.js, ui/tell-window.js | partial |
| `DialogScript.cpp` | ui/html-text.js, ui/tell-parameters.js, ui/tell-window.js | partial |
| `HelpLinkPolicy.cpp` | ui/help-link.js | port |
| `RawSurfaceAssetDeletion.cpp` | core/raw-region-ownership.js | partial |
| `RawSurfaceAssetNotice.cpp` | core/raw-campaign-session-aviaries.js | partial |
| `RawSurfaceAttack.cpp` | core/raw-campaign-session-flyers.js, core/raw-acid-fence.js, core/raw-campaign-session-arc-spire.js, core/raw-altar-sacrifice.js, core/raw-dais-lifecycle.js | partial |
| `RawSurfaceBalloon.cpp` | core/raw-campaign-session-types.js, core/raw-campaign-asset-bindings.js, core/raw-campaign-session-devastation.js | partial |
| `RawSurfaceBomb.cpp` | core/raw-campaign-session-spells.js, core/raw-campaign-session-devastation.js, core/raw-campaign-asset-bindings.js | partial |
| `RawSurfaceCarrier.cpp` | core/raw-campaign-session-carrier-regular.js | partial |
| `RawSurfaceContent.cpp` | core/raw-campaign-session-contents.js, core/raw-carrier-content-notice.js | partial |
| `RawSurfaceDaisDeletion.cpp` | core/raw-campaign-session-priests.js, core/raw-altar-sacrifice.js, core/raw-dais-lifecycle.js, core/raw-campaign-session-ritual.js | partial |
| `RawSurfaceDamageable.cpp` | core/client-production-sync.js, core/raw-player-technology.js, core/raw-campaign-session-damageable.js | partial |
| `RawSurfaceDecoration.cpp` | core/raw-decoration-regular.js, core/raw-campaign-session-regular.js | partial |
| `RawSurfaceDeletion.cpp` | core/squid-destroy-lifecycle.js, core/raw-surface-world.js | partial |
| `RawSurfaceDepend.cpp` | core/depend-form-host.js, core/raw-surface-world.js | partial |
| `RawSurfaceDudeRegular.cpp` | core/raw-campaign-session-damageable.js, core/raw-dude-drop.js, core/raw-campaign-asset-bindings.js, core/raw-campaign-hit-bindings.js | partial |
| `RawSurfaceFactoryRegular.cpp` | core/raw-campaign-asset-bindings.js, core/raw-campaign-session-devastation.js | partial |
| `RawSurfaceFence.cpp` | core/raw-campaign-session-fence.js | partial |
| `RawSurfaceFlare.cpp` | core/raw-campaign-session-flare.js | partial |
| `RawSurfaceFrameRegular.cpp` | core/raw-campaign-hit-bindings.js, core/raw-campaign-archer-bindings.js | partial |
| `RawSurfaceGump.cpp` | core/gump-form-host.js, core/raw-campaign-session-contents.js | partial |
| `RawSurfaceIce.cpp` | core/raw-campaign-session-ice.js | partial |
| `RawSurfaceLifetime.cpp` | core/raw-surface-world.js | partial |
| `RawSurfaceMana.cpp` | core/raw-campaign-session-arrivals.js | partial |
| `RawSurfacePlayerRegular.cpp` | core/raw-player-regular.js, core/raw-campaign-session-players.js | partial |
| `RawSurfacePriest.cpp` | core/raw-priest-regular.js, core/raw-priest-world-bindings.js | partial |
| `RawSurfacePriestDeletion.cpp` | core/raw-priest-world-bindings.js | partial |
| `RawSurfaceRegistryDeletion.cpp` | core/raw-campaign-session-players.js, core/raw-player-regular.js, core/raw-campaign-session-damageable.js | partial |
| `RawSurfaceResidenceRegular.cpp` | core/raw-residence-regular.js, core/raw-campaign-asset-bindings.js | partial |
| `RawSurfaceReward.cpp` | core/squid-reward.js, core/raw-campaign-session-owners.js | partial |
| `RawSurfaceRitual.cpp` | core/raw-priest-world-bindings.js, core/raw-campaign-asset-bindings.js, core/raw-campaign-session-devastation.js, core/raw-altar-sacrifice.js, core/raw-dais-lifecycle.js | partial |
| `RawSurfaceVortex.cpp` | core/raw-campaign-asset-bindings.js, core/raw-campaign-session-devastation.js | partial |

## 집계

todo 75 → 44, partial 69 → 99, port 878 → 879(`HelpLinkPolicy`). 이식 대상 1,022단위 중 port 879(86.0%)다. 남은 단위는 partial과 todo의 합으로 144 → 143단위, 줄은 33,538 → 33,448줄이다(partial에는 파일 전체 줄 수를 센다). 이번 재판정은 "웹 코드가 전혀 없다"는 오판을 바로잡는 것이어서 남은 양 자체는 거의 줄지 않았다.

## 아직 todo인 44행(웹 대응을 확인하지 못함)

`RawMenuControls`·`RawMenuChat/Html/Loading`(메뉴 허브와 채팅 창), `SkirmishSetup`·`SkirmishStartIslands`·`RawBattleIsland`·`RawCentralIsland`·`RawIslandPartition`(12차 스커미시 월드 생성), `Native*PacketReceivers`·`ClientPacketSources`·`RawPacketDelay`·`ClientMeta*`·`HudNsInfoMeta`·`Native*Meta*`·`ClientChallengeGlobals`(Native 고정 수신기·메타), `ClientConstruction*` 여섯(Native 건설·무작위 배치), `Movie*`·`SmackerDecoder`·`RawMovieControls`(동영상), `SoundMusicFile/Runtime`·`GifScreenshot`·`ExternalHelpClient`(웹 대체 경계), `AssetDeletionVtables`·`FortCoreDataLoad`·`NativeFortSaveVirtual`·`ClientTitleBitmaps`·`HudPlayerButtonDraw`·`Native{NsInfo,PlayerInfo}Input`·`RawNsInfoContext` 등이다. 이들은 웹에서 같은 일을 하는 모듈을 찾지 못했거나(스커미시·채팅·동영상) Native 고정 구조라 4D-8·5-C 단위에서 따로 다룬다.
