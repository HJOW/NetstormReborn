// NetStorm 웹 클론(webpj)의 앱 수명·장면 전환 모듈이다.
// 브라우저 안에 게임 Canvas 를 만들고, 컨테이너 크기·화면비(4:3, 16:10, 16:9)·기기 픽셀 비율에 맞춰
// Canvas 의 크기와 위치를 계산·적용하며, 포인터·키보드 입력을 게임 논리 좌표로 바꾸고,
// "로딩(자산 적재) → 클릭하여 시작 → 메인 메뉴(원본 복원)" 장면 전환과 프레임 루프를 제공한다.
// 메인 메뉴는 원본 자산(www/data)을 8비트 색 번호로 합성해 표시하고, 돌 버튼 입력·button.wav·메뉴 음악을 원본 규칙대로 처리한다.
// 메인 메뉴 버튼이 여는 다음 화면(캠페인·데모·도움말·옵션 등)과 게임 규칙은 아직 없다(후속 단계). 그 화면들의 본문(Tell 섹션)은 GameData 로 조회할 수 있다.
// 이 모듈은 import 시점에 DOM 을 건드리지 않으므로 Node 에서도 불러와 계산 함수를 검사할 수 있다.
// (2026-10-23 WB01: 실행 골격 최초 작성)
// (2026-10-23 WB03-M1: 원본 자산 적재·메인 메뉴·소리·원본 커서 연결)
// (2026-10-28 WB02-D: 데이터 아카이브(TAFF)·설정·Tell 섹션 조회(GameData) 연결, 메인 메뉴의 Edit 버튼 제거)
// (2026-10-31 WB03-N: 언어가 한국어면 갈무리11 글꼴·번역표 묶음을 받아 메인 메뉴·펼침 메뉴 라벨을 한국어로 그린다)
import { AssetLoader } from './assets/asset-loader.js';
import { GameData } from './assets/game-data.js';
import { GamePalette } from './assets/palette.js';
import { loadTerrainScene } from './assets/terrain-scene.js';
import {
    BUTTON_SOUND_NAME,
    MENU_ASSET_PATHS,
    MENU_BUNDLE,
    MENU_MUSIC_BUNDLE,
    MENU_MUSIC_NAME,
    MENU_SOUND_NAME,
    MENU_SOUND_PATH,
    buildMenuResources,
} from './assets/menu-assets.js';
import { KOREAN_BUNDLE, buildKoreanSupport } from './assets/korean-assets.js';
import { buildJapaneseSupport } from './assets/japanese-assets.js';
import { JAPANESE_BUNDLE, JAPANESE_FONT_FAMILY } from './assets/japanese-asset-config.js';
import { createJapaneseGlyphRasterizer } from './platform/japanese-font.js';
import { KOREAN_FONT_FAMILY } from './assets/korean-font-config.js';
import { AudioSystem, createAudioContext, volumeStepToCentibels } from './audio/audio-system.js';
import { SoundList } from './audio/sound-list.js';
import { MissionAudioAssets } from './audio/mission-audio-assets.js';
import { SceneMusicDriver } from './audio/scene-music-driver.js';
import { ThunderProcess, THUNDER_PROCESS_TYPE } from './core/thunder-process.js';
import { makePriestShieldNoticeHooks } from './core/sound-process.js';
import { confirmSendMoney, dispatchSendMoneyInput, isSendMoneyCommand, sendMoneyScriptCode } from './core/send-money-input.js';
import { dispatchChatInputKey } from './ui/hud-chat-geometry.js';
// 채팅 표시 옵션 열 개의 원문 키(오타 포함)이다. 값이 없으면 웹은 켜짐(1)을 기본으로 둔다. (2026-11-22 WB04-E1172)
// 채팅 도우미 메뉴가 입력 큐에 올리는 명령이다: 삽입(0x02000115, 채팅 창이 처리)·설정 대입(0x0200006d)·Edit Back/Front Format(0x02000105/106, E1179 부터 서식 입력 창을 연다)·수신자 추가/제거(0x02000027/101)·재부착(0x020000a6, 채팅 창이 처리). 이 목록 밖의 미구현 명령은 큐를 막으므로 여기 올려 소비한다. (2026-11-22 WB04-E1177)
const CHAT_HELPER_COMMANDS = new Set([0x02000115, 0x0200006d, 0x02000105, 0x02000106, 0x02000027, 0x02000101, 0x020000a6]);
const CHAT_OPTION_KEYS = new Set(['ShowBattle', 'ShowEveryone', 'ShowAllies', 'ShowObserver', 'ShowWisper', 'ShowMyAllies', 'ShowMyEmenys', 'ShowMyNeutrals', 'ShowChatWindow', 'PlaySoundMessage']);
import { ClientChatManagerGlobals, makeClientChatPacketHooks } from './core/client-chat-manager-globals.js';
import { ClientDotCommandGlobals } from './ui/dot-command-globals.js';
import { createRawChatWindowContext } from './ui/raw-chat-window-context.js';
import { dispatchChatPacket } from './ui/raw-chat-receive.js';
import { bindClientChatMessage, bindClientChatRecipients, bindClientChatSubmit, findClientChatSenderOwner, initializeClientChatManager, initializeClientIssueCommands, refreshClientChatRankPrefix } from './core/client-chat-runtime.js';
import { captureSessionStateDump, formatSessionStateDump } from './core/session-state-dump.js';
import { handleAllianceRequestInput } from './core/raw-player-participation.js';
import { createSoundState, SoundPlayer } from './audio/sound-player.js';
import { WebSoundDevice } from './audio/web-sound-device.js';
import { IndexedSurface } from './render/indexed-surface.js';
import { IndexedPresenter, normalizeScaling } from './render/presenter.js';
import { GameAgentHost } from './platform/game-agent-host.js';
import { installWebMcp } from './platform/webmcp-tools.js';
import { Screen, ScreenMode } from './render/screen.js';
import { Renderer } from './render/renderer.js';
import { SquidRenderer } from './render/squid-renderer.js';
import { prepareRendererFrame } from './render/raw-renderer-prelude.js';
import { appendRendererCloudDirty, refreshRendererCloudBackground } from './render/raw-renderer-clouds.js';
import { CampaignRendererDrawing } from './render/campaign-renderer-drawing.js';
import { GumpRedraw } from './render/gump-redraw.js';
import { GumpDrawState, GumpPixelTarget, setGumpDrawClip } from './render/gump-draw.js';
import { Settings, findSettingKey, getLocalStorage } from './platform/settings.js';
import { Progress, isProgressKey } from './platform/progress.js';
import { reportGameError } from './platform/error-reporting.js';
import { BrowserInputPoll } from './platform/browser-input-poll.js';
import { queueBrowserMouseEvent } from './platform/browser-mouse-events.js';
import { queueBrowserVirtualKeyEvent, queueBrowserCharacterEvent } from './platform/browser-key-events.js';
import { DETONATE_BOMB_COMMAND, dispatchDetonateInput } from './core/detonate-input.js';
import { dispatchRotationKeyInput } from './core/rotation-key-input.js';
import { dispatchPauseKeyInput } from './core/pause-key-input.js';
import { dispatchConstructionHistoryKey, dispatchLostPositionKey, initializeConstructionHistory } from './core/construction-history-key.js';
import { captureSavedCameraCenter, dispatchSavedCameraKey, initializeSavedCameraPositions } from './core/saved-camera-key.js';
import { VORTEX_COMMANDS, dispatchVortexCommandInput } from './core/vortex-command-input.js';
import { dispatchScenarioObjectInput, isScenarioObjectCommand } from './core/scenario-object-input.js';
import { InputCode, InputQueue } from './core/input-queue.js';
import { hasExtendedDisplayText, queueCommittedText } from './core/display-text-input.js';
import { GumpPointerBoundaryState, prepareGumpPointerBoundaryWithQueue } from './ui/gump-pointer-boundary.js';
import { ClientFrameClock } from './core/client-frame-clock.js';
import { ClientLocalStateGlobals } from './core/client-local-state-globals.js';
import { ClientNsInfoGlobals } from './core/client-ns-info-globals.js';
import { QuickHelpPopupState } from './ui/quick-help-popup-state.js';
import { ClientKencloudGlobals } from './core/client-kencloud-globals.js';
import { ClientColorSortInput } from './core/client-color-sort-input.js';
import { MemoryColorMapFiles } from './core/color-map-files.js';
import { ClientHtmlWindowGlobals } from './core/client-html-window-globals.js';
import { ClientConnectionGlobals } from './core/client-connection-globals.js';
import { ClientConnectionActivityGlobals } from './core/client-connection-activity-globals.js';
import { ClientChatWindowGlobals } from './core/client-chat-window-globals.js';
import { ClientRendererFrameGlobals } from './core/client-renderer-frame-globals.js';
import { currentAllocationBytes } from './core/current-allocation-bytes.js';
import { bindCampaignAiClientServices, bindCampaignClientInputServices, bindCampaignOwnershipTheme, bindCampaignProductionRefresh, captureCampaignMissionStarting, makeCampaignMissionStartEffects, runCampaignMissionStarting } from './core/campaign-client-services.js';
import { bindCampaignUserInputServices } from './core/campaign-user-input-services.js';
import { applyThemeTechnology, dispatchGetTechnologyInput, CHEAT_ALL_TECHNOLOGY_COMMAND } from './core/get-technology-input.js';
import { parseViewSpot } from './core/camera-home.js';
import { ARROW_CAMERA_STEPS, CampaignUserInput } from './core/campaign-user-input.js';
import { makeCampaignCursorClientEffects } from './core/campaign-cursor-client-effects.js';
import { RawContainedFinder } from './core/raw-contained-finder.js';
import { PRODUCTION_BRIDGE_COMMAND, PRODUCTION_REPEAT_COMMAND } from './core/production-shortcuts.js';
import { ProductionControlContext } from './core/production-control-context.js';
import { cleanupNetstormStateWorld } from './core/netstorm-state-world.js';
import { CLIENT_PARTICLE_PALETTE_INPUT } from './core/client-particle-palette-input.js';
import { ProductionMaintenanceGlobals, ProductionMaintenancePanel } from './core/raw-production-maintenance.js';
import { GameMenuActions } from './core/game-menu-actions.js';
import { gameMenuCommandValue, gameMenuScriptCode, isGameMenuCommand, queuePlayerListKey } from './core/game-menu-commands.js';
import { KnowledgeMenuKind, knowledgeScriptCode } from './core/raw-knowledge-menu.js';
import { RawFortBadgesGlobals } from './core/raw-fort-badges-globals.js';
import { resetAmbientSoundDeadline, stepAmbientSound } from './core/raw-ambient-sound.js';
import { pauseMissionWorldEntry } from './core/mission-lifecycle.js';
import { pauseMissionFrameForDialog } from './core/raw-mission-frame-gate.js';
import { dispatchGameTimeKey, readGameTimeSeconds } from './core/game-time-display.js';
import { idleMainMenuActivity } from './core/main-menu-activity.js';
import { frameDue, frameRateChoices, normalizeFrameRate, parseFrameRateTarget } from './core/frame-rate-option.js';
import { dispatchScreenshotKey, encodeScreenshotGif, screenshotFileName, screenshotPrimaryPalette } from './core/screenshot-gif.js';
import { languageChoices, languageOfChoice, parseLanguageTarget, savedLanguage, originalLanguageNumber } from './platform/language-choice.js';
import { difficultyChoices, parseDifficultyTarget } from './core/difficulty-option.js';
import { SKIRMISH_MISSION_NAME, applySkirmishAction, buildSkirmishLaunch, buildSkirmishScreen, createSkirmishFileSource, isSkirmishMissionName, loadSkirmishSettings, makeSkirmishEmptyWorldFort, makeSkirmishWorldPlan, parseSkirmishTarget, saveSkirmishSettings } from './core/skirmish-setup.js';

// 12차 스커미시의 시작 섬 fort 자료 묶음 이름이다(prepare-assets 의 'skirmish'). 시작할 때만 받는다. (2026-11-20 WB04-E1105)
const SKIRMISH_BUNDLE = 'skirmish';
import { dispatchViewKeyInput } from './core/view-key-input.js';
import { GameMenuButton } from './ui/game-menu-button.js';
import { CHAT_RECORD_BYTES, RawChatChannels } from './core/raw-chat-channels.js';
import { resetClientClockSynchronization } from './core/client-state-storage-cleanup.js';
import { resetStateBridgeDecayScan } from './core/raw-state-bridge-reset.js';
import { RawCampaignMissionStartBindings } from './core/raw-campaign-mission-start-bindings.js';
import { RawCampaignTutorialOneBindings } from './core/raw-campaign-tutorial-one-bindings.js';
import { tutorialObjectiveKey } from './core/tutorial-objective.js';
import { TutorialObjectiveGump } from './ui/tutorial-objective-gump.js';
import { MISSION_RELOAD_COMMAND, dispatchMissionAbort } from './core/mission-guide-input.js';
import { resolveMissionBeginName } from './core/mission-name-alias.js';
import { WORLD_DESTINATION_SELECTED_COMMAND } from './core/raw-user-selection.js';
import { ClientRandomGlobals } from './core/random.js';
import { ClientSpGlobals } from './core/client-sp-globals.js';
import { ClientSubscriberGlobals } from './core/client-subscriber-globals.js';
import { ClientGameplayGlobals } from './core/client-gameplay-globals.js';
import { ClientCoreDataGlobals } from './core/client-core-data-globals.js';
import { ClientPlayerRecords } from './core/client-player-records.js';
import { initializeClientPlayerRecords } from './core/client-player-runtime.js';
import { CampaignTemplateStorage } from './core/campaign-template-storage.js';
import { loadClientWorldAssets } from './core/client-world-assets.js';
import { ClientMissionAssets } from './core/client-mission-assets.js';
import { CampaignGameWorld } from './core/campaign-game-world.js';
import { runCampaignRequestFrame } from './core/campaign-request-frame.js';
import { parseFort } from './assets/fort.js';
import { createFortTypes } from './assets/type-flags.js';
import { PATCH_TYPE_COUNT } from './assets/type-table.js';
import { ClientCampaignGuide } from './core/client-campaign-guide.js';
import { bindTutorialUnitCosts } from './core/tutorial-init.js';
import { parseSettingsTransfer, serializeSettingsTransfer, storeSettingsTransfer } from './platform/settings-transfer.js';
import { SettingsTransferPanel, SETTINGS_LOAD_TARGET, SETTINGS_COPY_TARGET } from './ui/settings-transfer-panel.js';
import { ClientFortGlobals } from './core/client-fort-globals.js';
import { ClientFortCurrentName } from './core/client-fort-current-name.js';
import { refreshWindowTitle } from './core/window-title.js';
import { decodeWindows1252, encodeWindows1252 } from './assets/text-encoding.js';
import { displayTextCodec } from './assets/display-text.js';
import { PLAYER_IDENTITY_RECORD_SIZE, RawPlayerIdentity } from './core/raw-player-identity.js';
import { buildQuickHelpInfoHtml, createQuickHelpPicture, readAssetQuickHelpType } from './ui/quick-help-info.js';
import { currentRiftTypeNumber, readRiftHitPoints } from './core/squid-factory.js';
import { CampaignSessionAssets } from './core/raw-campaign-session-owners.js';
import { PriestPlacementAssets } from './core/priest-placement-assets.js';
import { RawSurfaceStorage } from './core/raw-surface-storage.js';
import { Kernel } from './core/kernel.js';
import { bindClientKernelDiagnostics, ClientTraceGlobals, logClientDisplayAssertion, traceClientObject, writeClientDiagnosticLog } from './core/client-diagnostics.js';
import { ClientTimeSource } from './core/client-time-source.js';
import { resetGameStorage } from './platform/reset-storage.js';
// 공개 모드는 원래 Client의 사본/입력 경계와 별도 Worker에만 연결한다.
import { ModTimeSource } from './modding/mod-time-source.js';
import { ModGameHost } from './modding/mod-game-host.js';
import { ModManager } from './modding/mod-manager.js';
import { ModStore } from './modding/mod-store.js';
import { ModWorker, validateModPackageIsolated } from './modding/mod-worker.js';
import { MOD_LIMITS } from './modding/mod-api.js';
import { ModPanel, MOD_PANEL_TARGET, modPanelTexts } from './ui/mod-panel.js';
import { PRODUCT_RESET_ALL_CONFIRMED_TARGET, PRODUCT_RESET_ALL_TARGET } from './assets/product.js';
import { createCanvasGlyphRasterizer } from './platform/glyph-rasterizer.js';
import { MainMenuScreen } from './ui/main-menu.js';
import { ClientGumpRoot } from './ui/client-gump-root.js';
import { TITLE_SCREEN_COUNT, titleScreenIndexAt, titleScreenOrigin, titleScreenRgba } from './ui/title-screens.js';
import { CHAT_FORMAT_COPY_LIMIT, CHAT_FORMAT_FRONT_VTABLE, chatFormatKey } from './ui/chat-format-dialog.js';
import { GumpHelp, GumpHelpState } from './ui/gump-help.js';
import { createClientProductionAssembly } from './ui/client-production-assembly.js';
import { ClientGumpBackground } from './ui/client-gump-background.js';
import { ClientHudDivider } from './ui/client-hud-divider.js';
import { ClientMinimap } from './ui/client-minimap.js';
import { ClientUberHud } from './ui/client-uber-hud.js';
import { createRawMinimapGlobals } from './ui/raw-minimap-controls.js';
import { createMetadisplayDirtyState } from './core/metadisplay-dirty.js';
import { RawUberRootLookup } from './ui/raw-uber-root-lookup.js';
import { projectGumpObject } from './ui/gump-form-binding.js';
import { UberGumpCamera } from './ui/uber-gump-camera.js';
import { TerrainPreview } from './ui/terrain-preview.js';
import { PopupMenu } from './ui/popup-menu.js';
import { buildPopupItems } from './ui/option-menus.js';
import { createTellWindow } from './ui/tell-window.js';
import { createMissionLoadingWindow, missionLoginProgressText } from './ui/mission-loading-window.js';
import { TELL_TIP_COMMAND, dispatchTellTip } from './ui/tell-tip.js';
import { applyScriptConfig, parseScriptCommand } from './ui/script-command.js';
import { dispatchHelpLink } from './ui/help-link.js';
import { resolveExternalLink } from './ui/external-link.js';
import { downloadBytes, openExternalLink } from './platform/external-opener.js';
import { configParseLong } from './assets/config.js';
import { HelpWindow, createHelpWindow } from './ui/help-window.js';
import { CURSOR_NORMAL, applyCursorStyle, buildCursorCss } from './ui/cursor.js';

// ---------------------------------------------------------------------------------------------
// 상수
// ---------------------------------------------------------------------------------------------

// 논리 화면의 높이(픽셀)이다. 원본이 1024x768 이 최대였으므로 모든 화면비에서 높이를 768 로 고정해
// UI 의 세로 배치를 원본과 같게 유지하고, 화면비가 넓을수록 가로 시야만 늘어나게 한다. (2026-10-23 WB01)
const LOGICAL_HEIGHT = 768;

// 미션의 F4/F5 가상 키와 같은 입력 큐에 예약되는 원본 홈·사제 보기 명령이다.
const MISSION_F4 = 0x00730000;
const MISSION_F5 = 0x00740000;
const MISSION_VIEW_HOME = 0x020000ae;
const MISSION_VIEW_PRIEST = 0x020000b8;
// ESC 게임 메뉴막대의 항목이 입력 큐로 보내는 명령 중 앱이 실제로 실행하는 것이다(목표 보기 0x91, 건물 숨김 0xf1, 설정 0x6d, 음량 0xe6·0xe7, 홈 0xae, 사제 보기 0xb8, Tell 0x95, 도움말 0x17, 미션 중단 0x8c·재시작 0x8d·전투 떠나기 0xc5).
// 나머지 메뉴 명령은 입력 큐를 막지 않도록 소비하되 실행하지 않고 콘솔에 기록하며, 메뉴에서는 비활성으로 보인다. (2026-11-20 WB04-E1084)
// F2(건물 숨김)·F3(Shift: 섬 테마)·F8(목표 보기) 가상 키와 F9(플레이어 목록) 가상 키다. 앞의 셋은 표시 키 분배기가, F9 는 명령 예약기가 처리한다. (2026-11-20 WB04-E1087)
const MISSION_VIEW_KEYS = new Set([0x710000, 0x720000, 0x760000, 0x770000]);
const MISSION_F9 = 0x780000;
// F6 은 선택 키 분배기가 지식 창 명령(0x020000a0)을 예약하는 가상 키다. (2026-11-20 WB04-E1088)
const MISSION_F6 = 0x750000;
// 객체 메뉴의 회수 명령 번호(0x0200005d)이다. 회수 경고 창(SalvageVortex·SalvageInf)의 `Salvage It` 단추가 같은 번호로 입력 큐에 올라가 `requestSalvage` 로 이어진다. (2026-11-22 WB04-E1190)
const SALVAGE_COMMAND = 0x0200005d;
// 지식 창 명령 세 개다: ShowTechnology(Storm 카드, F6), ShowTecSideMenu(Storm/World 선택), ShowTecMenu2(World 카드). (2026-11-20 WB04-E1088)
const KNOWLEDGE_COMMANDS = new Set([0x020000a0, 0x02000132, 0x02000133]);
// 안내·브리핑 창 단추 이름이 지식 명령(ShowTechnology·ShowTecSideMenu·ShowTecMenu2·GetTechnology)이면 그 명령 번호를, 아니면 0xffffffff 를 돌려준다. 원본 이름표(`KnowledgeScriptCode`)를 쓰되 CheatAllTechnology 는
// 본문 명령(`<$CheatAllTechnology,원소>`, `_runScriptCommand`)이 처리하므로 단추 경로에서는 제외한다. 브리핑·미션 완료 창의 `Review Knowledge`(313개)와 `GetTechnology` 단추(101개)가 이 경로를 쓴다. (2026-11-22 WB04-E1227)
function knowledgeButtonCode(name) { const code = knowledgeScriptCode(name); return code === CHEAT_ALL_TECHNOLOGY_COMMAND ? 0xffffffff : code; }
// 동맹 요청 입력이 처리하는 네 명령(요청 fb·해제 fa·확인 7e·무승부 선언 db)이다. (2026-11-20 WB04-E1126)
const ALLIANCE_INPUT_COMMANDS = new Set([0x020000fb, 0x020000fa, 0x0200007e, 0x020000db]);
const WIRED_MENU_COMMANDS = new Set([0x020000f0, 0x02000091, 0x020000f1, 0x0200006d, 0x020000e6, 0x020000e7, 0x020000ae, 0x020000b8, 0x02000095, 0x02000017, 0x0200008c, 0x0200008d, 0x020000c5, 0x020000a0, 0x02000132, 0x02000133, 0x020000cf, 0x02000088, 0x0200009f]);
// Session이 Tutorial 판정용으로 입력 큐에 올리는 통지 명령이다(발사기 배치 0x030000b0·포함 0x030000b1, 기술 0x020000b5, 포획 0x020000d7, 상승 0x020000df; 전역 005429b4 등의 PE 초기값).
// Tutorial 판정기(`tutorial.event`)가 각 명령을 받고 해당 단계가 쓰지 않으면 원본 주 루프처럼 그대로 버려진다. 큐 소비 보호 목록에 없으면 뒤의 클릭까지 막혀 모든 입력이 멈추므로 소비를 허용한다. (2026-11-20 WB04-E1083)
// 여기에 0x020000b4(내 너겟이 신전에 납품돼 소비될 때 — Early Mission 3 단계 A·6 단계 B 의 진행 조건), 0x030000b2(Cast/Drop 요청 성공 통지), 0x02000039(Gump 형 종속 form 이 있는 contained 해제의 UpdateContents)도 포함한다.
// 세 코드는 Session 이 큐에 올리지만 허용 목록에 없어, 큐 머리에서 3초 동안 모든 입력을 막은 뒤 "미구현 입력 사건을 버립니다" 로 버려졌고 두 튜토리얼 단계는 진행하지 못했다. (2026-11-22 WB04-E1231)
const TUTORIAL_NOTICE_COMMANDS = new Set([0x030000b0, 0x030000b1, 0x020000b5, 0x020000d7, 0x020000df, 0x020000b4, 0x030000b2, 0x02000039]);

// 지원하는 화면비 목록이다. name 은 설정·URL 에서 쓰는 이름, ratio 는 명목 가로세로비,
// width/height 는 그 화면비에서의 논리 화면 크기(픽셀, 가로는 짝수)다.
// 논리 폭 상한은1280이다. 4:3은 작은 창에서 원본1024x768을 유지한다.
export const SCREEN_MODES = Object.freeze([
    Object.freeze({ name: '4:3', ratio: 4 / 3, width: 1024, height: LOGICAL_HEIGHT }),
    Object.freeze({ name: '16:10', ratio: 16 / 10, width: 1280, height: 800 }),
    Object.freeze({ name: '16:9', ratio: 16 / 9, width: 1280, height: 720 }),
    Object.freeze({ name: '5:4', ratio: 5 / 4, width: 1280, height: 1024 }),
]);

// 큰4:3 창에서 사용하는 최대 논리 크기다. Canvas 표시 확대는FSR 없이 기존 브라우저 출력으로 처리한다.
const LARGE_CLASSIC_SIZE = Object.freeze({ width: 1280, height: 960 });

// 컨테이너 비율에 가장 가까운 화면비를 자동으로 고르라는 설정값이다. (2026-10-23 WB01)
export const AUTO_ASPECT = 'auto';

// Canvas 내부 픽셀 수(가로 x 세로)의 기본 상한이다. Canvas 는 화면에 그려지는 기기 픽셀 크기와 같게 만들지만,
// 5K 이상의 큰 화면에서 메모리와 그리기 비용이 과도해지지 않도록 이 값을 넘으면 같은 비율로 줄인다. (2026-10-23 WB01)
const DEFAULT_MAX_CANVAS_PIXELS = 16_777_216;

// 컨테이너 비율이 선택한 화면 모드의 비율과 이 값(상대 오차) 이내로 같으면, 비율을 유지하는 최대 크기 대신
// 컨테이너를 빈틈없이 채운다. 1366x768 은 정확한 16:9 가 아니라서 1920x1080 에서 1픽셀 틈이 생기는 것을 막는다.
// 0.5% 이내의 가로세로 배율 차이는 눈에 띄지 않는다. (2026-10-23 WB01)
const ASPECT_SNAP_TOLERANCE = 0.005;

// 프레임 간격의 상한(ms)이다. 가비지 수집 등으로 길게 멈춘 프레임이 한 번에 몰려 반영되지 않게 한다. (2026-10-23 WB01)
const MAX_FRAME_DELTA_MS = 250;

// 초당 프레임 수를 갱신하는 측정 구간(ms)이다. (2026-10-23 WB01)
const FPS_WINDOW_MS = 500;

// 원본 메인 메뉴의 창 제목(config.english 의 BlankWindowName)이다. 메인 메뉴에서 문서 제목으로 쓴다. (2026-10-23 WB03-M1)
export const MAIN_MENU_WINDOW_TITLE = 'NetStorm Main Menu';

// 데이터 폴더의 기본 위치(문서 기준 상대 URL)이다. 정적 서버는 webpj/www 를 공개하므로 www/data 를 가리킨다. (2026-10-23 WB03-M1)
const DEFAULT_ASSET_BASE = './data/';

// 앱의 상태 목록이다. 로딩 → 클릭 대기 → 메인 메뉴 순으로 진행하고, 로딩에 실패하면 ERROR 에서 클릭으로 재시도한다.
// IDLE 은 init 전, DESTROYED 는 destroy 후다. 메뉴 복귀·미션 상태는 해당 단계에서 추가한다. (2026-10-23 WB01)
export const AppState = Object.freeze({
    IDLE: 'idle',
    LOADING: 'loading',
    ERROR: 'error',
    CLICK_TO_START: 'clickToStart',
    // 클릭하여 시작 뒤 메인 메뉴 전에 보이는 시작 title 그림 세 장(원본 4ab3e0)이다. 입력이 있으면 건너뛴다. (2026-11-22 WB04-E1184)
    TITLE: 'title',
    MAIN_MENU: 'mainMenu',
    // 게임 규칙 없이 지면·절벽·받침을 점검하는 개발용 정적 화면이다. (2026-11-02 WB03-AA)
    TERRAIN_PREVIEW: 'terrainPreview',
    DESTROYED: 'destroyed',
});

// '클릭하여 시작'을 열기 전에 나머지 시작 자원(메뉴 음악·글꼴)을 기다리는 최대 시간(밀리초)이다. 이 시간이 지나면 자원이 아직 오는 중이어도 시작을 허용한다(음악은 계속 받는다). (2026-11-24 WB04-E1300)
export const START_READY_TIMEOUT_MS = 20000;

// '클릭하여 시작'을 열기 전의 고정 최소 대기 시간(밀리초)이다. 자원 완료 신호(글꼴·음악)로 잡히지 않는 처음 방문의 경합(스크립트 오류·언어 탐지 실패)을 덜기 위해, 첫 적재가 시작된 뒤 이 시간이 지나기 전에는 자원이 다 와도 클릭을 받지 않는다. 사용자 요구 6초. (2026-11-24 WB04-E1306)
export const START_MIN_WAIT_MS = 6000;

// 고정 최소 대기 값(옵션·주소 `?startwait=밀리초`)을 0~60000 정수로 정리한다. 비어 있거나 숫자가 아니면 fallback 이다. 사용: normalizeStartMinWaitMs('3000', 6000) → 3000. (2026-11-24 WB04-E1306)
export function normalizeStartMinWaitMs(value, fallback = START_MIN_WAIT_MS) {
    if (value === null || value === undefined || value === '') return fallback;
    const number = Number(value);
    return Number.isFinite(number) && number >= 0 ? Math.min(60000, Math.floor(number)) : fallback;
}

// 웹 전용 화면(로딩·오류·클릭하여 시작)의 언어별 문구이다. 원본 메인 메뉴의 버튼 라벨은 원본 글꼴로 그리므로 여기에 없다.
// 모르는 언어는 영어로 대체한다. (2026-10-23 WB01, 2026-10-23 WB03-M1: 임시 메뉴 문구 제거)
const TEXTS = Object.freeze({
    english: Object.freeze({
        title: 'NetStorm',
        loading: 'Loading...',
        loadFailed: 'Failed to load game data.',
        retry: 'Click to retry',
        clickToStart: 'Click to start',
        resetFailed: 'Could not reset game data. Check storage access and try again.',
        gameMenuButton: 'Menu',
        gameMenuButtonTitle: 'Game menu (ESC)',
    }),
    korean: Object.freeze({
        title: 'NetStorm',
        loading: '불러오는 중...',
        loadFailed: '게임 데이터를 불러오지 못했습니다.',
        retry: '클릭하여 다시 시도',
        clickToStart: '클릭하여 시작',
        resetFailed: '게임 데이터를 초기화하지 못했습니다. 저장소 접근을 확인한 후 다시 시도하세요.',
        gameMenuButton: '메뉴',
        gameMenuButtonTitle: '게임 메뉴 (ESC)',
    }),
    // 웹 전용 시작/오류/메뉴 단추의 일본어 문구다.
    japanese: Object.freeze({ title: 'NetStorm', loading: '読み込み中...', loadFailed: 'ゲームデータを読み込めませんでした。', retry: 'クリックして再試行', clickToStart: 'クリックして開始', resetFailed: '初期化できませんでした。保存領域へのアクセスを確認して再試行してください。', gameMenuButton: 'メニュー', gameMenuButtonTitle: 'ゲームメニュー (ESC)' }),
});

// 시작·로딩 안내 Canvas 글꼴 목록이다. 한국어 글꼴이 등록되기 전에는 뒤의 시스템 글꼴로 대체한다.
const FONT_FAMILY = `"${KOREAN_FONT_FAMILY}", "Malgun Gothic", "Segoe UI", monospace`;

// 게임 Canvas 가 포커스를 가졌을 때 브라우저의 기본 동작(스크롤 등)을 막을 키 코드 집합이다.
// 게임 루트가 입력을 소유하는 범위로만 한정하며, Tab 은 키보드 이동을 막지 않도록 제외한다. (2026-10-23 WB01)
// 저장 화면 위치 키(숫자 0~9, Shift+숫자의 영어 기호 `)!@#$%^&*(`)의 문자 코드 집합이다. 원본 입력 루프의 같은 키를 TAB·U 와 함께 카메라 키로 분류한다. (2026-11-21 WB04-E1154)
const SAVED_CAMERA_CHARACTERS = '0123456789!@#$%^&*()';
const SAVED_CAMERA_CODES = new Set([...SAVED_CAMERA_CHARACTERS].map((character) => character.charCodeAt(0)));

// Pause 가상 키(0x13)를 16비트 왼쪽으로 민 입력 코드와 일반 Pause 명령(게임 메뉴 Pause 와 같은 0x020000cf)이다. (2026-11-21 WB04-E1163)
const PAUSE_KEY_CODE = 0x130000;
const PAUSE_COMMAND = 0x020000cf;

const PREVENTED_KEYS = new Set([
    'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', 'Space', 'PageUp', 'PageDown', 'Home', 'End',
]);

// init 이 루트 요소에 덮어쓰고 destroy 가 되돌리는 인라인 스타일 속성 목록이다. (2026-10-23 WB01)
const ROOT_STYLE_KEYS = Object.freeze([
    'position', 'left', 'top', 'right', 'bottom', 'overflow', 'background', 'userSelect', 'webkitUserSelect',
]);

// ---------------------------------------------------------------------------------------------
// 순수 계산 함수 (DOM 을 쓰지 않으므로 Node 에서 검사할 수 있다)
// ---------------------------------------------------------------------------------------------

// 값을 [min, max] 범위로 제한한다. 상한·하한 계산에서 공통으로 쓴다. (2026-10-23 WB01)
function clamp(value, min, max) {
    return Math.min(max, Math.max(min, value));
}

// 기기 픽셀 단위 값을 내림한다. 정확히 맞는 크기가 부동소수점 오차(예: 1919.9999999999998)로 1픽셀 모자라지 않도록
// 아주 작은 보정값을 더한 뒤 내린다. (2026-10-23 WB01)
function floorPixel(value) {
    return Math.floor(value + 1e-6);
}

// 화면비 이름으로 화면 모드를 찾는다. 없으면 undefined 를 돌려준다. (2026-10-23 WB01)
export function findScreenMode(name) {
    return SCREEN_MODES.find((mode) => mode.name === name);
}

// 컨테이너 크기와 화면비 설정으로 사용할 화면 모드를 고른다.
// aspect 가 모드 이름이면 그 모드를, AUTO_ASPECT 이거나 모르는 이름이면 컨테이너 비율과
// 로그 비율 차이가 가장 작은 모드를 고른다. 컨테이너 크기를 쓸 수 없으면 첫 모드(4:3)다. (2026-10-23 WB01)
export function pickScreenMode(containerWidth, containerHeight, aspect = AUTO_ASPECT) {
    const forced = findScreenMode(aspect);
    if (forced) return forced;
    if (!(containerWidth > 0) || !(containerHeight > 0)) return SCREEN_MODES[0];
    const containerRatio = containerWidth / containerHeight;
    let best = SCREEN_MODES[0];
    let bestDistance = Infinity;
    // 모든 화면 모드를 돌며 컨테이너 비율과 가장 가까운 것을 찾는다.
    for (const mode of SCREEN_MODES) {
        const distance = Math.abs(Math.log(containerRatio / mode.ratio));
        if (distance < bestDistance) {
            bestDistance = distance;
            best = mode;
        }
    }
    return best;
}

// 컨테이너 크기·기기 픽셀 비율·화면비 설정에서 Canvas 배치를 계산한다. 입력이 같으면 결과도 같다.
// 논리 좌표(logical*)·CSS 표시 크기(css*, left/top)·내부 픽셀 크기(canvas*)를 분리해 돌려준다.
//  - 논리 좌표: 게임 코드가 그리는 좌표계. 화면비와 창의 CSS 픽셀 크기로 결정한다.
//  - CSS 표시 크기: 컨테이너 안에 비율을 유지하며 최대로 키운 크기. 기기 픽셀 경계에 맞춰 내림하고 가운데 정렬한다.
//    비율 차이가 ASPECT_SNAP_TOLERANCE 이내이면 컨테이너를 빈틈없이 채운다.
//  - 내부 픽셀 크기: 화면에 그려지는 기기 픽셀 크기와 같게 잡아 브라우저가 다시 늘리거나 줄이지 않게 한다(글자가 선명하다).
//    그릴 때는 drawScaleX/Y(논리 픽셀 1개당 내부 픽셀 수)를 변환 행렬로 적용하면 논리 좌표로 그릴 수 있다.
// 컨테이너가 0 이하이면 visible=false 이며 Canvas 를 숨겨야 한다. (2026-10-23 WB01)
export function computeLayout(
    containerWidth,
    containerHeight,
    devicePixelRatio = 1,
    aspect = AUTO_ASPECT,
    maxCanvasPixels = DEFAULT_MAX_CANVAS_PIXELS,
) {
    const dpr = Number.isFinite(devicePixelRatio) && devicePixelRatio > 0 ? devicePixelRatio : 1;
    const mode = pickScreenMode(containerWidth, containerHeight, aspect);
    // 고DPI 기기에서도 창의 CSS 크기에 맞춰 논리 좌표를 고른다. 기기 픽셀 배율은 Canvas 내부 크기에만 반영한다.
    const large = mode.name === '4:3' && containerWidth >= LARGE_CLASSIC_SIZE.width && containerHeight >= LARGE_CLASSIC_SIZE.height;
    const { width, height } = large ? LARGE_CLASSIC_SIZE : mode;
    const hidden = {
        visible: false,
        mode: mode.name,
        logicalWidth: width,
        logicalHeight: height,
        scale: 0,
        cssWidth: 0,
        cssHeight: 0,
        left: 0,
        top: 0,
        canvasWidth: width,
        canvasHeight: height,
        drawScaleX: 1,
        drawScaleY: 1,
        devicePixelRatio: dpr,
    };
    if (!(containerWidth > 0) || !(containerHeight > 0)) return Object.freeze(hidden);

    // 가로·세로 각각 컨테이너를 채우는 데 필요한 배율(논리 픽셀 1개의 CSS 픽셀 수)이다.
    const fillX = containerWidth / width;
    const fillY = containerHeight / height;
    // 비율을 유지한 채 컨테이너 안에 논리 화면 전체가 들어가는 최대 배율이다.
    const scale = Math.min(fillX, fillY);
    let cssWidth;
    let cssHeight;
    if (Math.abs(fillX / fillY - 1) <= ASPECT_SNAP_TOLERANCE) {
        // 비율이 사실상 같으면 컨테이너를 빈틈없이 채운다.
        cssWidth = floorPixel(containerWidth * dpr) / dpr;
        cssHeight = floorPixel(containerHeight * dpr) / dpr;
    } else {
        // 기기 픽셀 경계로 내림해 가장자리가 흐려지거나 컨테이너를 넘지 않게 한다.
        cssWidth = floorPixel(width * scale * dpr) / dpr;
        cssHeight = floorPixel(height * scale * dpr) / dpr;
    }
    if (!(cssWidth > 0) || !(cssHeight > 0)) return Object.freeze(hidden);

    let canvasWidth = Math.round(cssWidth * dpr);
    let canvasHeight = Math.round(cssHeight * dpr);
    const pixels = canvasWidth * canvasHeight;
    if (pixels > maxCanvasPixels) {
        // 너무 큰 화면은 같은 비율로 줄인다. 브라우저가 약간 늘려 보여 준다.
        const shrink = Math.sqrt(maxCanvasPixels / pixels);
        canvasWidth = Math.max(1, Math.floor(canvasWidth * shrink));
        canvasHeight = Math.max(1, Math.floor(canvasHeight * shrink));
    }
    return Object.freeze({
        visible: true,
        mode: mode.name,
        logicalWidth: width,
        logicalHeight: height,
        scale,
        cssWidth,
        cssHeight,
        left: floorPixel(((containerWidth - cssWidth) / 2) * dpr) / dpr,
        top: floorPixel(((containerHeight - cssHeight) / 2) * dpr) / dpr,
        canvasWidth,
        canvasHeight,
        drawScaleX: canvasWidth / width,
        drawScaleY: canvasHeight / height,
        devicePixelRatio: dpr,
    });
}

// 브라우저 화면 좌표(clientX/Y)를 Canvas 의 논리 좌표로 바꾼다. rect 는 Canvas 의 getBoundingClientRect() 결과다.
// 돌려주는 x, y 는 논리 픽셀의 정수(내림)이고, inside 는 논리 화면 안에 있는지 여부다.
// 포인터 캡처 중에는 Canvas 밖 좌표도 들어오므로 x, y 는 범위를 제한하지 않는다. rect 가 비면 null 이다. (2026-10-23 WB01)
export function clientToLogical(clientX, clientY, rect, logicalWidth, logicalHeight) {
    if (!rect || !(rect.width > 0) || !(rect.height > 0)) return null;
    const x = ((clientX - rect.left) * logicalWidth) / rect.width;
    const y = ((clientY - rect.top) * logicalHeight) / rect.height;
    return {
        x: Math.floor(x),
        y: Math.floor(y),
        inside: x >= 0 && x < logicalWidth && y >= 0 && y < logicalHeight,
    };
}

// 문자열 언어 이름을 영어/한국어/일본어 키로 바꾼다. 나머지는영어로 대체한다.
// ko/ko-KR/korean과 ja/ja-JP/japanese를 각각한국어/일본어로 본다. (2026-10-23 WB01)
// 브라우저가 알려 주는 선호 언어를 읽는다: `navigator.languages` 의 첫 항목을 우선하고(없으면 `language`, 옛 `userLanguage`) 비어 있으면 undefined 이다.
// 처음 방문처럼 저장된 선택이 없을 때 선호 언어를 정규화해 영어/한국어/일본어를 선택한다. (2026-11-24 WB04-E1300)
export function detectBrowserLanguage(navigatorLike = globalThis.navigator) {
    const first = Array.isArray(navigatorLike?.languages) || (navigatorLike?.languages && typeof navigatorLike.languages.length === 'number') ? navigatorLike.languages[0] : undefined;
    return first || navigatorLike?.language || navigatorLike?.userLanguage || undefined;
}

export function normalizeLanguage(value) {
    const text = String(value ?? '').toLowerCase();
    if (text === 'japanese' || /^ja(?:-|$)/u.test(text)) return 'japanese';
    return text === 'korean' || text.startsWith('ko') ? 'korean' : 'english';
}

// ---------------------------------------------------------------------------------------------
// 프레임 시계
// ---------------------------------------------------------------------------------------------

// requestAnimationFrame 타임스탬프에서 프레임 간격·누적 시간·초당 프레임 수를 계산하는 시계다.
// 화면 표시 기회(rAF)와 원본 게임 시각(WB04 이후의 Process/Regular)은 별개이므로 이 시계는 표시용이다.
// 탭이 숨겨졌다 돌아오면 reset() 으로 기준점을 다시 잡아 긴 부재 시간을 한 번에 따라잡지 않는다. (2026-10-23 WB01)
class FrameClock {
    // 시계를 만든다. maxDeltaMs 는 한 프레임 간격의 상한(ms)이다.
    constructor(maxDeltaMs = MAX_FRAME_DELTA_MS) {
        // 한 프레임 간격의 상한(ms)이다.
        this.maxDeltaMs = maxDeltaMs;
        // 직전 프레임의 타임스탬프(ms)다. null 이면 다음 tick 이 기준점만 잡고 간격은 0 으로 돌려준다.
        this.lastTime = null;
        // 지금까지 진행한 프레임 수다.
        this.frame = 0;
        // 정지 구간을 뺀 누적 경과 시간(ms)이다.
        this.elapsedMs = 0;
        // 최근 측정한 초당 프레임 수다.
        this.fps = 0;
        // fps 측정 구간의 시작 시각(ms)이다. null 이면 다음 tick 에서 새로 시작한다.
        this.fpsStart = null;
        // 현재 측정 구간 안에서 센 프레임 수다.
        this.fpsFrames = 0;
    }

    // 기준점을 비운다. 탭 복귀·재개 직후 첫 간격이 크게 잡히지 않도록 호출한다.
    reset() {
        this.lastTime = null;
        this.fpsStart = null;
        this.fpsFrames = 0;
    }

    // 한 프레임을 진행하고 직전 프레임과의 간격(ms, 상한 적용)을 돌려준다. now 는 rAF 타임스탬프다.
    tick(now) {
        const delta = this.lastTime === null ? 0 : clamp(now - this.lastTime, 0, this.maxDeltaMs);
        this.lastTime = now;
        this.frame += 1;
        this.elapsedMs += delta;
        if (this.fpsStart === null) {
            this.fpsStart = now;
            this.fpsFrames = 0;
        }
        this.fpsFrames += 1;
        const span = now - this.fpsStart;
        if (span >= FPS_WINDOW_MS) {
            this.fps = (this.fpsFrames * 1000) / span;
            this.fpsStart = now;
            this.fpsFrames = 0;
        }
        return delta;
    }
}

// ---------------------------------------------------------------------------------------------
// Netstorm 앱
// ---------------------------------------------------------------------------------------------

// 검사 전용 고정 시각 모드의 프레임 밀리초를 정한다: 1~60000 정수만 받고 그 밖(없음·0·문자)은 0(꺼짐)이다. 사용: normalizeFixedFrameMs('25') → 25. (2026-11-23 WB04-E1271)
function normalizeFixedFrameMs(value) { const number = Number(value); return Number.isInteger(number) && number >= 1 && number <= 60000 ? number : 0; }

// 주소창의 쿼리 문자열을 읽는다. 개발용 설정(?debug, ?aspect=16:9, ?lang=korean)에 쓴다.
// 브라우저가 아니거나 location 이 없으면 빈 값이다. (2026-10-23 WB01)
function readQueryParams() {
    return new URLSearchParams(globalThis.location?.search ?? '');
}

// 게임 앱 클래스이다. 호스트 페이지는 `new Netstorm(옵션).init(루트요소)` 로 시작하고 `destroy()` 로 정리한다.
// 옵션:
//  - language: 'english' | 'korean' | 'japanese'. 기본은 ?lang, 없으면 브라우저 언어.
//  - aspect: 'auto' | '4:3' | '16:10' | '16:9'. 기본은 ?aspect, 없으면 auto.
//  - debug: true 면 화면 구성·입력 정보를 겹쳐 그린다. 기본은 ?debug 유무.
//  - fillWindow: true(기본)면 루트 요소를 브라우저 창 전체로 채운다. false 면 호스트가 정한 루트 크기를 따른다.
//  - autoFocus: true(기본)면 시작할 때 Canvas 에 포커스를 줘 Enter/Space 로 바로 시작할 수 있게 한다.
//  - maxCanvasPixels: Canvas 내부 픽셀 수(가로 x 세로)의 상한(기본 16,777,216). 넘으면 같은 비율로 줄인다.
//  - assetBase: 데이터 폴더 URL(문서 기준, 기본 './data/'). manifest.json 과 원본 자산 복사본이 있는 곳이다.
//  - fetch: 자산을 받을 fetch 함수(기본은 루트 문서 window 의 fetch). 검사에서 가짜 함수를 넘길 수 있다.
//  - scaling: 'sharp' | 'nearest' | 'smooth'. 8비트 화면의 확대 방식(기본 ?scaling, 없으면 sharp).
//  - audio: false 면 소리를 쓰지 않는다(기본 true, ?mute 가 있으면 false).
//  - soundVolume / musicVolume: 원본 음량 단계 1~5(최초 기본값 5 / 5, 기존 저장값 우선).
//  - prepare: async (app) => void. 내장 자산 적재 뒤 실행하는 추가 준비 훅. 예외를 던지면 ERROR 상태가 된다.
//  - onStateChange: (새 상태, 이전 상태) => void. 상태가 바뀔 때 호출된다.
//  - onMenuCommand: (명령, 라벨) => void. 메인 메뉴 버튼이 실행됐을 때 호출된다(다음 화면은 아직 없음).
//  - previewMap: 개발용 정적 지형을 클릭하여 시작 뒤 연다. 기본은 ?map=파일이름, 없으면 null이다. 미션은 실행하지 않는다. (2026-11-02 WB03-AA)
//  - originalComparison: true 면 웹이 빼는 원본 화면 요소(8버튼·펼침 메뉴 전체 항목·User Made Campaigns 행)를 보이는 원본 비교 모드이다(기본 false).
//    cpppj 오라클과 비교하는 무브라우저 실행(tools/headless.mjs)이 쓴다. 그 요소의 기능은 구현하지 않는다. (2026-11-07 WB02-H)
// (2026-10-23 WB01, 2026-10-23 WB03-M1: 자산·소리·메뉴 옵션 추가)
export class Netstorm {
    // 앱을 만든다. DOM 은 init 에서 만든다.
    constructor(options = {}) {
        const params = readQueryParams();
        // 화면비 설정이다. 모르는 값이면 자동으로 대체한다.
        const requestedAspect = options.aspect ?? params.get('aspect') ?? AUTO_ASPECT;
        // 확정된 옵션 모음이다.
        this.options = {
            language: normalizeLanguage(options.language ?? params.get('lang') ?? savedLanguage(globalThis.localStorage) ?? detectBrowserLanguage()),
            debug: options.debug ?? params.has('debug'),
            previewMap: options.previewMap ?? (params.has('map') ? params.get('map') || 'savetheisland' : null),
            fillWindow: options.fillWindow ?? true,
            autoFocus: options.autoFocus ?? true,
            maxCanvasPixels: Math.max(1, Math.floor(options.maxCanvasPixels ?? DEFAULT_MAX_CANVAS_PIXELS) || DEFAULT_MAX_CANVAS_PIXELS),
            assetBase: options.assetBase ?? DEFAULT_ASSET_BASE,
            // 자산 일시적 실패 재시도 간격(밀리초) 목록이다. 기본(null)은 AssetLoader 의 기본값이며 빈 배열이면 재시도하지 않는다. 검사가 실패를 바로 확인하려고 줄인다. (2026-11-24 WB04-E1300)
            assetRetryDelays: options.assetRetryDelays ?? null,
            // '클릭하여 시작' 고정 최소 대기(밀리초)이다. 기본은 START_MIN_WAIT_MS(6초)이고 주소 `?startwait=0` 이나 옵션으로 바꾼다. 자동화 브라우저(`navigator.webdriver`, e2e·MCP)에서는 기본을 0 으로 두어 검사가 느려지지 않게 하며, 6초 동작을 시험할 때는 `?startwait=6000` 처럼 값을 직접 준다. (2026-11-24 WB04-E1306)
            startMinWaitMs: normalizeStartMinWaitMs(options.startMinWaitMs ?? params.get('startwait'), globalThis.navigator?.webdriver === true ? 0 : START_MIN_WAIT_MS),
            fetch: options.fetch ?? null,
            scaling: normalizeScaling(options.scaling ?? params.get('scaling')),
            audio: options.audio ?? !params.has('mute'),
            // 검사 전용 고정 시각 모드의 프레임 밀리초(0 이면 꺼짐)이다. 켜면 Client 시각 공급원이 프레임마다 이 값만큼만 전진하고(원본 cpppj `--fixed-frame-ms` 와 같음) 프레임은 requestAnimationFrame 이 아니라
            // `stepFixedFrames` 로만 진행한다. 같은 입력 단계표의 두 실행을 같은 시계·난수로 비교하는 L2·L3 결정론 대조용이며 주소 `?fixed=25` 로도 켠다. (2026-11-23 WB04-E1271)
            fixedFrameMs: normalizeFixedFrameMs(options.fixedFrameMs ?? params.get('fixed')),
            // 고정 시각 원본 화면 대조에서만 추가 튜토리얼 표시를 생략한다. 기본 게임과 고정 시각의 기본 표시는 안내를 포함한다.
            originalTutorialPresentation: normalizeFixedFrameMs(options.fixedFrameMs ?? params.get('fixed')) > 0 && Boolean(options.originalTutorialPresentation ?? (params.get('originalTutorialPresentation') === '1')),
            // 원본 비교 모드이다. 참이면 웹 판본이 빼는 원본 화면 요소(메인 메뉴 Multiplayer·Edit·Quit 버튼, 펼침 메뉴의 Direct Draw·Resolution·Technical Help 등,
            // Campaign 목록의 User Made Campaigns 행)를 원본처럼 보여 cpppj 오라클·원본 스크린샷과 같은 배치로 비교한다. 그 요소들의 동작(Edit·멀티플레이 등)은 여전히 구현하지 않는다.
            // 무브라우저 실행(tools/headless.mjs)의 오라클 비교 전용이며 기본값 false 이다. (2026-11-07 WB02-H)
            originalComparison: options.originalComparison ?? false,
            // 시작 title 그림(원본 4ab3e0: title0·title6·title7 을 3초씩, 입력하면 건너뜀)을 클릭하여 시작 뒤에 보일지 여부이다. 기본은 켜짐이며 원본 비교 모드·개발용 지형 미리보기(?map=)·자동화 브라우저(navigator.webdriver)에서는 기본으로 건너뛰고,
            // 주소 `?title=1` 이면 항상 켜고 `?title=0` 이면 항상 끈다. (2026-11-22 WB04-E1184)
            titleScreens: options.titleScreens ?? (params.get('title') === '1' || (params.get('title') !== '0' && !(options.originalComparison ?? false) && !params.has('map') && !globalThis.navigator?.webdriver)),
            // 12차 스커미시를 Campaign 목록에 보일지 여부이다. 월드 생성까지 연결돼(WB04-E1105) 기본 true 이며 원본 비교 모드이거나 주소에 `?skirmish=0` 이 있으면 숨긴다. (2026-11-20 WB04-E1102, WB04-E1105: 기본값을 켬)
            skirmish: options.skirmish ?? (!(options.originalComparison ?? false) && params.get('skirmish') !== '0'),
            // 스커미시 시작 함수 (설정) => boolean 이다. 기본(null)은 앱의 미션 시작이며 아직 연결 전이면 안내만 남기고 false 를 돌려준다. 검사가 시작 호출만 받도록 바꿔 끼운다. (2026-11-20 WB04-E1102)
            startSkirmish: options.startSkirmish ?? null,
            // 호스트가 정한 음량 단계(1~5)이다. 없으면(null) 저장된 설정을 쓴다. 정하면 저장하지 않고 이번 실행에서만 우선한다. (2026-10-29 WB03-F)
            soundVolume: options.soundVolume ?? null,
            musicVolume: options.musicVolume ?? null,
            // 바깥 링크를 여는 함수 (문서, 판정 결과) => 열었으면 true 이다. 기본(null)은 platform/external-opener.js 의 새 탭/창 열기이다.
            // 무브라우저 실행(tools/headless.mjs)과 검사가 실제 탭을 열지 않도록 바꿔 끼운다. (2026-11-07 WB03-AD)
            openExternal: options.openExternal ?? null,
            // 언어를 바꾼 뒤 페이지를 다시 불러오는 함수 () => void 이다. 기본(null)은 location.reload 이며 검사가 실제 다시 불러오기 없이 호출만 받도록 바꿔 끼운다. (2026-11-20 WB04-E1095)
            reload: options.reload ?? null,
            // 화면 저장 GIF 를 내려받는 함수 (문서, 이름, 바이트, 형식) => boolean 이다. 기본(null)은 platform/external-opener.js 의 downloadBytes 이며 검사가 실제 내려받기 없이 바이트만 받도록 바꿔 끼운다. (2026-11-20 WB04-E1100)
            download: options.download ?? null,
            prepare: options.prepare ?? (async () => {}),
            onStateChange: options.onStateChange ?? null,
            onMenuCommand: options.onMenuCommand ?? null,
        };
        // 현재 화면비 설정이다. setAspect 로 바꿀 수 있다.
        this._aspect = findScreenMode(requestedAspect) ? requestedAspect : AUTO_ASPECT;
        // 현재 앱 상태다.
        this._state = AppState.IDLE;
        // 개발용 지형 화면이다. 일반 메뉴에서는 null이며 destroy/메뉴 복귀에서 참조를 놓는다. (2026-11-02 WB03-AA)
        this._terrainPreview = null;
        // 게임을 넣은 루트 요소다. destroy 후에는 null 이다.
        this._root = null;
        // 게임 Canvas 요소다.
        this._canvas = null;
        // IME 포커스가 필요할 때만 만드는 실제 편집 요소다.
        this._clientImeElement = null;
        // Canvas 2D 그리기 컨텍스트다.
        this._ctx = null;
        // 가장 최근에 적용한 Canvas 배치다.
        this._layout = null;
        // 이벤트 리스너를 한꺼번에 해제하기 위한 컨트롤러다.
        this._abort = null;
        // 현재 문서의 실제 게임 입력·캡처·녹화 어댑터다. WebMCP 지원 시에만 만든다.
        this._agentHost = null;
        // 이 앱 수명이 등록한 표준 WebMCP 도구의 반납 핸들이다.
        this._agentBridge = null;
        // 루트 크기 변화 관찰자다.
        this._resizeObserver = null;
        // 예약된 requestAnimationFrame 번호다. 0 이면 예약 없음이다.
        this._frameHandle = 0;
        // 표시용 프레임 시계다.
        this._clock = new FrameClock();
        // Client 게임 시계와 보정 전역은 init에서 생성하고 destroy에서 반납한다.
        this._clientLocal = null; this._clientNsInfo = null; this._clientHelpPopup = null; this._gameClock = null; this._hiddenGamePause = false;
        // 모드 관리자/시계 장벽/설치 검증 취소는 앱 수명마다 새로 만든다.
        this._mods = null; this._modTimeSource = null; this._modValidation = null;
        // 웹 전용 모드 관리 DOM과 그 정지 한 단계는 같은 앱 수명에만 유지한다.
        this._modPanel = null;
        // 메인 메뉴의 설정 JSON 창과, 복원 뒤 초기 화면을 한 번만 건너뛸 표시다.
        this._settingsPanel = null; this._resumeSettingsToMainMenu = false;
        // 실제 앱의 비고정 초기 시계가 UTC초 경계를 준비하는 동안에는 로딩 표시만 갱신한다.
        this._clientClockPreparing = false;
        // 같은 Client 수명의 구름·HTML과 연결표·연결 활동 전역이다.
        this._clientKencloud = null; this._clientHtml = null; this._clientConnections = null; this._clientConnectionActivity = null;
        // 같은 Client가 장기 소유하는 게임·CRT 난수와 SP 저장소다.
        this._clientRandom = null; this._clientSp = null;
        // 같은 Client가 소유하는 원본 채팅 채널 100기록과 그 연산이다.
        this._clientChatRecords = null; this._clientChatChannels = null; this._clientChatWindows = null;
        // 채팅 manager 전역·점 명령 전역(제출 분배가 같은 표를 쓴다)과 최초 초기화 여부이다. (2026-11-22 WB04-E1172)
        this._clientChatManager = null; this._clientDotCommands = null; this._clientChatInitialized = false;
        // 미션 Session이 공유하는 효과음 이름표는 Client 수명에 소유한다.
        this._clientSoundNames = null;
        // 현재 브라우저 효과음 장치와 원본 재생 전역·준비 중인 수명이다.
        this._clientSoundState = null; this._clientSoundDevice = null; this._clientSoundPlayer = null; this._clientSoundPrepare = null;
        // 시작 팔레트와 최초 색 정렬 입력은 Client 수명 동안 같은 객체로 유지한다.
        this._clientPaletteScreen = null;
        // 동기 Renderer 프레임의 Canvas 전송 묶음과 현재 합친 논리 변경 범위다.
        this._clientScreenPresentBatch = false; this._clientScreenPresentBounds = null; this._clientColorSort = null;
        // 논리 화면에 대응하는 실제 Renderer와 월드 변경 영역 전달 장치다.
        this._clientRenderer = null; this._clientSurfaceDisplay = null; this._clientRendererFrame = null;
        // 설치 폴더에 쓰지 않는 색표 캐시는 같은 페이지의 앱 재시작 사이에 유지한다.
        this._clientColorMapFiles = new MemoryColorMapFiles();
        // Client 공통 Player·Core·Template과 Subscriber는 Session보다 오래 산다.
        this._clientSubscriber = null; this._clientGameplay = null; this._clientCoreData = null; this._clientPlayers = null; this._clientTemplates = null;
        // 자산 적재 뒤 구성해 Session보다 오래 유지하는 월드 물리와 Kernel이다.
        this._gameAssets = null; this._surfaceStorage = null; this._clientKernel = null; this._clientTrace = null; this._clientProductionGlobals = null; this._campaignProduction = null;
        // 커널이 소유한 번개 팔레트 프로세스(TYPE 55)의 PID 목록이다. 소멸 콜백에서 제거하며 세션 종료 전 정리한다. (2026-11-24 WB04-E1329)
        this._thunderProcesses = [];
        // 현재 Client 루트에 소유한 추가 튜토리얼 안내다. State 정리와 루트 반납 전에 자기 노드만 지운다.
        this._campaignTutorialObjective = null;
        // 미니맵의 장기 dirty 상태와 색 전역은 Client 수명에 둔다. (2026-11-20 WB04-E1082)
        this._clientMinimapState = null; this._clientMinimapGlobals = null; this._gameMenuButton = null; this._lastCampaignMission = null;
        // 게임 메뉴(Pause)가 올린 게임 시계 정지·음소거 깊이다. 미션을 떠날 때 되돌린다. (2026-11-20 WB04-E1089)
        this._menuPauseDepth = 0; this._menuMuteDepth = 0;
        // 마지막으로 처리한 프레임의 rAF 시각(ms)이다. 프레임 상한 판단에 쓴다. (2026-11-20 WB04-E1094)
        this._lastFrameStamp = null;
        // Shift-F7 화면 저장의 파일 번호다. 저장할 때마다 먼저 증가한 값을 이름에 쓴다(원본 GifOutName 번호). (2026-11-20 WB04-E1100)
        this._screenshotCounter = 0;
        // 12차 스커미시의 현재 월드 계획(시작 섬 fort 바이트·중앙섬 기록·맵 크기)이다. 스커미시가 아닌 미션을 시작하면 비운다. 세션이 미션 Start 때 services.ai.skirmishWorld 로 읽는다. (2026-11-20 WB04-E1105)
        this._skirmishPlan = null;
        // 메인 메뉴 무활동(45초) 감시 상태다. armed 가 false 이면 다음 메인 메뉴 프레임에서 기한을 새로 잡는다. (2026-11-20 WB04-E1092)
        this._mainMenuActivity = { armed: false, deadline: 0, pointer: { x: 0, y: 0 } };
        // 실제 미션 Guide는 자산과 장기 저장소가 준비된 뒤 생성한다.
        this._clientGuide = null; this._clientFort = null; this._clientFortName = null;
        // 원본 공통 Gump 루트와 입력 전역은 Client 수명에 보유한다.
        this._clientGump = null;
        // 설치된 GameWorld와 그 월드를 빌리는 현재 미션 세대다.
        this._campaignWorld = null; this._campaignWorldMissionToken = 0; this._campaignUserInput = null; this._campaignCursor = CURSOR_NORMAL; this._campaignCamera = null;
        // 현재 메뉴에 표시한 미션 진행 창의 소유 수명이다.
        this._missionLoadingWindow = null;
        // init 때 저장해 둔 루트 인라인 스타일이다. destroy 가 되돌린다.
        this._savedRootStyle = null;
        // 비동기 로딩 결과가 오래된 것인지 가리는 세대 번호다. destroy/재시도 때마다 올라간다.
        this._loadGeneration = 0;
        // 마지막 로딩 실패 원인이다. 없으면 null 이다.
        this._loadError = null;
        // 오디오 컨텍스트다. '클릭하여 시작' 입력 안에서 만든다.
        this._audioContext = null;
        // 오디오 준비 상태 문자열이다. none | unsupported | suspended | running | closed | failed | disabled.
        this._audioStatus = 'none';
        // 입력 상태다. pointer 는 논리 좌표, keys 는 눌린 키의 code 집합이다.
        this._input = { pointer: { x: -1, y: -1, inside: false, buttons: 0, type: 'mouse' }, keys: new Set() };
        // 재시작 시 현재 브라우저 입력과 창 활성 상태를 다시 빌리는 폴링 소유자다.
        this._clientInputPoll = new BrowserInputPoll(() => this._input, () => this._layout, () => this._root?.ownerDocument?.hasFocus?.() ?? true);
        // 원본 40칸 입력 큐와 창 안팎 정적 기록은 같은 Client 수명이다.
        this._clientInputQueue = new InputQueue(); this._clientPointerBoundary = new GumpPointerBoundaryState();
        // 원본 입력 소비자가 설치되기 전에는 독립 메뉴와 같은 사건을 중복 예약하지 않는다.
        this._clientNativeInputEnabled = false;
        // 원본 조합 중 키 중복을 막는 현재 브라우저 조합 상태다.
        this._clientComposing = false;
        // 같은 편집 동작의 후속 beforeinput을 중복 예약하지 않기 위한 본문이다.
        this._clientImePending = null;
        // 메인 메뉴 그리기 자원 묶음(buildMenuResources 결과)이다. 자산 적재 전에는 null 이며, 재시작해도 다시 받지 않는다.
        this._resources = null;
        // 시작 title 그림의 표시 시작 시각(밀리초, 첫 그리기 때 정해짐)과 그림별 RGBA 캔버스 캐시이다. (2026-11-22 WB04-E1184)
        this._titleStartedAt = null; this._titleBitmaps = [];
        // 대본 `<$ViewSpot>` 이 맡겨 둔 시작 카메라 위치(칸 단위 { x, y }, 없으면 null)이다. (2026-11-22 WB04-E1190)
        this._pendingViewSpot = null;
        // 같은 공개 자산 매니페스트와 현재 오디오의 미션 지연 적재 수명이다.
        this._assetLoader = null; this._missionAudioAssets = null;
        // 게임 데이터 조회 객체(GameData: 파일 조회·설정 층·언어 용어표·Tell 섹션)다. 자산 적재 전에는 null 이며 재시작해도 다시 만들지 않는다.
        this._gameData = null;
        // 버튼 효과음 파일 바이트다. 오디오 문맥이 생기면 해독한다.
        this._buttonSoundBytes = null;
        // 대화상자 목록 행 효과음(openSubGump.wav) 바이트이다. 자산 적재 전에는 null 이다. (2026-10-28 WB03-D)
        this._menuSoundBytes = null;
        // 가장 최근 프레임의 시각(초)이다. 대화상자의 Timeout 기한 계산에 쓴다. (2026-10-28 WB03-D)
        this._frameSeconds = 0;
        // 사용자 설정(옵션 메뉴 값)이다. init 에서 만들고 destroy 에서 비운다. 값 변경은 _onSettingChange 로 소리에 반영된다. (2026-10-29 WB03-F)
        this._settings = null;
        // 캠페인 진행 값(`Done*`)이다. init 에서 만들고 destroy 에서 비운다. 게임 데이터에 쓰인 진행 값은 _onConfigWrite 로 여기에 모여 로컬 저장소에 저장된다. (2026-10-30 WB03-H)
        this._progress = null;
        // 메뉴 음악 파일 바이트다. 뒤에서 받으며 다 받기 전에는 null 이다.
        this._musicBytes = null;
        // 메뉴 음악 상태 문자열이다. none | loading | loaded | decoding | ready | playing | failed.
        this._musicStatus = 'none';
        // 자산 적재 진행률이다. loaded/total 은 바이트 수다.
        this._loadProgress = { loaded: 0, total: 0 };
        // Web Audio 재생기(AudioSystem)다. '클릭하여 시작' 입력 안에서 만든다.
        this._audio = null;
        // 미션 장면 음악 구동기다. 미션 State7 에서 만들어 원소 곡을 순환하고 미션을 떠나면 비운다. (2026-11-20 WB04-E1119)
        this._sceneMusic = null;
        // 8비트 색 번호 화면 버퍼(논리 크기)다.
        this._surface = null;
        // 8비트 화면 표시기다.
        this._presenter = null;
        // 메인 메뉴 화면이다. 메인 메뉴에 처음 들어갈 때 만든다.
        this._mainMenu = null;
        // 시작 팁 창을 아직 보여 주지 않은 첫 메인 메뉴인지 여부이다(원본 전역 00542530, 초기 1). 환영 창이 아닌 시작 흐름이 끝나면 꺼진다. init 에서 다시 켠다. (2026-10-30 WB03-J)
        this._firstMenuPending = true;
        // 다음 프레임에 Canvas 를 반드시 다시 그려야 하는지 여부다(크기 변경 등).
        this._displayDirty = true;
        // init 때 저장한 문서 제목이다. destroy 가 되돌린다.
        this._savedTitle = null;
        // 마지막으로 적용한 커서 CSS 의 열쇠(커서 번호·배율·픽셀 비율)다. 같으면 다시 만들지 않는다.
        this._cursorKey = '';
        // 지금 적용된 원본 커서 번호이다(1 기본, 15 스크롤, 16 링크). (2026-10-31 WB03-L)
        this._cursorNumber = CURSOR_NORMAL;
        // 번호 → 현재 배율로 만든 CSS 커서 값의 캐시이다. 배율·픽셀 비율(_cursorSizeKey)이 바뀌면 비운다. 커서가 바뀔 때마다 그림을 새로 만들지 않으려는 것이다. (2026-10-31 WB03-L)
        this._cursorCache = new Map();
        // 캐시가 만들어진 배율·픽셀 비율의 열쇠이다. (2026-10-31 WB03-L)
        this._cursorSizeKey = '';
    }

    // ----- 읽기 전용 속성 -----

    // 현재 앱 상태이다 (AppState 값).
    get state() {
        return this._state;
    }

    // 현재 화면비 설정이다 ('auto' 또는 모드 이름).
    get aspect() {
        return this._aspect;
    }

    // 가장 최근에 적용한 Canvas 배치이다. init 전에는 null 이다.
    get layout() {
        return this._layout;
    }

    // 게임 Canvas 요소이다. init 전/destroy 후에는 null 이다.
    get canvas() {
        return this._canvas;
    }

    // 입력 상태이다. pointer(논리 좌표 x, y / inside / buttons / type)와 keys(눌린 키 code 집합)를 가진다.
    get input() {
        return this._input;
    }

    // 오디오 준비 상태 문자열이다.
    get audioStatus() {
        return this._audioStatus;
    }

    // 표시용 프레임 시계이다.
    get clock() {
        return this._clock;
    }

    // 현재 언어의 화면 문구 모음이다.
    get texts() {
        return TEXTS[this._gameData?.language ?? this.options.language] ?? TEXTS.english;
    }

    // 메인 메뉴 화면 객체이다. 메인 메뉴에 들어가기 전에는 null 이다. (2026-10-23 WB03-M1)
    get mainMenu() {
        return this._mainMenu;
    }

    // 메인 메뉴 그리기 자원 묶음이다. 자산 적재 전에는 null 이다. (2026-10-23 WB03-M1)
    get resources() {
        return this._resources;
    }

    // 게임 데이터 조회 객체이다. 자산 적재 전에는 null 이다. 콘솔에서 window.netstorm.gameData.tellSection('Credits') 처럼 확인할 수 있다. (2026-10-28 WB02-D)
    get gameData() {
        return this._gameData;
    }

    // 캠페인 진행 값 모음이다. init 전과 destroy 뒤에는 null 이다. 콘솔에서 window.netstorm.progress.snapshot() 으로 저장된 진행 값을 볼 수 있다.
    // 값을 쓰려면 window.netstorm.gameData.set('DoneTheWarBegins', 1) 처럼 게임 데이터에 쓴다(미션 스크립트의 Config 효과와 같은 경로, 저장도 함께 된다). (2026-10-30 WB03-H)
    get progress() {
        return this._progress;
    }

    // 메뉴 음악 상태 문자열이다. (2026-10-23 WB03-M1)
    get musicStatus() {
        return this._musicStatus;
    }

    // ----- 수명 -----

    // 루트 요소 안에 게임을 만들고 시작한다. 이미 시작된 앱이면 먼저 destroy 한다.
    // 자산 준비는 비동기로 진행되며, 끝나면 '클릭하여 시작' 상태가 된다. this 를 돌려준다.
    init(root) {
        try {
            return this._init(root);
        } catch (error) {
            reportGameError('initialization', error, { state: this._state });
            throw error;
        }
    }

    // 초기화 본문을 실행한다. 실패 보고는 공개 init 에서 맡고 기존 부분 정리와 예외 전달은 유지한다. (2026-11-19 WB04-E862)
    _init(root) {
        if (!root || root.nodeType !== 1) throw new TypeError('Netstorm.init: 게임을 넣을 DOM 요소가 필요합니다.');
        if (this._root) this.destroy();

        const doc = root.ownerDocument;
        // Canvas 를 먼저 만들어 보고, 2D 컨텍스트를 못 얻으면 루트를 건드리기 전에 실패한다.
        const canvas = doc.createElement('canvas');
        const ctx = canvas.getContext('2d', { alpha: false });
        if (!ctx) throw new Error('Netstorm.init: Canvas 2D 컨텍스트를 만들 수 없습니다.');

        // 원본처럼 공통 SP를 설정·화면 해석보다 먼저 초기화하고 실패 시 루트는 건드리지 않는다.
        this._initializeClientRuntime(doc.hidden);
        this._clientClockPreparing = true;
        this._root = root;
        this._canvas = canvas;
        this._ctx = ctx;
        this._abort = new AbortController();
        this._clock = new FrameClock();
        this._input = { pointer: { x: -1, y: -1, inside: false, buttons: 0, type: 'mouse' }, keys: new Set() };
        // 재시작 시 현재 브라우저 입력과 창 활성 상태를 다시 빌리는 폴링 소유자다.
        this._clientInputPoll = new BrowserInputPoll(() => this._input, () => this._layout, () => this._root?.ownerDocument?.hasFocus?.() ?? true);
        // 원본 40칸 입력 큐와 창 안팎 정적 기록은 같은 Client 수명이다.
        this._clientInputQueue = new InputQueue(); this._clientPointerBoundary = new GumpPointerBoundaryState();
        // 원본 입력 소비자가 설치되기 전에는 독립 메뉴와 같은 사건을 중복 예약하지 않는다.
        this._clientNativeInputEnabled = false;
        // 원본 조합 중 키 중복을 막는 현재 브라우저 조합 상태다.
        this._clientComposing = false;
        // 같은 편집 동작의 후속 beforeinput을 중복 예약하지 않기 위한 본문이다.
        this._clientImePending = null;
        this._audioStatus = 'none';
        // 저장된 설정을 읽는다(저장소를 쓸 수 없으면 기본값으로 계속). 호스트가 음량을 정했으면 저장하지 않고 우선 적용한다. (2026-10-29 WB03-F)
        const storage = getLocalStorage(doc.defaultView);
        // 검증 적재는 앱 종료에 즉시 취소한다. 실행 Worker는 관리자가 마지막 사건 뒤 반납한다.
        const modView = doc.defaultView, modValidation = new AbortController(); this._modValidation = modValidation;
        const modStore = new ModStore({ storage, validatePackage: (input) => validateModPackageIsolated(input, { view: modView, signal: modValidation.signal }) });
        this._mods = new ModManager({ host: new ModGameHost(this), source: this._modTimeSource, store: modStore, createWorker: (options) => ModWorker.open({ view: modView, ...options }), forcedOff: this.options.fixedFrameMs > 0 });
        this._settings = new Settings({ storage, onChange: (key) => this._onSettingChange(key) });
        if (this._settings.loadProblem) console.warn(`Netstorm: ${this._settings.loadProblem}. 기본 설정으로 시작합니다.`);
        if (this.options.soundVolume !== null) this._settings.set('soundVolume', this.options.soundVolume, { persist: false });
        if (this.options.musicVolume !== null) this._settings.set('musicVolume', this.options.musicVolume, { persist: false });
        // 저장된 캠페인 진행 값을 읽는다(저장소를 쓸 수 없으면 진행 값 없이 계속). 이전 수명에서 게임 데이터를 이미 만들었다면(재시작) 새로 읽은 값을 그 설정 층에 옮겨 적는다. (2026-10-30 WB03-H)
        this._progress = new Progress({ storage });
        if (this._progress.loadProblem) console.warn(`Netstorm: ${this._progress.loadProblem}. 진행 값 없이 시작합니다.`);
        this._syncGameData();
        // 새 수명은 처음 켠 게임처럼 시작 창 흐름과 Tell 이름 기록을 다시 시작한다. (2026-10-30 WB03-J)
        this._firstMenuPending = true;
        if (this._gameData) this._gameData.currentTell = '';
        this._presenter = new IndexedPresenter(doc, this.options.scaling);
        // 이전 수명에서 이미 받은 자원이 있으면(재시작) 팔레트를 바로 설치한다.
        if (this._resources) this._presenter.setPalette(this._resources.palette.rgb);
        this._displayDirty = true;
        this._cursorKey = '';
        this._cursorNumber = CURSOR_NORMAL;
        this._cursorCache.clear();
        this._cursorSizeKey = '';
        this._savedTitle = doc.title;
        try {
            this._applyRootStyle();
            this._applyCanvasStyle();
            root.appendChild(canvas);
            this._bindEvents();
            this._applyLayout();
            this._initializeClientRendererView();
            this._initializeAgentTools();
            if (this.options.autoFocus) canvas.focus({ preventScroll: true });
            this._startFrameLoop();
            this._load();
        } catch (error) {
            // 초기화 도중 실패하면 만든 것을 모두 정리하고 원인을 그대로 전달한다.
            this.destroy();
            throw error;
        }
        return this;
    }

    // 지원 문서에서만 실제 게임 도구를 등록한다. 초기화/반납 경합과 등록 오류가 일반 게임 시작을 중단하지 않게 한다.
    _initializeAgentTools() {
        const doc = this._root.ownerDocument, lifetime = this._abort;
        try {
            if (typeof doc.modelContext?.registerTool !== 'function') return;
            const host = new GameAgentHost(this); this._agentHost = host;
            installWebMcp(doc, host, { signal: lifetime.signal }).then((bridge) => {
                // 이전 init의 늦은 등록은 새 게임 수명에 남기지 않는다.
                if (this._abort !== lifetime) { bridge.dispose(); return host.dispose(); }
                this._agentBridge = bridge;
            }).catch((error) => {
                host.dispose().catch(() => {});
                if (this._abort === lifetime) reportGameError('webmcp-registration', error, { state: this._state });
            });
        } catch (error) { reportGameError('webmcp-initialize', error, { state: this._state }); }
    }

    // 원본 공통 시작의 SP 초기화와 게임 시계 생성을 같은 Client 소유자로 연결한다.
    _initializeClientRuntime(hidden) {
        this._clientTrace = new ClientTraceGlobals();
        this._clientProductionGlobals = new ProductionMaintenanceGlobals();
        const nativeSource = new ClientTimeSource(), random = new ClientRandomGlobals();
        // 고정 시각 모드이면 시각 공급원이 프레임마다 정해진 밀리초만 전진한다(난수 초기화 전에 켜서 유닉스 초도 같은 값에서 시작한다). (2026-11-23 WB04-E1271)
        if (this.options.fixedFrameMs > 0) nativeSource.enableFixed({ frameMilliseconds: this.options.fixedFrameMs });
        const source = new ModTimeSource(nativeSource); this._modTimeSource = source;
        const sp = new ClientSpGlobals(random, () => source.unixSeconds()); sp.store().initialize();
        const local = new ClientLocalStateGlobals(), clock = new ClientFrameClock(source, local.gameTicks());
        // 고정 시각 모드: cpppj 고정 실행은 첫 주 루프 프레임 전에 시계가 4틱(0.1초) 흘러 첫 프레임의 게임 시각이 0.125초다. 시계 원점은 위에서 이미 잡았으므로 그 뒤에 4틱 전진해 같은 출발 시각으로 맞춘다. (2026-11-23 WB04-E1281)
        if (this.options.fixedFrameMs > 0) for (let tick = 0; tick < 4; tick += 1) source.advanceFixed();
        const cloud = new ClientKencloudGlobals(), html = new ClientHtmlWindowGlobals(), connections = new ClientConnectionGlobals(local), activity = new ClientConnectionActivityGlobals();
        const subscriber = new ClientSubscriberGlobals(), gameplay = new ClientGameplayGlobals(subscriber), fort = new ClientFortGlobals();
        // 원본 ClientMain 의 최초 초기화(00439928·0043994c): 저장 화면 위치 10칸과 최근 건설 위치 5칸을 무효 좌표(-10000)로 쓴다. 미션·Renderer 교체에서는 다시 쓰지 않는다. 초기화가 없으면 저장하지 않은 숫자 키가 (0, 0) 으로 이동했다. (2026-11-21 WB04-E1154)
        initializeSavedCameraPositions(gameplay.camera().savedPositions()); initializeConstructionHistory(gameplay.camera().constructionHistory());
        const chatRecords = new Uint8Array(CHAT_RECORD_BYTES), chatChannels = new RawChatChannels(chatRecords, () => subscriber.mySubscriber().value);
        const coreData = new ClientCoreDataGlobals(), players = new ClientPlayerRecords(), templates = new CampaignTemplateStorage();
        const soundNames = new SoundList(); soundNames.initialize();
        players.bindCoreData(coreData);
        initializeClientPlayerRecords({ gameplayGlobals: () => gameplay, playerRecords: () => players });
        if (hidden) clock.pushGamePause();
        this._clientNsInfo = new ClientNsInfoGlobals(); this._clientHelpPopup = new QuickHelpPopupState();
        this._clientRandom = random; this._clientSp = sp; this._clientLocal = local; this._gameClock = clock; this._hiddenGamePause = Boolean(hidden);
        this._clientKencloud = cloud; this._clientHtml = html; this._clientConnections = connections; this._clientConnectionActivity = activity;
        this._clientSubscriber = subscriber; this._clientGameplay = gameplay; this._clientCoreData = coreData; this._clientPlayers = players; this._clientTemplates = templates; this._clientFort = fort;
        this._clientSoundNames = soundNames;
        this._clientChatRecords = chatRecords; this._clientChatChannels = chatChannels; this._clientChatWindows = new ClientChatWindowGlobals();
        this._clientChatManager = new ClientChatManagerGlobals(); this._clientDotCommands = new ClientDotCommandGlobals(); this._clientChatInitialized = false;
        this._clientRendererFrame = new ClientRendererFrameGlobals();
        // 독립 Client 소유자 조립은 기본 상대시계를 사용할 수 있다. 실제init은 이 뒤 준비 관문을 닫고_load에서원본초기화를 마친다.
        this._clientClockPreparing = false;
    }

    // 설정 해석 시 실제 helpWord 값을 같은 Client 캐시에 갱신한다.
    _syncClientHelpWord() {
        if (this._clientLocal && this._gameData) this._clientLocal.helpWord().interpret(this._gameData.get('helpWord'));
    }

    // 현재 설정에서 구름 전역 옵션을 읽는다. 설정이나 Client가 아직 없으면 실제 초기화 시 다시 적용한다.
    _syncClientCloudOptions() {
        if (this._clientKencloud && this._gameData) this._clientKencloud.readOptions(this._gameData);
    }

    // 옵션 메뉴 Language 선택을 적용한다. 선택을 설정(`language`)에 저장하고 주소의 `lang` 매개변수를 같은 출처 기록 교체(history.replaceState)로 지워(저장한 선택을 가리지 않게) 페이지를 다시 불러 새 언어 묶음으로 시작한다. 주소 이동(location.href·assign)은 외부 접속 감사가 금지하므로 쓰지 않는다. 현재 언어와 같으면 false 다. 다시 불러오는 방식은 options.reload() 로 바꿀 수 있다(검사용). (2026-11-20 WB04-E1095)
    _chooseLanguage(number) {
        const language = languageOfChoice(number);
        if (language === null || language === (this._gameData?.language ?? this.options.language)) return false;
        this._settings?.set('language', number);
        const view = this._view(), params = new URLSearchParams(view.location.search);
        if (params.has('lang')) { params.delete('lang'); const query = params.toString(); view.history.replaceState(null, '', `${view.location.pathname}${query ? `?${query}` : ''}${view.location.hash}`); }
        (this.options.reload ?? (() => view.location.reload()))();
        return true;
    }

    // 현재 논리 화면(8비트 팔레트 인덱스)을 GIF87a 로 인코딩해 브라우저 내려받기로 저장한다(원본 `Client::SaveGifScreenshot`). 팔레트는 현재 flash·날씨가 아니라 고정 primary 표의 RGB 이다. 번호는 저장마다 하나씩 올린다. (2026-11-20 WB04-E1100)
    _saveScreenshot() {
        const screen = this._clientPaletteScreen, colors = this._clientColorSort;
        if (!screen || !colors?.primary) throw new Error('화면 저장의 현재 화면·primary 팔레트 누락');
        const pixels = screen.lock();
        try {
            const bytes = encodeScreenshotGif(screen.width, screen.height, screen.pitch, pixels, screenshotPrimaryPalette(colors.primary.bytes));
            const name = screenshotFileName(this._screenshotCounter); this._screenshotCounter = (this._screenshotCounter + 1) >>> 0;
            (this.options.download ?? downloadBytes)(this._root?.ownerDocument ?? null, name, bytes, 'image/gif');
        } finally { screen.unlock(); }
    }

    // 현재 설정의 useBanners(섬 소유자 표식 숨김)를 소유자 색표 저장소에 적용한다(원본 `Client::InterpretOwnerColorOptions` → `SetEnabled(GetInt("useBanners"))`). 설정에 값이 없으면 원본 Config 처럼 0 이라 소유자 색 표식(섬 테두리 파랑 등)을 보인다. 색표 준비 전이면 무시된다. (2026-11-20 WB04-E1093)
    _syncClientOwnerColorOptions() {
        if (this._clientColorSort && this._gameData && this._clientKencloud) this._clientColorSort.ownerTables.setEnabled(this._gameData.readInt('useBanners', 0).value >>> 0, this._clientKencloud.worldRedraw());
    }

    // 원본 InterpretCursorOptions(cpppj Client::InterpretCursorOptions): 설정을 해석할 때마다 회전 방향을 0 으로 먼저 쓰고 `rotmode` 를 읽는다(누락이면 0, 부호 있는 전체 비트 보존).
    // 설정(옵션 메뉴 "Counterclockwise Piece Rotation"·C 키)이 바뀌어도 회전 방향 전역이 갱신되지 않아 반시계 회전이 동작하지 않았다. (2026-11-21 WB04-E1151)
    _syncClientCursorOptions() {
        const globals = this._clientGameplay?.cursorGlobals?.(); if (!globals || !this._gameData) return;
        globals.rotationMode().value = this._gameData.readInt('rotmode', 0).value >>> 0;
    }

    // 전체 State 부모가 선택한 위치에서 현재 웹 설정의 기존 소비자들을 다시 해석한다.
    _interpretClientStateOptions() {
        this._syncClientCloudOptions(); this._syncClientHelpWord(); this._syncClientOwnerColorOptions(); this._syncClientCursorOptions();
        this._surfaceStorage.transmission.state.compress = this._gameData.readInt('cs', this._surfaceStorage.transmission.state.compress).value | 0;
        if (this._clientTrace) this._clientTrace.log.level = this._gameData.readInt('tracelevel', this._clientTrace.log.level).value | 0;
        this._applySettings();
    }

    // 실제 게임 자산에서 장기 물리를 만들고 같은 Client 권한·시각·Kernel과 진단을 연결한다.
    _initializeClientWorldStorage(game, files) {
        if (this._surfaceStorage || !this._clientGameplay || !this._gameClock) throw new Error('Client 월드 저장소의 중복 또는 공통 수명 누락');
        const assets = new CampaignSessionAssets(game, files), metadata = new PriestPlacementAssets(game.typeTable, game.shapes, assets.loadedTypes);
        bindTutorialUnitCosts(this._gameData, game.typeTable, assets.loadedTypes); // 실제 안내 대본의 가격 변수가 원본 TYPE 비용을 읽도록 연결한다.
        const gameplay = this._clientGameplay, clock = this._gameClock;
        const storage = new RawSurfaceStorage(assets.types, assets.frames, metadata.shapes, { source: game, authority: gameplay.authority(), gameAux: gameplay.gameAux() });
        const kernel = new Kernel(); storage.processes.bindDevices(kernel, clock.time());
        storage.bindChecking(() => gameplay.aiDebug().checking().value);
        // 동일 Client가 보유한 현재 상세를 호출 순간에 읽어 기존 표시 진단보다 먼저 기록한다.
        storage.bindDisplayDiagnostics(gameplay.aiDebug().assertionDetail(), (expression, file, line) => logClientDisplayAssertion(gameplay.aiDebug().assertionDetail().value, expression, file, line));
        storage.enablePatchDestroyDiagnostics();
        storage.bindGraphClock(() => clock.gameSeconds()); bindClientKernelDiagnostics(kernel, storage, (line) => console.info(line));
        this._gameAssets = game; this._surfaceStorage = storage; this._clientKernel = kernel;
    }

    // 실제 fortPal 자산을 적용해 논리 색을 캡처한 뒤 창 모드 DIB와 출력 장치를 준비한다.
    _initializeClientPalette(game) {
        if (this._clientPaletteScreen || this._clientColorSort || !this._layout || !game) throw new Error('Client 팔레트의 중복 또는 시작 수명 누락');
        const screen = new Screen(this._layout.logicalWidth, this._layout.logicalHeight, { devices: {
            paletteChanged: (palette) => this._applyClientScreenPalette(palette), present: (rect, view) => this._presentClientScreen(rect, view),
        } });
        const colors = new ClientColorSortInput();
        screen.init(); screen.loadPalette(game.palette, 1);
        colors.captureWithPrimary(screen.logicalPalette());
        screen.bindColorSortInput(() => colors.logical);
        screen.initDibSection();
        if (!screen.setMode(ScreenMode.fallbackWindowed)) throw new Error('Client Screen의 창 모드 DIB 초기화 실패');
        screen.setClip(0, 0, screen.width, screen.height);
        this._clientPaletteScreen = screen; this._clientColorSort = colors;
        this._initializeClientRenderer(screen.width, screen.height);
    }

    // 논리 화면 크기에 맞는 Renderer와 그 변경 표를 빌리는 월드 표시 장치를 함께 교체한다.
    _initializeClientRenderer(width, height) {
        const renderer = new Renderer(width, height);
        this._clientRenderer = renderer; this._clientSurfaceDisplay = new SquidRenderer(renderer);
    }

    // 현재 Screen의 BGRX 색을 실제 표시기의 RGB 표로 전달하고 다음 화면 출력을 예약한다.
    _applyClientScreenPalette(palette) {
        if (!(palette instanceof Uint8Array) || palette.length !== 1024) throw new TypeError('Client Screen 팔레트 BGRX 계약');
        const rgb = new Uint8Array(768);
        // 색 번호와 원본 채널 순서를 보존하며 표시기 형식으로 옮긴다.
        for (let index = 0; index < 256; index += 1) {
            rgb[index * 3] = palette[index * 4 + 2]; rgb[index * 3 + 1] = palette[index * 4 + 1]; rgb[index * 3 + 2] = palette[index * 4];
        }
        this._presenter?.setPalette(rgb); this._displayDirty = true;
    }

    // 현재 DIB의 잘린 사각형을 실제 색 번호 표면에 복사하고 보이는 Canvas에 즉시 출력한다.
    _presentClientScreen(rect, view) {
        const surface = this._surface, layout = this._layout;
        if (!surface || !layout || !this._presenter || surface.width !== view.width || surface.height !== view.height ||
            layout.logicalWidth !== view.width || layout.logicalHeight !== view.height || !view.pixels || view.pitch < view.width) throw new Error('Client Screen의 현재 DIB 출력 수명 누락');
        // DIB 패딩을 제외하고 지정된 영역의 각 행만 같은 색 번호로 복사한다.
        for (let row = rect.top; row < rect.bottom; row += 1) {
            const source = row * view.pitch + rect.left, target = row * surface.width + rect.left;
            surface.pixels.set(view.pixels.subarray(source, source + rect.right - rect.left), target);
        }
        this._displayDirty = true;
        if (layout.visible) {
            if (!this._ctx) throw new Error('Client Screen의 현재 Canvas 문맥 누락');
            // 브라우저는 JS 프레임 종료 뒤 표시하므로 프레임 안의 여러 Screen 복사는 마지막에 한 번 전송한다.
            if (this._clientScreenPresentBatch) {
                const previous = this._clientScreenPresentBounds;
                this._clientScreenPresentBounds = previous ? { left: Math.min(previous.left, rect.left), top: Math.min(previous.top, rect.top), right: Math.max(previous.right, rect.right), bottom: Math.max(previous.bottom, rect.bottom) } : { ...rect };
            } else this._presenter.present(this._ctx, surface, layout, true, rect);
        }
    }

    // 최초 TYPE 등록 뒤 같은 primary로 소유자 표와 9색표 캐시를 만들고 Screen 명암 조회에 연결한다.
    _initializeClientPaletteCache(game) {
        const screen = this._clientPaletteScreen, colors = this._clientColorSort, clouds = this._clientKencloud;
        if (!screen || !colors || !clouds || !game || colors.mapsReady) throw new Error('Client 색표 캐시의 중복 또는 시작 수명 누락');
        const bridge = game.typeNumber(game.find('bridge'));
        colors.initializeCache(this._clientColorMapFiles, 'cache/!color.dat', screen.colors().basic[1], () => {
            colors.ownerTables.initialize(colors.primary, PATCH_TYPE_COUNT, bridge, clouds.worldRedraw());
        });
        screen.bindColorMaps(() => ({ bright: colors.maps[4], dark: colors.maps[6] }));
    }

    // 논리 크기가 바뀌면 현재 색과 loaded 상자·고정 색표를 유지한 새 창 모드 Screen을 만든다.
    _resizeClientPaletteScreen(width, height) {
        const previous = this._clientPaletteScreen, colors = this._clientColorSort;
        if (!previous || !colors?.mapsReady) throw new Error('Client Screen 크기 변경의 현재 색표 수명 누락');
        if (previous.width === width && previous.height === height) return;
        const palette = GamePalette.parse(previous.palette().slice()), mode = previous.flags;
        const screen = new Screen(width, height, { paletteLoaded: previous.paletteLoaded, devices: {
            paletteChanged: (value) => this._applyClientScreenPalette(value), present: (rect, view) => this._presentClientScreen(rect, view),
        } });
        screen.init(); screen.initDibSection();
        screen.bindColorSortInput(() => colors.logical); screen.bindColorMaps(() => ({ bright: colors.maps[4], dark: colors.maps[6] }));
        screen.loadPalette(palette, 0);
        if (!screen.setMode(mode)) throw new Error('Client Screen 크기 변경의 창 모드 초기화 실패');
        screen.setClip(0, 0, width, height); this._clientPaletteScreen = screen;
        this._initializeClientRenderer(width, height);
    }

    // 같은 Client의 구름·파일·Screen·최초 색표를 사용해 원본 팔레트 전환 부모를 실행한다.
    _changeClientFortPalette(name) {
        const data = this._gameData, cloud = this._clientKencloud, random = this._clientRandom, colors = this._clientColorSort;
        if (typeof name !== 'string' || !data || !cloud || !random || !this._clientPaletteScreen || !colors?.mapsReady) throw new Error('Client 팔레트 전환의 실제 수명 누락');
        const encoded = encodeWindows1252(name), argument = new Uint8Array(encoded.length + 1); argument.set(encoded);
        cloud.transition(argument, data.files, data, random.game, {
            log: (level, message) => console.info(`Palette ${level} ${message}`),
            // 현재 설정 경로를 실제 VFS에서 읽고 Screen의 지우기 관문을 통과한다.
            loadPalette: (path, clear) => {
                const end = path?.indexOf(0) ?? -1; if (end < 0) throw new RangeError('팔레트 파일 경로 NUL 누락');
                const name = decodeWindows1252(path.subarray(0, end)), bytes = data.files.read(name);
                if (!bytes) throw new Error(`팔레트 파일 누락: ${name}`);
                this._clientPaletteScreen.loadPalette(GamePalette.parse(bytes), clear);
            },
            // 같은 최초 색으로 9표를 다시 만들고 현재 World 표시기에도 실제 Screen 색을 전한다.
            buildMaps: () => {
                const screen = this._clientPaletteScreen; colors.rebuild(screen.colors().basic[1]);
                this._campaignWorld?.setPalette(screen.palette(), colors.logical, colors.maps);
                if (this._mainMenu) this._mainMenu.dirty = true; this._displayDirty = true;
            },
        });
    }

    // 원본 State 전체 정리(CleanupStateWorld)의 플레이어 반복(1~8번)을 SP 쪽만 옮긴다: 각 소유자의 SP 를 0 으로 쓴다(`resetOtherPlayer(player, 0)`, 로컬은 `setSp`).
    // SP 저장소 Set 은 소유자마다 Game 난수를 160회 쓰므로 한 번에 1,280회를 소비한다. 이 소비 순서가 지형 도장·구름·SP 잡음 등 뒤따르는 모든 난수 사용의 출발 상태를 정한다.
    // 앱은 아직 State 전체 정리(`cleanupClientStateWorld`)를 State 기계에 연결하지 않았고, 시작·메뉴·미션 시작 시점에는 나머지 정리 대상이 이미 비어 있어 난수 소비와 SP 비우기만 같은 자리에서 실행한다. 실제 SP 저장소가 없으면 SP 는 건너뛴다. (2026-11-23 WB04-E1280)
    // 비어 있지 않은 정리 효과 하나는 같이 실행한다: 다리 붕괴 스캔 초기화(Reset422490)이다(위 SP 와 달리 난수를 쓰지 않으며 L3 540→680프레임 비교에서 매달린 다리의 수명 감소 누락으로 드러났다). (2026-11-24 WB04-E1301)
    _clearPlayerSpForState() {
        const store = this._clientSp?.store();
        // 소유자 1~8 의 SP 를 float 0 의 DWORD 로 쓴다(저장소가 없으면 생략).
        if (store) for (let owner = 1; owner < 9; owner += 1) store.set(owner, 0);
        // 같은 정리의 Reset422490: 다리 붕괴 스캔의 범위(15000..23001)·커서·다음 기한(주기 + 현재 게임 시각)을 다시 잡는다. 이것이 없으면 스캔 범위가 -1 로 남아 매달린 다리가 영영 금 가지 않는다. (2026-11-24 WB04-E1301)
        const storage = this._surfaceStorage, clock = this._gameClock;
        if (storage && clock) resetStateBridgeDecayScan(storage.bridgeDecayScan, storage.bridgeDecayPeriod, clock.time().game);
    }

    // 원본 State 39(인트로) 설치의 정리(004b6e62 부모의 cleanup)이다. 시작 입력 직후, title 그림이나 메인 메뉴보다 앞서 한 번 실행한다. (2026-11-23 WB04-E1280)
    _installIntroStateEffects() {
        this._clearPlayerSpForState();
    }

    // 원본 State 3(메인 메뉴) 설치(004b6fa9 부모)의 Game 난수·시각 효과이다: 정리 → 메뉴 팔레트 전환(구름 구축, 같은 battlePal 이면 이후 State7 이 구름을 다시 만들지 않음) → 주변음 기한을 벽시각+10초로 다시 잡는다(ApplyExistingEditorAfterCheckpoint).
    // 같은 battlePal 로 전환하므로 메인 메뉴 표면·팔레트는 변하지 않는다(E1280 확인). 구름 구축이 Game 난수 801회를 쓴다. 자산·색표가 준비되지 않았으면 팔레트 전환은 건너뛴다. (2026-11-23 WB04-E1280)
    _installMainMenuStateEffects() {
        this._clearPlayerSpForState();
        if (this._gameData && this._clientKencloud && this._clientPaletteScreen && this._clientColorSort?.mapsReady) this._changeClientFortPalette(this._gameData.pathSpec('battlePal'));
        const local = this._clientLocal, clock = this._gameClock;
        if (local && clock) resetAmbientSoundDeadline({ set deadline(value) { local.activityEnd().value = value; } }, () => clock.wallSeconds());
    }

    // 같은 Client 설정·Player·월드 저장소에 Guide 수명을 연결한다.
    _initializeClientGuide() {
        if (this._clientGuide || !this._gameData || !this._surfaceStorage || !this._clientGameplay || !this._clientPlayers) throw new Error('Client Guide의 중복 또는 공통 수명 누락');
        this._clientGuide = new ClientCampaignGuide(this._gameData, this._clientGameplay, this._clientPlayers, this._surfaceStorage);
        // 선택된 이름 효과만 현재 World의 같은 Session에서 빌린다. World가 없으면 해당 효과를 요구한 위치에서 중단한다.
        const fortSession = () => { if (!this._campaignWorld) throw new Error('Fort 현재 이름의 실제 Session 미설치'); return this._campaignWorld.session().bindFortCurrentName(); };
        this._clientFortName = new ClientFortCurrentName(this._clientFort, this._clientTemplates, this._gameData, this._clientGameplay, this._surfaceStorage,
            { refreshTitle: () => this._refreshFortWindowTitle(), assertion: (line) => console.error(`Fort 현재 이름 원본 진단: ${line}`),
                snapshot: () => fortSession().snapshot(), notify: (slot) => fortSession().notify(slot), state: (slot) => fortSession().state(slot) });
        this._clientGuide.bindTutorialInit({ templates: this._clientTemplates, local: this._clientLocal, subscriber: this._clientSubscriber, types: this._gameAssets.typeTable,
            selectFort: (name) => { this._selectCampaignFort(name); const bytes = this._clientFort.currentBytes(); return decodeWindows1252(bytes.subarray(0, bytes.indexOf(0))); } });
        this._clientGuide.bindLoop({ dialogsClear: () => !this._mainMenu?.dialog, gameSeconds: () => this._gameClock.gameSeconds(),
            worldConnecting: (token) => Boolean(this._campaignWorld?.connecting && token === this._campaignWorldMissionToken), tellGlobal: (name, mode) => { if (mode !== 1) throw new Error('Guide init Tell 인자 위반'); this._openTell(name, { suppressWindow: true }); } });
    }

    // 현재 폴링과 같은 Client 입력 큐로 원본 포인터 안팎 보정을 실행한다.
    _prepareClientPointer(language) {
        return prepareGumpPointerBoundaryWithQueue(this._clientPointerBoundary, language, (code) => this._clientInputPoll.poll(code), this._clientInputQueue);
    }

    // 실제 Client 진단 DWORD와 현재 논리 화면으로 공통 Gump 루트를 만든다.
    _initializeClientGump() {
        if (this._clientGump || !this._layout || !this._clientGameplay) throw new Error('Client Gump의 중복 또는 공통 수명 누락');
        const gameplay = this._clientGameplay;
        this._clientGump = new ClientGumpRoot(this._layout.logicalWidth, this._layout.logicalHeight, () => gameplay.aiDebug().checking().value,
            (token) => {
                // 이동 전후 Gump 범위를 실제 Renderer dirty에 넣어 이전 픽셀도 복구한다.
                const root = this._clientGump, world = this._campaignWorld, renderer = this._clientRenderer;
                if (root && world && !world.connecting && renderer && root.node(token)) {
                    new GumpRedraw(root.collection(), { node: (current) => root.node(current), map: () => world.mapView(),
                        invalidate: (rect, flags) => renderer.invalidate(rect, flags) }).whole(token);
                }
                this._displayDirty = true;
            }, (text) => { gameplay.aiDebug().assertionDetail().value = text; }, (object) => {
                if (!this._surfaceStorage || !this._clientGameplay) throw new Error('월드 Gump 투영의 실제 Client 수명 누락');
                return projectGumpObject(this._surfaceStorage.pool, this._clientGameplay.camera().snapshot(), object);
            });
        // HUD 도킹 전의 지도 영역은 전체 논리 화면이다. 도킹이 같은 객체에 정수 LTRB를 다시 쓴다. (2026-11-20 WB04-E1082)
        Object.assign(this._clientGump.mapView(), { left: 0, top: 0, right: this._layout.logicalWidth, bottom: this._layout.logicalHeight });
    }

    // 현재 Fort·Guide·설정으로 원본 창 제목을 계산해 실제 문서에 적용한다.
    _refreshFortWindowTitle() {
        if (!this._root || !this._clientFort || !this._clientTemplates || !this._clientGameplay || !this._surfaceStorage || !this._gameData) throw new Error('Fort 제목의 실제 Client 수명 미설치');
        const bytes = this._clientFort.currentBytes(), end = bytes.indexOf(0);
        if (end < 0) throw new RangeError('Fort 제목의 현재 이름 NUL 범위');
        const fortName = decodeWindows1252(bytes.subarray(0, end)), selected = this._clientGuide?.current(), owner = this._clientGameplay.player.templateOwner;
        const headers = this._clientTemplates.headers(); if (owner >= 9) throw new RangeError('Fort 제목의 현재 Template 범위');
        const view = new DataView(headers.buffer, headers.byteOffset + owner * 20, 20);
        const state = { fortName, editor: this._clientGameplay.world.editor, mission: this._clientGameplay.world.missionPost,
            templatePresent: view.getUint32(0, true) !== 0, templateReadOnly: view.getUint32(8, true) !== 0,
            demo: selected?.runtime.gate.demo ?? 0, testBattle: selected?.runtime.mode64 ?? 0 };
        refreshWindowTitle(state, { pathSpec: (key, argument) => this._gameData.pathSpec(key, argument), missionTitle: () => selected?.script.get('title') ?? '', setTitle: (title) => { this._root.ownerDocument.title = title; } });
    }

    // 같은 Client의 현재 Fort 이름 선택 부모를 실행한다. 실제 Guide Init이 호출할 경계다.
    _selectCampaignFort(name) { if (!this._clientFortName) throw new Error('Client Fort 이름 장치 미설치'); this._clientFortName.set(name); }

    // 실제 앱 설정과 게임 자산에서 한 미션의 스크립트·Fort를 준비한다. 반환 소유자는 호출자가 반납한다.
    _prepareCampaignMission(name) {
        if (!this._gameData || !this._gameAssets || !this._surfaceStorage) throw new Error('미션 준비 전 실제 Client 자산 미설치');
        return new ClientMissionAssets(this._gameData, this._gameAssets, name);
    }

    // 미션 조립은 현재 Client의 난수·SP·Player·Core·Template·Subscriber·구름·HTML 전역과 시계를 대여한다. 이 호출은 Session을 생성하지 않는다. (2026-11-24 WB04-E1328: 누락된 kencloudGlobals·htmlWindowGlobals 연결)
    _campaignSharedServices() {
        const clock = this._gameClock, random = this._clientRandom, sp = this._clientSp, local = this._clientLocal;
        const subscriber = this._clientSubscriber, gameplay = this._clientGameplay, coreData = this._clientCoreData, players = this._clientPlayers, templates = this._clientTemplates;
        if (!clock || !random || !sp || !local || !subscriber || !gameplay || !coreData || !players || !templates || !this._clientSoundNames || !this._clientChatWindows || !this._clientKencloud || !this._clientHtml) throw new Error('캠페인의 실제 Client 공통 수명 미설치');
        const surface = this._surfaceStorage, kernel = this._clientKernel, request = this._clientGuide?.fortRequest();
        const colors = this._clientColorSort, screen = this._clientPaletteScreen;
        if (Boolean(colors) !== Boolean(screen) || (colors && !colors.mapsReady)) throw new Error('캠페인의 실제 Client 색표 수명 미설치');
        return { soundNames: this._clientSoundNames, chatGlobals: this._clientChatWindows, randomGlobals: random, spGlobals: sp, localStateGlobals: local, subscriberGlobals: subscriber, gameplayGlobals: gameplay, coreDataGlobals: coreData,
            playerRecords: players, templateStorage: templates, kencloudGlobals: this._clientKencloud, htmlWindowGlobals: this._clientHtml, connectionActivityGlobals: this._clientConnectionActivity, connectionRegistry: this._clientConnections, connectionFrameNumber: () => clock.time().number, ...(surface && kernel ? { surfaceStorage: surface, processKernel: kernel } : {}),
            ...(colors ? { colorSortInput: colors, ownerColorTables: colors.ownerTables, screenPalette: () => this._clientPaletteScreen.palette() } : {}),
            ...(this._clientGuide ? { missionRestrictions: this._clientGuide.restrictions(), missionOptions: gameplay.battleOptionsOwner() } : {}), ...(request ? { missionFortRequest: request } : {}),
            unixSeconds: () => clock.unixSeconds(), frameTime: () => clock.time(), paused: () => clock.paused(),
            // 지도 영역은 HUD 도킹이 쓰는 Gump 트리의 같은 객체다. Gump 루트가 없으면 전체 논리 화면이다. (2026-11-20 WB04-E1082)
            mapView: () => this._clientGump?.mapView() ?? { left: 0, top: 0, right: this._layout.logicalWidth, bottom: this._layout.logicalHeight } };
    }

    // State7 부모가 실제 앱의 장기 전역과 같은 프레임 시계를 빌리도록 한다.
    _campaignStateClient() {
        if (!this._surfaceStorage || !this._clientLocal || !this._clientGameplay || !this._clientKencloud || !this._clientHtml || !this._gameClock) throw new Error('캠페인 State7의 실제 Client 공통 수명 미설치');
        return { edition: () => 'patch1078', surfaceStorage: () => this._surfaceStorage, localStateGlobals: () => this._clientLocal,
            gameplayGlobals: () => this._clientGameplay, kencloudGlobals: () => this._clientKencloud, htmlWindowGlobals: () => this._clientHtml,
            time: () => this._gameClock.time(), wallSeconds: () => this._gameClock.wallSeconds() };
    }

    // 월드 설치가 실제 번역·설정·현재 창 제목을 사용하도록 표시 바이트 경계를 연결한다.
    _campaignInstallServices() {
        return {
            translateYou: () => {
                if (!this._resources || !this._gameData) throw new Error('미션 설치의 실제 번역 자원 누락');
                const encoded = displayTextCodec.encode(this._resources.translate('You'));
                return Uint8Array.from([...encoded, 0]);
            },
            setConfig: (key, bytes) => {
                if (!this._gameData || !(bytes instanceof Uint8Array)) throw new TypeError('미션 설정의 실제 저장소 또는 바이트 누락');
                const end = bytes.indexOf(0); if (end < 0) throw new RangeError('미션 설정 문자열 NUL 누락');
                this._gameData.set(key, displayTextCodec.decode(bytes.subarray(0, end)));
            },
            refreshWindowTitle: () => this._refreshFortWindowTitle(),
        };
    }

    // 같은 Client 등록표와 현재 예산 serial로 원본 SP 재부착을 실행한다. HUD가 아직 없거나 이미 반납됐으면 찾기만 하고 끝난다. (WB04-E1325)
    _refreshCampaignStormPower() {
        if (!this._clientGump || !this._clientProductionGlobals) return;
        this._clientGump.refreshStormPower(this._clientProductionGlobals.budgetSerial);
    }

    // 같은 앱의 현재 설정·Gump·입력 큐를 미션 소유권과 자산 후처리에 빌려준다.
    _campaignGameplayServices() {
        const services = {};
        // 보상·다른 플레이어 SP 정리도 같은 실제 예산 노드를 다시 부착한다. (WB04-E1325)
        services.refreshSp = () => this._refreshCampaignStormPower();
        bindCampaignOwnershipTheme({ configuration: () => {
            if (!this._gameData) throw new Error('미션 소유권의 실제 설정 수명 누락');
            return this._gameData;
        } }, services);
        bindCampaignProductionRefresh({ route: (...args) => {
            if (!this._clientGump) throw new Error('미션 생산 갱신의 실제 Gump 수명 누락');
            return this._clientGump.input().route(...args);
        } }, services);
        bindCampaignClientInputServices({ input: () => {
            if (!this._clientInputQueue) throw new Error('미션 명령의 실제 입력 큐 수명 누락');
            return this._clientInputQueue;
        } }, services);
        services.assets.queueCommand = services.developerQueue;
        // 건설17의 검증된 CPP x64 할당 입력을 Session에 명시한다. 원본 공통 입자 상수로 일반화하지 않는다.
        services.particlePaletteInput = CLIENT_PARTICLE_PALETTE_INPUT;
        // 객체 소멸 통지는 같은 Client의 현재 Gump 등록 ID에 전달한다.
        services.gumpNotice = (owner, parent) => { if (!this._clientGump) throw new Error('미션 Gump 객체 통지 수명 누락'); this._clientGump.objectGone(owner, parent); };
        // Player 내용물 변경은 같은 현재 등록표에서 조회한 Gump에 원본 사건을 직접 전달한다.
        services.playerContent = {
            gump: (id) => { if (!this._clientGump) throw new Error('미션 내용물 Gump 등록표 수명 누락'); return this._clientGump.createState().entries[id] ?? 0; },
            handle: (token, event, form, reason) => { if (!this._clientGump) throw new Error('미션 내용물 Gump 처리 수명 누락'); return this._clientGump.handle(token, event, form, reason); },
        };
        // AI 이름·난이도와 채팅은 같은 Client 설정·번역·Subscriber 기록을 읽는다.
        services.ai = { assertion: (expression) => console.error(`미션 AI 원본 진단: ${expression}`), allocationBytes: currentAllocationBytes, bridgeScratchBytes: currentAllocationBytes,
            blockingGumpOpen: () => this._clientGump.blockingPopups(),
            // 12차 스커미시: 세션이 미션 Start 때 읽는 월드 계획이다(일반 미션은 null). (2026-11-20 WB04-E1105)
            skirmishWorld: () => this._skirmishPlan };
        bindCampaignAiClientServices({ configuration: () => this._gameData, languageNumber: () => originalLanguageNumber(this._gameData?.language ?? this.options.language),
            translate: (text) => this._resources.translate(text), encodeDisplayText: (text) => displayTextCodec.encode(text),
            decodeDisplayText: (bytes) => displayTextCodec.decode(bytes), chatChannels: () => this._clientChatChannels }, services.ai);
        // 섬 피격·사제 방어와 생산 자금 안내는 같은 현재 Tell 창을, Factory 연구 도움말(0x03000080 kind=0)은 현재 도움말 창을 사용한다. (2026-11-24 WB04-E1328: 누락된 factoryHelp·removeConnectionStatus·templateStateBytes 연결)
        services.factoryTell = (section) => this._openTell(section);
        services.factoryHelp = (section) => { this._closeCampaignPopups(); this._openHelp(section); };
        services.removeConnectionStatus = (kind, player) => { this._campaignProduction?.ui?.warnings()?.remove(kind | 0, player | 0, 0); };
        services.templateStateBytes = currentAllocationBytes;
        // 로컬 서버가 41h 채팅 패킷을 되돌릴 때 받는 현재 UI 수신이다(채팅 창이 없으면 무시). (2026-11-22 WB04-E1172)
        services.chatReceive = (packet) => this._receiveCampaignChat(packet);
        services.priest = { shield: { tell: (section) => this._openTell(section) } };
        // 승패 안내와 AI·생산 할당은 같은 현재 미션·Gump와 웹 할당 바이트를 사용한다.
        services.production = { allocationBytes: currentAllocationBytes, effects: {
            configInt: (key) => this._gameData.readInt(key, 0).value | 0,
            modeAssertion: () => console.error('생산 재계산 원본 진단: inFortMode || inBattleMode'),
        } };
        // Player 정기 처리의 현재 진단 그룹과 문장을 같은 Client trace로 전달한다.
        services.player = { log: (group, text) => {
            if (!this._clientTrace) throw new Error('미션 Player 진단의 실제 Client 수명 누락');
            writeClientDiagnosticLog(this._clientTrace, group | 0, text);
        } };
        services.missionOutcome = {
            demo: () => this._clientGuide?.current()?.runtime.gate.demo ?? 0,
            tellBlocked: () => Boolean(this._mainMenu?.dialog),
            section: (name) => { const script = this._clientGuide?.current()?.script; return script?.section(name) != null && this._openTell(name); },
            closeTransient: () => this._closeCampaignPopups(),
            onSection: () => { const tutorial = this._clientGuide?.current()?.runtime.tutorial; if (!tutorial) throw new Error('미션 승패 안내의 실제 정지 장치 누락'); tutorial.gate().pauseForDialog(); },
        };
        // 미션 효과음은 같은 Client 이름표와 현재 재생기에 전달한다.
        const player = () => { if (!this._clientSoundPlayer) throw new Error('미션 효과음의 실제 재생기 미준비'); return this._clientSoundPlayer; };
        services.sound = { wallSeconds: () => this._gameClock.wallSeconds(), play: (...args) => player().play(...args),
            playOnceAt: (...args) => player().playOnceAt(...args), playLoopAt: (...args) => player().playLoopAt(...args),
            stop: (...args) => player().stop(...args), isPlaying: (...args) => player().isPlaying(...args) };
        // 희생 의식은 장면 음악 구동기에 sacrifice.mus 를 요청한다(원본 CampaignClientServices requestMusic → ClientAudio::RequestMusic). 구동기가 없으면(소리 끔) 건너뛴다. (2026-11-20 WB04-E1120)
        services.requestMusic = (name) => { this._sceneMusic?.request(name); };
        // 내용물 너겟 삭제는 같은 실제 입력 큐와 위치 효과음(guy.wav)을 사용한다(원문 CampaignClientServices contentLifetime). AI 가 너겟을 나르는 스커미시에서 처음 쓰인다. (2026-11-20 WB04-E1106)
        services.contentLifetime = { localNotice: (command) => { this._clientInputQueue.pushCommand(command, 0, 0); }, sound: (x, y) => { player().playNameAt(x, y, 'guy.wav', 0); } };
        // 이동 경로의 발소리는 같은 Client 위치 음향으로 원본 반복·우선순위 0을 전달한다.
        services.motion = { path: { stepSound: (_sid, x, y, sound) => player().playOnceAt(x, y, sound, 0, 0) } };
        // 건설 확정·간헐천은 같은 현재 설정·벽시계·Renderer와 원본 진단 출력을 사용한다.
        services.construction = { configInt: (key, fallback) => this._gameData.readInt(key, fallback).value | 0,
            wallSeconds: () => this._gameClock.wallSeconds(), refresh: () => this._clientRenderer.invalidateAll(),
            negativeQuantity: () => console.error('로컬 간헐천 원본 진단: reps >= 0'),
            boundsAssertion: (code) => console.error(`Player 간헐천 범위 원본 진단: ${code}`) };
        services.assets.postPop.playImpactSound = (sound) => player().play(sound, 0, 0, 0, 0, 0);
        // ----- 원본 CampaignClientServices 의 소리·안내 서비스(원문 priest.shield·unitLost·hit·carrierDropPopup·factory 진단·newTechnology). 연결이 없으면 해당 사건에서 '미연결' 오류로 프레임이 멈췄다. (2026-11-20 WB04-E1122)
        // 사제 보호막 안내음은 이름 기반 전역 재생(우선 인자 1), 번역은 키 그대로(Xlat 으로 섹션 이름을 바꾸지 않음), Tell 은 위의 현재 안내 창이다.
        const shieldTell = services.priest.shield.tell;
        services.priest.shield = { ...makePriestShieldNoticeHooks({ playByName: (...args) => player().playByName(...args) }, {}), translate: (key) => key, tell: shieldTell };
        // 사제 낙하음은 현재 위치의 priestFall.wav(반복 없음)이다.
        services.priest.fallSound = (_sid, x, y) => { player().playNameAt(x, y, 'priestFall.wav', 0); };
        // 공통 파괴 때 유닛 상실음은 전역 재생(반복 0·음량 0·좌우 0·우선 1·한도 0)이다.
        services.unitLost = () => { player().playByName('unitLost.wav', 0, 0, 0, 1, 0); };
        // 섬 피격 경고 사건은 종류 12 안내를 지운다(원본 removeMessage(12,0,0)). 이어서 경고음·Tell 이 따른다(세션이 공급).
        services.hit = { general: { warningEvent: () => { this._campaignProduction?.ui?.warnings()?.remove(12, 0, 0); } } };
        // Cast/Drop 비용 표시는 실제 생산 UI 의 떠다니는 SP 표시에 그린다.
        services.carrierDropPopup = (cost, x, y, reference) => {
            const ui = this._campaignProduction?.ui; if (!ui) throw new Error('Cast/Drop 비용표시의 실제 생산 UI 미설치');
            ui.showSp(cost, x, y, reference);
        };
        // 중복 워크샾 경고는 채널 4 의 원본 문구와 정수 좌표를 같은 진단 로그에 쓴다. 별도 경고창은 만들지 않는다.
        services.assets.factory = { ...services.assets.factory, warnDuplicate: (_sid, x, y) => { writeClientDiagnosticLog(this._clientTrace, 4, `WARNING!  Extra workshop destroyed at (${x | 0},${y | 0})\n`); } };
        // 전체 Fort snapshot(기술 획득·의식이 부르는 snapshotBattle/Fort)의 두 읽기 장치다(원본 UberGump::FortMissionSaveEffects 와 Client 단일 Badges 장부). 이 장치가 없으면 전투 모드(미션 포함)에서 기술을 얻는 순간
        // "현재 Mission 읽기 장치 미연결" 오류로 프레임이 멈췄다. Mission 은 현재 Guide 의 저장 토큰·이름(NUL 포함 원문 바이트)·+58 순번·+54 플래그를 읽고, 선택한 미션이 바뀌면 거부한다. (2026-11-22 WB04-E1227)
        const guideRuntime = () => this._clientGuide?.current()?.runtime ?? null;
        services.fortMission = {
            current: () => guideRuntime()?.saveToken ?? 0,
            name: () => { const runtime = guideRuntime(); if (!runtime) throw new Error('현재 Mission 없는 이름 물리 접근'); return Uint8Array.from([...Array.from(runtime.init.name, (letter) => letter.charCodeAt(0) & 0xff), 0]); },
            field: (token, offset) => {
                const runtime = guideRuntime(); if (!runtime || token !== runtime.saveToken) throw new Error('선택 Mission 의 저장 수명 만료');
                if (offset === 0x58) return runtime.state.sequence >>> 0; if (offset === 0x54) return runtime.state.flags >>> 0; throw new RangeError('Mission 저장의 실제 필드 범위');
            },
            assertion: () => { throw new Error('Mission 저장 원문 이름 진단'); },
        };
        this._clientFortBadges ??= new RawFortBadgesGlobals(); services.fortBadges = this._clientFortBadges;
        // 기술 획득(제단 의식 등): 이름은 현재 TYPE 설명의 번역, 설정은 같은 Config, 그림 서식의 %d 는 정수이며 안내는 현재 Tell 이다.
        services.newTechnology = {
            name: (type) => {
                const types = this._campaignWorld?.session().core().types; if (!types || !this._resources) throw new Error('기술 획득 이름의 실제 TYPE·번역 수명 누락');
                if (!Number.isInteger(type) || type < 0 || type >= types.length) throw new RangeError('기술 획득 이름의 현재 TYPE 범위');
                return this._resources.translate(types[type].description);
            },
            configure: (key, value) => { if (!this._gameData) throw new Error('기술 획득 설정의 실제 저장소 누락'); this._gameData.set(key, String(value)); },
            format: (pattern, value) => String(pattern).replace('%d', String(value | 0)),
            tell: (section) => this._openTell(section),
        };
        // 포획·신전 상승 안내의 Player 이름은 owner 만 채운 새 식별 기록을 현재 Player·채팅 기록에서 풀어 낸다(원본 priestTakenName → ResolveClientChatIdentity). 못 찾으면 SOMEONE 이다.
        // 이 연결이 없으면 사제가 포획되는 순간 '포획 안내 실제 Player 이름 미연결' 오류로 프레임이 멈췄다. (2026-11-20 WB04-E1122)
        services.priestTakenName = (owner, currentName) => {
            if (!this._clientPlayers || !this._clientChatRecords || !this._clientGameplay || !this._clientSubscriber || !this._surfaceStorage) throw new Error('포획 안내 Player 이름의 실제 Client 수명 누락');
            const record = new Uint8Array(PLAYER_IDENTITY_RECORD_SIZE); RawPlayerIdentity.initialize(record, 0);
            for (let byte = 0; byte < 4; byte += 1) record[byte] = (owner >>> (8 * byte)) & 0xff;
            const identity = new RawPlayerIdentity(this._clientPlayers.records(), new Uint8Array(0), this._clientChatRecords, {
                localOwner: () => this._clientGameplay.player.battleOwner, subscriber: () => this._clientSubscriber.mySubscriber().value,
                address: () => this._clientGameplay.localAddress().value, name: () => currentName,
                battle: () => this._surfaceStorage.ownerMode.battle, editor: () => this._clientGameplay.world.editor !== 0,
                metamap: () => this._surfaceStorage.ownerMode.challenge, unknownName: () => 'SOMEONE',
            });
            if (!identity.resolve(record)) return 'SOMEONE';
            let end = 8; while (end < 48 && record[end] !== 0) end += 1;
            if (end === 48) throw new RangeError('포획 안내 Player 이름 NUL 누락');
            return String.fromCharCode(...record.subarray(8, end));
        };
        // Session 초기 카메라 요청을 같은 앱의 실제 화면·변경 표·Gump에 적용한다.
        services.cameraCommit = (context) => this._commitCampaignCamera(context);
        // 카메라 패킷은 도킹 영역이 아닌 현재 논리 Screen의 전체 크기를 읽는다.
        services.cameraScreenSize = () => {
            const screen = this._clientPaletteScreen;
            if (!screen) throw new Error('미션 카메라의 실제 Screen 수명 누락');
            return { x: screen.width, y: screen.height };
        };
        // 표면 변경은 현재 Renderer의 부분 영역으로 전달한다. Canvas 전체 갱신 표시는 원본 dirty 영역을 넓히므로 여기서 켜지 않는다. (WB04-E1342)
        services.surfaceDisplay = {
            suppressed: () => {
                if (!this._clientSurfaceDisplay) throw new Error('미션 표면의 실제 Renderer 수명 누락');
                return this._clientSurfaceDisplay.suppressed();
            },
            invalidate: (rect, flags) => {
                if (!this._clientSurfaceDisplay) throw new Error('미션 표면의 실제 Renderer 수명 누락');
                this._clientSurfaceDisplay.invalidate(rect, flags);
            },
        };
        return services;
    }

    // 원본 Gump 팝업과 웹 메뉴의 현재 펼침 메뉴에 닫기를 요청한다.
    _closeCampaignPopups() {
        if (!this._clientGump) throw new Error('미션 팝업의 실제 Client Gump 누락');
        this._clientGump.closePopups(); this._mainMenu?.closePopup();
    }

    // 현재 앱 장치를 빌리는 장기 카메라 소유자를 만들고 화면 교체 뒤에도 같은 타이머를 유지한다.
    _campaignCameraDevice() {
        if (this._campaignCamera) return this._campaignCamera;
        const gameplay = this._clientGameplay, screen = this._clientPaletteScreen, renderer = this._clientRenderer, gump = this._clientGump;
        if (!gameplay || !screen || !renderer || !gump || !this._clientKencloud || !this._clientLocal || !this._gameClock) throw new Error('미션 카메라 장치의 실제 Client 수명 불일치');
        const game = { camera: () => gameplay.camera(), cursor: () => gameplay.cursor, selection: () => gameplay.selection,
            patchDiagnostic: () => gameplay.patchDiagnostic(), worldCells: () => gameplay.worldCells() };
        const client = { configuration: () => this._gameData, screen: () => this._clientPaletteScreen, renderer: () => this._clientRenderer,
            gameplay: () => game, localState: () => this._clientLocal, time: () => this._gameClock.time(),
            active: () => Boolean((this._root?.ownerDocument?.hasFocus?.() ?? true) && this._input.pointer.inside), poll: (code) => this._clientInputPoll.poll(code),
            cursor: () => { throw new Error('미션 소프트웨어 커서 장치 미연결'); } };
        const app = this, rebuild = { get value() { return app._displayDirty; }, set value(value) { app._displayDirty = Boolean(value); } };
        const lookup = new RawUberRootLookup(gump);
        const buttons = { redrawRoots: () => this._clientGump.redrawRoots(), serialGump: (serial) => lookup.reference(serial),
            blockingGumpOpen: () => this._clientGump.blockingPopups(), requestClosePopups: () => this._closeCampaignPopups(),
            currentDialog: () => this._clientGump.lifetimeState().currentDialog || (this._mainMenu?.dialog ? 1 : 0) };
        this._campaignCamera = new UberGumpCamera({ client, buttons: () => buttons, world: () => this._campaignWorld, gameMenuState: () => null,
            pointerBoundary: this._clientPointerBoundary, worldRedraw: this._clientKencloud.worldRedraw(), rebuild });
        return this._campaignCamera;
    }

    // 원본 497960 카메라 적용 장치를 현재 앱 Screen·Renderer·World에 연결한다.
    _commitCampaignCamera(context) {
        if (!this._clientGameplay || context?.camera !== this._clientGameplay.camera()) throw new Error('미션 카메라 적용의 실제 Client 수명 불일치');
        this._campaignCameraDevice().commitCameraNow();
    }

    // 현재 논리 화면과 Client 전역으로 카메라 Renderer·snapshot·한계를 원본 순서로 초기화한다.
    _initializeClientRendererView() {
        const layout = this._layout, game = this._clientGameplay, clouds = this._clientKencloud;
        if (!layout || !game || !clouds) throw new Error('Renderer 초기화의 현재 화면 또는 Client 수명 누락');
        if (!Number.isInteger(layout.logicalWidth) || !Number.isInteger(layout.logicalHeight)) throw new TypeError('Renderer 초기화의 논리 화면 정수 크기 누락');
        game.camera().initializeNativeRenderer(layout.logicalWidth, layout.logicalHeight, game.worldCells().value, clouds.worldRedraw(), game.mapRevision());
    }

    // 로컬 Login이 현재 Client의 설정·시계·진단과 표시 무효화를 직접 조회한다.
    _campaignLoginServices() {
        return {
            // 2bh 수신 시 현재 Client 채팅 장부에서 해당 subscriber의 채널만 해제한다(C++1366).
            removeChat: (subscriber, mask) => {
                if (!this._clientChatChannels) throw new Error('로그인 채널 해제의 현재 Client 수명 누락');
                this._clientChatChannels.removeFromChannel(subscriber, mask);
            },
            // State7은 실제 화면 크기와 공유 카메라 전역을 다시 초기화한다.
            initializeRenderer: () => this._initializeClientRendererView(),
            // State7에서 현재 battlePal을 조회해 같은 Client 팔레트 전환을 실행한다.
            battlePalette: () => this._changeClientFortPalette(this._gameData.pathSpec('battlePal')),
            // State7·13의 현재 HUD 모드를 같은 Client Gump 입력 트리에 숨김 포함으로 전달한다.
            uiMode: (mode) => {
                if (!this._clientGump || !this._clientLocal) throw new Error('로그인 HUD 모드의 실제 Client Gump 수명 누락');
                this._clientGump.input().route(this._clientLocal.hudModeCommand().value, mode, 0, true);
            },
            // 미션 시작과 로그인 진행 문구는 같은 현재 메뉴의 진행 창을 사용한다.
            // 시작 안내("Starting Mission...")는 원본 State7 처럼 지도 영역이 곧 HUD 를 뺄 것을 알고 왼쪽 85px 를 뺀 영역({85, 0, 너비, 높이}) 가운데에 놓는다. 로그인 진행 문구는 그 시점의 현재 지도 영역을 쓴다. (2026-11-21 WB04-E1131)
            startingProgress: () => { if (!this._resources) throw new Error('미션 진행 창의 실제 자원 수명 누락'); this._showMissionLoading(this._resources.translate('Starting Mission...'), { left: 85, top: 0, right: this._mainMenu?.width ?? 0, bottom: this._mainMenu?.height ?? 0 }); },
            progress: (kind, remaining) => { if (!this._resources) throw new Error('미션 진행 창의 실제 자원 수명 누락'); this._showMissionLoading(missionLoginProgressText(kind, remaining, (text) => this._resources.translate(text)), this._clientGump?.mapView() ?? null); },
            closeProgress: () => this._closeMissionLoading(),
            // 서버 포트는 공통 전역을, 다른 정수는 현재 설정과 기본값을 사용한다.
            configInt: (key, fallback) => {
                if (!this._gameData || !this._clientGameplay) throw new Error('로그인 설정의 실제 Client 수명 누락');
                return key === 'gameServerPort' ? this._clientGameplay.gameServerPort().value | 0 : this._gameData.readInt(key, fallback).value;
            },
            // 프레임 고정값과 달리 요청 순간의 벽시각과 게임 시각을 각각 읽는다.
            wallSeconds: () => this._gameClock.wallSeconds(),
            gameSeconds: () => this._gameClock.gameSeconds(),
            // 브라우저에는 C 런타임 clock()이 없어 같은 Client의 단조 밀리초를 사용한다.
            clock: () => this._gameClock.milliseconds(),
            // 공통 로그인/타이머 갱신은 메뉴와 화면을 무효화한다. SP 전용 재부착은 같은 Session의 services.refreshSp를 사용한다. (WB04-E1325)
            refresh: () => { if (this._mainMenu) this._mainMenu.dirty = true; this._displayDirty = true; },
            // 비전투·미션·대본 중단을 먼저 제외한 뒤 현재 Session의 전투 종료를 확인한다.
            playerUiBusy: () => {
                if (!this._surfaceStorage || !this._clientGameplay || !this._clientLocal) throw new Error('로그인 Player의 실제 Client 수명 누락');
                if (!this._surfaceStorage.ownerMode.battle || this._clientGameplay.world.missionPost || this._clientLocal.scriptBreak().value) return false;
                if (!this._campaignWorld) throw new Error('전투 종료 판정의 실제 World 누락');
                return this._campaignWorld.session().battleFinished();
            },
            // Player 원문 진단을 출력하고 같은 표시 화면을 무효화한다.
            playerNotice: (kind, owner, value) => { console.info(`Player UI ${kind} ${owner} ${value}`); if (this._mainMenu) this._mainMenu.dirty = true; this._displayDirty = true; },
            // 원본 MessageBoard 상태 통지를 현재 미션의 실제 경고 소유자로 전달한다. 미션 억제 판정은 WarningMessages가 수행한다. (WB04-E1326)
            status: (kind, owner, value) => {
                const messages = this._campaignProduction?.ui?.warnings();
                if (!messages) throw new Error('전투 상태 통지의 실제 MessageBoard 미연결');
                messages.battleStatus(kind | 0, owner | 0, value | 0);
            },
            // 원본 연결 진단을 브라우저 콘솔에 기록한다.
            log: (text) => console.info(text),
        };
    }

    // 현재 메뉴에 원본 본문·Cancel 메타를 가진 진행 창을 표시한다. `view` 는 창을 가운데에 놓을 보기 영역 `{ left, top, right, bottom }` 이고 없으면 화면 전체이다.
    // 원본 State7 의 시작 안내는 `{85, 0, 너비, 높이}`, 로그인 진행 문구는 그 시점의 현재 지도 영역(Gump 의 mapView)이다. (2026-11-21 WB04-E1131)
    _showMissionLoading(message, view = null) {
        const menu = this._mainMenu, resources = this._resources, data = this._gameData;
        if (!menu || !resources || !data) throw new Error('미션 진행 창의 실제 메뉴·자원 수명 누락');
        const area = view ? { left: view.left, top: view.top, width: view.right - view.left, height: view.bottom - view.top } : { width: menu.width, height: menu.height };
        const next = createMissionLoadingWindow({ message, cancelText: resources.translate('Cancel'), resources, gameData: data,
            screen: area, nowSeconds: this._frameSeconds, now: () => this._frameSeconds,
            onCommand: (command) => this._onDialogCommand(command), onSound: () => this._audio?.playSound(BUTTON_SOUND_NAME) });
        this._missionLoadingWindow?.close(); this._missionLoadingWindow = next; menu.setDialog(next); this._displayDirty = true;
    }

    // 다른 대화상자가 열린 경우에는 그 창을 건드리지 않고 진행 창 수명만 반납한다.
    _closeMissionLoading() {
        const window = this._missionLoadingWindow; if (!window) return;
        window.close(); this._missionLoadingWindow = null;
        if (this._mainMenu?.dialog === window) { this._mainMenu.setDialog(null); this._displayDirty = true; }
    }

    // 같은 실제 Client의 State7 부모에서 선택된 Fort World를 설치하고 State8 내부 접속을 건다.
    _runCampaignState7(services, viewport) {
        const client = this._campaignStateClient(), request = this._clientGuide?.fortRequest();
        if (!request || this._campaignWorld) throw new Error('캠페인 State7의 현재 요청 또는 World 수명 오류');
        runCampaignMissionStarting(client, () => {
            captureCampaignMissionStarting(client, request);
            // 시작 정보를 보관한 뒤 같은 전체 State 부모가 기존 저장소를 정리하고 World 없는 HUD를 만든다.
            cleanupNetstormStateWorld(this, 1);
            // 원본 State7 의 전투 장면 음악 시작(StartScene(true))이다. World 설치보다 앞이다. (2026-11-20 WB04-E1119)
            this._beginSceneMusic();
            // 원본 LoadMissionWorldBody 는 장면 음악 뒤, 월드·커서 설치 앞에서 Guide 미션이면 게임 시계를 정지하고 관문의 자기 정지 표식을 세운다(PauseMissionWorldEntry → PauseMissionFrameForDialog).
            // 정지는 관문의 첫 안내 창(`lockFrames` 10→0)이 열리고 닫힐 때까지 이어져 그동안 게임 시각이 멈춘다(이전에는 창이 뜰 때까지 4프레임 동안 게임이 흘렀다). (2026-11-23 WB04-E1281)
            pauseMissionWorldEntry(() => Boolean(this._clientGuide?.current()), () => pauseMissionFrameForDialog(this._clientGuide.current().runtime.gate, () => this._gameClock.paused(), () => this._gameClock.pushGamePause()));
            const world = this._installCampaignWorld(services, viewport); world.prepareLocalConnectionForState();
        }, (address, port) => {
            if (!this._campaignWorld) throw new Error('캠페인 State8의 현재 World 없음');
            this._campaignWorld.session().connectLocalConnectionForState(address, port);
        });
    }

    // 현재 Guide의 선택된 Fort와 같은 Client 공통 소유자로 GameWorld를 설치한다. State7·로그인은 호출자의 책임이다.
    _installCampaignWorld(services, viewport) {
        const current = this._clientGuide?.current(), request = this._clientGuide?.fortRequest();
        if (this._campaignWorld || !current || !request || request.consumed || !this._gameAssets || !this._gameData) throw new Error('캠페인 World의 현재 Guide/요청 또는 단독 수명 계약 오류');
        const fort = parseFort(current.script.readFortImage(request.fort), createFortTypes(this._gameAssets.typeTable));
        const install = { ...this._campaignInstallServices(), ...services?.install };
        const localLogin = services?.localLogin === null ? null : { ...this._campaignLoginServices(), ...services?.localLogin };
        const gameplay = this._campaignGameplayServices();
        const ownership = { ...gameplay.ownership, ...services?.ownership };
        const assets = { ...gameplay.assets, ...services?.assets, postPop: { ...gameplay.assets.postPop, ...services?.assets?.postPop } };
        const ai = { ...gameplay.ai, ...services?.ai, names: { ...gameplay.ai.names, ...services?.ai?.names }, participation: { ...gameplay.ai.participation, ...services?.ai?.participation } };
        const production = { ...gameplay.production, ...services?.production, effects: { ...gameplay.production.effects, ...services?.production?.effects } };
        const missionOutcome = { ...gameplay.missionOutcome, ...services?.missionOutcome };
        const world = new CampaignGameWorld(this._gameAssets, this._gameData.files, fort, request.fort, { ...gameplay, ...services, ...this._campaignSharedServices(), install, localLogin, ownership, assets, ai, production, missionOutcome }, viewport);
        this._campaignWorld = world; this._campaignWorldMissionToken = current.runtime.saveToken;
        return world;
    }

    // 현재 World의 Session을 반납하고 같은 미션 연결 대기 관문을 해제한다.
    _releaseCampaignWorld() {
        this._closeModPanel(false);
        this._mods?.end('leave');
        this._clearCampaignTutorialObjective();
        // World 가 없어도(State7 도중 실패 등) 이미 시작한 장면 음악은 메뉴 곡으로 되돌린다. (2026-11-20 WB04-E1119)
        this._endSceneMusic();
        if (!this._campaignWorld && !this._campaignProduction) return;
        if (this._campaignWorld) {
            // 공통 부모가 자식 정리 증빙·Core0·Runtime 반납·새 HUD 생성을 원문 순서로 수행한다.
            this._clearThunderFlashes(); cleanupNetstormStateWorld(this, 0); this._endSceneMusic(); return;
        }
        // 원본 월드 정리 어댑터(UberGump 004b2df0)는 Session·UI·Kernel 을 반납하기 전에 번개 프로세스(ClearThunderFlashes)와 초기 캡처 기한(005c8618)을 정리한다. (2026-11-20 WB04-E1121, 2026-11-24 WB04-E1329)
        this._clearThunderFlashes();
        if (this._clientLocal) this._clientLocal.captureEnd().value = 0;
        // 원본 State 월드 정리의 채팅 채널 정리(Clear445140 → ChatChannels::ResetOccupied: 점유된 기록의 이름·플래그·ID 를 모두 0 으로)와 시간 동기화 세 DWORD 초기화(Clear4c1f20 → ResetClientClockSynchronization)이다.
        // 이 둘이 없어 미션을 떠난 뒤에도 이전 미션의 채팅 대상 기록과 시간 응답 방식(replyMode 1)이 다음 미션까지 남았다. (2026-11-23 WB04-E1268)
        this._clientChatChannels?.resetOccupied();
        if (this._clientLocal) resetClientClockSynchronization(this._clientLocal);
        // 원본 State 월드 정리는 0059a8ac(건물 숨김, F2·View 메뉴)을 0 으로 되돌린다. 되돌리지 않으면 미션에서 켠 건물 숨김이 다음 미션에도 남아 처음부터 건물이 숨겨졌다. (2026-11-22 WB04-E1230)
        if (this._clientGameplay) this._clientGameplay.hideBuildings().value = 0;
        this._gameMenuButton?.destroy(); this._gameMenuButton = null; this._releaseCampaignPause();
        this._campaignProduction?.ui?.close(); this._campaignProduction?.hud?.close(); this._clientGump?.releaseFormHost(); this._campaignWorld?.dispose(); this._campaignWorld = null; this._campaignWorldMissionToken = 0; this._campaignUserInput = null; this._campaignProduction = null; this._campaignCursor = CURSOR_NORMAL;
    }

    // 로그인된 현재 World와 Guide의 실제 미션 Start를 같은 Client 수명에 준비한다.
    _prepareCampaignMissionStart() {
        const world = this._campaignWorld, current = this._clientGuide?.current(), gump = this._clientGump, chat = this._clientChatWindows;
        if (!world || !current || !gump || !chat || !world.session().localConnectionReady() || this._campaignWorldMissionToken !== current.runtime.saveToken || current.runtime.start) throw new Error('미션 Start의 현재 World·Guide·로그인 수명 오류');
        const client = { time: () => this._gameClock.time(), renderer: () => this._clientRenderer,
            menu: () => this._mainMenu ? { refreshStormPower: () => this._refreshCampaignStormPower(), receiveNativePacketChat: (packet) => this._receiveCampaignChat(packet) } : null };
        const effects = makeCampaignMissionStartEffects(client, world, () => {
            const token = chat.window.current; if (token) gump.events().bubble(token, 0x02000088, 0, 0);
        });
        effects.encodeDisplayText = (text) => displayTextCodec.encode(text); effects.hasExtendedDisplayText = hasExtendedDisplayText;
        const session = world.session(), runtime = current.runtime;
        const start = new RawCampaignMissionStartBindings(session, current.script, runtime.gate, runtime.state, session.missionStartSettings(), session.missionAiSettings(), effects);
        runtime.start = start; return start;
    }

    // 같은 로그인 World의 선택·강조·운반 입력기를 현재 Client 시계와 커서에 연결한다.
    _prepareCampaignUserInput() {
        const world = this._campaignWorld, current = this._clientGuide?.current(), clock = this._gameClock;
        if (!world || !current || !clock || !this._gameData || !this._clientInputQueue || this._campaignUserInput || !world.session().localConnectionReady() || this._campaignWorldMissionToken !== current.runtime.saveToken) throw new Error('미션 사용자 입력의 현재 수명 오류');
        const missing = (name) => { throw new Error(`미션 사용자 입력의 실제 ${name} 미연결`); };
        const client = { paused: () => clock.paused(), traceObject: (sid) => {
            if (!this._clientTrace || !this._surfaceStorage || !this._clientGameplay) throw new Error('미션 선택의 실제 객체 진단 수명 누락');
            traceClientObject({ surfaceStorage: () => this._surfaceStorage, gameplayGlobals: () => this._clientGameplay }, this._clientTrace, sid);
        },
            drawNativePathMarker: () => missing('경로 진단 표시'), presentNativeCursorClear: () => missing('커서 즉시 출력') };
        bindCampaignUserInputServices(client, world, (number) => { this._campaignCursor = number >>> 0; this._applyCursor(); }, (section) => this._openTell(section));
        this._campaignUserInput = new CampaignUserInput(world, this._gameData, { paused: () => clock.paused(), queue: this._clientInputQueue, cameraKey: (dx, dy) => this._campaignCameraDevice().requestKeyDelta(dx, dy) });
        return this._campaignUserInput;
    }

    // 현재 World 커서의 화면·Gump·선택·전송 효과를 같은 앱 장치에 연결한다.
    _prepareCampaignCursor() {
        const world = this._campaignWorld, gump = this._clientGump, resources = this._resources;
        if (!world || !world.session().localConnectionReady() || !this._campaignUserInput || !gump || !resources?.fonts || !this._clientInputPoll || !this._clientRenderer) throw new Error('미션 커서의 실제 Client 수명 누락');
        const session = world.session(), cursor = session.cursor(); gump.setFormHost(session.gumpFormHost());
        const client = { getRenderer: () => this._clientRenderer, fonts: () => this._resources.fonts,
            translate: (text) => this._resources.translate(text), poll: (code) => this._clientInputPoll.poll(code),
            getCursor: () => ({ set: (number) => { this._campaignCursor = number >>> 0; this._applyCursor(); } }) };
        const node = (token) => { const current = gump.node(token); if (!current) throw new Error('미션 커서의 실제 Gump 토큰 누락'); return current; };
        const controls = { pickGump: (point) => gump.events().pickTopLevel(point),
            dispatchGump: (token, ...args) => gump.events().dispatch(token, ...args), gumpParentSid: (token) => node(token).object,
            handleGump: (token, ...args) => gump.handle(token, ...args), requestClosePopups: () => this._closeCampaignPopups(), menus: () => this._campaignProduction?.ui ?? null };
        const effects = makeCampaignCursorClientEffects(client, controls);
        effects.lifecycle.highlight = (mode) => world.highlight(mode);
        effects.send = (target, packet) => session.sendLocalPacket(target | 0, packet);
        effects.drop.refresh = (sid, force, move) => { if (force || move !== 1) throw new Error('미션 커서 선택 갱신 인자 오류'); world.select(sid); };
        cursor.setEffects(effects); cursor.display.setExternal((token) => node(token).depth, null);
        cursor.display.setText(resources.fonts, (text) => this._resources.translate(text));
        return cursor;
    }

    // Ready가 된 Session을 State7에서 만든 같은 생산 패널 유지 상태에 연결한다. 독립 호출자는 같은 상태를 여기서 만든다.
    _prepareCampaignProduction() {
        const world = this._campaignWorld, current = this._clientGuide?.current(), globals = this._clientProductionGlobals, clock = this._gameClock;
        if (!world || world.connecting || !current?.runtime.start || !globals || !clock || !this._clientRenderer || this._campaignProduction?.maintenance || this._campaignWorldMissionToken !== current.runtime.saveToken) throw new Error('미션 생산 유지의 현재 수명 오류');
        // 생성 단계가 이미 정한 최초 슬롯·다리 기한은 다시 쓰지 않는다.
        const session = world.session(), production = this._campaignProduction ?? { session: null, panel: new ProductionMaintenancePanel(), maintenance: null, ui: null, worldBound: false }, panel = production.panel;
        if (!production.ui) { panel.initialized = true; panel.bridgeUntil = clock.wallSeconds(); panel.slots = session.battleOption(1) * 2 + 2; }
        globals.bridgeType = this._surfaceStorage.links.bridgeType;
        session.synchronizeProductionContents();
        production.session = session;
        production.maintenance = session.makeProductionMaintenance(globals, panel, () => clock.wallSeconds(), (mask) => { production.ui?.refresh(mask); this._clientRenderer.invalidateAll(); });
        this._campaignProduction = production; return production;
    }

    // 기존 Ready 호출자는 같은 Client 생성과 실제 World 도킹을 연속 수행한다.
    _prepareCampaignProductionUi() {
        const production = this._campaignProduction;
        if (!production?.session) throw new Error('생산 UI 도킹의 실제 Session 미연결');
        if (!production.ui) this._createCampaignProductionHud(production);
        this._dockCampaignProductionHud(production); return production.ui;
    }

    // World 없이 같은 Client 문맥으로 Uber HUD·생산·Chat 자식을 생성한다.
    _createCampaignProductionHud(production) {
        const root = this._clientGump, screen = this._clientPaletteScreen;
        if (!production || production.ui || !root || !screen || !this._clientColorSort || !this._gameAssets || !this._resources?.fonts || !this._clientSoundPlayer || !this._clientSoundNames) throw new Error('미션 생산 UI의 현재 수명 누락');
        // 우클릭 빠른 도움말(원본 RawMenuControls::BaseHelp): 노드의 대체 글자 포인터(주제 이름)가 있으면 제목+Help 항목 팝업을 연다. 항목은 F1 도움말 명령(0x02000017)에 주제 문자열 토큰을 실어 보낸다. (2026-11-22 WB04-E1236)
        const helpState = new GumpHelpState(); helpState.body = 0x61000002; helpState.event = 0x02000017;
        root.bindTextRelease((pointer) => Boolean(this._helpStrings?.delete(pointer))); // 노드 삭제 때 문구·주제 포인터를 반납한다(원본 GumpLifetime::DestructBase).
        const helpMenu = () => { const writer = production.ui?.helpMenu(); if (!writer) throw new Error('우클릭 도움말의 메뉴 작성기 미연결'); return writer; };
        const help = new GumpHelp(root.createState(), helpState, {
            node: (token) => root.node(token), text: (pointer) => this._helpString(pointer),
            beginMenu: (flags) => helpMenu().begin(flags), title: (pointer) => helpMenu().title(this._helpString(pointer)),
            entry: (body, command, topic) => helpMenu().entry(this._helpString(body), command, this._clientChatWindows.store(topic)),
            attachMenu: (x, y, parent) => helpMenu().attach(x, y, parent), sound: (name) => this._clientSoundPlayer.play(this._clientSoundNames.lookup(name), 0, 0, 0, 0, 0),
        });
        // 로그인은 Player 기록을 재할당하므로 HUD 생성 시의 뷰를 보관하지 않고 실제 조회 시점에 빌린다.
        const identity = () => new RawPlayerIdentity(this._clientPlayers.records(), new Uint8Array(0), this._clientChatRecords, {
            localOwner: () => this._clientGameplay.player.battleOwner, subscriber: () => this._clientSubscriber.mySubscriber().value,
            address: () => this._clientGameplay.localAddress().value, name: () => { const bytes = this._clientSubscriber.handle(), end = bytes.indexOf(0); return String.fromCharCode(...bytes.subarray(0, end)); },
            battle: () => this._surfaceStorage.ownerMode.battle, editor: () => this._clientGameplay.world.editor !== 0,
            metamap: () => this._surfaceStorage.ownerMode.challenge, unknownName: () => 'SOMEONE',
        });
        // 입력/객체 효과가 실행되는 때에만 현재 실제 World를 요구한다.
        const currentWorld = () => { const world = this._campaignWorld; if (!world) throw new Error('HUD 효과의 현재 실제 World 미연결'); return world; };
        const context = new ProductionControlContext(this._surfaceStorage, this._clientGameplay, this._clientPlayers, this._clientGameplay.battleOptionsOwner(), this._clientSp, this._clientColorSort.ownerTables);
        const devices = {
            now: () => this._gameClock.wallSeconds(), gameSeconds: () => this._gameClock.gameSeconds(), translate: (value) => this._resources.translate(value),
            sound: (name) => this._clientSoundPlayer.play(this._clientSoundNames.lookup(name), 0, 0, 0, 0, 0), tell: (section) => this._openTell(section),
            about: () => { throw new Error('생산 정보 메뉴 미연결'); },
            menu: { data: this._gameData, helpWord: () => this._clientLocal.helpWord().get(),
                ownerName: (owner) => displayTextCodec.decode(Uint8Array.from(identity().nameForOwner(owner), (letter) => letter.charCodeAt(0))),
                objectMenu: { get world() { return currentWorld(); }, get identity() { return identity(); }, staticText: (value) => this._clientChatWindows.store(value), tell: (name) => this._openTell(name),
                    player: { translate: (value) => this._resources.translate(value), assertion: (value) => console.error(`Player 메뉴 진단: ${value}`),
                        // Player 이름 칸의 표시 부호화 바이트(이진 문자열)를 화면 문자열로 해독한다(한국어 이름이 제목·동맹 줄에서 깨지지 않게). (2026-11-23 WB04-E1290)
                        decodeName: (value) => displayTextCodec.decode(Uint8Array.from(value, (letter) => letter.charCodeAt(0))),
                        parseAddress: () => { throw new Error('Player 메뉴 주소 해석 미연결'); }, reliability: () => { throw new Error('Player 메뉴 메타맵 신뢰도 미연결'); },
                        ignored: (subscriber) => this._clientChatWindows.ignored.includes(subscriber),
                        // 현재 채팅 입력줄의 수신자 접두에 그 Player 이름이 들어 있으면 1 이다(원본 0042e9e0). 채팅 창이 없으면 0 이다. 단독 미션 Players 메뉴는 자기 자신 행에서만 이 값을 읽고 쓰지 않는다. (2026-11-22 WB04-E1169)
                        whispered: (subscriber) => { const chat = this._campaignProduction?.ui?.chat(); return chat?.current() ? (chat.containsRecipient(identity().nameForSubscriber(subscriber)) ? 1 : 0) : 0; } },
                    // 제단 받침(dais)은 같은 좌표의 제단(altar)을 찾아 그 메뉴로 위임한다(원본 447930). 제단 TYPE 은 게임 보조 상태의 값이다. 위임 자체는 객체 메뉴 열기가 같은 몸체로 처리한다. (2026-11-21 WB04-E1137)
                    open: { altar: (sid) => { const session = production.session, raw = session.core().pool.slot(sid), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength), world = session.world(); return world.find(view.getFloat32(14, true), view.getFloat32(18, true), world.storage.gameAux.altarType); },
                        delegate: () => { throw new Error('사제 메뉴 제단 위임 미연결'); },
                        project: (x, y) => { const point = this._clientGameplay.camera().snapshot().project(x, y); return [point.x, point.y]; },
                        content: (sid) => production.session.priests().carrier.findContained(sid, 1, 0, 0), range: (type) => production.session.core().types[type].range,
                        screenWidth: () => this._clientPaletteScreen.width } },
                command: (...args) => this._clientInputQueue.pushCommand(...args), invalidate: (rect) => this._clientRenderer.invalidate(rect, 0), gameMenu: this._campaignGameMenuDevices(production), knowledge: this._campaignKnowledgeDevices(), warnings: this._campaignWarningDevices(production),
                // Players ▸ Send Money ▸ Send X 숫자 입력 창의 확정 장치(원본 004c9350)이다. (2026-11-22 WB04-E1168)
                sendMoney: { confirm: (target, text) => confirmSendMoney(target, text, this._campaignWorld.session().moneyInputHooks()) },
                // 채팅 도우미 ▸ Format ▸ Edit Back/Front Format 입력 창의 확정 장치(원문 0x4c9600/0x4c9680)이다. (2026-11-22 WB04-E1179)
                chatFormat: { confirm: (mode, text, vtable) => this._confirmChatFormat(vtable, text) },
                // 미션 화면 아래 채팅 창(F3)의 Client 장기 상태와 현재 값 조회 장치이다. 로컬 단독 미션은 수신자·서버 전송이 없어 입력은 로그에 되울린다. (2026-11-22 WB04-E1169)
                chat: this._campaignChatDevices(production, () => ui) },
            changed: () => { this._clientRenderer.invalidateAll(); this._displayDirty = true; }, closePopups: () => this._closeCampaignPopups(),
            poll: (code) => this._clientInputPoll.poll(code), cursor: (number) => { this._campaignCursor = number; this._applyCursor(); },
            setNodeHelp: (token, text, word) => this._setNodeHelp(token, text, word), baseHandle: (...args) => help.handle(...args), menuInstances: () => this._mainMenu?.popup?.isOpen ? 1 : 0,
        };
        // 원본 `InstallHud` 순서: 전체 Uber 루트가 세로·미니맵·채팅 구분선과 미니맵·생산 패널을 만들고 모드 3으로 도킹해 지도 영역을 정한다. (2026-11-20 WB04-E1082)
        let ui = null;
        const background = new ClientGumpBackground(root, this._gameAssets, () => 35, () => screen.shadeMaps(), () => screen.colors(), devices.baseHandle);
        const divider = new ClientHudDivider(root, { background, backgroundColor: () => 35, dividerColor: () => 55, baseHandle: devices.baseHandle, closePopups: devices.closePopups,
            menuInstances: devices.menuInstances, poll: devices.poll, cursor: devices.cursor });
        this._clientMinimapState ??= createMetadisplayDirtyState({ fullRefreshOwner: this._clientGameplay.mapRevision() });
        this._clientMinimapGlobals ??= createRawMinimapGlobals();
        const minimap = new ClientMinimap(root, { storage: this._surfaceStorage, dirty: this._clientMinimapState, globals: this._clientMinimapGlobals,
            view: () => {
                const data = this._clientGameplay.camera().snapshot().data, map = root.mapView();
                return { camera: [data.getUint32(0, true), data.getUint32(4, true)], viewport: { left: map.left, top: map.top, right: map.right, bottom: map.bottom }, pixelsX: data.getInt32(8, true), pixelsY: data.getInt32(12, true) };
            },
            center: (x, y) => currentWorld().centerCamera(x, y), move: (x, y) => currentWorld().moveFromMinimap(x, y), inputMode: () => currentWorld().inputMode(), poll: () => this._clientInputPoll.poll(0xffff),
            baseHandle: devices.baseHandle, changed: devices.changed, setNodeHelp: devices.setNodeHelp, translate: devices.translate });
        const hud = new ClientUberHud(root, { setNodeHelp: devices.setNodeHelp, translate: devices.translate, assets: this._gameAssets, panelGlobals: this._clientProductionGlobals, divider, minimap,
            // 채팅 창은 생산 UI 조립이 만든 `ui.chat()` 이다(Uber 루트가 생산 패널 뒤에 `createChat` 을 부른다). (2026-11-22 WB04-E1169)
            chat: { create: () => ui.chat().create(), setVisible: (visible) => ui?.chat()?.setVisible(visible), setInput: (visible) => ui?.chat()?.setInput(visible), refreshTechnology: () => ui.chat().refreshTechnology() },
            fullDeck: () => this._clientGameplay.fullDeck(), mapRevision: () => this._clientGameplay.mapRevision(),
            createMinimap: () => minimap.create(),
            createProduction: () => { ui = createClientProductionAssembly(root, context, this._gameAssets, this._resources.fonts, screen, this._clientColorSort, this._clientProductionGlobals, production.panel, devices); return ui.construct(); },
            closePopups: () => this._closeCampaignPopups(), baseHandle: devices.baseHandle, invalidate: (rect, flags) => this._clientRenderer.invalidate(rect, flags),
            finishResize: () => { this._clientKencloud.worldRedraw().value = 1; this._campaignWorld?.resize(this._layout.logicalWidth, this._layout.logicalHeight); this._clientRenderer.invalidateAll(); this._displayDirty = true; } });
        production.hud = hud; production.minimap = minimap; hud.createForState(); production.ui = ui; return ui;
    }

    // Ready가 된 같은 World를 기존 HUD에 연결하고 최초 도킹·실제 Form·미니맵·웹 메뉴 단추를 설치한다.
    _dockCampaignProductionHud(production) {
        const world = this._campaignWorld, session = world?.session(), screen = this._clientPaletteScreen;
        if (!production?.ui || !production.hud || !session || production.session !== session || production.worldBound) throw new Error('생산 HUD의 실제 World 도킹 수명 오류');
        const ui = production.ui; ui.bindWorld(session); production.hud.dockForWorld();
        ui.finishInstall(); production.minimap.setPalette(screen.colors()); production.minimap.bindWorld(session.core()); production.worldBound = true;
        // 웹 전용: 화면 우측 상단 버튼으로도 ESC 와 같은 게임 메뉴막대를 열고 닫는다. 버튼은 Canvas 위 DOM 요소라 열린 메뉴막대보다 항상 앞에 있다. (2026-11-20 WB04-E1084)
        this._gameMenuButton?.destroy();
        this._gameMenuButton = new GameMenuButton(this._root, { label: this.texts.gameMenuButton, title: this.texts.gameMenuButtonTitle, fontFamily: this._gameData?.language === 'japanese' ? `"${JAPANESE_FONT_FAMILY}", sans-serif` : undefined, onClick: () => { this._toggleCampaignGameMenu(); this._canvas?.focus(); } });
        this._gameMenuButton.layout(this._layout); this._gameMenuButton.show();
        return ui;
    }

    // ESC 게임 메뉴막대가 읽는 현재 게임 조회 장치다(원본 `CurrentGameMenuHooks`·`RawCampaignSession::BindGameMenus` 대응). 값은 모두 호출 시점에 다시 읽는다. (2026-11-20 WB04-E1084)
    _campaignGameMenuDevices(production) {
        const session = () => this._clientGameplay.requireSession();
        return {
            // 원본 메뉴 준비는 SP 비트를 회전하고 RawGameMenuBar의 검산+3은 같은 실제 추적 상자에 쓴다. (WB04-E1327)
            prepare: () => this._clientSp.store().rotate(), tracking: this._clientSp.store().trackingOwner,
            global: (address) => this._campaignMenuGlobal(address, production),
            // 설정 층에 있는 키(소리·음량 등)는 Settings 가, 그 밖의 키(rotmode·theammode 등)는 게임 데이터가 현재 값을 가진다.
            config: (key) => {
                // 섬 소유자 표식(useBanners)의 현재 값은 원본 5319c0 처럼 소유자 색표 저장소가 가진다. (2026-11-20 WB04-E1090)
                if (key === 'useBanners') return this._clientColorSort.ownerTables.enabled | 0;
                const stored = this._settings?.get(key); return stored !== undefined ? Number(stored) | 0 : this._gameData.readInt(key, key === 'theammode' ? 1 : 0).value | 0;
            },
            demo: () => Boolean(this._clientGuide?.current()?.runtime.gate.demo),
            // 옵션 메뉴 Frame Rate 하위 항목(60·75·120 FPS)이다. (2026-11-20 WB04-E1094)
            frameRates: () => frameRateChoices(this._settings?.get('maxFPS')), languages: () => languageChoices(this.options.language), difficulties: () => this._campaignWorld ? [] : difficultyChoices(this._settings?.get('difficulty')),
            // 모드 관리는 웹 추가 기능이며 비교용 고정 오라클에는 제공하지 않는다.
            modMenuLabel: this.options.fixedFrameMs > 0 || this.options.originalComparison ? undefined : () => modPanelTexts(this._gameData?.language ?? this.options.language).menu,
            player: (owner) => this._clientPlayers.records().subarray(owner * 180, owner * 180 + 180),
            // Player 이름은 표시 부호화 바이트이므로 한국어 이름이 깨지지 않게 디코딩한다. (2026-11-21 WB04-E1166)
            decodeName: (bytes) => displayTextCodec.decode(Uint8Array.from(bytes)),
            refreshPriest: (owner) => session().refreshPriestParticipation(owner),
            enabled: (code) => WIRED_MENU_COMMANDS.has(code >>> 0), configEnabled: () => true, storeText: (value) => this._clientChatWindows.store(value),
            about: () => this._resources.translate('About'), technicalHelp: () => 0, width: () => this._layout.logicalWidth,
        };
    }

    // 원본 전역 주소 식별자를 현재 앱의 값으로 바꾼다. 명령 전역 주소는 명령 ID를 돌려준다.
    _campaignMenuGlobal(address, production) {
        const commandId = gameMenuCommandValue(address); if (commandId !== null && commandId !== undefined) return commandId >>> 0;
        const game = this._clientGameplay, modes = this._surfaceStorage.ownerMode, session = () => this._clientGameplay.requireSession();
        switch (address) {
        case 0x5c85a4: return game.world.editor ? 1 : 0;
        case 0x594fa4: return game.world.missionPost ? 1 : 0;
        case 0x594fbc: return modes.battle ? 1 : 0;
        case 0x594fb8: return modes.fort ? 1 : 0;
        case 0x594fc8: return modes.challenge ? 1 : 0;
        case 0x594fa0: return this._surfaceStorage.links.mission ? 1 : 0;
        case 0x59a8ac: return game.hideBuildings().value >>> 0;
        case 0x540c70: return game.player.battleOwner >>> 0;
        case 0x55b4b0: return this._gameClock.paused() ? 1 : 0;
        // 웹 소리 장치는 사용자 시작 입력 뒤 항상 있으므로 소리 하위 메뉴는 활성이다.
        case 0x5c7b14: return 1;
        case 0x531900: return Number(this._settings?.get('soundVolume') ?? 5);
        case 0x531904: return Number(this._settings?.get('musicVolume') ?? 5);
        case 0x531860: return this._layout.logicalWidth;
        case 0x531864: return this._layout.logicalHeight;
        case 0x563e38: return this._clientGump.createState().collection.root;
        default:
            if (address > 0x52f81e && address < 0x52f827) return session().hiddenPlayers()[address - 0x52f81e] >>> 0;
            throw new Error(`게임 메뉴의 미연결 전역: 0x${address.toString(16)}`);
        }
    }

    // 오른쪽 위 경고·상태 안내가 읽는 벽 시각·억제·화면 크기와 메시지 이름·모드 조회 장치다(cpppj `UberGump` 의 `pauseContext`·`EnableWarnings`). 값은 호출 시점에 다시 읽는다. 메타맵 참가자 이름은 웹 제외 범위라 연결하지 않는다. (2026-11-20 WB04-E1089)
    _campaignWarningDevices(production) {
        const empty = new Uint8Array(1), game = () => this._clientGameplay;
        const meta = () => { throw new Error('웹 제외 메타맵 참가자 이름 미연결'); };
        return { now: () => this._gameClock.wallSeconds(), suppressed: () => Boolean(game().world.missionPost),
            view: () => ({ width: this._layout.logicalWidth, height: this._layout.logicalHeight, palette: this._clientPaletteScreen.colors(), inset: this._gameMenuButton?.logicalInset(this._layout.logicalWidth) ?? 0 }),
            context: { challenge: () => this._surfaceStorage.ownerMode.challenge ? 1 : 0, battle: () => this._surfaceStorage.ownerMode.battle ? 1 : 0,
                playerName: (owner) => { if (owner > 0 && owner < 9) this._clientPlayers.records().subarray(owner * 180 + 12, owner * 180 + 44); },
                playerText: (owner) => owner <= 0 || owner >= 9 ? empty : this._clientPlayers.records().subarray(owner * 180 + 12, owner * 180 + 44),
                challengePlayerName: meta, challengePlayerText: meta,
                gameTimeSeconds: () => readGameTimeSeconds(game().battleClockStamp(), () => game().battleClockOrigin().value, () => Math.trunc(performance.now())) } };
    }

    // 게임 메뉴 Pause(Shift-F9)를 실행한다. 원본 `GameMenuActions` 의 Pause 분기가 정지 여부를 읽어 시계·소리를 멈추거나 되돌리고 'Game Paused!' 안내를 만들거나 지운다. 정지·음소거 깊이는 미션을 떠날 때 되돌리도록 센다. (2026-11-20 WB04-E1089)
    _dispatchCampaignPause(event) {
        const game = this._clientGameplay, clock = this._gameClock, warnings = this._campaignProduction?.ui?.warnings(), world = this._campaignWorld;
        if (!game || !clock || !warnings || !world || !this._clientKencloud) throw new Error('일시정지의 현재 장치 수명 누락');
        const read = (address) => {
            if (address === 0x00594fa4) return game.world.missionPost ? 1 : 0;
            if (address === 0x005c85a4) return game.world.editor ? 1 : 0;
            throw new Error('일시정지의 미연결 전역');
        };
        const actions = new GameMenuActions({ read, hideBuildings: game.hideBuildings(), redraw: this._clientKencloud.worldRedraw() }, {
            command: (address) => gameMenuCommandValue(address), paused: () => clock.paused(),
            status: (kind, owner, enabled, parameter, text) => {
                if (kind !== 11 || owner || parameter || text) throw new Error('다른 메뉴 상태 안내 분기 미연결');
                warnings.pauseStatus(enabled !== 0);
            },
            cancelCursor: () => world.session().cursor().cancel(),
            pauseClock: () => { clock.pushGamePause(); this._menuPauseDepth += 1; }, resumeClock: () => { clock.popGamePause(); this._menuPauseDepth = Math.max(0, this._menuPauseDepth - 1); },
            muteSound: () => this._setCampaignMute(true), resumeSound: () => this._setCampaignMute(false),
        });
        actions.handle(event.code, event.x, event.y);
    }

    // 효과음 재생기와 음악 음량 노드를 함께 음소거하거나 되돌린다(원본 PushMute/PopMute). 깊이를 세어 미션 반납 때 남은 만큼 되돌린다. (2026-11-20 WB04-E1089)
    _setCampaignMute(on) {
        if (on) { this._clientSoundPlayer?.pushMute(); this._audio?.pushMute(); this._menuMuteDepth += 1; return; }
        if (this._menuMuteDepth <= 0) return;
        this._clientSoundPlayer?.popMute(); this._audio?.popMute(); this._menuMuteDepth -= 1;
    }

    // 게임 메뉴가 올려 둔 일시정지·음소거를 모두 되돌린다. 미션을 떠나도 다음 미션의 시계·소리가 멈춘 채 남지 않게 한다. (2026-11-20 WB04-E1089)
    _releaseCampaignPause() {
        while (this._menuPauseDepth > 0) { this._gameClock?.popGamePause(); this._menuPauseDepth -= 1; }
        while (this._menuMuteDepth > 0) this._setCampaignMute(false);
    }

    // F6 지식 창이 읽는 현재 화면 크기·언어·번역·정적 문자열·팝업 표식 장치다(원본 `RawMenuControls::OpenKnowledge` 의 호출자 몫). 값은 호출 시점에 다시 읽는다. (2026-11-20 WB04-E1088)
    _campaignKnowledgeDevices() {
        return { width: () => this._layout.logicalWidth, height: () => this._layout.logicalHeight, language: () => originalLanguageNumber(this._gameData?.language ?? this.options.language),
            storeText: (value) => this._clientChatWindows.store(value), mark: (command) => this._clientHelpPopup.mark(command) };
    }

    // 지식 창(Storm 카드·World 카드·선택 메뉴)을 연다. Player 가 알려지지 않았거나 메타맵이면 열지 않는다. 열었으면 true. (2026-11-20 WB04-E1088)
    _openCampaignKnowledge(kind) {
        const knowledge = this._campaignProduction?.ui?.knowledge(); if (!knowledge) return false;
        const opened = knowledge.open(kind); this._clientRenderer.invalidateAll(); this._displayDirty = true; return opened;
    }

    // ESC 게임 메뉴막대를 열거나 닫는다(원본 `ToggleGameMenu`). 메뉴가 없는 화면이면 아무것도 하지 않는다. 우측 상단 버튼의 눌림 표시도 함께 맞춘다.
    _toggleCampaignGameMenu() {
        const bar = this._campaignProduction?.ui?.gameMenu(); if (!bar) return false;
        const open = bar.toggle(); this._gameMenuButton?.setPressed(open); this._displayDirty = true; return open;
    }

    // 현재 Client 의 시각·난수·Kernel 과 미션 월드(있으면)의 풀·Player·SP·Graph·장부를 판번호 1 TSV 로 돌려준다(원본 `UberGump::SessionStateDump`, 검사 전용).
    // 월드가 없으면 `world 0` 덤프이다. cpppj 오라클 덤프와 `tools/session-dump-compare.mjs` 로 L2·L3 대조에 쓴다. 상태를 바꾸지 않고 난수도 쓰지 않는다. (2026-11-21 WB04-E1129)
    // { includeAi: true }를 명시한 검사만 현행 C++와 같은 실제 AI 본문·행동 기한 행을 추가한다. 생략하면 기존 덤프 바이트를 유지한다.
    sessionDump(options = {}) {
        const clock = this._gameClock, random = this._clientRandom; if (!clock || !random) throw new Error('Session 덤프의 현재 시계·난수 수명 누락');
        const session = this._campaignWorld?.session() ?? null, kernel = session ? session.core().kernel : this._clientKernel;
        return formatSessionStateDump(captureSessionStateDump(session ? session.core().pool.edition : 'patch1078', clock.time(), random, kernel, session, options));
    }

    // 검사 전용: 고정 시각 모드(?fixed=밀리초)에서 프레임을 가상 시각으로 정확히 count 개 진행한다(requestAnimationFrame 을 쓰지 않는다). 프레임마다 가상 시각이 `fixedFrameMs` 씩만 전진하고 Client 시계·난수·Kernel 이 그 시각으로 돈다.
    // 프레임 사이에 이벤트 루프를 한 번 넘겨(setTimeout 0) 이미 시작한 비동기 작업(예: 해독이 끝난 효과음)이 정리되게 하지만, 아직 준비되지 않은 자산을 기다려 주지는 않는다: 호출하는 쪽이 `_prepareCampaignAudio()` 같은 준비를 먼저 마쳐야 한다.
    // 지금까지 진행한 프레임 수를 돌려준다(`_fixedFrameCount`). 고정 시각 모드가 아니면 오류이다. 사용: await app.stepFixedFrames(90). (2026-11-23 WB04-E1271)
    async stepFixedFrames(count = 1) {
        if (!(this.options.fixedFrameMs > 0)) throw new Error('고정 시각 모드(?fixed=밀리초)에서만 프레임을 직접 진행합니다.');
        const view = this._view();
        for (let index = 0; index < count; index += 1) {
            this._fixedStamp = (this._fixedStamp ?? 0) + this.options.fixedFrameMs; this._frameHandle = 0; this._runFrameSafely(this._fixedStamp); this._fixedFrameCount = (this._fixedFrameCount ?? 0) + 1;
            await new Promise((resolve) => view.setTimeout(resolve, 0));
        }
        return this._fixedFrameCount ?? 0;
    }

    // 채팅 제출·수신이 빌리는 Client 조회 묶음이다(원본 Client 의 chat/설정/번역/소리 접근자를 앱의 장기 객체에 묶는다). 값은 호출할 때마다 현재 객체에서 읽는다. (2026-11-22 WB04-E1172)
    _campaignChatClient() {
        const self = this;
        return {
            chatManagerGlobals: () => self._clientChatManager, coreDataGlobals: () => self._clientCoreData, currentSubscriber: () => self._clientSubscriber.mySubscriber().value >>> 0,
            gameplayGlobals: () => self._clientGameplay, chatRecords: () => self._clientChatRecords, chatWindowGlobals: () => self._clientChatWindows, dotCommandGlobals: () => self._clientDotCommands,
            configuration: () => ({ get: (key) => self._gameData.get(key), set: (key, value) => self._gameData.set(key, value) }),
            translate: (text) => self._resources.translate(text), encodeDisplayText: (text) => displayTextCodec.encode(text), decodeDisplayText: (bytes) => displayTextCodec.decode(bytes),
            writeDiagnosticLog: (level, text) => console.info(`Netstorm: ${String(text).trim()}`), nsInfoGlobals: () => self._clientNsInfo,
            worldAudio: () => ({ sounds: () => ({ playByName: (name) => self._clientSoundPlayer.play(self._clientSoundNames.lookup(name), 0, 0, 0, 0, 0) }) }),
            requestThunderFlash: (brightness) => { this._startThunderFlash(brightness); },
            menu: () => ({ receiveNativePacketChat: (packet) => self._receiveCampaignChat(packet) }),
        };
    }

    // 원본 `ReadChatGlobal`: 채팅 송수신이 읽는 원문 전역 주소의 현재 값이다(관계표·숨김 Player·모드·manager·색·종류). 연결하지 않은 주소는 거부한다. (2026-11-22 WB04-E1172)
    _readChatGlobal(address) {
        const modes = this._surfaceStorage.ownerMode, window = this._clientChatWindows.window;
        if (address >= 0x595200 && address < 0x595200 + 81 * 4 && !(address & 3)) return this._clientPlayers.alliances()[(address - 0x595200) / 4] >>> 0;
        if (address >= 0x52f81e && address < 0x52f827) { if (!this._campaignWorld) throw new Error('채팅 관계 관전자의 현재 월드 없음'); return this._campaignWorld.session().hiddenPlayers()[address - 0x52f81e] >>> 0; }
        switch (address) {
        case 0x540c74: return this._clientSubscriber.mySubscriber().value >>> 0;
        case 0x540c70: return this._clientGameplay.player.battleOwner >>> 0;
        case 0x594fb8: return Number(modes.fort); case 0x594fbc: return Number(modes.battle); case 0x594fc8: return Number(modes.challenge);
        case 0x548740: return window.current >>> 0; case 0x548710: return this._clientChatWindows.receivedMask >>> 0; case 0x548748: return this._clientChatWindows.newsTime >>> 0;
        case 0x548714: return this._clientChatManager.current().value >>> 0;
        case 0x5ca92c: return 0; // 알림 Gump serial(PulseRegisteredIfIdle)은 아직 연결하지 않았다.
        case 0x5b5dd8: return this._screenColors().named[33].index >>> 0;
        case 0x52edf6: return 0x41; case 0x544c5c: return 0;
        default: throw new RangeError(`실제 Chat 전역 조회 미연결: ${address.toString(16)}`);
        }
    }

    // 현재 화면 팔레트의 색표이다(채팅 색 조회에 쓴다).
    _screenColors() { return this._clientPaletteScreen.colors(); }

    // 채팅 표시 옵션(`ShowBattle` 등 열 개)과 효과 옵션의 현재 값이다. 원본은 이 키에 기본값이 없고 값이 없으면 스택 잔여값을 읽으므로(Netstorm.exe 0x42ed30 역분석: 지역 변수를 초기화하지 않음) 웹은 모두 켜진 1 을 기본으로 둔다. (2026-11-22 WB04-E1172)
    _chatOption(key) {
        const stored = this._settings?.get(key) ?? this._gameData.get(key);
        if (stored === undefined || stored === null || stored === '') return CHAT_OPTION_KEYS.has(key) ? 1 : 0;
        return Number.parseInt(String(stored), 10) | 0;
    }

    // 미션 채팅 창의 제출·수신·옵션 장치 묶음이다(원본 `UberGump::MakeChatContext`). 제출은 원본 파이프라인(대상 해석→점 명령 분배→say→패킷)을 쓰고, 단독 미션은 로컬 서버(9)를 거쳐 41h 수신으로 돌아와 로그에 나온다. (2026-11-22 WB04-E1172)
    _campaignChatDevices(production, getUi) {
        const client = this._campaignChatClient(), surface = this._surfaceStorage;
        // Client 수명에 한 번만 점 명령 네 개와 chat manager 를 초기화한다.
        if (!this._clientChatInitialized) { initializeClientIssueCommands(client); initializeClientChatManager(client); this._clientChatInitialized = true; }
        const byteString = (text) => Array.from(displayTextCodec.encode(text), (byte) => String.fromCharCode(byte)).join('');
        const nameText = () => { const bytes = this._clientSubscriber.handle(), end = bytes.indexOf(0); return String.fromCharCode(...bytes.subarray(0, end < 0 ? bytes.length : end)); };
        const nameBytes = () => Uint8Array.from([...nameText()].map((letter) => letter.charCodeAt(0) & 0xff).concat(0));
        const context = createRawChatWindowContext({ translate: (text) => byteString(this._resources.translate(text)) });
        bindClientChatRecipients(context, client, surface, nameText);
        const packet = { ...makeClientChatPacketHooks(this._clientChatManager), currentName: nameBytes, read: (address) => this._readChatGlobal(address),
            send: (target, bytes) => { if (!this._campaignWorld) throw new Error('일반 Chat 송신의 현재 실제 월드 없음'); this._campaignWorld.session().sendMutableLocalPacket(target, bytes); },
            log: (level, text) => console.info(`Netstorm: ${String(text).trim()}`), zeroAddress: () => {}, localReceive: (bytes) => this._receiveCampaignChat(bytes) };
        bindClientChatMessage(context, client, surface, nameText, packet); // 이름 getter 는 문자열이고, 패킷의 현재 이름만 NUL 포함 바이트이다.
        const menus = { chatGeometryBackend: () => getUi().chat().chatGeometryBackend(), invokeChatCommand: (callback) => getUi().chat().invokeChatCommand(callback), printChat: (text) => getUi().chat().printChat(text) };
        bindClientChatSubmit(context, client, menus);
        return { globals: this._clientChatWindows, dotCommands: this._clientDotCommands, config: (key) => this._chatOption(key),
            read: (address) => address === 0x5cae94 ? this._input.pointer.x | 0 : address === 0x5cae98 ? this._input.pointer.y | 0 : address === 0x563e38 ? this._clientGump.createState().entries[0] : address === 0x548714 ? this._clientChatManager.current().value : this._campaignMenuGlobal(address, production),
            recipient: context.recipient, sendChat: context.sendChat, resolve: context.resolve, complete: context.editor.complete,
            clipboardRead: (bytes) => this._chatClipboardRead(bytes), clipboardWrite: (bytes) => this._chatClipboardWrite(bytes),
            refreshTechnology: () => refreshClientChatRankPrefix(client, this._gameAssets, surface) };
    }

    // 41h 채팅 패킷 수신이다(원본 `ReceiveNativePacketChat`): 무시·채널·관계 옵션을 거쳐 로그에 줄을 더하고 필요하면 채팅 영역을 연다. 채팅 창이 없으면 아무것도 하지 않는다. (2026-11-22 WB04-E1172)
    _receiveCampaignChat(packet) {
        const chat = this._campaignProduction?.ui?.chat(); if (!chat?.current()) return;
        const client = this._campaignChatClient(), name = () => { const bytes = this._clientSubscriber.handle(), end = bytes.indexOf(0); return String.fromCharCode(...bytes.subarray(0, end < 0 ? bytes.length : end)); };
        dispatchChatPacket(packet, {
            read: (address) => this._readChatGlobal(address), ignored: (subscriber) => this._clientChatWindows.ignored.includes(subscriber), config: (key) => this._chatOption(key),
            owner: (subscriber) => findClientChatSenderOwner(client, this._surfaceStorage, name(), subscriber), wake: () => {}, sound: (file) => this._clientSoundPlayer?.play(this._clientSoundNames.lookup(file), 0, 0, 0, 0, 0),
            append: (token, text, context) => chat.appendLog(text, context), show: () => chat.open(), log: (level, text) => console.info(`Netstorm: ${String(text).trim()}`),
            setNewsTime: (stamp) => { this._clientChatWindows.newsTime = stamp >>> 0; }, news: (text) => console.info('Netstorm: 서버 뉴스는 웹에서 표시하지 않습니다.', text),
        });
        this._displayDirty = true;
    }

    // 미션 화면에서 Gump 모달 입력 창(Send X 숫자 창)이 열려 있으면 그 토큰을, 아니면 0 을 돌려준다. 열려 있는 동안 키보드·마우스 입력은 모두 이 창이 받는다. (2026-11-22 WB04-E1168)
    _campaignInputDialogOpen() {
        const ui = this._campaignProduction?.ui;
        // Send X 숫자 창과 채팅 서식 입력 창(E1179) 중 열려 있는 쪽의 토큰이다.
        return this._campaignDisplayActive() ? ((ui?.sendMoney?.()?.current() ?? 0) || (ui?.chatFormat?.()?.current() ?? 0)) : 0;
    }

    // 채팅 도우미의 Edit Back/Front Format 입력 창을 연다(원문 0x4d4580/0x4d48f0). 초기 문자열은 현재 설정 `<구독자>chatFormat`(Back)·`chatFormat2`(Front)이고, 열려 있던 도우미 메뉴는 닫는다. (2026-11-22 WB04-E1179)
    _openChatFormat(front) {
        const dialog = this._campaignProduction?.ui?.chatFormat?.(); if (!dialog) throw new Error('채팅 서식 창의 실제 장치 미연결');
        const client = this._campaignChatClient(), stored = client.configuration().get(chatFormatKey(client.currentSubscriber(), front)) ?? '';
        const initial = Array.from(client.encodeDisplayText(String(stored)), (byte) => String.fromCharCode(byte)).join('');
        this._closeCampaignPopups(); dialog.open(front, initial); this._displayDirty = true;
    }

    // 서식 창의 OK/Enter 확정이다(원문 0x4c9600/0x4c9680): 입력 문자열을 300바이트 한도로 복사해 설정 `<구독자>chatFormat`(Back)·`chatFormat2`(Front)에 쓰고 chat manager 를 다시 초기화하며 항상 1 을 돌려준다.
    // 원문의 복사(GetText)는 길이+1 이 한도를 넘으면 -1 로 거부하고 출력 지역 변수를 건드리지 않아 쓰레기가 저장되지만, 웹은 쓰레기를 재현하지 않고 빈 값을 쓴다. (2026-11-22 WB04-E1179)
    _confirmChatFormat(vtable, text) {
        const front = (vtable >>> 0) === CHAT_FORMAT_FRONT_VTABLE, client = this._campaignChatClient();
        const value = text.length + 1 > CHAT_FORMAT_COPY_LIMIT ? '' : text;
        client.configuration().set(chatFormatKey(client.currentSubscriber(), front), client.decodeDisplayText(Uint8Array.from(value, (letter) => letter.charCodeAt(0) & 0xff)));
        initializeClientChatManager(client); return 1;
    }

    // 방향키를 세계(조각 회전·카메라 이동)로 보내면 안 되는 입력 UI 가 열려 있는지이다: 대화창·도움말 창, 팝업(우클릭 객체 메뉴 등), 게임 메뉴막대, 지식 창.
    // 채팅 입력줄과 Send X 입력 창은 입력 루프의 앞쪽 분기가 먼저 처리한다. 사용: if (this._campaignArrowKeyBlocked()) ... (2026-11-23 WB04-E1291, cpppj 1328 의 입력 UI 관문)
    _campaignArrowKeyBlocked() {
        const ui = this._campaignProduction?.ui;
        return Boolean(this._mainMenu?.dialog) || this._clientGump.blockingPopups() || Boolean(ui?.gameMenu()?.isOpen()) || Boolean(ui?.knowledge()?.isOpen());
    }

    // 채팅 입력줄이 지금 보이는지이다(미션 화면에서 입력줄이 보이는 동안 문자 키는 이 입력줄로 간다). 모달 입력 창과 달리 마우스는 월드로 간다. (2026-11-22 WB04-E1170)
    _campaignChatInputActive() {
        return this._campaignDisplayActive() && !this._campaignInputDialogOpen() && Boolean(this._campaignProduction?.ui?.chat()?.inputVisible());
    }

    // 입력줄이 받는 문자 키인지이다: 한 글자(Shift 만 허용)와 Backspace·Delete·Enter·좌우·Home·End. 그 밖의 키(F 키·ESC 등)는 평소 단축키 경로를 쓴다. (2026-11-22 WB04-E1170)
    _isChatTextKey(event) {
        if (event.altKey || event.metaKey || event.isComposing) return false;
        // 원본 편집기의 클립보드 키는 Ctrl+Insert(복사, 선택이 없으면 전체)·Shift+Insert(붙여넣기)·Shift+Delete(잘라내기)이다(text-entry-gump.js). Ctrl+C/X/A 는 원본에 없다. Ctrl+V 는 브라우저 paste 사건(`_onPaste`)이 맡는다. (2026-11-22 WB04-E1178)
        if (event.ctrlKey) return event.code === 'Insert' && !event.shiftKey;
        return (typeof event.key === 'string' && event.key.length === 1) || ['Backspace', 'Delete', 'Insert', 'Enter', 'ArrowLeft', 'ArrowRight', 'Home', 'End'].includes(event.key);
    }

    // 입력 창이 열린 동안의 키 하나를 원본 입력 큐에 올린다(printable 문자·Backspace·Enter·방향/Home/End/Delete 가상 키). 브라우저 기본 동작과 게임 단축키로는 보내지 않는다.
    // 한글 등 조합 중인 입력은 이 경로가 아니라 IME 경로가 맡는다. (2026-11-22 WB04-E1168)
    _queueCampaignDialogKey(event, released) {
        if (event.isComposing || this._clientComposing || !this._clientInputQueue) return;
        const pointer = this._input.pointer;
        if (event.key === 'Tab' || event.key === 'Backspace' || event.key === 'Enter' || event.key === 'Escape' || event.key === ' ' || (typeof event.key === 'string' && event.key.length === 1) || /^(?:Arrow|Home|End|Delete|Insert)/u.test(event.key ?? '')) event.preventDefault();
        queueBrowserVirtualKeyEvent(this._clientInputQueue, event, pointer, released);
        if (!released) queueBrowserCharacterEvent(this._clientInputQueue, event, pointer);
    }

    // 송금 명령을 현재 세션의 경제 장치로 실행한다(원본 UberGump 의 송금 일반 입력). X 는 숫자 입력 창(Send X Storm Power, 원본 SaveFortNameDialog::ShowMoney)을 모달로 열고 OK/Enter 가 확정(원본 004c9350)한다.
    // 모두 보내기는 `asksend(대상)` 확인 안내 창을 띄우고 그 단추가 모두 보내기 명령을 다시 올린다. (2026-11-20 WB04-E1126, 2026-11-22 WB04-E1168: Send X 숫자 입력 창 연결)
    _dispatchCampaignSendMoney(event) {
        const world = this._campaignWorld; if (!world) throw new Error('송금 일반 입력의 실제 월드 없음');
        const hooks = { ...world.session().moneyInputHooks(),
            numberDialog: (target) => { const dialog = this._campaignProduction?.ui?.sendMoney?.(); if (!dialog) throw new Error('송금 숫자 입력 창의 실제 장치 미연결'); this._closeCampaignPopups(); dialog.open(target); this._displayDirty = true; },
            askAll: (target) => { this._openTell(`asksend(${target})`); } };
        dispatchSendMoneyInput(event, hooks); this._clientRenderer.invalidateAll(); this._displayDirty = true;
    }

    // 동맹 요청·해제·확인·무승부 선언 입력을 처리한다(원본 HandleAllianceRequestInput). 보내는 packet 은 현재 로컬 서버(9)의 mutable 송신 경계로 간다. (2026-11-20 WB04-E1126)
    _dispatchCampaignAlliance(event) {
        const world = this._campaignWorld; if (!world || !this._clientPlayers || !this._clientChatWindows || !this._clientGameplay) throw new Error('동맹 입력의 실제 월드·Player·채팅 수명 누락');
        handleAllianceRequestInput(event.code, event.y | 0, this._clientChatWindows.allianceRequests, this._clientPlayers.records(), {
            clock: () => Math.trunc(performance.now()) >>> 0, localOwner: () => this._clientGameplay.player.battleOwner,
            send: (packet) => world.session().sendMutableLocalPacket(9, packet), tell: (text) => this._openTell(text) });
    }

    // 메뉴막대 항목이 입력 큐에 올린 명령을 실행한다. 연결되지 않은 명령은 큐를 막지 않도록 소비하고 콘솔에 기록한다(메뉴에서는 비활성이다).
    _dispatchCampaignMenuCommand(event) {
        const code = (event.code & 0x03ffffff) >>> 0;
        // Player 메뉴의 동맹 요청·해제와 안내 창의 확인·무승부 선언은 입력 처리기가 packet 을 서버(9)로 보낸다. (2026-11-20 WB04-E1126)
        if (ALLIANCE_INPUT_COMMANDS.has(code)) { this._dispatchCampaignAlliance(event); return false; }
        if (!WIRED_MENU_COMMANDS.has(code)) { console.warn(`Netstorm: 게임 메뉴 명령 0x${code.toString(16)} 은(는) 아직 연결되지 않아 실행하지 않았습니다.`); return false; }
        // 미션 중단(0x8c): 인자 0 은 ABORT 확인 창, 그 밖은 전투 떠나기 예약이다. 재시작(0x8d)·전투 떠나기(0xc5)는 월드를 반납하므로 true 를 돌려 입력 반복을 끝내게 한다. (2026-11-20 WB04-E1085)
        if (code === 0x0200008c) { dispatchMissionAbort(event, this._clientInputQueue, (section) => this._openTell(section)); return false; }
        // 하단 채팅 창 숨김/표시(F3·View 메뉴, 0x02000088)는 이 입력 프레임의 Gump 전달(`gump.input().route`)이 Uber 루트 도킹에서 이미 구분선을 옮긴다. 여기서는 다시 옮기지 않고(두 번 옮기면 되돌아간다) 지도 영역이 바뀌었으므로 다시 그리기만 한다. (2026-11-21 WB04-E1159)
        if (code === 0x02000088) { this._clientRenderer.invalidateAll(); this._displayDirty = true; return false; }
        if (code === 0x0200008d) return this._restartCampaignMission();
        if (code === 0x020000c5) return this._leaveCampaignMission();
        const text = () => { const value = this._clientChatWindows.strings.get(event.y >>> 0); return value === undefined ? '' : String(value); };
        if (code === 0x020000cf) { this._dispatchCampaignPause(event); this._displayDirty = true; }
        else if (KNOWLEDGE_COMMANDS.has(code)) this._openCampaignKnowledge(code === 0x02000132 ? KnowledgeMenuKind.Side : code === 0x02000133 ? KnowledgeMenuKind.World : KnowledgeMenuKind.Storm);
        // GetTechnology(0x0200009f): 브리핑·안내 창의 `$Button=이름,GetTechnology,기술번호` 가 올린 명령이다. y 의 전체 DWORD 를 기술 번호로 원본 492a30(Session.learnNewTechnology)에 넘긴다(뗌은 거부). (2026-11-22 WB04-E1227)
        else if (code === 0x0200009f) { dispatchGetTechnologyInput(event, (type) => { const world = this._campaignWorld; if (!world) { console.warn('Netstorm: GetTechnology 는 미션 월드가 있을 때만 기술을 지급합니다.'); return; } world.session().learnNewTechnology(type); }); this._clientRenderer.invalidateAll(); this._displayDirty = true; }
        else if (code === 0x020000f1) { this._dispatchCampaignViewAction(event); this._clientRenderer.invalidateAll(); }
        else if (code === 0x020000f0) { this._campaignProduction?.ui?.gameMenu()?.openPlayerList(); this._displayDirty = true; }
        else if (code === 0x0200006d) {
            // 설정 명령의 인자는 `키=값` 문자열이다. 설정 층의 키는 Settings 가 저장하고 그 밖의 키는 게임 데이터에 쓴다.
            const [key, value] = text().split('='); if (!key) return;
            this._applyCampaignConfig(key, Number(value));
            this._displayDirty = true;
        } else if (code === 0x020000e6 || code === 0x020000e7) this._settings?.set(code === 0x020000e6 ? 'soundVolume' : 'musicVolume', event.y | 0);
        else if (code === 0x02000095) {
            // Tell 명령은 섹션 이름이거나 `SetIslands(0|1)` 같은 원본 설정 호출이다.
            this._tellOrSetIslands(text());
        }
        return false;
    }

    // 메뉴·표시 키가 쓰는 설정 한 줄을 적용한다. 설정 층의 키는 Settings 가, 그 밖은 게임 데이터가 저장하며 두 쓰기 모두 설정 변경 통지(`_onSettingChange`·`_onConfigWrite`)에서 소유자 색표(useBanners) 등을 다시 동기한다(원본 `onChanged`). 섬 소유자 표식은 월드도 다시 그린다. (2026-11-20 WB04-E1090, WB04-E1093: 설정 키 승격과 통지 동기로 정리)
    _applyCampaignConfig(key, value) {
        if (this._settings?.get(key) !== undefined) this._settings.set(key, value); else this._gameData.set(key, value);
        if (key === 'useBanners') this._clientRenderer.invalidateAll();
    }

    // Tell 이름이 `SetIslands(0|1)` 이면 섬 테마 설정을 바꾸고 월드를 다시 그리며, 그 밖에는 Tell 창을 연다. 메뉴·표시 키가 같이 쓴다. (2026-11-20 WB04-E1087)
    _tellOrSetIslands(section) {
        const islands = /^SetIslands\((\d)\)$/.exec(section);
        if (islands) { this._gameData.set('theammode', Number(islands[1])); this._clientKencloud.worldRedraw().value = 1; this._displayDirty = true; } else this._openTell(section);
    }

    // 표시 키 분배기가 읽고 쓰는 현재 표시 전역(원문 0059a8ac/0059a8b0)이다.
    _viewKeyGlobals() {
        const game = this._clientGameplay, redraw = this._clientKencloud.worldRedraw();
        return { get hideBuildings() { return game.hideBuildings().value; }, set hideBuildings(value) { game.hideBuildings().value = value >>> 0; }, get redraw() { return redraw.value; }, set redraw(value) { redraw.value = value >>> 0; } };
    }

    // 표시 키 분배기의 실제 장치다. F7 은 소유자 색표 저장소의 useBanners 를 뒤집고(`_applyCampaignConfig`) 월드를 다시 그린다. (2026-11-20 WB04-E1090)
    _viewKeyHooks() {
        const game = this._clientGameplay, modes = this._surfaceStorage.ownerMode;
        return { paused: () => this._gameClock.paused(), readInt: (key, holder) => { const found = this._gameData.readInt(key, holder.value); holder.value = found.value; return found.changed || this._gameData.get(key) != null; },
            tell: (section) => this._tellOrSetIslands(section), queue: (command, first, second) => this._clientInputQueue.pushCommand(command, first, second),
            useBanners: () => this._clientColorSort.ownerTables.enabled, writeBanners: (value) => this._applyCampaignConfig('useBanners', value),
            fort: () => Boolean(this._campaignWorld?.session().core().bridgeState.mission), metamap: () => Boolean(modes.challenge), editor: () => Boolean(game.world.editor), highlight: (mode) => this._campaignWorld.highlight(mode) };
    }

    // 원본 생산 패널 idle 순서로 현재 내용물과 재고·다리 공급을 갱신한다.
    _runCampaignProductionFrame() {
        const production = this._campaignProduction, world = this._campaignWorld, globals = this._clientProductionGlobals;
        if (!production || !world || world.connecting || production.session !== world.session() || !globals || !this._gameData || !this._surfaceStorage) throw new Error('미션 생산 프레임의 현재 수명 오류');
        globals.stockEnabled = this._gameData.readInt('useProductTimers', 1).value >>> 0; globals.bridgeType = this._surfaceStorage.links.bridgeType;
        production.session.updateProductionContents(); production.maintenance.run();
    }

    // 생산 패널의 원본 슬롯 명령을 현재 기술 내용물과 실제 커서에 전달한다.
    _dispatchCampaignBridgeProduction(event) {
        const world = this._campaignWorld, production = this._campaignProduction, names = this._clientSoundNames, sound = this._clientSoundPlayer;
        if (!world || !production || production.session !== world.session() || !names || !sound) throw new Error('미션 다리 생산의 실제 수명 누락');
        const session = production.session, context = session.productionContext(), owner = context.gameplay.player.templateOwner;
        if (!context.gameplay.player.playerKnown) throw new Error('생산의 명시 PlayerKnown 진단');
        const records = context.players.records(), at = owner * 180 + 0x9c;
        if (!Number.isInteger(owner) || owner < 0 || owner >= 9 || at + 4 > records.length) throw new RangeError('생산의 실제 Player 기술 필드 없음');
        const technology = new DataView(records.buffer, records.byteOffset + at, 4).getUint32(0, true);
        if (technology > 0xffff) throw new RangeError('생산의 기술 SID 범위');
        const finder = new RawContainedFinder(context.storage.pool, session.productionFinder());
        // 실제 기술 내용물의 원본 순서와 슬롯 BYTE를 그대로 사용한다.
        for (let sid = finder.begin(technology); sid; sid = finder.next()) {
            const raw = context.storage.pool.slot(sid);
            if (((raw[20] | (raw[21] << 8)) & 0x40) && raw[23] === (event.x >>> 0)) {
                session.cursor().take(sid, 0); sound.play(names.lookup('getPiece.wav'), 0, 0, 0, 0, 0); return true;
            }
        }
        return false;
    }

    // 같은 Player·월드 전역에 원본 홈·사제 보기 메뉴 명령을 분배한다.
    _dispatchCampaignViewAction(event) {
        const game = this._clientGameplay, storage = this._surfaceStorage, world = this._campaignWorld, players = this._clientPlayers;
        if (!game || !storage || !world || !players || !this._clientKencloud) throw new Error('미션 ViewHome의 현재 장치 수명 누락');
        const read = (address) => {
            if (address === 0x00595344) return game.player.playerKnown;
            if (address === 0x00594fc8) return Number(storage.ownerMode.challenge);
            if (address === 0x00540c70) return game.player.battleOwner;
            throw new Error('미션 ViewHome의 미연결 전역');
        };
        const actions = new GameMenuActions({ read, hideBuildings: game.hideBuildings(), redraw: this._clientKencloud.worldRedraw() }, {
            command: (address) => gameMenuCommandValue(address),
            player: (owner) => { if (owner >= 9) throw new RangeError('미션 ViewHome Player 범위'); return players.records().subarray(owner * 180, (owner + 1) * 180); },
            centerObject: (sid) => world.centerOnObject(sid), viewPriest: (owner) => world.viewYourPriest(owner),
            assertion: (line) => console.error(`미션 ViewHome 원본 진단: ${line}`),
        });
        actions.handle(event.code, event.x, event.y);
    }

    // TAB(최근 건설 위치 순환)·U(최근 손실 위치로 이동)·숫자 키(저장한 화면 위치 호출)·Shift+숫자(현재 화면 중심 저장)를 원본 UberGump 의 순서대로 현재 카메라 전역에 전달한다(cpppj UberGump.cpp 1740~1757행).
    // 이력·저장 칸·손실 좌표는 Client 장기 전역이며 미션이 바뀌어도 이어진다. 정지 중이면 소비만 하고, 저장은 선택음 뒤의 현재 지도 중심을 쓴다. 카메라 이동은 미니맵 클릭과 같은 중심 요청이다. (2026-11-21 WB04-E1154: 세 분배기가 앱에 연결돼 있지 않아 TAB·U·숫자 키가 동작하지 않았다)
    _dispatchCampaignCameraKey(event) {
        const game = this._clientGameplay, world = this._campaignWorld, camera = game.camera();
        const paused = () => this._gameClock.paused(), cells = () => game.worldCells().value;
        const center = (x, y) => { world.centerCamera(x, y); this._clientRenderer.invalidateAll(); };
        const lost = game.gameAux().lostPosition();
        if (dispatchConstructionHistoryKey(event, camera.constructionHistory(), cells, { paused, center })) return true;
        if (dispatchLostPositionKey(event, () => lost[0], () => lost[1], cells, { paused, center })) return true;
        return dispatchSavedCameraKey(event, camera.savedPositions(), {
            paused, center, language: () => (originalLanguageNumber(this._gameData?.language ?? this.options.language)),
            sound: () => { this._clientSoundPlayer.play(this._clientSoundNames.lookup('select.wav'), 0, 0, 0, 0, 0); },
            // 선택음 뒤의 현재 지도 영역과 렌더러 시점으로 지도 중심(칸 좌표)을 구한다. 칸 크기는 카메라 상태의 정수 칸 픽셀(+8·+12)을 float 으로 본 값이다.
            capture: () => {
                const view = camera.rendererView(), map = world.mapView(), bits = new DataView(new ArrayBuffer(4)), cellBits = (offset) => { bits.setFloat32(0, view.data.getInt32(offset, true), true); return bits.getUint32(0, true); };
                return captureSavedCameraCenter({ pixels: [view.pixels.x, view.pixels.y], cellFloatBits: [cellBits(8), cellBits(12)] }, [map.left, map.top, map.right, map.bottom]);
            },
        });
    }

    // 현재 포인터 폴링과 지원된 큐 사건을 Guide→Gump→World 순서로 처리한다.
    _runCampaignInputFrame() {
        // DOM 모달 동안 기존 큐는 보류한다. 패널의 입력을 가짜 Gump 토큰으로 전달하지 않는다.
        if (this._modPanel) return;
        const world = this._campaignWorld, queue = this._clientInputQueue, tutorial = this._clientGuide?.current()?.runtime.tutorial, gump = this._clientGump, user = this._campaignUserInput;
        if (!world || !queue || !tutorial || !gump || !user) throw new Error('미션 입력 프레임의 현재 장치 수명 누락');
        // 대본 `<$ViewSpot>` 이 맡겨 둔 시작 카메라 위치를 월드가 도는 첫 프레임에 적용한다. (2026-11-22 WB04-E1190)
        this._applyPendingViewSpot();
        const mouse = this._prepareClientPointer(originalLanguageNumber(this._gameData?.language ?? this.options.language));
        tutorial.event(mouse);
        this._campaignCameraDevice().pollEdgeScroll(mouse);
        const dialog = Boolean(this._mainMenu?.dialog), cursorActive = world.session().cursorDropSettings().lockDepth === 0;
        const pointer = gump.notifyPointer(mouse, world.inputMode(), cursorActive, !dialog);
        if (pointer.overlay === 0) this._campaignCursor = CURSOR_NORMAL;
        if (!dialog) user.updateCursor(mouse);
        // 원본 4d6b54: Alt 또는 가운데 버튼을 누른 동안 포인터 쪽으로 카메라를 옮기고, 이 프레임의 큐 사건은 일반 입력으로 나누지 않고 버린다. (2026-11-21 WB04-E1145)
        const cameraPan = this._campaignCameraDevice().pollCameraPan(mouse);
        // 원본 큐의 순서를 지키며 미구현 사건 앞을 추월하지 않는다.
        for (;;) {
            if (this._modPanel) return;
            const next = queue.peek(); if (!next) break;
            if (cameraPan) { queue.pop(false); continue; }
            // Send X 숫자 창 같은 Gump 모달이 열려 있으면 모든 사건이 그 창으로만 가고 월드·단축키로는 가지 않는다(원본의 모달 dialog 우선 전달). (2026-11-22 WB04-E1168)
            if (this._campaignInputDialogOpen()) { const modal = queue.pop(false); gump.input().route(modal.code, modal.x, modal.y); this._displayDirty = true; continue; }
            const code = (next.code & 0x03ffffff) >>> 0, release = Boolean(next.code & InputCode.release);
            // 채팅 입력줄의 키는 편집기로 보내되 ESC와 F3는 일반 분배기로 보낸다. F3는 입력줄이 보여도 채팅창을 다시 닫을 수 있어야 한다. (2026-11-28 WB04-E1486)
            if (!(next.code & InputCode.command) && code !== InputCode.leftButton && code !== InputCode.rightButton && code !== InputCode.middleButton && code !== 0x1b && code !== 0x720000 && this._campaignChatInputActive()) {
                const typed = queue.pop(false); gump.input().route(typed.code, typed.x, typed.y); this._displayDirty = true; continue;
            }
            // 선택 키: P/R(사제)·N(다음 운반체)·H(집으로 점프, 원본 도움말 "F4 or H Jump to your Home Temple"). H 가 빠져 있어 F4 만 집으로 점프했다. (2026-11-21 WB04-E1146)
            // 객체 가상 6c 로 가는 명령: 신전 세 명령·시나리오 객체 다섯 명령(MakeMine·InflictDamage·DestroyPiece·TurnToObelisk·Meltdown)·DetonateBomb. 큐에서 소비하지 못하면 이후 입력이 막힌다. (2026-11-21 WB04-E1150)
            const rotationKey = code === 0x43 || code === 0x63;
            // 카메라 키: TAB(9)·U(0x55/0x75)·저장 화면 위치 숫자/기호. 수정키는 `next.code` 의 높은 비트이며 `code` 에는 없다. (2026-11-21 WB04-E1154)
            const cameraKey = code === 9 || code === 0x55 || code === 0x75 || SAVED_CAMERA_CODES.has(code);
            const objectCommand = VORTEX_COMMANDS.includes(code) || isScenarioObjectCommand(code) || code === DETONATE_BOMB_COMMAND;
            const selectionKey = code === 0x50 || code === 0x70 || code === 0x52 || code === 0x72 || code === 0x4e || code === 0x6e || code === 0x48 || code === 0x68;
            const productionKey = 'qwaszyxed'.includes(String.fromCharCode(code).toLowerCase()) && code < 256;
            // 방향키(←0x25 ↑0x26 →0x27 ↓0x28): 활성 조각의 ←/→ 는 회전, 그 밖에는 카메라 이동이다(11.01, 아래 분기). (2026-11-23 WB04-E1291: 이전에는 ←/→ 만 조각을 든 때 회전으로 소비하고 ↑/↓ 는 미구현 사건으로 버려졌다)
            const cursorCancel = code === 0x1b, arrowKey = ARROW_CAMERA_STEPS.has(code), gameTimeKey = code === 0x54 || code === 0x74;
            const unsupported = (code !== 13 && !CHAT_HELPER_COMMANDS.has(code) && code !== 0x02000017 && code !== 0x0200005d && code !== 0x02000060 && code !== 0x03000080 && code !== 0x03000032 && code !== 0x03000063 && code !== 0x03000076 && code !== 0x0300011a && code !== 0x030000de && code !== WORLD_DESTINATION_SELECTED_COMMAND && code !== PRODUCTION_BRIDGE_COMMAND && code !== PRODUCTION_REPEAT_COMMAND && code !== MISSION_F4 && code !== MISSION_F5 && code !== MISSION_F6 && code !== MISSION_VIEW_HOME && code !== MISSION_VIEW_PRIEST && !MISSION_VIEW_KEYS.has(code) && code !== MISSION_F9 && !TUTORIAL_NOTICE_COMMANDS.has(code) && !isGameMenuCommand(code) && !isSendMoneyCommand(code) && !objectCommand && !rotationKey && !cameraKey && code !== PAUSE_KEY_CODE && !selectionKey && !productionKey && !cursorCancel && !arrowKey && !gameTimeKey && code !== InputCode.leftButton && code !== InputCode.rightButton);
            if (unsupported) {
                // 미구현 사건 앞을 추월하지 않는다. 다만 처리하지 못하는 사건이 3초(벽 시각) 넘게 큐 머리에 남으면 이후 모든 입력이 영구히 막히므로 오류로 기록하고 버린다. (2026-11-21 WB04-E1150)
                const key = `${next.code}:${next.x}:${next.y}`, wall = this._gameClock.wallSeconds();
                if (this._stalledInput?.key !== key) this._stalledInput = { key, since: wall };
                else if (wall - this._stalledInput.since > 3) { console.error(`미구현 입력 사건을 버립니다: 0x${(next.code >>> 0).toString(16)} (${next.x}, ${next.y})`); queue.pop(false); this._stalledInput = null; continue; }
                break;
            }
            this._stalledInput = null;
            const event = queue.pop(false);
            // 11.01 방향키(cpppj UberGump::Event): 대화창·도움말·메뉴·팝업이 없을 때만 세계 입력으로 먼저 보낸다(활성 조각 ←/→ 회전, 그 밖은 카메라 32픽셀). 입력 UI 가 열려 있으면 Gump 로만 보내고 세계 꼬리로는 다시 보내지 않는다.
            // 채팅 입력줄·Send X 입력 창이 열린 경우는 이 위쪽 분기에서 이미 입력줄로 갔다. (2026-11-23 WB04-E1291)
            if (arrowKey) {
                if (tutorial.event(event)) continue;
                if (this._campaignArrowKeyBlocked()) { gump.input().route(event.code, event.x, event.y); this._displayDirty = true; continue; }
                user.event(event); continue;
            }
            if (tutorial.event(event) || gump.input().route(event.code, event.x, event.y)) continue;
            // 원본 UberGump(UI 밖 누름): Gump 가 소비하지 않은 좌·우 버튼 누름은 열린 팝업(우클릭 객체 메뉴 등)을 먼저 닫고 같은 사건을 월드로 진행한다.
            // 이 닫기가 없으면 메뉴를 연 채 바깥을 클릭해도 메뉴가 남았다. (2026-11-21 WB04-E1135, 실제 미션 우클릭 시험에서 확인)
            if (!release && (code === InputCode.leftButton || code === InputCode.rightButton) && !this._mainMenu?.dialog) this._closeCampaignPopups();
            if (code === InputCode.leftButton && !release && !this._mainMenu?.dialog) user.event(event);
            // 원본 UberGump: 지식 창이 열려 있으면 ESC 는 팝업만 닫는다. (2026-11-20 WB04-E1088)
            else if (cursorCancel && !release && !this._mainMenu?.dialog && this._campaignProduction?.ui?.knowledge()?.isOpen()) { this._campaignProduction.ui.knowledge().close(); this._displayDirty = true; }
            // 원본 ESC: 선택을 먼저 정리하고 조각을 들고 있으면(커서 잠금 0) 취소, 아니면 가로 게임 메뉴막대를 열거나 닫는다. (2026-11-20 WB04-E1084)
            else if (cursorCancel && !release && !this._mainMenu?.dialog && world.session().cursorDropSettings().lockDepth !== 0) { world.resetSelection(); this._toggleCampaignGameMenu(); }
            else if ((code === MISSION_F4 || code === MISSION_F5 || code === MISSION_F6 || selectionKey || productionKey || cursorCancel || code === InputCode.rightButton) && !release && !this._mainMenu?.dialog) {
                const picked = user.event(event);
                if (code === InputCode.rightButton && picked) {
                    const ui = this._campaignProduction?.ui; if (ui && !ui.openPriest(picked, event) && !ui.openFactory(picked, event)) ui.openObject(picked, event);
                }
            }
            else if (code === 0x02000017 && !release && !this._mainMenu?.dialog) {
                const type = event.y >>> 0, types = world.session().core().types, staticTopic = this._clientChatWindows.strings.get(type);
                if (staticTopic !== undefined) { this._closeCampaignPopups(); this._openHelp(staticTopic); continue; }
                if (!type || type >= 256 || type >= types.length) throw new RangeError('생산 정보 Help TYPE 범위 오류');
                const display = this._helpTypeDisplay(type);
                this._closeCampaignPopups(); this._openHelp(types[type].name.split('\0', 1)[0] + this._resources.translate('Type'), display.information, display.picture);
            }
            // 채팅 도우미 명령 중 채팅 창이 처리하지 않은 것: 설정 대입은 `키=값` 문자열(둘째 인자가 채팅 문자열 토큰)을 설정에 쓰고, 서식 편집(Edit Back/Front Format)은 웹에서 아직 대화창이 없어 안내만 남긴다. (2026-11-22 WB04-E1177)
            else if (CHAT_HELPER_COMMANDS.has(code) && !release) {
                if (code === 0x0200006d) { const assignment = this._clientChatWindows.strings.get(event.y >>> 0); if (assignment === undefined) throw new Error('채팅 설정 대입의 문자열 토큰 없음'); applyScriptConfig(assignment, (key, value) => this._applyCampaignConfig(key, value)); this._displayDirty = true; }
                // Edit Back Format(0x02000105)·Edit Front Format(0x02000106)은 원본 서식 입력 창을 연다. (2026-11-22 WB04-E1179)
                else if (code === 0x02000105 || code === 0x02000106) this._openChatFormat(code === 0x02000106);
            }
            // 문자 Enter: 채팅 입력줄이 보이지 않으면 입력줄을 보이고 채팅 구분선을 올린다(원본 `OpenChatInput`). 이미 보이면 입력줄이 먼저 처리한다. (2026-11-22 WB04-E1170)
            else if (code === 13 && !release && !this._mainMenu?.dialog) { const chatWindow = this._campaignProduction?.ui?.chat(); if (chatWindow) { dispatchChatInputKey(event, () => (chatWindow.inputVisible() ? 1 : 0), () => chatWindow.setInput(true), () => chatWindow.open()); this._displayDirty = true; } }
            else if (code === SALVAGE_COMMAND && !release && !this._mainMenu?.dialog) world.session().requestSalvage(event.y >>> 0);
            else if (code === 0x02000060 && !release && !this._mainMenu?.dialog) {
                // 원본 packed 인자의 상위 WORD는 TYPE, 하위 WORD는 사제 SID다.
                const packed = event.y >>> 0; world.session().makePriestMenu({ tell: (name) => this._openTell(name) }).selectBuild(packed >>> 16, packed & 0xffff);
            }
            else if ((code === 0x03000080 || code === 0x03000032 || code === 0x03000063) && !release && !this._mainMenu?.dialog) {
                // Factory 연구·업그레이드·순환(0x03000063)은 같은 packed 인자와 실제 Session 명령으로 전달한다. 순환 명령이 허용 목록에 없어 3초 뒤 버려지던 것을 고쳤다. (2026-11-22 WB04-E1225)
                const packed = event.y >>> 0; world.session().commandFactory(packed & 0xffff, event.code, packed);
            }
            else if ((code === 0x0300011a || code === 0x03000076) && !release && !this._mainMenu?.dialog) {
                // 운반체 메뉴의 Cast(0x0300011a)·Drop(0x03000076): 원본 42aa50 은 이벤트의 y 를 내용물 검색 부모로 읽고 현재 메뉴의 부모도 같은 SID 이다. (2026-11-22 WB04-E1225)
                const parent = event.y & 0xffff; world.session().commandCarrierDrop(parent, event.code, parent);
            }
            else if (code === 0x030000de && !release && !this._mainMenu?.dialog) {
                // 사제 기도(0x030000de): 원본 4940b0 과 같은 packed y 로 기도 요청을 보낸다(하위 WORD=사제 SID, 상위 WORD=희망 종류). (2026-11-22 WB04-E1225)
                const packed = event.y >>> 0; world.session().commandPriest(packed & 0xffff, event.code, packed);
            }
            else if (code === PRODUCTION_BRIDGE_COMMAND && !release && !this._mainMenu?.dialog) this._dispatchCampaignBridgeProduction(event);
            else if (code === PAUSE_KEY_CODE) {
                // 원본 Pause 가상 키: 현재 일반 Pause 명령을 큐에 예약한다(대화창이 열려 있으면 처리하지 않는다). 예약된 명령은 게임 메뉴 Pause 와 같은 경로로 시계를 멈추거나 푼다. (2026-11-21 WB04-E1163)
                if (!release && !this._mainMenu?.dialog) dispatchPauseKeyInput(event, { command: () => PAUSE_COMMAND, queue: (command, first, second) => { this._clientInputQueue.pushCommand(command, first, second); } });
            }
            else if (cameraKey) {
                // 대화창이 열려 있으면 처리하지 않는다. 키 뗌은 분배기가 소비하지 않고 큐에서는 그대로 버린다.
                if (!release && !this._mainMenu?.dialog) { this._dispatchCampaignCameraKey(event); this._displayDirty = true; }
            }
            else if (rotationKey) {
                // 원본 C/c 키: 회전 방향을 반전해 설정(rotmode)에 기록한다(정지 중이면 소비만). 대화창이 열려 있으면 처리하지 않는다. (2026-11-21 WB04-E1151)
                if (!this._mainMenu?.dialog) {
                    const rotation = this._clientGameplay.cursorGlobals().rotationMode();
                    dispatchRotationKeyInput(event, { get mode() { return rotation.value >>> 0; }, set mode(value) { rotation.value = value >>> 0; } },
                        { paused: () => this._gameClock.paused(), configure: (assignment) => applyScriptConfig(assignment, (key, value) => this._applyCampaignConfig(key, value)) });
                    this._displayDirty = true;
                }
            }
            else if (objectCommand) {
                // 원본 UberGump 의 일반 입력 순서(신전 → 시나리오 객체 → Detonate)로 현재 객체의 가상 6c 에 전달한다. 반환값은 다음 입력에 쓰지 않는다.
                const session = world.session();
                dispatchVortexCommandInput(event, (target, command, parameter) => { session.commandObject(target, command, parameter); })
                    || dispatchScenarioObjectInput(event, (target, command) => { session.commandObject(target, command); })
                    || dispatchDetonateInput(event, (target, command) => { session.commandObject(target, command); });
                this._displayDirty = true;
            }
            else if (code === MISSION_VIEW_HOME || code === MISSION_VIEW_PRIEST) this._dispatchCampaignViewAction(event);
            // T/t: 게임 시간 표시 켜기·끄기다. 정지 중에는 바꾸지 않고(원본 정지 관문) 안내는 Kernel 프레임 뒤에 만든다. (2026-11-20 WB04-E1097)
            else if (gameTimeKey) { if (!release && !this._mainMenu?.dialog) dispatchGameTimeKey(event, this._clientGameplay.showGameTime(), () => this._gameClock.paused()); }
            // F2·Shift-F3·F8 은 표시 키 분배기, F9 는 플레이어 목록 명령 예약이다. 뗌은 소비만 한다. (2026-11-20 WB04-E1087)
            else if (MISSION_VIEW_KEYS.has(code) || code === MISSION_F9) { if (!release && !this._mainMenu?.dialog) { if (code === MISSION_F9) queuePlayerListKey(event, this._clientInputQueue); else if (dispatchScreenshotKey(event, () => this._saveScreenshot(), (section) => { this._openTell(section); })) { /* Shift-F7 은 화면 저장만 하고 일반 F7 표식 토글로 가지 않는다. */ } else dispatchViewKeyInput(event, this._viewKeyGlobals(), this._viewKeyHooks()); } }
            else if (isGameMenuCommand(code) && !release && !this._mainMenu?.dialog) { if (this._dispatchCampaignMenuCommand(event)) break; }
            // Player 메뉴·안내 창의 송금 명령(고정 300/700/1000·All·X)이다. 뗌과 열린 창 뒤의 입력은 소비만 한다. (2026-11-20 WB04-E1126)
            else if (isSendMoneyCommand(code)) { if (!release && !this._mainMenu?.dialog) this._dispatchCampaignSendMoney(event); }
        }
    }

    // 같은 로그인 Session과 Start에 현재 안내·시계·입력·선택 장치를 연결한다.
    _prepareCampaignTutorial() {
        const world = this._campaignWorld, current = this._clientGuide?.current(), gump = this._clientGump, clock = this._gameClock;
        if (!world || !current?.runtime.guided || !current.runtime.start || current.runtime.tutorial || !gump || !clock || !world.session().localConnectionReady() || this._campaignWorldMissionToken !== current.runtime.saveToken) throw new Error('Tutorial의 현재 World·Guide·Start 수명 오류');
        const session = world.session(), runtime = current.runtime;
        // 미션 프레임 관문이 연 안내 창은 원본처럼 Briefing 단계로 열어 ESC 가 닫지 않는다(원본 frameEffects.tell → TryTell(section, true)). (2026-11-23 WB04-E1264)
        const tell = (section) => current.script.section(section) !== null && this._openTell(section, { briefing: true });
        const effects = {
            frame: { gameSeconds: () => clock.time().game, paused: () => clock.paused(), dialogsClear: () => !this._mainMenu?.dialog,
                knowledgeOpen: () => session.cursorDropSettings().lockDepth === 0,
                guideOpen: () => Boolean(this._clientHtml.window.instances || this._mainMenu?.dialog), blockingGumpOpen: () => gump.blockingPopups(),
                endDemo: () => this._clientInputQueue.push(MISSION_RELOAD_COMMAND, 0, 0), pause: () => clock.pushGamePause(), resume: () => clock.popGamePause() },
            tell, closeTransient: () => this._closeCampaignPopups(), selected: () => world.selected(),
            resetSelection: () => world.resetSelection(), clearSelection: () => world.clearSelection(),
        };
        const tutorial = new RawCampaignTutorialOneBindings(session, current.script, runtime.start, runtime.gate, runtime.state, effects);
        runtime.tutorial = tutorial; return tutorial;
    }

    // 현재 미션 표면을 준비하고 같은 Client Kernel을 한 번 실행한 뒤 자산 보기를 갱신한다.
    _runCampaignKernelFrame() {
        const world = this._campaignWorld, kernel = this._clientKernel;
        if (!world || !kernel || world.connecting || world.session().connectionCallbackActive()) throw new Error('미션 Kernel 프레임의 현재 World·Kernel 수명 오류');
        // 원본 주 루프의 PollBattleResources(614)는 Kernel 뒤에 간헐천 보충·스펠 소비를 한다. 웹의 finishKernelFrame 은 객체 보기를 다시 읽는 일이라 새로 생긴 간헐천이 같은 프레임에 보이도록 그 앞에 둔다. (2026-11-22 WB04-E1232)
        world.prepareKernelFrame(); kernel.runFrame(); world.runBattleResources(); world.finishKernelFrame();
        // 원본 `00439b43..64`: 전투 모드이고 T 키로 켠 게임 시간 표시 값이 있으면 매 프레임 Game Time 안내(종류 16, 0.1초 기한)를 같은 20칸 표에 요청한다. (2026-11-20 WB04-E1097)
        if (this._surfaceStorage.ownerMode.battle && this._clientGameplay.showGameTime().value) this._campaignProduction?.ui?.warnings()?.gameTimeStatus();
    }

    // 현재 미션의 실제 화면·구름·카메라·Gump에 원본 Renderer의 표시 전 준비를 연결한다.
    _prepareCampaignRendererFrame() {
        const world = this._campaignWorld, screen = this._clientPaletteScreen, renderer = this._clientRenderer;
        const globals = this._clientRendererFrame, clouds = this._clientKencloud, game = this._clientGameplay;
        const gump = this._clientGump, data = this._gameData, random = this._clientRandom?.game;
        if (!world || world.connecting || !screen || !renderer || !globals || !clouds || !game || !gump || !data || !random || !this._layout) throw new Error('미션 Renderer 준비의 현재 장치 수명 오류');
        const camera = game.camera(), snapshot = camera.snapshot(), view = world.mapView(), queue = globals.cloudQueue();
        const box = (value) => ({ value });
        const denominator = { get value() { return camera.denominator().current; } };
        const cloudDraw = (rect, x, y, alternate) => clouds.draw(screen, [rect.left, rect.top, rect.right, rect.bottom, x, y], data.files, data, random, alternate);
        prepareRendererFrame({ scan: globals.scan(), backgroundRedraw: renderer.backgroundRedraw, dirtyCount: box(renderer.cameraDirtyEntries().length), drawCounter: globals.cloudDirtyCount() },
            { mode: { get value() { return screen.flags; } }, force: clouds.worldRedraw(), width: box(screen.width), height: box(screen.height),
                initialHeight: box(this._layout.logicalHeight), cameraY: { get value() { return snapshot.pixels.y; } },
                left: box(view.left), top: box(view.top), right: box(view.right), bottom: box(view.bottom) }, {
                markCloudDirty: () => clouds.markDirty(),
                queueCloudDirty: (rect) => appendRendererCloudDirty(queue, rect, game.aiDebug().checking().value, { assertion: (line) => console.error(`Renderer 구름 사각형 원본 진단: ${line}`) }),
                overrideActive: () => gump.createState().collection.overrideHead,
                updateClouds: () => clouds.update(data.files, data, random), rowDirty: (row, y) => clouds.rowDirty(row >>> 0, y >>> 0),
                invalidate: (rect, flags) => renderer.invalidate(rect, flags), clearDirty: () => renderer.clearDirty(),
                invalidateBackground: () => refreshRendererCloudBackground(queue, { get value() { return snapshot.pixels.x; } }, { get value() { return snapshot.pixels.y; } }, denominator, { draw: cloudDraw }),
            });
    }

    // 미션/State/Client 반납 전에 실제 표시 줄과 장기 루트 참조를 함께 정리한다.
    _clearCampaignTutorialObjective() {
        if (!this._campaignTutorialObjective) return;
        this._campaignTutorialObjective.clear(); this._campaignTutorialObjective = null;
        this._clientRenderer?.invalidateAll();
    }

    // 현재 실제 gate를 읽어 창이 없는 지도에만 안내를 제출한다. 진행/예약/승패 판정은 호출하지 않는다.
    _updateCampaignTutorialObjective() {
        const root = this._clientGump, current = this._clientGuide?.current(), world = this._campaignWorld;
        if (!root) { if (this._campaignTutorialObjective) throw new Error('튜토리얼 안내의 Client 루트 반납 순서 오류'); return; }
        const ui = this._campaignProduction?.ui; let key = '';
        if (!this.options.originalTutorialPresentation && this._campaignDisplayActive() && current?.runtime.guided
            && current.runtime.saveToken === this._campaignWorldMissionToken && !this._mainMenu?.dialog && !root.lifetimeState().currentDialog
            && !this._clientHtml?.window.instances && !ui?.knowledge()?.isOpen() && !ui?.gameMenu()?.isOpen()) {
            key = tutorialObjectiveKey(current.script.name, current.runtime.gate, current.runtime.mode64);
        }
        if (!key && !this._campaignTutorialObjective) return;
        if (!this._campaignTutorialObjective) this._campaignTutorialObjective = new TutorialObjectiveGump(root, this._resources.fonts);
        const colors = this._clientPaletteScreen.colors();
        if (this._campaignTutorialObjective.sync(key ? this._resources.translate(key) : '', world?.mapView() ?? root.mapView(), colors.basic[7], colors.basic[0])) this._clientRenderer.invalidateAll();
    }

    // 현재 미션의 준비·DIB 합성·구름 정리·표시를 원본 순서로 한 번 실행한다.
    _drawCampaignRendererFrame() {
        const world = this._campaignWorld, screen = this._clientPaletteScreen, renderer = this._clientRenderer;
        const clouds = this._clientKencloud, game = this._clientGameplay, data = this._gameData, gump = this._clientGump;
        if (!world || world.connecting || !screen || !renderer || !clouds || !game || !data || !gump || !this._clientRendererFrame || !this._clientRandom) throw new Error('미션 Renderer 출력의 현재 장치 수명 오류');
        this._updateCampaignTutorialObjective();
        const camera = game.camera();
        const drawing = new CampaignRendererDrawing({ world, screen, camera: camera.snapshot(), globals: this._clientRendererFrame,
            clouds, files: data.files, config: data, random: this._clientRandom.game, denominator: camera.denominator(), clearOutsideMap: true,
            tint: () => { throw new Error('미션 날씨색 장치 미연결'); }, hideBuildings: () => game.hideBuildings().value,
            uiCollect: (view, output, limit) => gump.collection().collect(view, output, limit),
            drawUi: (token, pixels, pitch, height, clip) => {
                const state = new GumpDrawState(); setGumpDrawClip(state, clip);
                gump.draw(token, state, new GumpPixelTarget(pixels, pitch, height, state));
            } });
        this._prepareCampaignRendererFrame(); screen.setClip(0, 0, screen.width, screen.height);
        const pixels = screen.lock();
        try { renderer.draw(pixels, screen.pitch, 0, drawing); } finally { screen.unlock(); }
        clouds.cleanDirty(); renderer.present((rect) => screen.update(rect)); clouds.worldRedraw().value = 0;
    }

    // 준비된 미션이 현재 메뉴 장면을 대신 표시하는지 확인한다.
    _campaignDisplayActive() {
        const world = this._campaignWorld;
        return this._state === AppState.MAIN_MENU && Boolean(world && !world.connecting && !world.session().connectionCallbackActive());
    }

    // 게임을 정리한다. 프레임 예약·이벤트·관찰자·오디오·진행 중 로딩을 모두 해제하고 Canvas 를 제거한다.
    // 여러 번 호출해도 안전하며, 이후 init 으로 다시 시작할 수 있다. 처음 골격의 destroy(div) 형태로 불러도 인자는 무시한다.
    // init 도중 실패한 반쪽 상태도 이 함수가 정리하므로 상태 이름이 아니라 루트 요소의 유무로 판단한다.
    destroy() {
        if (!this._root) return;
        this._settingsPanel?.close(); // 입력 중인 JSON과 DOM 사건이 이전 앱 수명을 붙잡지 않게 한다.
        this._closeModPanel(false);
        // 저장 검증과 늦은 쓰기를 먼저 취소한다. 비동기 종료는 World/Session 사본만 보유한다.
        this._modValidation?.abort(); this._modValidation = null;
        this._mods?.dispose().catch((error) => reportGameError('mod-dispose', error)); this._mods = null;
        // 진행 중인 로딩 결과가 정리 뒤에 상태를 바꾸지 못하게 세대를 올린다.
        this._loadGeneration += 1;
        this._stopFrameLoop();
        this._abort?.abort();
        this._abort = null;
        this._agentBridge?.dispose(); this._agentBridge = null;
        // 녹화 트랙·Blob 주소는 오디오 문맥을 닫기 시작하기 전에 같은 앱 수명에서 반납한다.
        this._agentHost?.dispose().catch((error) => reportGameError('webmcp-dispose', error, { state: this._state })); this._agentHost = null;
        this._resizeObserver?.disconnect();
        this._resizeObserver = null;
        this._mainMenu?.cancel();
        this._closeMissionLoading();
        this._mainMenu = null;
        this._terrainPreview = null;
        this._settings = null;
        this._progress = null;
        // Guide를 먼저 반납하고 장기 Process 소유자는 TYPE·Kernel과 시계가 살아 있는 동안 반납한다.
        this._clientGuide?.dispose(); this._clientGuide = null; this._clientFortName = null; this._clientFort = null;
        this._releaseCampaignWorld();
        this._clientGump?.dispose(); this._clientGump = null;
        this._clientConnections?.shutdownDevices();
        this._clientKencloud = null; this._clientHtml = null; this._clientConnections = null; this._clientConnectionActivity = null;
        this._surfaceStorage?.dispose();
        this._clientSoundDevice?.dispose(); this._clientSoundDevice = null; this._clientSoundPlayer = null; this._clientSoundState = null; this._clientSoundPrepare = null;
        this._gameClock = null; this._modTimeSource = null; this._clientClockPreparing = false; this._clientLocal = null; this._clientNsInfo = null; this._clientHelpPopup = null; this._hiddenGamePause = false; this._clientSoundNames = null;
        this._clientSp = null; this._clientRandom = null;
        this._clientChatRecords = null; this._clientChatChannels = null; this._clientChatWindows = null;
        this._clientChatManager = null; this._clientDotCommands = null; this._clientChatInitialized = false;
        this._clientPaletteScreen = null; this._clientColorSort = null; this._clientRenderer = null; this._clientSurfaceDisplay = null; this._clientRendererFrame = null;
        this._clientTemplates = null; this._clientPlayers = null; this._clientCoreData = null; this._clientGameplay = null; this._clientSubscriber = null;
        this._clientKernel = null; this._surfaceStorage = null; this._gameAssets = null; this._clientTrace = null; this._clientProductionGlobals = null; this._campaignCamera = null; this._clientMinimapState = null; this._clientMinimapGlobals = null;
        this._missionAudioAssets?.dispose(); this._missionAudioAssets = null;
        if (this._audio) {
            // 음악을 멈추고 문맥을 닫는다. 닫기에 실패해도 정리를 막지 않는다.
            this._audio.close();
            this._audio = null;
            this._audioContext = null;
        } else if (this._audioContext) {
            // 닫기에 실패해도 정리를 막지 않는다.
            Promise.resolve(this._audioContext.close?.()).catch(() => {});
            this._audioContext = null;
        }
        // 받은 음악 바이트는 재시작을 위해 남기고, 오디오 문맥에 묶인 해독·재생 상태만 되돌린다.
        if (this._musicStatus === 'decoding' || this._musicStatus === 'ready' || this._musicStatus === 'playing') this._musicStatus = 'loaded';
        this._presenter?.release();
        this._presenter = null;
        this._surface = null;
        if (this._savedTitle !== null && this._root.ownerDocument) this._root.ownerDocument.title = this._savedTitle;
        this._savedTitle = null;
        this._clientImeElement?.remove(); this._clientImeElement = null;
        this._canvas?.remove();
        this._restoreRootStyle();
        this._root = null;
        this._canvas = null;
        this._ctx = null;
        this._layout = null;
        this._input.keys.clear();
        this._setState(AppState.DESTROYED);
    }

    // 화면비 설정을 바꾼다. 'auto' 이거나 SCREEN_MODES 의 이름이어야 하며 그 밖의 값은 RangeError 다.
    // 시작 후에 부르면 즉시 배치를 다시 계산한다.
    setAspect(name) {
        if (name !== AUTO_ASPECT && !findScreenMode(name)) {
            throw new RangeError(`Netstorm.setAspect: 지원하지 않는 화면비입니다: ${name}`);
        }
        this._aspect = name;
        if (this._canvas) this._applyLayout();
    }

    // ----- 상태 -----

    // 앱 상태를 바꾸고 알림 훅을 부른다. 같은 상태로의 전환은 무시한다.
    // 호스트가 넘긴 훅이 예외를 던져도 상태 전환과 로딩 흐름이 깨지지 않도록 예외는 콘솔에만 남긴다.
    _setState(next) {
        if (next === this._state) return;
        const previous = this._state;
        this._state = next;
        try {
            this.options.onStateChange?.(next, previous);
        } catch (error) {
            console.error('Netstorm: onStateChange 훅에서 예외가 발생했습니다.', error);
        }
    }

    // 자산을 적재하고 추가 준비 훅을 실행한다. 성공하면 클릭 대기, 실패하면 오류 화면(클릭으로 재시도)이 된다.
    // 메인 메뉴 필수 묶음(menu)은 기다리고, 음악 묶음(menu-music)은 뒤에서 받는다. 이미 받은 자원은 재시도·재시작에서 다시 받지 않는다.
    // 오래된 세대의 결과는 버린다. 준비 단계에서는 오디오를 재생하지 않는다.
    async _load() {
        // 고정 최소 대기의 기준이 되는 첫 적재 시작 시각이다(이후 재적재에서는 이미 지났으므로 다시 기다리지 않는다). (2026-11-24 WB04-E1306)
        this._loadStartedAt ??= Date.now();
        this._loadGeneration += 1;
        const generation = this._loadGeneration;
        this._loadError = null;
        this._setState(AppState.LOADING);
        try {
            // 실제 관리자가 있을 때만 저장 검증을 기다린다. 없는 경로의 준비 훅 호출 순서는 유지한다.
            if (this._mods) { await this._mods.load(); if (generation !== this._loadGeneration) return; }
            // 같은Client의 SP/RNG 초기화 뒤 원본초경계와 최초Capture를 준비한다. 취소된이전앱의결과는 새수명에 적용하지 않는다.
            if (this._clientClockPreparing) {
                const clock = this._gameClock;
                const ready = await clock.prepareOrigin({ cancelled: () => generation !== this._loadGeneration || !this._root });
                if (!ready || generation !== this._loadGeneration || clock !== this._gameClock) return;
                this._clientClockPreparing = false;
            }
            if (!this._resources) {
                const loader = this._createAssetLoader();
                await loader.loadManifest();
                if (generation !== this._loadGeneration) return;
                // 선택 언어의 추가 묶음을 메뉴 뒤에 받으므로 진행률의 전체 크기에 미리 더해 둔다. (2026-10-31 WB03-N)
                const languageBundle = this.options.language === 'japanese' ? JAPANESE_BUNDLE : this.options.language === 'korean' ? KOREAN_BUNDLE : null;
                const languageTotal = languageBundle ? this._bundleBytes(loader, languageBundle) : 0;
                let menuTotal = 0;
                const files = await loader.loadBundle(MENU_BUNDLE, (loaded, total) => {
                    menuTotal = total;
                    if (generation === this._loadGeneration) this._loadProgress = { loaded, total: total + languageTotal };
                });
                if (generation !== this._loadGeneration) return;
                const languageSupport = this.options.language === 'japanese' ? await this._loadJapanese(loader, menuTotal, languageTotal, generation) : this.options.language === 'korean' ? await this._loadKorean(loader, menuTotal, languageTotal, generation) : null;
                if (generation !== this._loadGeneration) return;
                this._resources = buildMenuResources(files, languageSupport);
                // 설정 층에는 웹 사용자 설정(언어·음량)과 저장된 캠페인 진행 값을 원본 options.cfg 자리에 넣는다. 해상도·전체화면 값은 넣지 않는다.
                // 이후 게임 데이터에 쓰이는 값은 _onConfigWrite 가 설정·진행 저장소로 옮긴다. (2026-10-30 WB03-H: 진행 값과 쓰기 통지 연결)
                // 선택 언어 자료를 느슨한 파일로 합친다. 일본어 준비 실패는 제품 추가 문구도 영어로 대체한다. (2026-10-31 WB03-O)
                this._gameData = GameData.build({
                    files: languageSupport?.languageFiles?.size ? new Map([...files, ...languageSupport.languageFiles]) : files,
                    language: this.options.language === 'japanese' && !languageSupport ? 'english' : this.options.language,
                    options: { ...this._settings.snapshot(), ...this._progress.snapshot() },
                    onChange: (key, value) => this._onConfigWrite(key, value),
                    skirmish: this.options.skirmish,
                });
                this._assetLoader = loader;
                this._buttonSoundBytes = files.get(MENU_ASSET_PATHS.buttonSound) ?? null;
                this._menuSoundBytes = files.get(MENU_SOUND_PATH) ?? null;
                this._presenter?.setPalette(this._resources.palette.rgb);
                this._applyCursor();
                this._startMusicDownload(loader);
            }
            // 같은 자산이 재사용되는 재시작에서도 Client 장기 물리는 새로 구성한다.
            if (!this._surfaceStorage) {
                const palettePath = this._gameData.pathSpec('GamePalSpec', this._gameData.pathSpec('fortPal'));
                this._initializeClientWorldStorage(loadClientWorldAssets(this._gameData.files, palettePath), this._gameData.files);
            }
            this._syncClientCloudOptions(); this._syncClientHelpWord();
            if (!this._clientPaletteScreen) this._initializeClientPalette(this._gameAssets);
            if (!this._clientColorSort.mapsReady) this._initializeClientPaletteCache(this._gameAssets);
            this._syncClientOwnerColorOptions(); this._syncClientCursorOptions();
            if (!this._clientGump) this._initializeClientGump();
            if (!this._clientGuide) this._initializeClientGuide();
            await this.options.prepare(this);
        } catch (error) {
            if (generation !== this._loadGeneration) return;
            reportGameError('asset-loading', error, { state: this._state, generation });
            this._loadError = error;
            this._setState(AppState.ERROR);
            return;
        }
        if (generation !== this._loadGeneration) return;
        // 처음 방문(캐시 없음)에서 아직 받는 중인 자원(메뉴 음악·글꼴 적용)이 끝나기 전에 클릭하면 안 되므로 끝나거나 제한 시간이 지날 때까지 로딩 화면(100%)에 머문다. (2026-11-24 WB04-E1300)
        await this._awaitStartReadiness();
        if (generation !== this._loadGeneration) return;
        this._setState(AppState.CLICK_TO_START);
        if (this._resumeSettingsToMainMenu) { // 같은 문서에서 사용자가 설정을 적용한 뒤에는 메인 메뉴로 바로 복귀한다.
            this._resumeSettingsToMainMenu = false; this._unlockAudio(); this._installIntroStateEffects();
            this._firstMenuPending = false; this._enterMainMenu(); this._openTell('Blank');
        }
    }

    // '클릭하여 시작'을 열기 전에 시작 때 필요한 나머지 자원이 준비되기를 기다린다: 문서 글꼴 적재(`document.fonts.ready`)와 뒤에서 받는 메뉴 음악 묶음(성공이든 실패든 끝날 때까지).
    // 어느 쪽이 영영 끝나지 않아도 START_READY_TIMEOUT_MS 가 지나면 넘어간다(음악은 계속 받는다). 오류를 던지지 않는다. 기다릴 것이 없으면 바로 돌아온다.
    // 이전에는 메뉴·언어 묶음만 받으면 곧바로 클릭을 받아, 처음 방문에서 아직 받는 음악·글꼴과 엇갈리는 경합이 있었다. (2026-11-24 WB04-E1300, 사용자 요구)
    async _awaitStartReadiness() {
        const view = this._view(), pending = [];
        const fontsReady = this._root?.ownerDocument?.fonts?.ready;
        if (fontsReady && typeof fontsReady.then === 'function') pending.push(Promise.resolve(fontsReady).catch(() => {}));
        if (this._musicSettled && this._musicStatus === 'loading') pending.push(this._musicSettled);
        // 고정 최소 대기: 첫 적재 시작으로부터 startMinWaitMs 가 지날 때까지 남은 시간만큼 더 기다린다. 이미 지났거나 0 이면 기다리지 않는다. (2026-11-24 WB04-E1306)
        const floor = Math.max(0, this.options.startMinWaitMs - (Date.now() - (this._loadStartedAt ?? Date.now())));
        let floorTimer = null;
        if (floor > 0) pending.push(new Promise((resolve) => { floorTimer = view.setTimeout(resolve, floor); }));
        if (pending.length === 0) return;
        let timer = null;
        // 상한은 최소 대기보다 짧을 수 없다(최소 대기를 상한이 끊지 않는다).
        const timeout = new Promise((resolve) => { timer = view.setTimeout(resolve, Math.max(START_READY_TIMEOUT_MS, this.options.startMinWaitMs)); });
        await Promise.race([Promise.all(pending), timeout]);
        view.clearTimeout(timer); if (floorTimer !== null) view.clearTimeout(floorTimer);
    }

    // 매니페스트에서 묶음에 속한 파일의 전체 크기(바이트)를 구한다. 묶음이 없으면 0 이다. (2026-10-31 WB03-N)
    _bundleBytes(loader, bundle) {
        return loader.entries(bundle).reduce((sum, entry) => sum + entry.size, 0);
    }

    // 일본어 자료/실제CDN 글꼴을 현재 세대에 연결한다. 실패하면 원래 영어 자원으로 계속한다.
    async _loadJapanese(loader, menuTotal, languageTotal, generation = this._loadGeneration) {
        try {
            const files = await loader.loadBundle(JAPANESE_BUNDLE, (loaded) => {
                if (generation === this._loadGeneration) this._loadProgress = { loaded: menuTotal + loaded, total: menuTotal + languageTotal };
            });
            if (generation !== this._loadGeneration) return null;
            // 공용 번역의 실제 문자를 글꼴 선적재에 사용한다. 텍스트를 외부URL에 넣지 않는다.
            const text = [...files.values()].map((bytes) => new TextDecoder('utf-8').decode(bytes)).join('');
            const support = await buildJapaneseSupport(files, { createRasterizer: () => createJapaneseGlyphRasterizer({ document: this._root.ownerDocument, text }) });
            // 기다리는 동안 취소한 세대의 스타일시트는 현재 앱에 남기지 않는다.
            if (generation !== this._loadGeneration) { support.unicode.rasterizer.dispose(); return null; }
            return support;
        } catch (error) {
            if (generation === this._loadGeneration) reportGameError('japanese-assets', error, { state: this._state, generation }, 'Netstorm: 일본어 글꼴·번역표를 준비하지 못해 영어 메뉴로 계속합니다.');
            return null;
        }
    }

    // 현재 적재 세대에서 한국어 글꼴·번역을 준비하고, 실패하면 진단을 남기고 영어 자료로 대체한다.
    // 이전 세대의 응답과 진행률은 새 앱 수명에서 무시한다. (2026-11-19 WB04-E944)
    async _loadKorean(loader, menuTotal, koreanTotal, generation = this._loadGeneration) {
        try {
            const files = await loader.loadBundle(KOREAN_BUNDLE, (loaded) => {
                if (generation === this._loadGeneration) this._loadProgress = { loaded: menuTotal + loaded, total: menuTotal + koreanTotal };
            });
            if (generation !== this._loadGeneration) return null;
            const document = this._root.ownerDocument;
            return await buildKoreanSupport(files, { createRasterizer: ({ regular, bold }) => createCanvasGlyphRasterizer({ document, regular, bold }) });
        } catch (error) {
            if (generation === this._loadGeneration) {
                reportGameError('korean-assets', error, { state: this._state, generation },
                    'Netstorm: 한국어 글꼴·번역표를 준비하지 못해 영어 메뉴로 계속합니다.');
            }
            return null;
        }
    }

    // 데이터 폴더 URL 과 fetch 함수로 자산 적재기를 만든다. 데이터 폴더는 루트 문서의 주소를 기준으로 푼다. (2026-10-23 WB03-M1)
    _createAssetLoader() {
        const doc = this._root.ownerDocument;
        const view = this._view();
        const fetchFunction = this.options.fetch ?? (typeof view.fetch === 'function' ? view.fetch.bind(view) : null);
        if (!fetchFunction) throw new Error('이 브라우저에서는 fetch 를 쓸 수 없습니다.');
        return new AssetLoader({ baseUrl: new URL(this.options.assetBase, doc.baseURI), fetchFunction, ...(this.options.assetRetryDelays ? { retryDelays: this.options.assetRetryDelays } : {}) });
    }

    // 메뉴 음악 묶음을 뒤에서 받는다. 다 받으면 오디오가 준비된 경우 해독해 메인 메뉴에서 재생한다.
    // 실패해도 게임은 계속되며(원본도 음악 파일이 없으면 소리 없이 진행) 콘솔과 상태 문자열로만 알린다. (2026-10-23 WB03-M1)
    _startMusicDownload(loader) {
        if (this._musicBytes || this._musicStatus === 'loading' || !this.options.audio) return;
        this._musicStatus = 'loading';
        // 클릭하여 시작 관문이 기다릴 수 있도록 내려받기 약속(성공·실패 모두 끝나면 이행)을 보관한다. (2026-11-24 WB04-E1300)
        this._musicSettled = loader
            .loadBundle(MENU_MUSIC_BUNDLE)
            .then((files) => {
                this._musicBytes = files.get(MENU_ASSET_PATHS.music) ?? null;
                if (!this._musicBytes) throw new Error(`메뉴 음악 파일이 묶음에 없습니다: ${MENU_ASSET_PATHS.music}`);
                this._musicStatus = 'loaded';
                this._prepareMusic();
            })
            .catch((error) => {
                this._musicStatus = 'failed';
                if (this._root) reportGameError('audio-download', error, { state: this._state, asset: MENU_ASSET_PATHS.music });
            });
    }

    // 사용자의 시작 입력(클릭·Enter·Space)을 처리한다.
    // ERROR 에서는 로딩을 다시 시도하고, CLICK_TO_START 에서는 오디오를 준비한 뒤 메인 메뉴로 넘어간다.
    // 오디오 활성화는 사용자 입력 처리 안에서 동기로 요청해야 브라우저 정책을 통과하므로 상태 전환보다 먼저 한다.
    _activate() {
        if (this._state === AppState.ERROR) {
            this._load();
        } else if (this._state === AppState.TITLE) {
            // 시작 title 그림 중의 입력은 남은 그림을 건너뛴다(원본 waitTitleBitmap 이 입력 때 true 를 돌려주는 흐름). (2026-11-22 WB04-E1184)
            this._finishTitleScreens();
        } else if (this._state === AppState.CLICK_TO_START) {
            this._unlockAudio();
            // 원본은 시작 때 State 39(인트로)를 설치하며 정리를 한 번 실행한다(title 그림·메인 메뉴보다 앞). (2026-11-23 WB04-E1280)
            this._installIntroStateEffects();
            // 시작 title 그림이 켜져 있으면 먼저 보이고(입력하면 건너뜀), 아니면 바로 메인 메뉴로 간다. (2026-11-22 WB04-E1184)
            if (this.options.titleScreens && this._resources?.titleScreens) this._enterTitleScreens(); else this._enterMainMenu();
            // 개발용 맵도 클릭/오디오 활성화 관문 뒤에만 연다. 없는 맵은 메뉴를 유지하며 원인을 알린다. (2026-11-02 WB03-AA)
            if (this.options.previewMap) {
                try { this.previewMap(this.options.previewMap); }
                catch (error) { console.warn('Netstorm: 지형 미리보기를 열지 못했습니다.', error); }
            }
        }
    }

    // 시작 title 그림 표시를 시작한다(원본 4ab3e0). 첫 그림부터 3초씩 보이고, 클릭·키 입력이 있거나 세 장이 끝나면 `_finishTitleScreens` 가 메인 메뉴로 보낸다. (2026-11-22 WB04-E1184)
    // 고정 시각 모드에서는 시작 시각을 들어온 순간(마지막 프레임의 가상 시각)으로 잡아 세 장이 정확히 3초씩, 즉 프레임 밀리초로 나눈 프레임 수만큼 보이게 한다(원본 cpppj 의 고정 프레임 실행이 9초·25ms 프레임에 360프레임인 것과 같다). (2026-11-23 WB04-E1271)
    _enterTitleScreens() {
        this._titleStartedAt = this.options.fixedFrameMs > 0 ? (this._fixedStamp ?? 0) : null; this._titleBitmaps = [];
        this._setState(AppState.TITLE);
        this._displayDirty = true;
    }

    // 시작 title 그림을 끝내고 메인 메뉴로 간다(입력으로 건너뛰거나 세 장이 모두 끝났을 때). 이미 메인 메뉴이면 아무것도 하지 않는다. (2026-11-22 WB04-E1184)
    _finishTitleScreens() {
        if (this._state !== AppState.TITLE) return;
        this._titleStartedAt = null; this._titleBitmaps = [];
        this._enterMainMenu();
    }

    // 시작 title 그림의 현재 장을 검은 화면 가운데에 그린다. 첫 프레임에서 시작 시각을 잡고 경과 시간으로 장을 고르며 세 장이 끝나면 메인 메뉴로 간다. 그림은 처음 쓸 때 한 번만 RGBA 캔버스로 만든다. (2026-11-22 WB04-E1184)
    _drawTitleScreen(ctx, width, height, now) {
        if (this._titleStartedAt === null || this._titleStartedAt === undefined) this._titleStartedAt = now;
        const index = titleScreenIndexAt(now - this._titleStartedAt), screens = this._resources?.titleScreens;
        if (index >= TITLE_SCREEN_COUNT || !screens) { this._finishTitleScreens(); return; }
        let bitmap = this._titleBitmaps[index];
        if (!bitmap) {
            const image = screens[index].image, canvas = this._root.ownerDocument.createElement('canvas'); canvas.width = image.width; canvas.height = image.height;
            canvas.getContext('2d').putImageData(new ImageData(titleScreenRgba(screens[index]), image.width, image.height), 0, 0);
            bitmap = this._titleBitmaps[index] = canvas;
        }
        const origin = titleScreenOrigin(width, height, bitmap.width, bitmap.height);
        ctx.drawImage(bitmap, origin.x, origin.y);
    }

    // 메인 메뉴로 들어간다. 메뉴 화면을 만들고(처음 한 번) 현재 논리 크기로 배치하며, 창 제목과 메뉴 음악을 원본처럼 맞춘다.
    // (2026-10-23 WB03-M1)
    _enterMainMenu() {
        if (!this._resources) return;
        if (!this._mainMenu) {
            this._mainMenu = new MainMenuScreen({
                resources: this._resources,
                onCommand: (command, label, point) => this._onMenuCommand(command, label, point),
                onButtonSound: () => this._audio?.playSound(BUTTON_SOUND_NAME),
                // 원본 비교 모드는 원본 8버튼 배치를 쓴다. (2026-11-07 WB02-H)
                includeAllButtons: this.options.originalComparison,
                // 메인 메뉴 배경은 팔레트 전환이 만든 Kencloud 구름 캐시이다(원본 DrawKencloud). 캐시가 없으면 메뉴가 구름 GIF 를 깐다. (2026-11-24 WB04-E1298)
                cloudSource: () => this._clientKencloud?.cacheImage() ?? null,
            });
        }
        this._ensureSurface();
        // 원본 State 3 설치의 정리·메뉴 팔레트 전환(구름 구축)·주변음 기한 재설정을 메뉴 상태로 바꾸기 전에 실행한다. (2026-11-23 WB04-E1280)
        this._installMainMenuStateEffects();
        this._setState(AppState.MAIN_MENU);
        this._root.ownerDocument.title = this._resources.translate(MAIN_MENU_WINDOW_TITLE);
        this._displayDirty = true;
        this._playMenuMusic();
        this._runMainMenuStartup();
    }

    // 메인 메뉴에 들어갈 때의 시작 창 흐름이다(원본 State3 부모 004b6fa9 이후의 Tell 순서).
    //  - WelcomeGiven 이 0 이면 환영 창(`Welcome`)만 연다(`Welcome2` 를 지나면 welcomeGiven=1 이 저장된다).
    //  - 아니면 `Blank` 를 보낸 뒤, 첫 메뉴이고 tellTips 가 켜져 있으면 팁 창(`Tips`, "Did You Know?")을 연다. 첫 메뉴 표시는 이 갈래의 끝에서 꺼진다.
    // 원본은 팁 창 앞에 "Not Validated" 창(`UserNotUpdated`, 자동 업데이트 검증 실패 안내)도 겹쳐 열지만, 이 창은 구현하지 않기로 한 프로젝트 결정(2026-09-30)과
    // 웹에는 업데이터가 없다는 점에 따라 열지 않는다. 원본의 저장 상태 확인 질문(startup)도 웹에는 저장 상태가 없어 없다. (2026-10-30 WB03-J)
    _runMainMenuStartup() {
        const data = this._gameData;
        if (!data || !this._mainMenu) return;
        if (configParseLong(data.get('WelcomeGiven') ?? '0') === 0) {
            this._openTell('Welcome');
            return;
        }
        this._openTell('Blank');
        if (this._firstMenuPending && configParseLong(data.get('tellTips') ?? '0') !== 0) this._openTell('Tips');
        this._firstMenuPending = false;
    }

    // 메인 메뉴 버튼이 실행됐을 때 호출된다. 원본은 여기서 Tell 섹션(캠페인·데모·크레딧)이나 펼침 메뉴(도움말·옵션)를 연다.
    // 그 화면들은 아직 복원하지 않았으므로 호스트 훅과 콘솔에만 알린다. Tell 명령이면 원본 데이터에서 섹션을 실제로 찾아 줄 수를 함께 알린다.
    // (2026-10-23 WB03-M1, 2026-10-28 WB02-D: Tell 섹션 조회 확인 추가)
    // point 는 menu 명령이면 펼침 메뉴의 왼쪽 위 기준점(버튼 가운데 x, 버튼 위쪽 + 칸 정의의 popupDy)이다. (2026-10-29 WB03-F: 펼침 메뉴 연결)
    _onMenuCommand(command, label, point = null) {
        try {
            this.options.onMenuCommand?.(command, label);
        } catch (error) {
            console.error('Netstorm: onMenuCommand 훅에서 예외가 발생했습니다.', error);
        }
        // Tell 명령(Campaign·Demo·Credits)은 대화상자를, menu 명령(Help·Options)은 펼침 메뉴를 연다.
        if (command.type === 'tell') {
            this._openTell(command.target);
            return;
        }
        if (command.type === 'menu') {
            this._openPopup(command.target, point);
            return;
        }
        console.info(`Netstorm: 메인 메뉴 '${label}' 실행(다음 화면은 아직 구현되지 않음)`, command);
    }

    // 메인 메뉴의 펼침 메뉴(Help·Options)를 point(왼쪽 위 기준점)에 연다. 모르는 대상이면 열지 않는다. 기준점은 원본 스크린샷 측정값과 같은 규칙으로 정해진다(ui/main-menu.js popupAnchor). 열었으면 true. (2026-10-29 WB03-F)
    _openPopup(target, point) {
        if (!this._mainMenu || !this._resources || !this._settings) return false;
        // 원본 비교 모드는 웹이 빼는 원본 항목도 만든다. Technical Help 는 원본 가용성 판정대로 비활성이다(웹에는 기술 도움말 프로그램이 없음). (2026-11-07 WB02-H)
        const items = buildPopupItems(target, this._settings, { translate: this._resources.translate, includeAll: this.options.originalComparison, technicalHelpAvailable: false, language: this._gameData?.language ?? this.options.language });
        if (!items) {
            console.warn(`Netstorm: 알 수 없는 펼침 메뉴 '${target}' 입니다.`);
            return false;
        }
        const popup = new PopupMenu({
            resources: this._resources,
            screen: { width: this._mainMenu.width, height: this._mainMenu.height },
            onCommand: (command) => this._onPopupCommand(command),
            onSound: () => this._audio?.playSound(MENU_SOUND_NAME),
            // 관측 보고용 대상 이름이다. (2026-11-07 WB02-H)
            name: target,
        });
        // 포인터 위치가 없으면(호스트가 직접 호출) 현재 포인터 위치를 쓴다.
        popup.open(items, point ?? { x: Math.max(0, this._input.pointer.x), y: Math.max(0, this._input.pointer.y) });
        this._mainMenu.setPopup(popup);
        this._displayDirty = true;
        return true;
    }

    // 펼침 메뉴 행의 명령을 실행한다(메뉴는 이미 닫혀 있다). Toggle 은 설정 키를 뒤집고, Volume 은 `키=단계` 로 음량을 정하며, Tell 은 대화상자를 연다.
    // F1Help 는 일반 도움말 창을 연다(2026-10-30 WB03-G).
    _onPopupCommand(command) {
        const name = String(command.name).toLowerCase();
        if (name === 'f1help') {
            this._openHelp('F1Help');
        } else if (name === 'toggle') {
            this._settings?.toggle(command.argument);
        } else if (name === 'volume') {
            const [key, value] = String(command.argument).split('=');
            this._settings?.set(key, Number(value));
        } else if (name === 'tell') {
            this._openTell(command.argument);
        } else {
            console.info(`Netstorm: 펼침 메뉴 명령 '${command.name}' 은(는) 아직 구현되지 않았습니다.`, command);
        }
        this._displayDirty = true;
    }

    // 확인이 끝난 Reset All 을 실행한다: 게임 소유 저장 항목을 지우고 현재 앱을 정리한 다음 페이지를 다시 불러온다. 확인은 게임 안 Tell 창(`Patch1101.ResetAll`)이 받고
    // 확정 단추(`Patch1101.ResetAllConfirmed`)가 `_openTell` 을 거쳐 이 함수를 부른다(브라우저 confirm() 은 쓰지 않는다). 저장소 접근이 막히면 안내하고 앱을 유지한다.
    // (2026-11-19 WB04-E863, 2026-11-23 WB04-E1251: confirm() 을 게임 안 확인 창으로 교체)
    _resetAll() {
        const view = this._view();
        try {
            resetGameStorage(getLocalStorage(view));
        } catch (error) {
            reportGameError('reset-storage', error, { state: this._state });
            view.alert(this.texts.resetFailed);
            return;
        }
        this.destroy();
        view.location.reload();
    }

    // Options의 웹 전용 관리 화면이다. 같은 게임 시계의 정지 한 단계만 빌린다.
    _openModPanel() {
        if (!this._root || !this._mods || !this._clientGump || this._state !== AppState.MAIN_MENU) return false;
        if (this._modPanel) { this._modPanel.focus(); return true; }
        this._closeCampaignPopups();
        const clock = this._gameClock, view = this._view(); let held = true;
        clock.pushGamePause();
        try {
            this._modPanel = new ModPanel({ parent: this._root, manager: this._mods, language: this._gameData?.language ?? this.options.language, maxPackageBytes: MOD_LIMITS.maxPackageBytes,
                validatePackage: (input, signal) => validateModPackageIsolated(input, { view, signal }),
                onClose: () => { this._modPanel = null; if (held) { held = false; clock.popGamePause(); } this._displayDirty = true; if (this._mainMenu) this._mainMenu.dirty = true; } });
            return true;
        } catch (error) { if (held) { held = false; clock.popGamePause(); } reportGameError('mod-panel', error); return false; }
    }

    // World/App 반납 때는 숨겨질 캔버스에 초점을 돌리지 않는다. 여러 번 호출해도 한 번만 반납한다.
    _closeModPanel(restoreFocus = false) { this._modPanel?.close({ restoreFocus }); }

    // 메인 메뉴 Options의 두 JSON 동작을 연다. 기존 모드 창과 동시에 입력을 받지 않는다.
    _openSettingsTransfer(mode) {
        if (!this._root || !this._settings || !this._progress || this._state !== AppState.MAIN_MENU || this._campaignWorld || this._clientGuide?.current()) return false;
        if (this._settingsPanel) { this._settingsPanel.focus(); return true; }
        this._closeModPanel(false); this._closeCampaignPopups();
        const clock = this._gameClock; let held = true; clock.pushGamePause(); // 편집 중 자동 데모와 게임 시계를 정지하는 한 단계다.
        try {
            this._settingsPanel = new SettingsTransferPanel({ parent: this._root, language: this._gameData?.language ?? this.options.language, mode,
                value: mode === 'copy' ? serializeSettingsTransfer(this._settings, this._progress) : '',
                onApply: (source) => this._restoreSettingsTransfer(source),
                onClose: () => { this._settingsPanel = null; if (held) { held = false; clock.popGamePause(); } this._input.keys.clear(); this._input.pointer.buttons = 0; this._canvas?.focus({ preventScroll: true }); this._displayDirty = true; } });
        } catch (error) { if (held) { held = false; clock.popGamePause(); } throw error; }
        return true;
    }

    // 입력 전체를 검증·저장한 뒤 같은 앱을 다시 초기화한다. 언어·사운드·진행 캐시도 새 값으로 읽는다.
    _restoreSettingsTransfer(source) {
        const value = parseSettingsTransfer(source); // 빈 문자열은 취소이며 어떤 값도 바꾸지 않는다.
        if (!value) { this._settingsPanel?.close(); return; }
        if (this._campaignWorld || this._clientGuide?.current()) throw new Error('설정은 메인 메뉴에서만 불러올 수 있습니다.');
        const view = this._view(), root = this._root; // 다시 초기화할 같은 문서/게임 루트다.
        storeSettingsTransfer(getLocalStorage(view), value);
        const language = languageOfChoice(value.settings.language) ?? normalizeLanguage(detectBrowserLanguage(view.navigator)); // 명시 언어 또는 현재 브라우저 자동 선택이다.
        const params = new URLSearchParams(view.location.search); // 이전 lang 주소가 복원된 설정보다 우선하지 않게 지운다.
        if (params.has('lang')) { params.delete('lang'); const query = params.toString(); view.history.replaceState(null, '', `${view.location.pathname}${query ? `?${query}` : ''}${view.location.hash}`); }
        this._settingsPanel?.close(); this.destroy();
        this._resources = null; this._gameData = null; this.options.language = language;
        this.options.soundVolume = null; this.options.musicVolume = null;
        this._resumeSettingsToMainMenu = true; this.init(root);
    }

    // 설정 값이 바뀌었을 때 소리·음악·음량과 미션 효과음의 좌우 교환을 바로 적용한다.
    // 바뀐 값은 게임 데이터 설정 층에도 옮겨 적어 두 곳이 어긋나지 않게 한다(되돌아오는 통지는 막는다). (2026-10-30 WB03-H)
    _onSettingChange(key) {
        if (key === 'sound' || key === 'music' || key === 'soundVolume' || key === 'musicVolume' || key === 'swapLeftRightSpeakers') this._applySettings();
        const value = this._settings?.get(key);
        if (value !== undefined) this._gameData?.set(key, value, { silent: true });
        this._syncClientCloudOptions(); this._syncClientHelpWord(); this._syncClientOwnerColorOptions(); this._syncClientCursorOptions();
    }

    // 게임 데이터 설정 층에 쓴 값을 저장소로 옮긴다(원본에서는 메인 루프가 변경 표시를 보고 options.cfg 를 저장한다). 진행 값(`Done*`)은 진행 저장소에,
    // 설정 키(대소문자 무시)는 설정 저장소에 넣어 로컬 저장소에 바로 저장한다. 그 밖의 값(`global.` 실행 중 값 등)은 저장하지 않는다. (2026-10-30 WB03-H)
    _onConfigWrite(key, value) {
        this._syncClientCloudOptions(); this._syncClientHelpWord(); this._syncClientOwnerColorOptions(); this._syncClientCursorOptions();
        if (isProgressKey(key)) {
            this._progress?.set(key, value);
            return;
        }
        const settingKey = findSettingKey(key);
        if (settingKey) this._settings?.set(settingKey, value);
    }

    // 설정과 진행 저장소의 현재 값을 게임 데이터 설정 층에 옮겨 적는다(통지 없이). 게임 데이터는 재시작해도 다시 만들지 않으므로 init 이 저장소를 새로 읽을 때마다 맞춘다.
    // 게임 데이터가 아직 없으면(첫 적재 전) 하지 않으며, 첫 적재가 같은 값을 넣는다. (2026-10-30 WB03-H)
    _syncGameData() {
        const data = this._gameData;
        if (!data || !this._settings || !this._progress) return;
        // 설정과 진행 값을 하나씩 옮긴다.
        for (const [key, value] of Object.entries({ ...this._settings.snapshot(), ...this._progress.snapshot() })) data.set(key, value, { silent: true });
        this._syncClientCloudOptions(); this._syncClientHelpWord(); this._syncClientCursorOptions();
    }

    // 현재 설정(소리 켜짐·음악 켜짐·음량 단계)을 재생기에 적용한다. 음악을 다시 켠 경우 메인 메뉴 곡을 이어 재생한다. (2026-10-29 WB03-F)
    _applySettings() {
        const settings = this._settings;
        const audio = this._audio;
        if (!settings || !audio) return;
        audio.setSoundVolumeStep(settings.get('soundVolume'));
        audio.setMusicVolumeStep(settings.get('musicVolume'));
        audio.setSoundEnabled(settings.get('sound') === 1);
        audio.setMusicEnabled(settings.get('music') === 1);
        this._syncCampaignSoundSettings();
        if (settings.get('music') === 1) this._playMenuMusic();
    }

    // Tell 대상으로 대화상자를 열어 메인 메뉴 위에 올린다(원본 Tell 명령 004cf730). 대상은 `섹션이름` 또는 `섹션이름(인자,…)` 이다(지역 인자는 본문의 `{param.1}` 이 읽는다).
    // `Blank` 는 창을 열지 않는 명령이다. 섹션이 없으면 원본처럼 오류 로그만 남기고 아무것도 열지 않는다. 본문이 영숫자 없는 빈 내용이어도 열지 않는다.
    // 본문의 스크립트 명령(`<$config,키=값>` 등)은 창이 올라간 뒤(창이 없어도) 순서대로 실행한다. 열린 대화상자의 명령은 _onDialogCommand 가 받는다. 열었으면 true 를 돌려준다.
    // 본문의 링크(`<a href>`)를 누르면 _onDialogLink 가 받는다. 본문이 화면보다 길면 스크롤바가 생긴다(시계는 _frameSeconds).
    // (2026-10-28 WB03-D, 2026-10-30 WB03-J: Blank·지역 인자·본문 스크립트 명령 추가, 2026-10-31 WB03-K: 링크·스크롤바 연결)
    // `briefing` 은 원본 UberGump::TryTell 의 briefing 인자(창을 열 때 단계를 Briefing 으로 두는지)이다. 미션 프레임 관문이 연 안내 창(시작 브리핑·시간 대본 구역)만 참이고 그 밖의 Tell 은 Dialog 단계라,
    // ESC 는 Dialog 단계의 창만 닫는다(_escapeCloseTell). 창을 연 뒤 그 창 객체(`_tellDialog`)와 단계(`_tellBriefing`)를 기억한다. 사용: this._openTell('Credits'); this._openTell('Briefing', { briefing: true }). (2026-11-23 WB04-E1264)
    _openTell(name, { suppressWindow = false, briefing = false } = {}) {
        // Reset All 확정(`Patch1101.ResetAllConfirmed`)은 확인 창이 열린 직후 한 번만 효력이 있다(원본 UberGump 의 resetAllSubmission_ 가드: 그 단추가 낸 토큰만 소비). 다른 Tell·링크에서 오는 같은 이름은 무시하고,
        // 그 밖의 모든 Tell(취소의 Blank 포함)은 무장을 풀어 오래된 확인 창의 확정이 나중에 되살아나지 않게 한다. (2026-11-23 WB04-E1251)
        const resetTarget = String(name).trim().toLowerCase();
        const resetArmed = this._resetAllArmed === true; this._resetAllArmed = false;
        if (resetTarget === PRODUCT_RESET_ALL_CONFIRMED_TARGET.toLowerCase()) {
            if (!resetArmed) { console.warn('Netstorm: 확인 창을 거치지 않은 Reset All 확정은 무시합니다.'); return false; }
            this._resetAll(); return true;
        }
        if (resetTarget === MOD_PANEL_TARGET.toLowerCase()) return this._openModPanel();
        if (resetTarget === SETTINGS_LOAD_TARGET.toLowerCase()) return this._openSettingsTransfer('load');
        if (resetTarget === SETTINGS_COPY_TARGET.toLowerCase()) return this._openSettingsTransfer('copy');
        if (!this._mainMenu || !this._gameData || !this._resources) return false;
        // 옵션 메뉴 Frame Rate 항목의 대상(`Patch1101.SetFrameRate(n)`)은 창을 열지 않고 maxFPS 설정에 써서 바로 반영한다(cpppj 후순위 F1). (2026-11-20 WB04-E1094)
        const frameRate = parseFrameRateTarget(name);
        if (frameRate !== null) { this._settings?.set('maxFPS', frameRate); this._displayDirty = true; return true; }
        // 언어 선택(`Patch1101.SetLanguage(n)`)은 설정에 저장하고 페이지를 다시 불러 적용한다. 이미 그 언어이면 아무것도 하지 않는다. (2026-11-20 WB04-E1095)
        const languageChoice = parseLanguageTarget(name);
        if (languageChoice !== null) return this._chooseLanguage(languageChoice);
        // 난이도 선택은 메인 메뉴에서만 저장한다. 미션 중의 오래된 메뉴/직접 Tell도 같은 관문에서 막는다.
        const difficulty = parseDifficultyTarget(name);
        if (difficulty !== null) { if (!this._campaignWorld && !this._clientGuide?.current()) this._settings?.set('difficulty', difficulty); this._displayDirty = true; return true; }
        // 스커미시 설정 변경(`Patch1101.SkirmishSet(...)`)은 창 없이 설정에 쓰고 알맞은 화면을 다시 연다. 시작(`Patch1101.SkirmishStart`)은 창 없이 미션 시작으로 간다(cpppj UberGump TryTell). (2026-11-20 WB04-E1102)
        const skirmish = parseSkirmishTarget(name);
        if (skirmish.kind === 'set' || skirmish.kind === 'start') {
            const settings = this._loadSkirmishSettings();
            // 시작하지 못했으면(월드 생성 미연결·후보 부족) 단추가 닫은 설정 첫 화면을 다시 열어 사용자가 화면 밖으로 떨어지지 않게 한다. (2026-11-20 WB04-E1102)
            if (skirmish.kind === 'start') { if (this._startSkirmish(settings)) return true; this._openTell('Patch1101.Skirmish'); return false; }
            const next = applySkirmishAction(settings, skirmish.args); this._storeSkirmishSettings(settings);
            return this._openTell(next, { suppressWindow, briefing });
        }
        // 미션 중에는 안내 창이 왼쪽 HUD 를 뺀 지도 영역의 가운데에 놓인다(원본 스크린샷의 Briefing·Success 창). 메인 메뉴에서는 화면 전체이다. (2026-11-21 WB04-E1130)
        const view = this._campaignDisplayActive() ? this._campaignWorld.mapView() : null;
        const tellScreen = view ? { left: view.left, top: view.top, width: view.right - view.left, height: view.bottom - view.top } : { width: this._mainMenu.width, height: this._mainMenu.height };
        const result = createTellWindow({
            bodyFor: (target, presented) => this._skirmishScreenBody(target) ?? presented,
            gameData: this._gameData,
            resources: this._resources,
            name,
            missionSection: (section) => this._clientGuide?.current()?.script.section(section) ?? null,
            suppressWindow,
            screen: tellScreen,
            nowSeconds: this._frameSeconds,
            now: () => this._frameSeconds,
            onCommand: (command) => this._onDialogCommand(command),
            onLink: (destination) => this._onDialogLink(destination),
            onSound: (kind) => this._audio?.playSound(kind === 'menu' ? MENU_SOUND_NAME : BUTTON_SOUND_NAME),
            onUnsupported: (what, text) => console.warn(`Netstorm: Tell '${name}' 의 ${what} 은(는) 아직 지원하지 않습니다.`, text),
            // 원본 비교 모드는 웹이 빼는 목록 행(User Made Campaigns)도 만든다. (2026-11-07 WB02-H)
            excludedLists: this.options.originalComparison ? [] : undefined,
        });
        if (result.missing && !suppressWindow) console.warn(`Netstorm: TELL cannot find "${name}".`);
        if (result.window) {
            this._mainMenu.setDialog(result.window);
            this._tellDialog = result.window; this._tellBriefing = briefing === true;
            this._displayDirty = true;
            // Reset All 확인 창이 실제로 열렸을 때만 확정 단추를 한 번 받을 수 있다. (2026-11-23 WB04-E1251)
            if (resetTarget === PRODUCT_RESET_ALL_TARGET.toLowerCase()) this._resetAllArmed = true;
        }
        // 본문 안의 스크립트 명령은 원본이 명령 큐에 넣어 다음 입력에서 처리하는 것과 같게, 창을 올린 뒤 순서대로 실행한다.
        for (const command of result.inlineCommands) this._runScriptCommand(command);
        return result.window !== null;
    }

    // 맡겨 둔 ViewSpot 좌표가 있고 미션 월드가 돌고 있으면 카메라를 그 칸으로 옮기고 맡김을 비운다(없으면 아무것도 하지 않는다). 칸 단위 좌표는 TAB·U 키 이동과 같은 `world.centerCamera` 로 보낸다. (2026-11-22 WB04-E1190)
    _applyPendingViewSpot() {
        const spot = this._pendingViewSpot, world = this._campaignWorld;
        // 미션 시작(State9 Start 의 카메라 초기화·홈 이동)이 끝난 뒤에만 옮긴다. 그 전에 옮기면 이어지는 홈 이동이 덮어쓴다.
        if (!spot || !world || !this._campaignDisplayActive() || this._clientGuide?.current()?.runtime.gate.started !== 1) return;
        this._pendingViewSpot = null; world.centerCamera(spot.x, spot.y); this._clientRenderer?.invalidateAll(); this._displayDirty = true;
    }

    // 현재 미션 Session 에 선택한 원소(theme)의 기술을 모두 지급한다(원본 UberGump 의 CheatAllTechnology 입력 0x02000028 → ApplyThemeTechnology). 미션 월드가 없으면 안내만 남기고 지급하지 않는다(원본은 logic_error).
    // 지급한 기술은 다음 프레임의 생산 내용물 갱신(`updateProductionContents`)이 생산 패널에 반영한다(원본의 `rebuild_`). (2026-11-22 WB04-E1189)
    _applyThemeTechnology(theme) {
        const world = this._campaignWorld;
        if (!world) { console.warn('Netstorm: CheatAllTechnology 는 미션 월드가 있을 때만 기술을 지급합니다.'); return false; }
        applyThemeTechnology(world.session(), theme);
        this._clientRenderer?.invalidateAll(); this._displayDirty = true;
        return true;
    }

    // 스크립트 명령(본문의 `<$이름,인자>` 와 버튼 명령 공통) 하나를 실행한다. Config 는 `키=값` 을 게임 데이터 설정에 쓰고(설정·진행 저장소로 이어짐), Warning 은 콘솔에 남긴다.
    // 아직 연결하지 않은 명령은 안내만 한다. 실행했으면 true 이다. (2026-10-30 WB03-J)
    _runScriptCommand(command) {
        const name = String(command.name).toLowerCase();
        if (name === 'config' && this._gameData) {
            applyScriptConfig(String(command.argument), (key, value) => this._gameData.set(key, value));
            return true;
        }
        if (name === 'warning') {
            // 미션 HUD 가 있으면 오른쪽 위 경고 표시에 올린다(원본 UberGump 의 Warning 명령 → warnings_->Add). 인자의 `~B~E~y` 같은 서식 부호는 경고판이 해석한다. PriestImmobile·islandUnderAttack·WarnPriestTaken·
            // WarnYourPriestTaken·WarnNeutralPriestTaken·AllianceBroken 안내 섹션이 이 명령 한 줄뿐이라, 이전에는 콘솔에만 남아 미션 중 경고 문구가 화면에 나오지 않았다. HUD 가 없으면(메뉴 등) 콘솔에만 남긴다. (2026-11-22 WB04-E1228)
            const board = this._campaignProduction?.ui?.warnings();
            if (board) { board.add(String(command.argument ?? '')); this._displayDirty = true; } else console.warn(`Netstorm: ${command.argument}`);
            return true;
        }
        // `<$ViewSpot,(가로x세로)>`: 미션 시작 카메라를 그 칸으로 옮긴다(RunForIt 의 [Init]). Init 은 월드가 생기기 전에 열리므로 좌표를 맡겨 두었다가 월드가 도는 첫 입력 프레임에 적용한다(원본은 명령 큐에 넣어 이어지는 입력에서 처리). (2026-11-22 WB04-E1190)
        if (name === 'viewspot') { this._pendingViewSpot = parseViewSpot(String(command.argument ?? '')); this._applyPendingViewSpot(); return true; }
        // `<$CheatAllTechnology,원소>`: 선택한 원소(0 바람·1 비·2 천둥·3 태양)의 기술을 모두 지급한다. Fury War(bc8fury76)의 원소 선택 단추와 AI 사제 희생 안내가 쓴다. (2026-11-22 WB04-E1189)
        if (name === 'cheatalltechnology') return this._applyThemeTechnology(Number.parseInt(String(command.argument ?? '').trim(), 10) | 0);
        console.info(`Netstorm: 스크립트 명령 '${command.name}' 은(는) 아직 구현되지 않았습니다.`, command);
        return false;
    }

    // 12차 스커미시: 설정 저장소에서 현재 스커미시 설정(AI 수·AI별 난이도/동맹·금지 유닛)을 읽는다. 값이 없거나 범위 밖이면 기본값이다. (2026-11-20 WB04-E1102)
    _loadSkirmishSettings() {
        return loadSkirmishSettings((key) => String(this._settings?.get(key) ?? ''));
    }

    // 12차 스커미시: 설정을 설정 저장소에 쓴다(바뀐 값만 저장·통지된다). (2026-11-20 WB04-E1102)
    _storeSkirmishSettings(settings) {
        saveSkirmishSettings((key, value) => { this._settings?.set(key, value); }, settings);
    }

    // 12차 스커미시: 대상이 설정 화면(첫 화면·AI 수·AI 한 명·금지 유닛)이면 현재 설정과 언어로 만든 Tell 본문을, 아니면 null 을 돌려준다.
    // 금지 유닛 이름은 메뉴 번역표(한국어 xlat)로 번역한다. 언어 번호는 원문 규칙(0 영어, 7 한국어)이다. (2026-11-20 WB04-E1102)
    _skirmishScreenBody(target) {
        const parsed = parseSkirmishTarget(target);
        if (!['main', 'count', 'ai', 'bans'].includes(parsed.kind) || !this._gameData || !this._resources) return null;
        return buildSkirmishScreen(parsed, this._loadSkirmishSettings(), originalLanguageNumber(this._gameData.productLanguage), (text) => this._resources.translate(text));
    }

    // 12차 스커미시: 설정으로 게임을 시작한다. 시작 함수(options.startSkirmish)가 있으면 그것을 부르고(검사용), 없으면 무작위 시작 자료를 만들어 시작 섬 fort 묶음을 받은 뒤 생성 미션을 시작한다(비동기).
    // 메인 메뉴 밖이거나 필요한 자료·Guide 가 없거나 시작 섬 후보가 모자라면 안내만 남기고 false 이다. 시작을 예약했으면 true 이다. (2026-11-20 WB04-E1102, WB04-E1105: 실제 시작 연결)
    _startSkirmish(settings) {
        if (this.options.startSkirmish) return Boolean(this.options.startSkirmish(settings));
        if (!this._assetLoader || !this._clientGuide || !this._gameData || this._state !== AppState.MAIN_MENU) { console.warn('Netstorm: 스커미시 시작에 필요한 자료·Guide·메인 메뉴가 없습니다.'); return false; }
        const launch = buildSkirmishLaunch(settings, Math.floor(Math.random() * 0x100000000) >>> 0);
        if (!launch.islands.length) { console.warn('Netstorm: 스커미시 시작 섬 후보가 모자랍니다.'); return false; }
        this._beginSkirmish(launch).catch((error) => { reportGameError('skirmish-start', error, { state: this._state }); });
        return true;
    }

    // 12차 스커미시: 시작 섬 fort 묶음을 받아 월드 계획과 가상 대본·빈 월드 fort 파일 읽기를 만들고 생성 미션 'Skirmish' 를 시작한다. 현재 미션이 있으면 먼저 떠난다. (2026-11-20 WB04-E1105)
    async _beginSkirmish(launch) {
        const generation = this._loadGeneration, files = await this._assetLoader.loadBundle(SKIRMISH_BUNDLE);
        if (generation !== this._loadGeneration || !this._clientGuide || this._state !== AppState.MAIN_MENU) return false; // 받는 동안 앱이 다시 시작됐거나 메인 메뉴를 떠났으면 그만둔다.
        const plan = makeSkirmishWorldPlan(launch, (fort) => files.get(`d/${fort}.fort`) ?? null);
        const emptyWorld = makeSkirmishEmptyWorldFort(plan.islands[0].bytes);
        if (!emptyWorld.length) throw new Error(`스커미시 빈 월드 fort 를 만들 수 없음: ${plan.islands[0].fort}`);
        const data = this._gameData;
        const source = createSkirmishFileSource(launch.script, emptyWorld, data.pathSpec('missionSpec', SKIRMISH_MISSION_NAME), data.pathSpec('fortSpec', SKIRMISH_MISSION_NAME));
        if (this._campaignWorld || this._clientGuide.current()) this._leaveCampaignMission();
        this._skirmishPlan = plan; this._markSkirmishRun(SKIRMISH_MISSION_NAME);
        this._lastCampaignMission = { name: SKIRMISH_MISSION_NAME, demo: 0, mode: 0, source };
        this._clientGuide.begin(SKIRMISH_MISSION_NAME, 0, 0, source);
        return true;
    }

    // 12차 스커미시: 이번 미션이 생성 스커미시인지 설정(skirmishRun)에 적는다. AI 난이도와 세계 간헐천 예산이 이 값으로 AI별 설정·참가자 수를 쓴다. 스커미시가 아니면 이전 월드 계획도 비운다. (2026-11-20 WB04-E1105)
    _markSkirmishRun(name) {
        const skirmish = isSkirmishMissionName(name);
        if (!skirmish) this._skirmishPlan = null;
        this._gameData?.set('skirmishRun', skirmish ? '1' : '0', { silent: true });
    }

    // 시작 팁 창의 Prior Tip / Next Tip / OK 버튼(`TellTip,1|2|3`)을 처리한다(원본 TellTip). 현재 팁 번호는 `tipNumber` 설정이며, 바뀐 번호가 설정에 쓰여 저장된다.
    // OK(3)는 다음 팁 번호만 저장하고 창을 다시 열지 않으며, 나머지는 같은 `Tips` 창을 새 번호로 다시 연다. 버튼 인자가 숫자로 시작하지 않으면 0 으로 본다. (2026-10-30 WB03-J)
    _runTellTip(command) {
        const data = this._gameData;
        if (!data) return;
        const argument = String(command.argument ?? '');
        const parameter = argument.length > 0 && argument[0] >= '0' && argument[0] <= '9' ? configParseLong(argument) : 0;
        dispatchTellTip({ code: TELL_TIP_COMMAND, y: parameter }, configParseLong(data.get('tipNumber') ?? '0') >>> 0, {
            format: (value) => String(value),
            setConfig: (key, value) => data.set(key, value),
            tell: (section) => this._openTell(section),
        });
    }

    // 일반 도움말 창을 열어 메인 메뉴 위에 올린다(원본 F1Help: 메인 메뉴 Help 목록의 General Help 행과 F1 키). 이미 열려 있으면 그 창에서 주제만 첫 주제로 되돌린다.
    // 도움말 파일(`Quick Help Spec`)이 없으면 원본처럼 오류 로그만 남기고 열지 않는다. 바깥 주소 링크는 `_openExternalAddress` 로 새 탭/창에 연다(2026-11-07 WB03-AD, 그 전에는 콘솔 안내만). 열었으면 true 를 돌려준다. (2026-10-30 WB03-G)
    // 노드의 도움말 문구(text=false) 또는 주제 이름(word=true)을 첫 NUL까지 소유한다. 이전 토큰을 반납한 뒤 새 토큰을 받으며 null·빈 값은 포인터를 지운다(원본 SetNodeHelp, E1236·E1347).
    _setNodeHelp(token, text, word) {
        const node = this._clientGump?.node(token); if (!node) throw new Error('도움말 실제 노드 없음');
        // C 문자열처럼 첫 NUL 뒤의 내용은 문구·주제·빈 값 판정에 포함하지 않는다.
        const terminated = text === null ? '' : String(text).split('\0', 1)[0];
        const strings = this._helpStrings ??= new Map(), key = word ? 'alternateTextPointer' : 'textPointer';
        strings.delete(node[key]); node[key] = 0;
        // 원본00464ed0/00464f80: 비어 있지 않은 문구는 LOOK(1000)/HELP(200)를 켠다. 빈 도움말은 HELP를 유지하고 빈 LOOK만 비트를 지운다. (WB04-E1324)
        if (terminated === '') { if (!word) node.flags = (node.flags & ~0x1000) >>> 0; return; }
        this._nextHelpString = (this._nextHelpString ?? 0x61000100) + 1; if (this._nextHelpString > 0x61ffffff) throw new Error('도움말 문자열 주소 영역 소진');
        strings.set(this._nextHelpString, terminated); node[key] = this._nextHelpString; node.flags = (node.flags | (word ? 0x200 : 0x1000)) >>> 0;
    }

    // 도움말 포인터/토큰의 문자열이다(원본 HelpText): 0x61000002 는 현재 helpWord 캐시(없으면 번역한 "Help"), 0x61000001 은 themeHelp, 0x61000003 은 F1Help, 그 밖은 SetNodeHelp 가 저장한 문자열이다. (2026-11-22 WB04-E1236)
    _helpString(pointer) {
        if (!pointer) return ''; if (pointer === 0x61000001) return 'themeHelp'; if (pointer === 0x61000003) return 'F1Help';
        if (pointer === 0x61000002) return this._clientLocal?.helpWord().get() ?? this._resources.translate('Help');
        const text = this._helpStrings?.get(pointer); if (text === undefined) throw new Error('도움말 문자열 토큰 없음'); return text;
    }

    // 도움말 TYPE 정보의 출처 묶음 { types, info(type), hitPoints(type) }이다(원본 UberGump help.type·help.hitPoints: world_ 가 있으면 Session 의 현재 표, 없으면 ReadAssetQuickHelpType 과 장기 TYPE 표·현재 약화/특별 번호).
    // 월드가 없는 메인 메뉴 Help 는 장기 표면 저장소의 TYPE 표(월드 설치 전 값)를 읽으며, 필요한 장기 소유자가 없으면 null 이다. (2026-11-23 WB04-E1240)
    _helpTypeSource() {
        const world = this._campaignWorld;
        if (world) return { types: world.session().core().types, info: (type) => world.session().helpTypeInfo(type), hitPoints: (type) => world.session().hitPoints().typeHitPoints(type) };
        const storage = this._surfaceStorage, gameplay = this._clientGameplay, game = this._gameAssets;
        if (!storage || !gameplay || !game) return null;
        const types = storage.types, aux = gameplay.gameAux();
        return { types, info: (type) => readAssetQuickHelpType(game, types, type),
            // 원본 MakeCurrentTypeHitPoints: 현재 표에서 선택 레코드를 읽고 진단·1/4 HP 가 켜진 경우만 같은 표 기준 번호를 쓴다.
            hitPoints: (type) => {
                if (!Number.isInteger(type) || type < 0 || type >= types.length) throw new RangeError('현재 TYPE HP 번호 범위');
                const record = types[type];
                return readRiftHitPoints(record, { checking: () => gameplay.aiDebug().checking().value, quarter: () => aux.priestQuarterHp, priest: () => aux.priestType, dais: () => aux.daisType },
                    { number: () => currentRiftTypeNumber(record, types, types), assertion: (expression, file, line) => storage.assertDisplay(expression, file, line) });
            } };
    }

    // 현재 TYPE 하나에 대한 도움말 정보 패널 생성기와 그림이다(F1 생산 정보·링크 이동·메인 메뉴 Help 가 같이 쓴다). 출처는 _helpTypeSource 가 월드 유무로 고른다. 출처가 없으면 던진다. (2026-11-22 WB04-E1229, 2026-11-23 WB04-E1240: 월드 없는 출처)
    _helpTypeDisplay(type) {
        const source = this._helpTypeSource(); if (!source) throw new Error('도움말 TYPE 정보의 실제 출처 미연결');
        const types = source.types;
        const information = () => buildQuickHelpInfoHtml({ type: () => source.info(type),
            description: () => this._resources.translate(types[type].description), hitPoints: () => source.hitPoints(type),
            translate: (text) => this._resources.translate(text), substitute: (text) => this._gameData.substitute(text), config: (key) => this._gameData.get(key) ?? '' });
        return { information, picture: createQuickHelpPicture(this._gameAssets, type) };
    }

    // 도움말 주제 이름으로 TYPE 정보를 찾는다(원본 QuickHelpRequest::Open(0, 주제)): 끝의 `type`·`help` 를 차례로 떼고 `%s`·`sun%s`·`wind%s` 이름을 TYPE 표에서 대소문자 없이 찾아, 찾으면(번호 1 이상) 정보 패널·그림을 만든다.
    // 찾지 못하면(원본은 TYPE 0) 정보 없이 본문만 보인다. 미션 월드가 없는 메인 메뉴 도움말도 장기 TYPE 표와 ReadAssetQuickHelpType 으로 같은 정보를 만든다. 어떤 주제에서도 던지지 않는다. (2026-11-22 WB04-E1229, 2026-11-23 WB04-E1240)
    _resolveHelpType(topic) {
        const source = this._helpTypeSource(); if (!source || !this._resources || !this._gameData || !this._gameAssets) return null;
        let stem = String(topic ?? '').split('\0', 1)[0]; if (!stem || stem.length >= 80) return null;
        for (const suffix of ['type', 'help']) if (stem.length >= suffix.length && stem.slice(-suffix.length).toLowerCase() === suffix) stem = stem.slice(0, -suffix.length);
        const types = source.types, nameOf = (record) => String(record.name ?? '').split('\0', 1)[0].toLowerCase();
        let found = -1;
        for (const pattern of [stem, `sun${stem}`, `wind${stem}`]) { found = types.findIndex((record) => nameOf(record) === pattern.toLowerCase()); if (found >= 0) break; }
        if (found < 1 || found >= 256 || found >= types.length) return null;
        return this._helpTypeDisplay(found);
    }

    _openHelp(topic, information = null, picture = null) {
        if (!this._mainMenu || !this._gameData || !this._resources) return false;
        const open = this._mainMenu.dialog;
        if (open instanceof HelpWindow && !open.closed) {
            open.open(topic, information, picture);
            this._mainMenu.dirty = true;
            return true;
        }
        const result = createHelpWindow({
            gameData: this._gameData,
            resources: this._resources,
            topic, information, picture,
            screen: { width: this._mainMenu.width, height: this._mainMenu.height },
            now: () => this._frameSeconds,
            resolveType: (name) => this._resolveHelpType(name),
            // 첫 주제에서 Back 으로 닫으면 직전 팝업 명령(F6 지식 창 등)을 같은 문자열 토큰 인자로 다시 큐에 올린다(원본 QuickHelpControl: popup.command/Argument). (2026-11-22 WB04-E1237)
            onBackToPopup: () => { const popup = this._clientHelpPopup; if (!popup?.command || !this._clientInputQueue) return; const token = this._clientChatWindows.store(String.fromCharCode(...popup.argumentBytes())); this._clientInputQueue.pushCommand(popup.command, token, token); },
            onClose: () => {
                if (this._mainMenu?.dialog === result.window) this._mainMenu.setDialog(null);
                this._displayDirty = true;
            },
            onCommand: (command) => this._onDialogCommand(command),
            onExternal: (address) => this._openExternalAddress(address, '도움말'),
            onSound: () => this._audio?.playSound(BUTTON_SOUND_NAME),
            onUnsupported: (what, text) => console.warn(`Netstorm: 도움말의 ${what} 은(는) 아직 지원하지 않습니다.`, text),
        });
        if (result.missing) console.warn(`Netstorm: 도움말 파일을 찾지 못했습니다: ${result.path}`);
        if (!result.window) return false;
        this._mainMenu.setDialog(result.window);
        this._displayDirty = true;
        return true;
    }

    // 대화상자에서 실행된 명령(버튼·목록 행·Timeout)을 처리한다. 원본처럼 먼저 대화상자를 닫고(OnExit 명령 실행) 명령을 실행한다.
    // `Tell,이름` 은 다음 대화상자를 열고, `DoNothing` 은 닫기만 한다. `Config,키=값` 은 설정을 쓰고 `TellTip,번호` 는 팁 창을 처리한다(2026-10-30 WB03-J).
    // 연결된 Tutorial의 MissionBegin은 현재 Guide로 연결한다. 그 밖의 미구현 명령은 안내만 남긴다.
    // 닫는 창이 Tell 창이면 그 창의 브리핑 단계를 읽어 두었다가 이어서 열리는 Tell 에 넘긴다(원본 UberGump 의 `Tell` 명령은 현재 단계가 Briefing 이면 다음 창도 Briefing 으로 연다). (2026-11-23 WB04-E1264)
    _onDialogCommand(command) {
        const dialog = this._mainMenu?.dialog ?? null;
        const briefing = dialog !== null && dialog === this._tellDialog && this._tellBriefing === true;
        const onExit = dialog ? dialog.close() : null;
        this._mainMenu?.setDialog(null);
        this._displayDirty = true;
        this._runDialogCommand(command, briefing);
        if (onExit) this._runDialogCommand(onExit, briefing);
    }

    // 대화상자 본문의 링크(Tell 의 `<a href>`)를 처리한다. 원본 HtmlWindowGump::Handle 의 tagEvent 처럼 대화상자를 먼저 닫고(모달 해제) 링크를 분배한 뒤,
    // 닫힌 창의 $OnExit 명령을 실행한다(원본은 닫힌 창을 정리할 때 OnExit 를 큐에 넣으므로 링크 처리보다 뒤다). (2026-10-31 WB03-K)
    _onDialogLink(destination) {
        const dialog = this._mainMenu?.dialog ?? null;
        const onExit = dialog ? dialog.close() : null;
        this._mainMenu?.setDialog(null);
        this._displayDirty = true;
        this._followLink(destination);
        if (onExit) this._runDialogCommand(onExit);
    }

    // 링크 목적지를 원본 규칙(ui/help-link.js)으로 분배한다: `cmd:` 는 스크립트 명령으로 실행하고, 도움말 주제는 도움말 창을 열며(원본 mode 1, 학습 0),
    // 바깥 주소(`http:`·`mailto:` 등)는 `_openExternalAddress` 가 판정해 열 수 있는 것만 브라우저 새 탭/창으로 연다(2026-11-07 WB03-AD, 그 전에는 콘솔 안내만). 접두·본문 한도를 넘으면 알리고 아무것도 하지 않는다. (2026-10-31 WB03-K)
    _followLink(destination) {
        try {
            dispatchHelpLink(destination, {
                script: (body) => {
                    const command = parseScriptCommand(body);
                    this._runDialogCommand({ name: command.name, argument: command.argument });
                },
                help: (topic) => this._openHelp(topic),
                external: (original) => this._openExternalAddress(original, '링크'),
            });
        } catch (error) {
            if (!(error instanceof RangeError)) throw error;
            console.warn(`Netstorm: 링크 목적지가 원본 한도(접두 80, 본문 256)를 넘어 따라가지 않습니다: ${destination}`);
        }
    }

    // 링크·도움말의 바깥 주소 하나를 처리한다(2026-11-05 사용자 결정: 원본의 셸 실행 대신 브라우저 새 탭/창으로 연다).
    // ui/external-link.js 가 열 수 있다고 판정한 주소(치환이 끝난 http/https 와 mailto)만 열고, 그 밖(file:·미치환 자리표시·호스트 없는 주소·다른 접두)은
    // 이유와 함께 콘솔로 알리기만 한다. 팝업 차단을 피하도록 링크를 누른 포인터 입력 처리 안에서 동기로 부르며, 열기에 실패해도 게임 진행은 계속한다.
    // 열기 방식은 options.openExternal(link, document) 로 바꿀 수 있고(무브라우저 실행·검사용), 기본은 platform/external-opener.js 이다.
    // 열었으면 true, 열지 않았거나 실패했으면 false 를 돌려준다. where 는 콘솔 안내에 붙는 출처 이름이다. (2026-11-07 WB03-AD)
    _openExternalAddress(original, where) {
        const link = resolveExternalLink(original);
        if (!link.open) {
            console.info(`Netstorm: ${where}의 바깥 주소 '${original}' 는 열지 않습니다(${link.reason})`);
            return false;
        }
        const opened = (this.options.openExternal ?? openExternalLink)(this._root?.ownerDocument ?? null, link);
        if (!opened) console.warn(`Netstorm: ${where}의 바깥 주소 '${original}' 를 열지 못했습니다.`);
        return opened === true;
    }

    // 닫힌 뒤 실행할 대화상자 명령 하나를 처리한다. (2026-10-28 WB03-D)
    // `briefing` 은 이 명령을 낸 창이 Briefing 단계였는지이며 `Tell` 명령이 다음 창에 그대로 넘긴다. (2026-11-23 WB04-E1264)
    _runDialogCommand(command, briefing = false) {
        const name = String(command.name).toLowerCase();
        if (name === 'tell') this._openTell(command.argument, { briefing });
        else if (name === 'config') this._runScriptCommand(command);
        else if (name === 'telltip') this._runTellTip(command);
        else if (name === 'missionbegin') this._beginCampaignFromDialog(command.argument);
        // 데모 선택 창(Demoville)의 항목이다: 같은 Guide 에 데모 표시를 켜고 시작한다. (2026-11-20 WB04-E1092)
        else if (name === 'startdemo') this._beginCampaignDemo(command.argument);
        // 미션 나가기 확인(ABORT)·완료 창의 버튼이다: LeaveBattle·MissionAbort 는 메인 메뉴로, MissionRestart 는 같은 미션 다시 시작이다. (2026-11-20 WB04-E1085)
        // MissionAbort 단추는 미션 중이면 원본처럼 명령 큐로 보낸다(인자 0 은 ABORT 확인 창 Main Menu/Replay Mission/Continue Mission, 그 밖은 LeaveBattle 예약). 이전에는 인자를 무시하고 곧바로 미션을 떠났다. (2026-11-22 WB04-E1231)
        else if (name === 'missionabort' && this._campaignWorld && this._clientInputQueue) { const argument = Number.parseInt(String(command.argument ?? '').trim(), 10) | 0; this._clientInputQueue.pushCommand(0x0200008c, argument, argument); }
        else if (name === 'leavebattle' || name === 'missionabort') this._leaveCampaignMission();
        else if (name === 'missionrestart') this._restartCampaignMission();
        // 안내 창 단추의 송금(SendAllBucks 등)·동맹(ConfirmAlliance 등) 이름은 원본 이름표로 명령 번호를 얻어 입력 큐에 올린다. 인자(대상 owner)는 정수로 읽어 두 인자에 같게 싣는다. (2026-11-20 WB04-E1126)
        else if (this._queueDialogPlayerCommand(command)) return;
        else if (name !== 'donothing') console.info(`Netstorm: 대화상자 명령 '${command.name}' 은(는) 아직 구현되지 않았습니다.`, command);
    }

    // 대화상자 단추 명령이 송금·동맹·회수(Salvage, 회수 경고 창의 `Salvage It` 단추, E1190)·지식(ShowTechnology·GetTechnology 등, E1227) 이름이면 명령 번호로 바꿔 입력 큐에 올리고 true 를 돌려준다. 그 밖의 이름은 false 이다. (2026-11-20 WB04-E1126)
    _queueDialogPlayerCommand(command) {
        const name = String(command.name), code = sendMoneyScriptCode(name) !== 0xffffffff ? sendMoneyScriptCode(name) : (['requestalliance', 'breakalliance', 'confirmalliance', 'declaredraw'].includes(name.toLowerCase()) ? gameMenuScriptCode(name) : name.toLowerCase() === 'salvage' ? SALVAGE_COMMAND : knowledgeButtonCode(name));
        if (code === 0xffffffff || !this._clientInputQueue) return false;
        // 지식 명령(ShowTechnology·GetTechnology 등)은 미션 월드가 있을 때만 큐에 올린다(원본은 월드 없이 받으면 오류). 월드가 없을 때 올리면 입력 루프가 돌지 않아 다음 미션 시작에 묵은 명령이 실행된다. (2026-11-22 WB04-E1227)
        if (knowledgeButtonCode(name) !== 0xffffffff && !this._campaignWorld) { console.warn(`Netstorm: 대화상자 명령 '${name}' 은(는) 미션 월드가 있을 때만 실행합니다.`); return true; }
        const target = Number.parseInt(String(command.argument ?? '').trim(), 10) | 0;
        this._clientInputQueue.pushCommand(code, target, target); return true;
    }

    // Tutorial 1~6과 연결된 캠페인 미션을 현재 Guide에 시작한다. 다른 미션과 기존 World 교체는 실제 수명 연결 전까지 거부한다.
    // 인자는 먼저 별칭을 풀어 쓴다: Priest Training 7 완료 안내의 Training Menu 단추가 가리키는 원본 오타 `BC1Menu` 는 bc1menu76 으로 연결한다. (2026-11-23 WB04-E1245)
    _beginCampaignFromDialog(argument) {
        const name = resolveMissionBeginName(argument);
        if (!/^tutorial[1-6]$/iu.test(name) && !['thewarbegins', 'masterofwhirligigs', 'savetheisland', 'fragilefortune', 'thunderingpower', 'dissolvedalliance', 'thenoose', 'rainvsrain', 'runforit', 'guardmyback', 'surrounded', 'breakingthrough', 'totherescue', 'vicious', 'enemyterritory', 'finalconfrontation', 'bc1menu76', 'bc2bridging76', 'bc3acid_bar76', 'bc4bow_rush76', 'bc5walkers76', 'bc6mows76', 'bc7sun_bars76', 'bc8fury76', 'bc9ssun_bars76'].includes(name.toLowerCase())) { console.info(`Netstorm: 미션 '${name}' 은(는) 아직 연결되지 않았습니다.`); return false; }
        if (!this._clientGuide || this._state !== AppState.MAIN_MENU) throw new Error('미션 시작의 현재 Guide·메뉴 수명 누락');
        // 현재 미션이 있으면 원본 MissionBegin 처럼 먼저 떠나고(Guide 닫기·월드 반납·메인 메뉴 복귀) 새 미션을 시작한다. (2026-11-20 WB04-E1085)
        if (this._campaignWorld || this._clientGuide.current()) this._leaveCampaignMission();
        this._markSkirmishRun(name);
        this._lastCampaignMission = { name, demo: 0, mode: 0 }; this._clientGuide.begin(name); return true;
    }

    // 메인 메뉴를 45초 동안 건드리지 않으면 Auto-Demo 옵션이 켜진 경우 DemoStormRages 데모를 시작한다(원본 `004cee80` 무활동 감시). 대화창·펼침 메뉴가 열려 있거나 포인터가 움직이면 기한을 다시 잡고, 미션 중에는 감시를 해제해 돌아올 때 새로 잡는다. (2026-11-20 WB04-E1092)
    _idleMainMenuActivity() {
        const menu = this._mainMenu, settings = this._settings, state = this._mainMenuActivity;
        if (!menu || !settings || this._campaignDisplayActive()) { state.armed = false; return; }
        const now = () => this._frameSeconds, here = this._input.pointer;
        if (!state.armed) { state.armed = true; state.deadline = now() + 45; state.pointer = { x: here.x, y: here.y }; }
        let demo = false;
        idleMainMenuActivity(state, () => settings.get('autoDemo') === 1, () => menu.dialog || this._settingsPanel ? 1 : 0, () => this._input.pointer, () => menu.dialog || this._settingsPanel ? 1 : 0, 0,
            { popup: () => Boolean(menu.popup?.isOpen), now, queue: () => { demo = true; }, kill: () => {} });
        // 큐 예약 뒤 같은 프레임에서 데모를 시작한다(원본은 입력 큐에 StartDemo 를 올린다). 재무장해 데모 뒤 메인 메뉴로 돌아오면 45초를 다시 센다.
        if (demo) { state.armed = false; this._beginCampaignDemo('DemoStormRages'); }
    }

    // 데모 미션(Demoville 의 DemoStormRages·GuardMyBack·DissolvedAlliance)을 데모 표시(1)로 시작한다(원본 `UberGump` 의 StartDemo → `BeginMission(이름, 1)`). 현재 미션이 있으면 먼저 떠난다. 연결하지 않은 이름은 안내만 남긴다. (2026-11-20 WB04-E1092)
    _beginCampaignDemo(argument) {
        const name = String(argument ?? '');
        if (!['demostormrages', 'guardmyback', 'dissolvedalliance'].includes(name.toLowerCase())) { console.info(`Netstorm: 데모 '${name}' 은(는) 아직 연결되지 않았습니다.`); return false; }
        if (!this._clientGuide || this._state !== AppState.MAIN_MENU) throw new Error('데모 시작의 현재 Guide·메뉴 수명 누락');
        if (this._campaignWorld || this._clientGuide.current()) this._leaveCampaignMission();
        this._markSkirmishRun(name);
        this._lastCampaignMission = { name, demo: 1, mode: 0 }; this._clientGuide.begin(name, 1); return true;
    }

    // 현재 단독 미션을 떠나 메인 메뉴로 돌아간다(원본 `LeaveBattle` 의 단독 미션 경로). Guide 미션을 닫고 HUD·월드·Session을 반납한 뒤 메인 메뉴 표면을 다시 그린다. 떠났으면 true 다. (2026-11-20 WB04-E1085)
    _leaveCampaignMission() {
        if (!this._campaignWorld && !this._clientGuide?.current()) return false;
        // 미션 관문이 안내 창 때문에 건 시계 정지(`gate.scriptPaused`)가 남은 채 떠나면 정지 깊이가 다음 미션까지 넘어가 새 미션의 시계가 멈춘 채 시작한다.
        // 원본 State 정리의 ResumeGame(정지 중이면 한 단계 풀기)과 같이 이 미션이 건 한 단계를 먼저 되돌린다. (2026-11-21 WB04-E1132)
        const gate = this._clientGuide?.current()?.runtime.gate;
        if (gate?.scriptPaused) { this._gameClock?.popGamePause(); gate.scriptPaused = 0; }
        this._closeCampaignPopups(); this._clientGuide?.close(); this._releaseCampaignWorld();
        this._mainMenu?.setDialog(null); this._ensureSurface(); if (this._mainMenu) this._mainMenu.dirty = true; this._displayDirty = true; this._applyCursor(); return true;
    }

    // 마지막으로 시작한 미션을 떠난 뒤 같은 이름·모드로 다시 시작한다(원본 `MissionRestart`/Reload). 기록이 없으면 아무것도 하지 않는다. 다시 시작했으면 true 다. (2026-11-20 WB04-E1085)
    _restartCampaignMission() {
        const last = this._lastCampaignMission; if (!last || !this._clientGuide) return false;
        this._mods?.end('restart');
        this._leaveCampaignMission(); this._clientGuide.begin(last.name, last.demo, last.mode, last.source ?? null); return true;
    }

    // 이미 받은 공개 자산으로 정적 지형을 연다. 플레이어/미션 상태·진행 저장·현재 미션 Config는 바꾸지 않는다.
    // 사용: 클릭하여 시작한 뒤 app.previewMap('savetheisland'). 실패 시 기존 화면을 유지하고 오류를 던진다. (2026-11-02 WB03-AA)
    previewMap(name) {
        if (![AppState.MAIN_MENU, AppState.TERRAIN_PREVIEW].includes(this._state) || !this._gameData || !this._resources) {
            throw new Error('지형 미리보기는 시작 입력 뒤에 열 수 있습니다.');
        }
        const scene = loadTerrainScene(this._gameData.files, name);
        const preview = new TerrainPreview(scene, this._resources, this.options.language);
        preview.resize(this._layout.logicalWidth, this._layout.logicalHeight);
        this._mainMenu?.cancel();
        this._terrainPreview = preview;
        this._setState(AppState.TERRAIN_PREVIEW);
        this._root.ownerDocument.title = `NetStorm - ${scene.name}`;
        this._displayDirty = true;
        return preview;
    }

    // 개발용 지형 화면에서 메뉴로 돌아온다. 시작 환영/팁 흐름을 다시 실행하지 않는다. (2026-11-02 WB03-AA)
    closeTerrainPreview() {
        if (!this._terrainPreview) return false;
        this._terrainPreview.cancel();
        this._terrainPreview = null;
        this._input.keys.clear();
        this._setState(AppState.MAIN_MENU);
        this._root.ownerDocument.title = this._resources.translate(MAIN_MENU_WINDOW_TITLE);
        if (this._mainMenu) this._mainMenu.dirty = true;
        this._displayDirty = true;
        return true;
    }

    // 논리 화면 크기의 8비트 표면을 준비한다. 크기가 바뀌면 새로 만들고 메인 메뉴를 다시 배치한다. (2026-10-23 WB03-M1)
    _ensureSurface() {
        const layout = this._layout;
        if (!layout) return;
        const width = layout.logicalWidth;
        const height = layout.logicalHeight;
        if (!this._surface || this._surface.width !== width || this._surface.height !== height) {
            this._surface = new IndexedSurface(width, height);
            this._mainMenu?.resize(width, height);
            this._displayDirty = true;
        } else if (this._mainMenu && (this._mainMenu.width !== width || this._mainMenu.height !== height)) {
            this._mainMenu.resize(width, height);
        }
        this._terrainPreview?.resize(width, height);
    }

    // 루트 요소가 속한 문서의 window 를 돌려준다. iframe 안에서도 그 문서의 프레임 예약·픽셀 비율·오디오를 쓰기 위해서다.
    // 루트가 없으면(정리 후) 전역 객체를 돌려준다.
    _view() {
        return this._root?.ownerDocument.defaultView ?? globalThis;
    }

    // ----- 오디오 -----

    // 원본 현재 소리 화면은 카메라 snapshot과 현재 미션 지도 영역을 사용한다.
    _campaignSoundView() {
        const screen = this._clientPaletteScreen, gameplay = this._clientGameplay;
        if (!screen || !gameplay) throw new Error('미션 음향의 현재 화면·카메라 수명 누락');
        const pixels = gameplay.camera().snapshot().pixels, clip = this._campaignWorld?.mapView() ?? { left: 0, top: 0, right: screen.width, bottom: screen.height };
        return { cameraX: pixels.x, cameraY: pixels.y, left: clip.left, top: clip.top, right: clip.right, bottom: clip.bottom };
    }

    // 같은 재생 전역에 현재 효과음 옵션과 전체 음량을 적용한다.
    _syncCampaignSoundSettings() {
        const state = this._clientSoundState, player = this._clientSoundPlayer, settings = this._settings;
        if (!state || !player || !settings) return;
        state.enabled = Boolean(state.device && settings.get('sound') === 1);
        state.swapSpeakers = settings.get('swapLeftRightSpeakers') === 1;
        state.maxPlaying = this._gameData?.readInt('maxSimulSounds', 8).value ?? 8;
        player.setMasterVolume(volumeStepToCentibels(settings.get('soundVolume')));
    }

    // 현재 Client에 실제 또는 비활성 효과음 장치를 한 번 준비하고 동시 요청은 공유한다.
    async _prepareCampaignAudio() {
        if (this._clientSoundPlayer) return this._clientSoundPlayer;
        if (this._clientSoundPrepare) return this._clientSoundPrepare;
        const audio = this._audio, names = this._clientSoundNames;
        if (!names || !this._gameClock || !this._settings) throw new Error('미션 효과음의 실제 Client 수명 누락');
        const task = (async () => {
            if (audio && this.options.audio) await this._prepareMissionSounds();
            if (this._audio !== audio || this._clientSoundNames !== names || !this._gameClock) throw new Error('미션 효과음 준비 중 Client 수명 변경');
            const state = createSoundState(); state.currentView = () => this._campaignSoundView();
            let device = null, hooks;
            if (audio && this.options.audio) {
                device = new WebSoundDevice(audio, this._missionAudioLoader()); hooks = device.hooks(); state.initialized = state.device = true;
            } else {
                // 오디오 미지원 또는 사용자가 오디오를 끈 경우 원본 noAudio 장치처럼 재생을 비활성화한다.
                const unavailable = () => { throw new Error('비활성 효과음 장치 호출'); };
                hooks = { status: unavailable, play: unavailable, setPosition: unavailable, setVolume: unavailable, setPan: unavailable, stop: unavailable,
                    duplicate: () => ({ result: -1, buffer: 0 }), load: () => ({ buffer: 0, attenuation: 0 }) };
            }
            const player = new SoundPlayer(names, state, hooks, (expression, line) => console.error(`소리 진단 ${line}: ${expression}`));
            this._clientSoundState = state; this._clientSoundDevice = device; this._clientSoundPlayer = player; this._syncCampaignSoundSettings();
            return player;
        })();
        this._clientSoundPrepare = task;
        try { return await task; } finally { if (this._clientSoundPrepare === task) this._clientSoundPrepare = null; }
    }

    // 현재 매니페스트와 활성 재생기를 빌려 필요한 미션 음향 하나만 준비한다.
    _missionAudioLoader() {
        if (!this._assetLoader || !this._audio || this._audio.closed) throw new Error('미션 음향의 실제 자산·재생기 수명 누락');
        if (!this._missionAudioAssets) this._missionAudioAssets = new MissionAudioAssets(this._assetLoader, this._audio, { language: this.options.language });
        return this._missionAudioAssets;
    }

    // 미션 효과음의 현재 원본 파일을 비동기로 받고 해독한다.
    _prepareMissionSound(name) { return this._missionAudioLoader().loadSound(name); }
    // 미션 시작 전 현재 언어의 효과음을 제한된 요청으로 준비한다.
    _prepareMissionSounds(options) { return this._missionAudioLoader().preloadSounds(options); }
    // 선택한 미션 음악 한 곡만 비동기로 받고 해독한다.
    _prepareMissionMusic(name) { return this._missionAudioLoader().loadMusic(name); }

    // 대기 중인 Fort 요청의 음향을 한 번 준비하고 현재 요청의 실패만 오류 화면에 기록한다.
    _beginCampaignAudioForRequest(guide) {
        if (this._clientSoundPlayer || this._clientSoundPrepare) return;
        const generation = this._loadGeneration, request = guide.fortRequest();
        this._prepareCampaignAudio().catch((error) => {
            if (generation !== this._loadGeneration || this._clientGuide !== guide || guide.fortRequest() !== request) return;
            reportGameError('mission-audio', error, { state: this._state, fort: request?.fort });
            this._loadError = error; this._setState(AppState.ERROR); this._displayDirty = true;
        });
    }

    // AudioContext 와 재생기(AudioSystem)를 만들고 재생 재개를 요청한다. 사용자 입력 처리 안에서 불러야 한다.
    // 문맥은 원본 표본 빈도(22,050Hz)로 만들고, 받은 효과음·음악을 해독해 둔다.
    // 지원하지 않거나 실패해도 게임은 계속되며 상태 문자열로만 알린다. 메인 메뉴에서 다음 입력 때 다시 시도한다.
    // (2026-10-23 WB01, 2026-10-23 WB03-M1: 재생기·해독 연결)
    _unlockAudio() {
        if (!this.options.audio) {
            this._audioStatus = 'disabled';
            return;
        }
        try {
            if (!this._audioContext) {
                const context = createAudioContext(this._view());
                if (!context) {
                    this._audioStatus = 'unsupported';
                    return;
                }
                this._audioContext = context;
                this._audio = new AudioSystem(context, { soundVolumeStep: this._settings.get('soundVolume'), musicVolumeStep: this._settings.get('musicVolume') });
                this._applySettings();
                this._prepareSounds();
                this._prepareMusic();
            }
            const context = this._audioContext;
            this._audioStatus = context.state;
            if (context.state !== 'running') {
                context
                    .resume()
                    .then(() => {
                        if (this._audioContext === context) this._audioStatus = context.state;
                    })
                    .catch((error) => {
                        if (this._audioContext !== context) return;
                        this._audioStatus = 'failed';
                        reportGameError('audio-unlock', error, { state: this._state });
                    });
            }
        } catch (error) {
            this._audioStatus = 'failed';
            reportGameError('audio-unlock', error, { state: this._state });
        }
    }

    // 받은 버튼 효과음을 현재 재생기에 해독해 등록한다. 실패하면 소리 없이 계속한다. (2026-10-23 WB03-M1)
    _prepareSounds() {
        const audio = this._audio;
        if (!audio) return;
        if (this._buttonSoundBytes) {
            audio.loadSound(BUTTON_SOUND_NAME, this._buttonSoundBytes).catch((error) => {
                if (this._audio === audio) reportGameError('audio-decode', error, { state: this._state, asset: BUTTON_SOUND_NAME });
            });
        }
        if (this._menuSoundBytes) {
            audio.loadSound(MENU_SOUND_NAME, this._menuSoundBytes).catch((error) => {
                if (this._audio === audio) reportGameError('audio-decode', error, { state: this._state, asset: MENU_SOUND_NAME });
            });
        }
    }

    // 받은 메뉴 음악을 현재 재생기에 해독한다. 재생기와 음악 바이트가 모두 있을 때만 진행하며, 끝나면 메인 메뉴에서 재생한다.
    // (2026-10-23 WB03-M1)
    _prepareMusic() {
        const audio = this._audio;
        if (!audio || !this._musicBytes || this._musicStatus !== 'loaded') return;
        this._musicStatus = 'decoding';
        audio
            .loadMusic(MENU_MUSIC_NAME, this._musicBytes)
            .then(() => {
                if (this._audio !== audio) return;
                this._musicStatus = 'ready';
                this._playMenuMusic();
            })
            .catch((error) => {
                if (this._audio !== audio) return;
                this._musicStatus = 'failed';
                reportGameError('audio-music', error, { state: this._state, asset: MENU_MUSIC_NAME });
            });
    }

    // 미션 State7 에서 장면 음악 구동기를 만들고 원소 곡 순환을 시작한다(원본 StartScene(true)). 재생기가 없거나 오디오를 끈 실행이면 아무 일도 없다.
    // 첫 곡 선택 난수는 원본처럼 공유 게임 난수(Client 난수 전역의 `game`)에서 한 번 뽑는다. 이전(E1119)에는 월드 난수열을 건드리지 않으려고 Math.random 을 썼지만, 원본 StartScene(true)이 게임 난수를 한 번 소비하므로
    // 그렇게 하면 미션 이후의 난수열(게임 난수 모드 0 은 고정 시드)이 원본과 한 칸 어긋난다. 곡 받기 실패는 오류로 기록하되 게임은 계속한다. (2026-11-20 WB04-E1119, 2026-11-23 WB04-E1258: 공유 게임 난수로 교체)
    _beginSceneMusic() {
        const audio = this._audio;
        if (!audio || audio.closed || !this.options.audio || !this._gameClock || !this._assetLoader) return;
        this._endSceneMusic();
        const loader = this._missionAudioLoader();
        this._sceneMusic = new SceneMusicDriver({
            audio, prepare: (name) => loader.loadMusic(name), release: (name) => loader.releaseMusic(name),
            wallSeconds: () => this._gameClock.wallSeconds(), random: { next: (limit) => { const random = this._clientRandom; if (!random) throw new Error('장면 음악의 공유 게임 난수 수명 누락'); return random.game.next(limit); } },
            // 날씨(천둥) 효과음은 원본 MakeSceneMusicHooks 처럼 SoundPlayer::PlayByName(이름, 0,0,0,0,0)을 거친다. 그래야 게임 메뉴 음소거(PushMute)·소리 옵션·동시 재생 한도가 같이 적용된다. (2026-11-23 WB04-E1239)
            weatherSound: (name) => { try { this._clientSoundPlayer?.playByName(name, 0, 0, 0, 0, 0); } catch (error) { if (this._audio === audio) reportGameError('scene-music', error, { state: this._state, asset: name }); } },
            // 원본 MakeSceneMusicHooks(00469c80)처럼 천둥 효과음 앞에 번개 프로세스를 생성한다. (2026-11-24 WB04-E1329)
            thunderFlash: () => { this._startThunderFlash(0); },
            onError: (error, name) => { if (this._audio === audio) reportGameError('scene-music', error, { state: this._state, asset: name }); },
        });
        this._sceneMusic.begin();
    }

    // 미션을 떠날 때 장면 음악 구동기를 끝내고 메뉴 곡으로 돌아간 뒤 비운다. 시작하지 않았으면 아무 일도 없다. (2026-11-20 WB04-E1119)
    _endSceneMusic() {
        const driver = this._sceneMusic;
        if (!driver) return;
        this._sceneMusic = null;
        driver.end();
    }

    // 메인 메뉴이고 음악이 해독돼 있으면 메뉴 곡(ser22.mus)을 반복 재생한다(원본 SceneMusic Start: 전투 밖 곡). (2026-10-23 WB03-M1)
    _playMenuMusic() {
        // 미션 장면 음악이 도는 동안에는 메뉴 곡을 다시 틀지 않는다(미션 중에도 상태는 메인 메뉴다). (2026-11-20 WB04-E1119)
        if (this._state !== AppState.MAIN_MENU || !this._audio || this._sceneMusic?.active) return;
        if (this._musicStatus !== 'ready' && this._musicStatus !== 'playing') return;
        if (this._audio.playMusic(MENU_MUSIC_NAME)) this._musicStatus = 'playing';
    }

    // ----- 스타일과 배치 -----

    // 루트 요소의 스타일을 정한다. 기존 인라인 값은 저장해 destroy 에서 되돌린다. CSS 파일은 쓰지 않는다.
    // fillWindow 면 루트가 브라우저 창 전체를 채우고, 아니면 호스트가 정한 크기를 따르되 Canvas 의 절대 배치 기준이 되게 한다.
    _applyRootStyle() {
        const root = this._root;
        const style = root.style;
        this._savedRootStyle = {};
        // 되돌릴 속성의 현재 인라인 값을 모두 저장한다.
        for (const key of ROOT_STYLE_KEYS) this._savedRootStyle[key] = style[key];
        if (this.options.fillWindow) {
            style.position = 'fixed';
            style.left = '0';
            style.top = '0';
            style.right = '0';
            style.bottom = '0';
        } else if (!style.position && root.ownerDocument.defaultView?.getComputedStyle(root).position === 'static') {
            style.position = 'relative';
        }
        style.overflow = 'hidden';
        style.background = '#000';
        style.userSelect = 'none';
        style.webkitUserSelect = 'none';
    }

    // 저장해 둔 루트 인라인 스타일을 되돌린다.
    _restoreRootStyle() {
        if (!this._root || !this._savedRootStyle) return;
        // 저장한 속성을 모두 원래 값으로 되돌린다.
        for (const key of ROOT_STYLE_KEYS) this._root.style[key] = this._savedRootStyle[key];
        this._savedRootStyle = null;
    }

    // Canvas 요소의 고정 스타일을 정한다. 위치와 크기는 _applyLayout 이 계산해 덮어쓴다.
    // 터치 스크롤을 막고, 확대 시 픽셀이 뭉개지지 않게 하며, 키보드 입력을 받도록 포커스를 가능하게 한다.
    _applyCanvasStyle() {
        const canvas = this._canvas;
        const style = canvas.style;
        style.position = 'absolute';
        style.display = 'block';
        style.margin = '0';
        style.padding = '0';
        style.border = '0';
        style.outline = 'none';
        style.touchAction = 'none';
        style.imageRendering = 'pixelated';
        style.visibility = 'hidden';
        canvas.tabIndex = 0;
    }

    // 루트 크기에서 Canvas 배치를 다시 계산해 적용한다. 바뀐 부분만 DOM 에 쓰고, 바뀌면 곧바로 한 번 그려 깜박임을 줄인다.
    _applyLayout() {
        const root = this._root;
        const canvas = this._canvas;
        if (!root || !canvas) return;
        const view = this._view();
        const dpr = view.devicePixelRatio || 1;
        const next = computeLayout(root.clientWidth, root.clientHeight, dpr, this._aspect, this.options.maxCanvasPixels);
        const previous = this._layout;
        this._layout = next;
        if (!previous || previous.canvasWidth !== next.canvasWidth || previous.canvasHeight !== next.canvasHeight) {
            // Canvas 내부 크기를 바꾸면 내용이 지워지고 컨텍스트 상태도 초기화된다.
            canvas.width = next.canvasWidth;
            canvas.height = next.canvasHeight;
        }
        const style = canvas.style;
        style.width = `${next.cssWidth}px`;
        style.height = `${next.cssHeight}px`;
        style.left = `${next.left}px`;
        style.top = `${next.top}px`;
        style.visibility = next.visible ? 'visible' : 'hidden';
        this._gameMenuButton?.layout(next);
        this._displayDirty = true;
        if (this._state === AppState.MAIN_MENU || this._state === AppState.TERRAIN_PREVIEW) this._ensureSurface();
        if (this._clientPaletteScreen) this._resizeClientPaletteScreen(next.logicalWidth, next.logicalHeight);
        this._clientGump?.resize(next.logicalWidth, next.logicalHeight);
        // HUD가 있으면 구분선을 재배치해 지도·생산 패널·미니맵을 같은 도킹으로 다시 맞추고, 없으면 지도 영역은 전체 화면이다. (2026-11-20 WB04-E1082)
        if (this._campaignProduction?.hud) this._campaignProduction.hud.reflow(next.logicalWidth, next.logicalHeight);
        else if (this._clientGump) Object.assign(this._clientGump.mapView(), { left: 0, top: 0, right: next.logicalWidth, bottom: next.logicalHeight });
        this._campaignWorld?.resize(next.logicalWidth, next.logicalHeight);
        this._applyCursor();
        if (next.visible) this._render(view.performance?.now?.() ?? 0);
    }

    // 포인터 아래에서 보일 원본 커서 번호를 고른다. 메인 메뉴에서 포인터가 게임 화면 안에 있으면 메뉴(열린 대화상자의 본문)가 고르고(링크 위 16, 스크롤 가능한 본문 위 15),
    // 그 밖은 기본 커서(1)이다. 고른 번호의 커서 자원이 묶음에 없으면 기본 커서로 대신한다. 원본은 매 포인터 갱신마다 이 선택을 다시 한다(NotifyPointer 의 Overlay). (2026-10-31 WB03-L)
    _wantedCursor() {
        const pointer = this._input.pointer;
        if (this._state !== AppState.MAIN_MENU || !this._mainMenu || !this._resources || !pointer.inside) return CURSOR_NORMAL;
        if (this._campaignDisplayActive() && !this._mainMenu.dialog) return this._resources.cursors[this._campaignCursor] ? this._campaignCursor : CURSOR_NORMAL;
        const number = this._mainMenu.cursorAt(pointer.x, pointer.y);
        return this._resources.cursors[number] ? number : CURSOR_NORMAL;
    }

    // 원본 커서를 현재 화면 배율에 맞춰 Canvas 에 적용한다. 어떤 번호의 커서를 쓸지는 _wantedCursor 가 고른다.
    // 자원이 없거나 번호·배율·픽셀 비율이 지난번과 같으면 아무것도 하지 않는다. 매 프레임(_onFrame: 포인터 이동·스크롤·창 열림/닫힘으로 포인터 아래 내용이 바뀜)과 화면 배치를 바꿀 때 부른다.
    // (2026-10-23 WB03-M1, 2026-10-31 WB03-L: 번호별 커서와 캐시 추가)
    _applyCursor() {
        const layout = this._layout;
        if (!this._canvas || !this._resources || !layout?.visible) return;
        const number = this._wantedCursor();
        const scale = Math.max(1, Math.round(layout.scale));
        const sizeKey = `${scale}:${layout.devicePixelRatio}`;
        const key = `${number}:${sizeKey}`;
        if (key === this._cursorKey) return;
        // 배율·픽셀 비율이 바뀌면 이전 배율로 만든 커서 그림은 버린다.
        if (sizeKey !== this._cursorSizeKey) {
            this._cursorCache.clear();
            this._cursorSizeKey = sizeKey;
        }
        this._cursorKey = key;
        this._cursorNumber = number;
        try {
            let css = this._cursorCache.get(number);
            if (!css) {
                css = buildCursorCss(this._root.ownerDocument, this._resources.cursors[number], layout.scale, layout.devicePixelRatio);
                this._cursorCache.set(number, css);
            }
            applyCursorStyle(this._canvas, css);
        } catch (error) {
            // 커서 그림을 만들 수 없는 환경이면 기본 커서를 그대로 쓴다.
            console.warn('Netstorm: 원본 커서를 적용하지 못했습니다.', error);
        }
    }

    // ----- 이벤트 -----

    // 대상에 리스너를 붙인다. destroy 의 abort 로 한꺼번에 해제되도록 signal 을 함께 넘긴다.
    _listen(target, type, handler, options = {}) {
        const app = this;
        // 입력 처리에서 생긴 예외에 이벤트 종류와 앱 상태를 더한 뒤 브라우저에 원래 예외를 전달한다. (2026-11-19 WB04-E862)
        function onEvent(event) {
            try {
                return handler.call(this, event);
            } catch (error) {
                reportGameError('event', error, { state: app._state, eventType: type });
                throw error;
            }
        }
        target.addEventListener(type, onEvent, { ...options, signal: this._abort.signal });
    }

    // 입력·크기·가시성 이벤트를 연결한다.
    _bindEvents() {
        const canvas = this._canvas;
        const doc = this._root.ownerDocument;
        const view = doc.defaultView;

        this._listen(canvas, 'pointerdown', (event) => this._onPointerDown(event));
        this._listen(canvas, 'pointermove', (event) => this._onPointerMove(event));
        this._listen(canvas, 'pointerup', (event) => this._onPointerUp(event));
        this._listen(canvas, 'pointercancel', (event) => this._onPointerCancel(event));
        this._listen(canvas, 'pointerleave', (event) => this._onPointerLeave(event));
        // 시작·재시도 입력은 오디오 정책을 모든 포인터 종류에서 통과하도록 click 으로 받는다.
        this._listen(canvas, 'click', (event) => this._onClick(event));
        // 우클릭은 게임 입력이므로 브라우저 메뉴를 막는다.
        this._listen(canvas, 'contextmenu', (event) => event.preventDefault());
        this._listen(canvas, 'keydown', (event) => this._onKeyDown(event));
        this._listen(canvas, 'keyup', (event) => this._onKeyUp(event));
        // 확정 전 조합 키는 큐에 넣지 않고 조합 종료 본문만 원자적으로 예약한다.
        this._listen(canvas, 'compositionstart', () => this._onCompositionStart());
        this._listen(canvas, 'compositionend', (event) => this._onCompositionEnd(event));
        // 마우스 휠은 열린 도움말 창 본문을 스크롤한다(웹 적응). 도움말이 열려 있을 때만 페이지 스크롤을 막는다. (2026-10-30 WB03-G)
        this._listen(canvas, 'wheel', (event) => this._onWheel(event), { passive: false });
        // IME 요소가 없는 때의 relatedTarget=null도 실제 포커스 해제이므로 누름을 정리한다.
        this._listen(canvas, 'blur', (event) => { if (!this._clientImeElement || event.relatedTarget !== this._clientImeElement) this._clearInput(); });
        this._listen(doc, 'visibilitychange', () => this._onVisibilityChange());
        // 붙여넣기: 채팅 입력줄·Send X 숫자 창이 활성이면 클립보드 본문을 확정 문자로 입력줄에 넣는다. (2026-11-22 WB04-E1175)
        this._listen(doc, 'paste', (event) => this._onPaste(event));

        // 비동기 크기 관찰 오류는 현재 상태와 함께 보고한 뒤 브라우저에 전달한다. (2026-11-19 WB04-E944)
        if (typeof view?.ResizeObserver === 'function') {
            this._resizeObserver = new view.ResizeObserver(() => {
                try {
                    this._applyLayout();
                } catch (error) {
                    reportGameError('layout', error, { state: this._state, source: 'resize-observer' });
                    throw error;
                }
            });
            this._resizeObserver.observe(this._root);
        } else if (view) {
            this._listen(view, 'resize', () => this._applyLayout());
        }
        this._watchPixelRatio();
    }

    // 기기 픽셀 비율(화면 배율·창을 옮긴 모니터)이 바뀌면 배치를 다시 계산하도록 감시한다.
    // 한 번 바뀌면 새 비율로 감시를 다시 건다. 지원하지 않는 환경에서는 아무것도 하지 않는다.
    _watchPixelRatio() {
        const view = this._root?.ownerDocument.defaultView;
        if (typeof view?.matchMedia !== 'function' || !this._abort) return;
        const query = view.matchMedia(`(resolution: ${view.devicePixelRatio || 1}dppx)`);
        this._listen(
            query,
            'change',
            () => {
                this._applyLayout();
                this._watchPixelRatio();
            },
            { once: true },
        );
    }

    // 실제 문자 편집이 활성화될 때만 IME가 포커스할 수 있는 입력 요소를 만든다.
    _activateClientTextInput(x, y) {
        if (!this._root || !this._canvas || !this._layout?.visible || !this._clientNativeInputEnabled || !Number.isFinite(x) || !Number.isFinite(y)) throw new Error('Client IME 입력의 활성 화면·좌표 누락');
        let element = this._clientImeElement;
        if (!element) {
            element = this._root.ownerDocument.createElement('textarea'); this._clientImeElement = element;
            element.setAttribute('aria-label', this.options.language === 'japanese' ? '文字入力' : this.options.language === 'korean' ? '문자 입력' : 'Text input'); element.setAttribute('autocomplete', 'off'); element.setAttribute('autocapitalize', 'off');
            element.spellcheck = false; element.tabIndex = -1;
            Object.assign(element.style, { position: 'fixed', width: '1px', height: '1px', opacity: '0', padding: '0', border: '0', resize: 'none', overflow: 'hidden', pointerEvents: 'none' });
            this._listen(element, 'keydown', (event) => this._onKeyDown(event)); this._listen(element, 'keyup', (event) => this._onKeyUp(event));
            this._listen(element, 'compositionstart', () => this._onCompositionStart());
            this._listen(element, 'compositionend', (event) => { this._onCompositionEnd(event); element.value = ''; });
            this._listen(element, 'beforeinput', (event) => this._onClientBeforeInput(event));
            this._listen(element, 'input', () => { if (!this._clientComposing) element.value = ''; });
            this._listen(element, 'blur', (event) => { if (event.relatedTarget !== this._canvas) this._clearInput(); });
            this._root.appendChild(element);
        }
        const rect = this._canvas.getBoundingClientRect();
        element.style.left = `${rect.left + x * rect.width / this._layout.logicalWidth}px`;
        element.style.top = `${rect.top + y * rect.height / this._layout.logicalHeight}px`;
        element.focus({ preventScroll: true }); return element;
    }

    // 편집 종료는 미완성 조합을 버리고 기존 Canvas 포커스로 돌아간다.
    _deactivateClientTextInput() {
        const element = this._clientImeElement; if (!element) return;
        this._clientComposing = false; this._clientImePending = null; element.value = ''; this._canvas?.focus({ preventScroll: true }); element.remove(); this._clientImeElement = null;
    }

    // 포인터 이벤트로 입력 상태의 포인터 정보를 갱신한다. 논리 화면과 맞지 않으면 갱신하지 않는다.
    _updatePointer(event) {
        const layout = this._layout;
        if (!layout?.visible) return;
        const point = clientToLogical(
            event.clientX,
            event.clientY,
            this._canvas.getBoundingClientRect(),
            layout.logicalWidth,
            layout.logicalHeight,
        );
        if (!point) return;
        const pointer = this._input.pointer;
        pointer.x = point.x;
        pointer.y = point.y;
        pointer.inside = point.inside;
        pointer.buttons = event.buttons ?? 0;
        pointer.type = event.pointerType || 'mouse';
    }

    // 버튼을 눌렀을 때: 포커스를 받고, 화면 밖으로 끌어도 이벤트를 받도록 포인터를 붙잡는다.
    // 메인 메뉴에서는 왼쪽 버튼 누름만 돌 버튼에 전달한다(원본: 오른쪽·가운데 버튼은 반응 없음). 게임 화면 밖(레터박스) 누름은 무시한다.
    // (2026-10-23 WB01, 2026-10-23 WB03-M1: 메인 메뉴 입력 연결)
    _onPointerDown(event) {
        this._canvas.focus({ preventScroll: true });
        try {
            this._canvas.setPointerCapture(event.pointerId);
        } catch (error) {
            // 포인터 붙잡기에 실패해도 입력 처리는 계속한다.
        }
        this._updatePointer(event);
        this._clientInputPoll.recordMouseDown(event.button);
        if (this._clientNativeInputEnabled && this._layout?.visible) queueBrowserMouseEvent(this._clientInputQueue, event, this._input.pointer, false);
        this._retryAudioOnGesture();
        const pointer = this._input.pointer;
        if (this._state === AppState.MAIN_MENU && this._mainMenu && (!this._campaignDisplayActive() || this._mainMenu.dialog) && event.button === 0 && pointer.inside) {
            this._mainMenu.pointerDown(pointer.x, pointer.y);
        }
        // 전체 원본 입력 소비자 연결 전에는 미션 좌클릭을 같은 큐에 예약한다.
        if (!this._clientNativeInputEnabled && this._campaignDisplayActive() && !this._mainMenu?.dialog && this._campaignUserInput && (event.button === 0 || event.button === 2) && pointer.inside) {
            queueBrowserMouseEvent(this._clientInputQueue, event, pointer, false);
        }
        if (this._state === AppState.TERRAIN_PREVIEW && event.button === 0 && pointer.inside) this._terrainPreview?.pointerDown(pointer.x, pointer.y);
    }

    // 포인터가 움직였을 때 좌표를 갱신하고, 메인 메뉴의 붙잡힌 버튼 모양을 갱신한다(원본 Broadcast). 포인터 아래 커서 모양은 다음 프레임의 _onFrame 이 고른다. (원본도 매 갱신마다 선택한다)
    _onPointerMove(event) {
        this._updatePointer(event);
        if (this._state === AppState.MAIN_MENU && this._mainMenu && (!this._campaignDisplayActive() || this._mainMenu.dialog)) this._mainMenu.pointerMove(this._input.pointer.x, this._input.pointer.y);
        if (this._state === AppState.TERRAIN_PREVIEW) this._terrainPreview?.pointerMove(this._input.pointer.x, this._input.pointer.y);
    }

    // 버튼을 뗐을 때 버튼 상태를 갱신하고, 메인 메뉴에서는 왼쪽 버튼 뗌을 돌 버튼에 전달한다(안쪽에서 떼면 실행).
    _onPointerUp(event) {
        this._updatePointer(event);
        this._input.pointer.buttons = event.buttons ?? 0;
        if (this._clientNativeInputEnabled && this._layout?.visible) queueBrowserMouseEvent(this._clientInputQueue, event, this._input.pointer, true);
        if (this._state === AppState.MAIN_MENU && this._mainMenu && (!this._campaignDisplayActive() || this._mainMenu.dialog) && event.button === 0) {
            this._mainMenu.pointerUp(this._input.pointer.x, this._input.pointer.y);
        }
        if (!this._clientNativeInputEnabled && this._campaignDisplayActive() && !this._mainMenu?.dialog && this._campaignUserInput && (event.button === 0 || event.button === 2)) queueBrowserMouseEvent(this._clientInputQueue, event, this._input.pointer, true);
        if (event.button === 0) this._terrainPreview?.cancel();
    }

    // 브라우저가 포인터 입력을 취소했을 때(터치 제스처 등) 버튼 상태를 비우고 메뉴 누름을 실행 없이 취소한다. (2026-10-23 WB03-M1)
    _onPointerCancel(event) {
        this._updatePointer(event);
        this._input.pointer.buttons = event.buttons ?? 0;
        this._mainMenu?.cancel();
        this._terrainPreview?.cancel();
    }

    // 포인터가 Canvas 를 벗어났을 때 안쪽 표시를 끈다. 가장자리 스크롤 등은 이 값을 기준으로 멈춘다.
    _onPointerLeave(event) {
        this._input.pointer.inside = false;
        this._input.pointer.buttons = event.buttons ?? 0;
        // 펼침 메뉴의 행 강조는 포인터를 따라가므로 Canvas 밖으로 나가면 지운다. (2026-10-29 WB03-F)
        if (this._mainMenu?.popup?.pointerLeave()) this._mainMenu.dirty = true;
    }

    // 왼쪽 클릭을 시작·재시도 입력으로 처리한다.
    _onClick(event) {
        if (event.button === 0) this._activate();
    }

    // ESC 로 열린 Tell 창을 닫는다(원본 UberGump.cpp:821 은 브리핑이 아닌 모든 Tell 을 Dialog 단계로 두고 :1810~1823 은 Briefing·LoadingMission 이면 ESC 를 무시, Dialog 면 `Post{"Tell","Blank"}` 로 창을 닫는다).
    // 닫을 대상은 `_openTell` 이 연 Tell 창이면서 브리핑 단계가 아닌 창뿐이다(미션 로딩 창·도움말 창은 다른 경로라 대상이 아니다). 펼침 메뉴가 열려 있으면 그쪽이 먼저 닫히므로 건드리지 않는다. 닫았으면 true. 사용: if (this._escapeCloseTell()) return;
    // 닫는 길은 `Tell,Blank` 단추와 같은 `_onDialogCommand` 라서 창 종료 명령(OnExit)과 Reset All 확인 해제도 그대로 따른다. (2026-11-23 WB04-E1264)
    _escapeCloseTell() {
        const dialog = this._mainMenu?.dialog ?? null;
        if (!dialog || dialog !== this._tellDialog || this._tellBriefing === true || this._mainMenu?.popup) return false;
        this._onDialogCommand({ name: 'Tell', argument: 'Blank' });
        return true;
    }

    // 키를 눌렀을 때: 눌린 키 집합에 넣고, 게임이 쓰는 스크롤 키의 브라우저 기본 동작을 막고, Enter/Space 는 시작 입력으로 쓴다.
    _onKeyDown(event) {
        this._clientInputPoll.recordKeyDown(event.code, this._input.keys.has(event.code) || event.repeat);
        this._input.keys.add(event.code);
        // 시작 title 그림 중에는 아무 키나 누르면 남은 그림을 건너뛴다(자동 반복은 무시). (2026-11-22 WB04-E1184)
        if (this._state === AppState.TITLE) { if (!event.repeat) this._finishTitleScreens(); return; }
        // 원본 UberGump 의 ESC: 브리핑·미션 로딩 단계가 아닌 Tell 창(Dialog 단계)이 열려 있으면 `Tell Blank` 로 닫는다. 이 키는 월드 입력 큐로 보내지 않는다. (2026-11-23 WB04-E1264)
        if (event.code === 'Escape' && !event.repeat && this._escapeCloseTell()) { event.preventDefault(); this._retryAudioOnGesture(); return; }
        // Gump 입력 창(Send X 숫자 창)이 열려 있으면 모든 키를 그 창의 입력으로 보낸다(게임 단축키·TAB·숫자 화면 저장으로 새지 않는다). (2026-11-22 WB04-E1168)
        if (!this._clientNativeInputEnabled && this._campaignInputDialogOpen()) { this._queueCampaignDialogKey(event, false); this._retryAudioOnGesture(); return; }
        // 채팅 입력줄이 보이는 동안 문자 키는 입력줄로 간다(게임 단축키로 새지 않는다). (2026-11-22 WB04-E1170)
        if (!this._clientNativeInputEnabled && this._campaignChatInputActive() && this._isChatTextKey(event)) { this._queueCampaignDialogKey(event, false); this._retryAudioOnGesture(); return; }
        if (this._clientNativeInputEnabled && !this._clientComposing) {
            const editing = event.target === this._clientImeElement && this._clientImeElement !== null;
            if (editing && !event.isComposing) this._clientImePending = null;
            queueBrowserVirtualKeyEvent(this._clientInputQueue, event, this._input.pointer, false);
            const queued = queueBrowserCharacterEvent(this._clientInputQueue, event, this._input.pointer);
            if (editing && queued && typeof event.key === 'string' && [...event.key].length === 1) this._markClientImePending(event.key);
        }
        // 조합 중 Enter 등은 기존 메뉴 명령으로도 중복 실행하지 않는다.
        if (this._clientNativeInputEnabled && (this._clientComposing || event.isComposing)) return;
        if (!this._clientNativeInputEnabled && this._campaignDisplayActive() && !this._mainMenu?.dialog && (event.code === 'F4' || event.code === 'F5' || event.code === 'F6' || event.code === 'F2' || event.code === 'F3' || event.code === 'F7' || event.code === 'F8' || event.code === 'F9' || event.code === 'Pause' || event.code === 'ArrowLeft' || event.code === 'ArrowRight' || event.code === 'ArrowUp' || event.code === 'ArrowDown')) {
            event.preventDefault(); queueBrowserVirtualKeyEvent(this._clientInputQueue, event, this._input.pointer, false);
        }
        // 문자 키 허용 목록에 H(집으로 점프)를 더했다. 전에는 H 가 큐에 들어가지 않아 F4 만 집으로 점프했다. (2026-11-21 WB04-E1146)
        // ESC 는 조각을 들고 있으면 취소, 아니면 게임 메뉴막대 전환이므로 항상 큐에 넣고 입력 프레임이 판단한다. (2026-11-20 WB04-E1084)
        if (!this._clientNativeInputEnabled && this._campaignDisplayActive() && !this._mainMenu?.dialog && !event.isComposing && !this._clientComposing && !event.ctrlKey && !event.altKey && !event.metaKey && (/^[chnprqwaszyxetud]$/iu.test(event.key ?? '') || event.key === 'Escape' || event.key === 'Enter')) queueBrowserCharacterEvent(this._clientInputQueue, event, this._input.pointer);
        // TAB 과 숫자 줄(0~9, Shift 는 영어 기호 표 `)!@#$%^&*(`)이다. 숫자는 키보드 배열과 무관하게 Digit 코드로 원본 문자 코드를 만들고, TAB 은 브라우저 포커스 이동을 막는다. (2026-11-21 WB04-E1154)
        if (!this._clientNativeInputEnabled && this._campaignDisplayActive() && !this._mainMenu?.dialog && !event.isComposing && !this._clientComposing && !event.ctrlKey && !event.altKey && !event.metaKey && !event.repeat) {
            const digit = /^Digit([0-9])$/u.exec(event.code ?? '');
            if (event.code === 'Tab') { event.preventDefault(); queueBrowserCharacterEvent(this._clientInputQueue, event, this._input.pointer); }
            else if (digit) {
                const index = Number(digit[1]), character = event.shiftKey ? SAVED_CAMERA_CHARACTERS[10 + (index + 9) % 10] : SAVED_CAMERA_CHARACTERS[index];
                this._clientInputQueue.push(((character.charCodeAt(0) | (event.shiftKey ? InputCode.shift : 0)) >>> 0), this._input.pointer.x | 0, this._input.pointer.y | 0);
            }
        }
        if (PREVENTED_KEYS.has(event.code)) event.preventDefault();
        this._retryAudioOnGesture();
        // 미리보기의 키 정책은 게임 명령과 분리한다. Escape는 메뉴로 돌아가며 방향키는 카메라만 바꾼다. (2026-11-02 WB03-AA)
        if (this._state === AppState.TERRAIN_PREVIEW) {
            if (event.code === 'Escape') { event.preventDefault(); this.closeTerrainPreview(); }
            else if (this._terrainPreview?.keyDown(event.code)) event.preventDefault();
            return;
        }
        // Esc 는 열려 있는 펼침 메뉴를 닫는다(원본 입력 0x020000be: RequestClosePopups). (2026-10-29 WB03-F)
        if (event.code === 'Escape' && this._state === AppState.MAIN_MENU && this._mainMenu?.closePopup()) this._displayDirty = true;
        // F1 은 일반 도움말을 연다(원본 입력 0x70 → F1Help). 브라우저의 도움말 동작은 막는다. 열린 도움말 창은 PageUp·PageDown·위·아래 화살표로 본문을 스크롤한다. (2026-10-30 WB03-G)
        if (this._state === AppState.MAIN_MENU && this._mainMenu) {
            if (event.code === 'F1') {
                event.preventDefault();
                // 원본 OpenQuickHelpWindow(QuickHelpControl.cpp:50~53)는 현재 모달이 도움말이 아니면(Tell·브리핑·확인 창이 열려 있으면) 열지 않고, 열 때 먼저 팝업을 닫는다.
                // 이전에는 열린 Tell 창을 $OnExit 없이 도움말로 바꿔치웠다. (2026-11-22 WB04-E1235)
                if (!event.repeat && (!this._mainMenu.dialog || this._mainMenu.dialog instanceof HelpWindow)) { this._mainMenu.closePopup?.(); this._openHelp('F1Help'); }
            } else if (this._mainMenu.dialog instanceof HelpWindow && this._mainMenu.dialog.keyDown(event.code)) {
                this._mainMenu.dirty = true;
            }
        }
        if (!event.repeat && (event.code === 'Enter' || event.code === 'Space' || event.code === 'NumpadEnter')) this._activate();
    }

    // 마우스 휠을 열린 대화상자(도움말 창, 스크롤바가 있는 Tell 창)에 넘긴다. 처리했으면 페이지가 따라 스크롤되지 않게 막는다. (2026-10-30 WB03-G, 2026-10-31 WB03-K: Tell 창 추가)
    _onWheel(event) {
        const dialog = this._mainMenu?.dialog;
        if (this._state !== AppState.MAIN_MENU || !dialog || typeof dialog.wheel !== 'function') return;
        event.preventDefault();
        if (dialog.wheel(event.deltaY)) this._mainMenu.dirty = true;
    }

    // 채팅 입력줄 편집기의 복사 장치이다(원본 clipboardWrite): 선택(없으면 전체) 바이트를 표시 부호화에서 해독해 앱 사본에 두고 브라우저 클립보드에도 쓴다(사용자 제스처 안에서만 허용되며 실패해도 사본은 남는다). (2026-11-22 WB04-E1178)
    _chatClipboardWrite(bytes) {
        const end = bytes.indexOf(0), text = displayTextCodec.decode(bytes.subarray(0, end < 0 ? bytes.length : end));
        this._chatClipboard = text; try { globalThis.navigator?.clipboard?.writeText?.(text)?.catch?.(() => {}); } catch { /* 클립보드 쓰기를 못 해도 앱 사본으로 계속한다. */ }
    }

    // 도우미 `Paste` 항목·편집기 붙여넣기가 읽는 클립보드 사본이다(마지막 복사·붙여넣기 본문). 브라우저 클립보드는 비동기라 동기 읽기 장치는 앱 사본만 쓴다. 256바이트 NUL 문자열로 채운다. (2026-11-22 WB04-E1178)
    _chatClipboardRead(bytes) {
        const encoded = displayTextCodec.encode(this._chatClipboard ?? ''), length = Math.min(encoded.length, bytes.length - 1);
        bytes.fill(0, 0, Math.min(bytes.length, 256)); bytes.set(encoded.subarray(0, length)); bytes[length] = 0;
    }

    // 붙여넣기 사건을 활성 입력창(채팅 입력줄·Send X 숫자 창)의 확정 문자로 바꿔 입력 큐에 올린다. 한 줄 입력이므로 줄바꿈은 공백으로 바꾸고 원본 입력줄 한도(128)로 자른다. 입력창이 없으면 기본 동작에 맡긴다.
    // 입력줄의 길이·금지 문자 규칙은 문자 입력 경로가 그대로 적용한다. (2026-11-22 WB04-E1175)
    _onPaste(event) {
        if (this._clientNativeInputEnabled || !(this._campaignInputDialogOpen() || this._campaignChatInputActive())) return;
        const text = String(event.clipboardData?.getData('text/plain') ?? '').replace(/[\r\n\t]+/gu, ' ').slice(0, 128);
        event.preventDefault(); if (!text) return;
        this._chatClipboard = text;
        const pointer = this._input.pointer;
        try { if (!queueCommittedText(this._clientInputQueue, text, pointer.x | 0, pointer.y | 0)) throw new Error('붙여넣기 입력 큐 공간 부족'); } catch (error) { reportGameError('paste', error, { state: this._state }); }
    }

    // 키를 뗐을 때 눌린 키 집합에서 뺀다.
    _onKeyUp(event) {
        this._input.keys.delete(event.code);
        if (event.target === this._clientImeElement && this._clientImePending?.text === event.key) this._clientImePending = null;
        if (!this._clientNativeInputEnabled && this._campaignInputDialogOpen()) { this._queueCampaignDialogKey(event, true); return; }
        if (!this._clientNativeInputEnabled && this._campaignChatInputActive() && this._isChatTextKey(event)) { this._queueCampaignDialogKey(event, true); return; }
        if (this._clientNativeInputEnabled && !this._clientComposing) queueBrowserVirtualKeyEvent(this._clientInputQueue, event, this._input.pointer, true);
        else if (this._campaignDisplayActive() && (event.code === 'F4' || event.code === 'F5')) queueBrowserVirtualKeyEvent(this._clientInputQueue, event, this._input.pointer, true);
    }

    // 원본 입력을 사용하는 동안 조합 키를 일반 문자·가상 키 경로에서 제외한다.
    _onCompositionStart() { if (this._clientNativeInputEnabled) { this._clientComposing = true; this._clientImePending = null; } }

    // 같은 입력의 후속 beforeinput만 한 이벤트 턴 동안 중복 검사한다.
    _markClientImePending(text) {
        const pending = { text }; this._clientImePending = pending;
        const view = this._root?.ownerDocument?.defaultView ?? globalThis;
        view.setTimeout(() => { if (this._clientImePending === pending) this._clientImePending = null; }, 0);
    }

    // 확정 본문을 같은 원본 큐에 원자적으로 넣고 미지원·공간 부족은 진단한다.
    _queueClientCommittedText(text) {
        try {
            const pointer = this._input.pointer;
            if (!queueCommittedText(this._clientInputQueue, text, pointer.x | 0, pointer.y | 0)) throw new Error('확정 문자 입력 큐 공간 부족');
            return true;
        } catch (error) { reportGameError('composition', error, { state: this._state }); return false; }
    }

    // 확정된 조합 본문만 큐에 넣고 같은 후속 입력을 중복하지 않는다.
    _onCompositionEnd(event) {
        if (!this._clientComposing) return;
        this._clientComposing = false;
        if (!this._clientNativeInputEnabled || !event?.data) return;
        if (this._queueClientCommittedText(event.data)) this._markClientImePending(event.data);
    }

    // 키다운 없이 삽입된 텍스트를 받고 이미 큐에 넣은 동일 입력은 건너뛴다.
    _onClientBeforeInput(event) {
        if (!this._clientNativeInputEnabled || this._clientComposing || event.isComposing || !event.data || !['insertText', 'insertReplacementText'].includes(event.inputType)) return;
        event.preventDefault();
        if (this._clientImePending?.text === event.data) { this._clientImePending = null; return; }
        this._clientImePending = null; this._queueClientCommittedText(event.data);
    }

    // 포커스를 잃거나 탭이 숨겨질 때 눌린 키·버튼이 남지 않도록 입력 상태를 비우고, 메뉴 누름을 실행 없이 취소한다.
    _clearInput() {
        this._clientComposing = false; this._clientImePending = null;
        this._input.keys.clear();
        this._input.pointer.buttons = 0;
        this._input.pointer.inside = false;
        this._mainMenu?.cancel();
        this._terrainPreview?.cancel();
    }

    // 시작 이후의 입력에서 오디오가 아직 재생 가능 상태가 아니면 다시 활성화를 시도한다.
    _retryAudioOnGesture() {
        if (this._state === AppState.MAIN_MENU && this._audioStatus !== 'running' && this._audioStatus !== 'unsupported' && this._audioStatus !== 'disabled') {
            this._unlockAudio();
        }
    }

    // 탭이 숨겨지면 프레임 루프를 멈추고 입력을 비우며 소리를 멈추고, 돌아오면 시계 기준점을 다시 잡고 재개한다.
    // 원본 효과음·음악 버퍼는 창이 포커스를 잃으면 들리지 않으므로(GLOBALFOCUS 없음) 웹에서는 탭 숨김을 그 대응으로 쓴다.
    // (2026-10-23 WB01, 2026-10-23 WB03-M1: 소리 일시 정지 추가)
    _onVisibilityChange() {
        const hidden = this._root.ownerDocument.hidden;
        if (hidden) {
            if (this._gameClock && !this._hiddenGamePause) { this._gameClock.pushGamePause(); this._hiddenGamePause = true; }
            this._stopFrameLoop();
            this._clearInput();
        } else {
            if (this._gameClock && this._hiddenGamePause) { this._gameClock.popGamePause(); this._hiddenGamePause = false; }
            this._clock.reset();
            this._displayDirty = true;
            this._startFrameLoop();
        }
        // 탭 전환 중 오디오 장치 오류는 표시 루프와 독립적으로 보고한다. (2026-11-19 WB04-E944)
        const audio = this._audio;
        if (!audio) return;
        const update = hidden ? audio.suspend() : audio.resume();
        update.catch((error) => {
            if (this._audio === audio) reportGameError('audio-visibility', error, { state: this._state, hidden });
        });
    }

    // ----- 프레임 루프 -----

    // 프레임 루프를 시작한다. 이미 예약돼 있거나 탭이 숨겨져 있으면 아무것도 하지 않는다.
    _startFrameLoop() {
        // 고정 시각 모드의 프레임은 stepFixedFrames 가 직접 진행하므로 표시 주기(rAF)에 맡기지 않는다. (2026-11-23 WB04-E1271)
        if (this.options.fixedFrameMs > 0) return;
        if (this._frameHandle || !this._root || this._root.ownerDocument.hidden) return;
        this._frameHandle = this._view().requestAnimationFrame((now) => {
            // 실행된 예약을 먼저 반납해 프레임 본문 앞부분의 실패도 다음 예약을 막지 않게 한다.
            this._frameHandle = 0;
            this._runFrameSafely(now);
        });
    }

    // 한 프레임의 오류를 원래 객체·상태·미션·시각과 함께 기록하고 다음 표시 기회를 예약한다.
    // 실패한 프레임을 다시 실행하지 않으며 정상 종료·숨김·고정 시각의 예약 규칙은 그대로 따른다. (2026-11-26 사용자 결정, WB04-E1338)
    _runFrameSafely(now) {
        try {
            this._onFrame(now);
        } catch (error) {
            reportGameError('frame', error, { state: this._state, frameTime: now, mission: this._lastCampaignMission?.name ?? null, fixedFrame: this._fixedFrameCount ?? null });
            this._displayDirty = true;
            this._startFrameLoop();
        }
    }

    // 프레임 루프를 멈춘다. 예약된 프레임이 없으면 아무것도 하지 않는다.
    _stopFrameLoop() {
        if (!this._frameHandle) return;
        this._view().cancelAnimationFrame(this._frameHandle);
        this._frameHandle = 0;
    }

    // 매 프레임 호출된다. 시계를 진행하고 화면을 그린 뒤 다음 프레임을 예약한다.
    // 게임 갱신(Process/Regular)은 WB04 이후에 여기서 시계 값과 별도의 게임 시각으로 호출한다.
    _onFrame(now) {
        this._frameHandle = 0;
        if (!this._canvas) return;
        // 대기 동안 Native 프레임/입력/Guide/Kernel/RNG는 진행하지 않고 DOM과 실제 시간 감시는 계속한다.
        if (this._mods?.pending) { this._startFrameLoop(); return; }
        // 해상도 미디어/resize 알림이 없어도 현재 기기 배율을 맞춘다. 게임 프레임 상한과 시계 처리 전에 표시만 복구한다.
        if (this._root && Number.isFinite(this._layout?.devicePixelRatio) && this._layout.devicePixelRatio > 0) {
            const ratio = this._view()?.devicePixelRatio;
            const dpr = Number.isFinite(ratio) && ratio > 0 ? ratio : 1;
            if (this._layout.devicePixelRatio !== dpr) this._applyLayout();
        }
        // 원본 프레임 상한(maxFPS, 기본 75)보다 빠르게 오는 표시 주기(rAF)는 건너뛰고 다음 기회에 처리한다. 60Hz 표시에서는 상한이 걸리지 않는다. (2026-11-20 WB04-E1094)
        if (!frameDue(now, this._lastFrameStamp, normalizeFrameRate(this._settings?.get('maxFPS')))) { this._startFrameLoop(); return; }
        this._lastFrameStamp = now;
        // 비동기InitFullTime 중에는 실제Client 프레임/시계/RNG를 소비하지 않고 로딩화면만 진행한다.
        if (this._clientClockPreparing) { this._clock.tick(now); this._frameSeconds = now / 1000; this._render(now); this._startFrameLoop(); return; }
        // 원본 Client 순서대로 화면 입력과 갱신 전에 같은 게임 프레임을 고정한다.
        // 고정 시각 모드의 시작 그림 구간은 시각만 전진하고 주 루프 프레임(번호·시간차)으로 세지 않는다(원본 cpppj 의 고정 프레임 실행과 같음). (2026-11-23 WB04-E1271)
        if (this.options.fixedFrameMs > 0 && this._state === AppState.TITLE) {
            // 시작 그림 구간에는 주 루프의 갱신(주변음·바람의 Game 난수 소비 등)이 돌지 않고 시각만 흐르며 그림만 그린다(원본 cpppj 의 시작 그림 루프와 같음). (2026-11-23 WB04-E1271)
            this._gameClock?.advanceWithoutCapture(); this._clock.tick(now); this._frameSeconds = now / 1000; this._render(now); return;
        }
        this._gameClock?.captureFrame();
        this._clock.tick(now);
        this._frameSeconds = now / 1000;
        // 원본 루프 2번: 현재 미션의 로컬 State만 Poll한다. 수신 콜백 안의 재진입은 정리와 전환을 미룬다.
        const stateWorld = this._campaignWorld, guide = this._clientGuide;
        // 아직 World가 없는 대기 요청은 음향을 준비한 뒤 기존 Poll 전 요청만 State7로 넘긴다.
        if (this._state === AppState.MAIN_MENU && guide?.pending().pending() && !stateWorld) {
            this._beginCampaignAudioForRequest(guide);
            if (this._clientSoundPlayer) runCampaignRequestFrame(guide, () => {}, () => this._runCampaignState7({}, { x: this._layout.logicalWidth, y: this._layout.logicalHeight }));
        } else if (stateWorld && !stateWorld.session().connectionCallbackActive()) stateWorld.session().pollLocalConnectionState();
        // 원본 루프 3번: 자연 종료한 버퍼를 포함해 패킷 처리 전에 현재 재생 수를 갱신한다.
        this._clientSoundPlayer?.recount();
        // 원본 루프 4번: Poll 뒤 현재 World를 다시 조회해 같은 내부 전송을 진행한다. 완료 후 화면 설치는 별도 경계다.
        const packetWorld = this._campaignWorld;
        if (packetWorld && !packetWorld.session().connectionCallbackActive()) packetWorld.session().pumpLocalConnection();
        // Ready인 같은 미션에 Start·Tutorial을 준비한 뒤 World 연결 관문을 연다.
        const readyWorld = this._campaignWorld;
        if (this._state === AppState.MAIN_MENU && readyWorld?.connecting && !readyWorld.session().connectionCallbackActive() && readyWorld.session().localConnectionReady()) {
            this._prepareCampaignMissionStart(); this._prepareCampaignUserInput(); this._prepareCampaignCursor(); this._prepareCampaignTutorial(); readyWorld.completeLocalConnection();
        }
        // State7에서 생성한 HUD의 실제 Session/Form 도킹은 FinishMissionWorldLoad 위치에서 수행한다. Guide의 기술 추가보다 패널 Form(SID 순서)이 먼저다. (E1305·E1383)
        if (this._campaignDisplayActive() && !this._campaignProduction?.maintenance) this._prepareCampaignProduction();
        if (this._campaignDisplayActive() && !this._campaignProduction.worldBound) this._prepareCampaignProductionUi();
        // 원본 루프 8번의 Guide를 상태·패킷 처리 뒤, 입력 처리보다 먼저 실행한다.
        this._clientGuide?.run();
        this._mods?.beforeInput();
        if (this._campaignDisplayActive()) this._runCampaignInputFrame();
        if (this._state === AppState.MAIN_MENU) {
            // 원본 UserInput 은 매 프레임 마우스 위치를 폴링해 Gump 에 알린다. 그래서 포인터가 멈춰 있어도 그 아래에 새로 열린 펼침 메뉴 행이 바로 강조된다
            // (cpppj 오라클 help-popup/options-popup 관측). 브라우저는 포인터가 움직일 때만 사건을 주므로 마지막 포인터 위치를 매 프레임 다시 알린다. (2026-11-07 WB02-H)
            const pointer = this._input.pointer;
            if (this._mainMenu && pointer.inside && (!this._campaignDisplayActive() || this._mainMenu.dialog)) this._mainMenu.pointerMove(pointer.x, pointer.y);
            this._mainMenu?.update(this._frameSeconds);
            this._idleMainMenuActivity();
            // 스크롤·창 열림/닫힘·키 입력으로 포인터 아래 내용이 바뀌었을 수 있으므로 커서를 다시 고른다(원본은 매 포인터 갱신마다 선택). (2026-10-31 WB03-L)
            this._applyCursor();
        }
        this._runAmbientFrame();
        // 원본 주 루프는 주변음·바람 뒤에 RunClientBridgeDecay(00422bc0)와 SceneFrame(469f60: 미션 장면 음악의 곡 기한을 확인해 다음 원소 곡으로 넘김)을 이 순서로 하고, 그 뒤에 미니맵 갱신·Gump 정리·Kernel 이 온다.
        // 장면 음악이 곡을 고를 때 쓰는 Game 난수는 주변음·바람과 같은 줄이라 이 순서가 난수열을 정한다. 이전에는 장면 음악을 입력보다 앞에서 했고 다리 붕괴는 Kernel 준비 안에 있었다. (2026-11-23 WB04-E1266, 장면 음악 연결 2026-11-20 WB04-E1119)
        if (this._campaignDisplayActive()) this._campaignWorld.runBridgeDecay();
        this._sceneMusic?.frame();
        // 원본 Kernel 단계에서 같은 현재 World를 준비하고 단일 Client Kernel을 실행한다.
        // 원본 메인 루프는 Gump Idle(464170) 앞에서 죽은 Gump 정리(004662b0 cleanupGumps)를 매 프레임 한 번 실행한다. 이 정리가 없으면 만료된 안내(Game Time 등)가 Kill 큐에만 쌓이고 등록표에서 빠지지 않아
        // 개수가 999 한도에 닿으면 "Gump 원본 개수 한도 초과" 로 게임이 멈춘다. (2026-11-21 WB04-E1134, 몽키 시험의 Vicious 3분 진행에서 발견)
        // 미니맵 갱신(원본 RefreshMinimap 481320)은 Gump 정리·Idle·Kernel 앞이다(이전에는 Kernel 뒤라 한 프레임 앞선 모습을 그렸다). (2026-11-23 WB04-E1266)
        if (this._campaignDisplayActive()) { this._runCampaignProductionFrame(); this._campaignProduction?.hud?.refresh(); this._clientGump.lifetime().cleanup(); this._clientGump.idle(); this._runCampaignKernelFrame(); }
        // 로그인 중에도 원본 자원 프레임의 기한 예약을 실행한다. 활성 월드는 위 Kernel 프레임에서 이미 호출했다. (WB04-E1313)
        else if (this._state === AppState.MAIN_MENU && this._campaignWorld && !this._campaignWorld.session().connectionCallbackActive()) this._campaignWorld.runBattleResources();
        this._syncDialogToMapView();
        this._render(now);
        // 원본 주 루프 13번(439c33): 화면을 그린 뒤 Player 결과 알림을 한다. known 이고 전투 모드일 때만 Session 이 필요하다. (2026-11-22 WB04-E1233)
        if (this._campaignDisplayActive() && this._clientGameplay.player.playerKnown && this._surfaceStorage.ownerMode.battle) this._campaignWorld.session().notifyPlayerResults();
        this._mods?.afterFrame();
        this._startFrameLoop();
    }

    // 원본 Client::StartThunderFlash(00470d40): 현재 Player 의 기술 부모(+0x9c DWORD)가 살아 있을 때만 TYPE 55·flags 0x10 번개 프로세스(ThunderProcess)를 실제 ProcessHost 에 부착한다.
    // 창 모드와 마찬가지로 웹에서도 팔레트는 바꾸지 않지만, 실제 ProcessForm 할당과 수명(1/60초 뒤 자체 종료)이 원본 SidPool·Kernel 과 같아야 L3 덤프가 일치한다. (2026-11-24 WB04-E1329)
    _startThunderFlash(brightness = 0) {
        if (!this._clientGameplay?.player?.playerKnown) return;
        const storage = this._surfaceStorage, kernel = this._clientKernel, screen = this._clientPaletteScreen, clock = this._gameClock;
        if (!storage || !kernel || !screen || !clock) throw new Error('번개 기술 부모의 실제 표면 저장소·커널·팔레트·시계 없음');
        if (storage.ownerMode.challenge) throw new Error('번개 메타맵 00479650 의 실제 기록 조회 미복원');
        const players = this._clientCoreData.players;
        const at = (this._clientGameplay.player.battleOwner >>> 0) * 180 + 0x9c;
        if (at < 0 || at + 4 > players.length) throw new RangeError('번개 현재 Player 기술 DWORD 범위');
        const parent = (players[at] | (players[at + 1] << 8) | (players[at + 2] << 16) | (players[at + 3] << 24)) >>> 0;
        if (!parent) return;
        if (parent > 65535 || parent >= storage.pool.capacity) throw new RangeError('번개 기술 부모의 실제 SID 범위');
        const process = new ThunderProcess(false, {
            wallSeconds: () => clock.wallSeconds(),
            fullScreen: () => false,
            readPalette: (start, count, destination) => {
                const pal = screen.palette();
                // 요청한 부분 팔레트 [start, start+count) 의 BGRX 바이트를 ScreenColor 객체로 채운다.
                for (let index = 0; index < count; index += 1) {
                    const offset = (start + index) * 4;
                    destination[index] = { blue: pal[offset], green: pal[offset + 1], red: pal[offset + 2], reserved: pal[offset + 3] };
                }
            },
            applyPalette: (start, count, colors) => {
                const bytes = new Uint8Array(Math.max(count, 1) * 4);
                // ScreenColor 배열을 Screen.setPalette 가 읽는 BGRX 바이트 배열로 옮긴다.
                for (let index = 0; index < count; index += 1) {
                    const offset = index * 4, color = colors[index];
                    bytes[offset] = color.blue & 0xff; bytes[offset + 1] = color.green & 0xff; bytes[offset + 2] = color.red & 0xff; bytes[offset + 3] = color.reserved & 0xff;
                }
                screen.setPalette(start, count, bytes, true);
            },
            restorePalette: () => { screen.setPalette(0, 256, null, true); },
        }, (pid) => {
            const index = this._thunderProcesses.indexOf(pid);
            if (index >= 0) this._thunderProcesses.splice(index, 1);
        }, brightness);
        const attached = storage.processes.host.attach(process, THUNDER_PROCESS_TYPE, parent, 0x10);
        if (attached) this._thunderProcesses.push(kernel.find(attached));
    }

    // 원본 Client::ClearThunderFlashes: 등록된 번개 프로세스를 뒤에서부터 즉시 종료해 Form·목록을 함께 반납한다. (2026-11-24 WB04-E1329)
    _clearThunderFlashes() {
        const storage = this._surfaceStorage, kernel = this._clientKernel;
        if (!storage || !kernel) { this._thunderProcesses.length = 0; return; }
        // 매 kill 의 소멸 콜백이 배열에서 해당 PID 를 지우므로 매번 마지막 항목을 다시 읽는다.
        while (this._thunderProcesses.length > 0) {
            const pid = this._thunderProcesses[this._thunderProcesses.length - 1];
            const process = kernel.get(pid);
            if (!(process instanceof ThunderProcess)) throw new Error('번개 등록 목록과 실제 Kernel 불일치');
            storage.processes.host.kill(process, 0x10);
        }
    }

    // 원본 주 루프 603·604(00439aeb·00439af0): 주변음(먼 천둥·독수리·고래, 120초 주기)과 공유 바람을 매 프레임 갱신한다. 둘은 같은 Game 난수를 소비하므로 게임 정지·메뉴·음향 장치 유무와 무관하게 실행하고(장치가 없으면 소리만 건너뛴다),
    // 번개 섬광(hooks.flash)은 원본 Client::RequestThunderFlash 처럼 창 모드에서도 실제 ThunderProcess(TYPE 55)를 부착해 SidPool·Kernel 수명을 맞춘다. (2026-11-22 WB04-E1234, 2026-11-24 WB04-E1329: ThunderProcess 연결)
    _runAmbientFrame() {
        const random = this._clientRandom?.game, local = this._clientLocal, clock = this._gameClock; if (!random || !local || !clock) return;
        const state = this._ambientSound ??= { kind: 0, forced: 0 };
        stepAmbientSound({ get deadline() { return local.activityEnd().value; }, set deadline(value) { local.activityEnd().value = value; }, get kind() { return state.kind; }, set kind(value) { state.kind = value; }, get forced() { return state.forced; } }, random, {
            wallSeconds: () => clock.wallSeconds(), currentDialog: () => (this._mainMenu?.dialog ? 1 : 0), flash: (brightness) => { this._startThunderFlash(brightness); },
            sound: (name, loop, volume, pan, priority, limit) => { this._clientSoundPlayer?.playByName(name, loop, volume, pan, priority, limit); },
        });
        this._surfaceStorage?.updateAmbientWind(random);
    }

    // 미션 화면(HUD 도킹으로 지도 영역이 왼쪽 85px 줄어든 상태)이 준비된 뒤 이미 열려 있던 안내 창을 같은 지도 영역의 가운데로 다시 놓는다. 튜토리얼 첫 안내 창은 HUD 도킹보다 먼저 열리므로 화면 전체 가운데에 놓였다가 어긋났다.
    // 안내 창(TellWindow)만 대상이다(도움말 창 등은 자체 위치 규칙을 쓴다). (2026-11-21 WB04-E1130)
    _syncDialogToMapView() {
        const dialog = this._mainMenu?.dialog;
        if (!dialog?.followsMapView || !this._campaignDisplayActive()) return;
        const view = this._campaignWorld.mapView(), screen = dialog.screen;
        const wanted = { left: view.left, top: view.top, width: view.right - view.left, height: view.bottom - view.top };
        if ((screen.left ?? 0) === wanted.left && (screen.top ?? 0) === wanted.top && screen.width === wanted.width && screen.height === wanted.height) return;
        dialog.screen = wanted; dialog.place(); this._mainMenu.dirty = true; this._displayDirty = true;
    }

    // ----- 그리기 -----

    // 현재 상태에 맞는 장면을 Canvas 에 그린다. 웹 전용 화면은 논리 좌표로 그리고 내부 픽셀 배율(drawScaleX/Y)은 변환 행렬로 적용한다.
    // 메인 메뉴는 8비트 표면을 합성해 표시기로 Canvas 전체에 그리며, 바뀐 것이 없으면(디버그 표시 제외) 다시 그리지 않는다.
    // (2026-10-23 WB01, 2026-10-23 WB03-M1: 원본 메인 메뉴 표시)
    _render(now) {
        const ctx = this._ctx;
        const layout = this._layout;
        if (!ctx || !layout?.visible) return;
        const { logicalWidth: width, logicalHeight: height, drawScaleX, drawScaleY } = layout;
        if (this._campaignDisplayActive() && this._mainMenu && this._surface) {
            // 기존 Tell 창은 별도 Gump 이식 전까지 실제 월드 위에 합성한다. 창이 있거나 바뀌면 이전 픽셀을 전체 복구한다.
            const dialog = this._mainMenu.dialog;
            if (dialog || this._mainMenu.dirty || this._displayDirty) this._clientRenderer.invalidateAll();
            // 같은 동기 프레임의 실제 변경 범위만 합친다. 오류가 나도 앞서 복사한 부분은 finally에서 표시한다.
            this._clientScreenPresentBatch = true; this._clientScreenPresentBounds = null;
            try { this._drawCampaignRendererFrame(); }
            finally {
                this._clientScreenPresentBatch = false;
                const dirty = this._clientScreenPresentBounds; this._clientScreenPresentBounds = null;
                if (dirty) this._presenter.present(ctx, this._surface, layout, true, dirty);
            }
            if (dialog) { dialog.draw(this._surface); this._presenter.present(ctx, this._surface, layout, true); }
            this._mainMenu.dirty = false; this._displayDirty = false;
            this._campaignCameraDevice().applyCameraFrame();
            if (this.options.debug) { ctx.setTransform(drawScaleX, 0, 0, drawScaleY, 0, 0); this._drawDebug(ctx); }
            return;
        }
        if (this._state === AppState.TERRAIN_PREVIEW && this._terrainPreview && this._surface) {
            const changed = this._terrainPreview.dirty;
            if (!changed && !this._displayDirty && !this.options.debug) return;
            if (changed) this._terrainPreview.render(this._surface);
            this._presenter.present(ctx, this._surface, layout, changed);
            this._displayDirty = false;
            return;
        }
        if (this._state === AppState.MAIN_MENU && this._mainMenu && this._surface) {
            const changed = this._mainMenu.dirty;
            if (!changed && !this._displayDirty && !this.options.debug) return;
            if (changed) this._mainMenu.render(this._surface);
            this._presenter.present(ctx, this._surface, layout, changed);
            this._displayDirty = false;
            ctx.setTransform(drawScaleX, 0, 0, drawScaleY, 0, 0);
            if (this.options.debug) this._drawDebug(ctx);
            return;
        }
        ctx.setTransform(drawScaleX, 0, 0, drawScaleY, 0, 0);
        ctx.imageSmoothingEnabled = false;
        ctx.fillStyle = '#000';
        ctx.fillRect(0, 0, width, height);

        switch (this._state) {
            case AppState.LOADING:
                this._drawLoading(ctx, width, height, now);
                break;
            case AppState.ERROR:
                this._drawError(ctx, width, height);
                break;
            case AppState.CLICK_TO_START:
                this._drawClickToStart(ctx, width, height, now);
                break;
            case AppState.TITLE:
                this._drawTitleScreen(ctx, width, height, now);
                break;
            default:
                break;
        }
        this._displayDirty = true;
        if (this.options.debug) this._drawDebug(ctx);
    }

    // 글자를 그린다. align 은 가로 정렬, alpha 는 투명도다.
    _drawText(ctx, text, x, y, size, color, align = 'center', alpha = 1) {
        ctx.globalAlpha = alpha;
        ctx.fillStyle = color;
        ctx.font = `${size}px ${(this._gameData?.language ?? this.options.language) === 'japanese' ? `"${JAPANESE_FONT_FAMILY}", sans-serif` : FONT_FAMILY}`;
        ctx.textAlign = align;
        ctx.textBaseline = 'middle';
        ctx.fillText(text, x, y);
        ctx.globalAlpha = 1;
    }

    // 로딩 화면: 제목과 회전하는 표시, 자산 적재 진행 막대·백분율을 그린다(웹 전용 화면).
    // (2026-10-23 WB01, 2026-10-23 WB03-M1: 진행 막대 추가)
    _drawLoading(ctx, width, height, now) {
        const texts = this.texts;
        this._drawText(ctx, texts.title, width / 2, height / 2 - 70, 64, '#d8e4f0');
        const angle = (now / 1000) * Math.PI * 2;
        ctx.strokeStyle = '#7aa7d8';
        ctx.lineWidth = 4;
        ctx.beginPath();
        ctx.arc(width / 2, height / 2 + 10, 22, angle, angle + Math.PI * 1.5);
        ctx.stroke();
        const { loaded, total } = this._loadProgress;
        const ratio = total > 0 ? Math.min(1, loaded / total) : 0;
        const barWidth = 320;
        const barLeft = Math.round(width / 2 - barWidth / 2);
        const barTop = Math.round(height / 2 + 52);
        ctx.strokeStyle = '#4f7aa6';
        ctx.lineWidth = 2;
        ctx.strokeRect(barLeft, barTop, barWidth, 10);
        ctx.fillStyle = '#7aa7d8';
        ctx.fillRect(barLeft + 2, barTop + 2, Math.round((barWidth - 4) * ratio), 6);
        this._drawText(ctx, `${texts.loading} ${Math.floor(ratio * 100)}%`, width / 2, height / 2 + 90, 24, '#9fb4c8');
    }

    // 로딩 실패 화면: 실패 안내와 원인, 재시도 안내를 그린다.
    _drawError(ctx, width, height) {
        const texts = this.texts;
        const reason = String(this._loadError?.message ?? this._loadError ?? '').slice(0, 120);
        this._drawText(ctx, texts.title, width / 2, height / 2 - 90, 64, '#d8e4f0');
        this._drawText(ctx, texts.loadFailed, width / 2, height / 2 - 10, 26, '#ff8a80');
        if (reason) this._drawText(ctx, reason, width / 2, height / 2 + 30, 18, '#c9a0a0');
        this._drawText(ctx, texts.retry, width / 2, height / 2 + 90, 24, '#d8e4f0');
    }

    // '클릭하여 시작' 화면: 제목과 깜박이는 시작 안내를 그린다. 이 화면에서는 소리를 재생하지 않는다.
    _drawClickToStart(ctx, width, height, now) {
        const texts = this.texts;
        this._drawText(ctx, texts.title, width / 2, height / 2 - 60, 72, '#d8e4f0');
        const blink = 0.55 + 0.45 * Math.sin(now / 350);
        this._drawText(ctx, texts.clickToStart, width / 2, height / 2 + 50, 30, '#9fd0ff', 'center', blink);
    }

    // 개발용 정보 겹침: 상태·화면 구성·입력·오디오·음악·프레임 수를 왼쪽 위에 그린다. ?debug 로 켠다.
    // (2026-10-23 WB01, 2026-10-23 WB03-M1: 음악·확대 방식 표시 추가)
    _drawDebug(ctx) {
        const layout = this._layout;
        const pointer = this._input.pointer;
        const lines = [
            `state ${this._state}  aspect ${this._aspect}  fps ${this._clock.fps.toFixed(1)}`,
            `mode ${layout.mode}  logical ${layout.logicalWidth}x${layout.logicalHeight}  scaling ${this.options.scaling}`,
            `css ${layout.cssWidth}x${layout.cssHeight} @(${layout.left},${layout.top})  scale ${layout.scale.toFixed(3)}`,
            `canvas ${layout.canvasWidth}x${layout.canvasHeight}  draw ${layout.drawScaleX.toFixed(3)}x${layout.drawScaleY.toFixed(3)}  dpr ${layout.devicePixelRatio}`,
            `pointer ${pointer.x},${pointer.y} inside=${pointer.inside} buttons=${pointer.buttons} (${pointer.type})`,
            `keys ${[...this._input.keys].join(' ')}  audio ${this._audioStatus}  music ${this._musicStatus}`,
        ];
        ctx.fillStyle = 'rgba(0, 0, 0, 0.55)';
        ctx.fillRect(8, 8, 620, lines.length * 20 + 8);
        // 정보 줄을 위에서부터 차례로 그린다.
        lines.forEach((line, index) => this._drawText(ctx, line, 14, 22 + index * 20, 14, '#b8ffb8', 'left'));
    }
}

export default Netstorm;
