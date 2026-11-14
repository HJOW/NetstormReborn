# webpj 원본 관측 연결 감사 (WB04-E465~E466, 2026-11-15)

E465에서 `RawCameraPan`·`RawEdgeScroll`·`ClientCameraGlobals`가 "cpppj에도 직접 시험이 없다"고 적혀 있었지만 실제로는 cpppj 시험과 원본 관측 fixture(`camera-pan-x86.tsv` 259행 등)가 있다는 것을 발견했다. 같은 오기가 다른 이식 단위에도 있는지 두 방향으로 전수 감사했다. 감사는 기준선 `1f00a25a`의 `cpppj/tests`를 `git grep`으로 찾은 것이며(스크립트는 세션 임시 폴더), 아래 표는 **웹이 아직 같은 시험/관측을 연결하지 않은 것**의 이월 목록이다. 연결한 항목은 단위 문서·manifest·LEFT_JOBS 체크 목록에서 제거/체크한다.

## A. manifest 에 연결되지 않은 cpppj fixture 중 이미 port/partial 인 단위의 것

`webpj/tests/fixtures/manifest.json`에 없는 cpppj fixture 324개 중, 시험 파일 이름이 이미 이식한 단위와 맞는 것이다(나머지는 todo/제외 단위의 것이라 해당 단위를 옮길 때 연결한다).

| fixture | cpppj 시험 | 웹에 이식된 단위 |
| --- | --- | --- |
| ai-debug-names | AiDebugMenuTests.cpp | RawAiDebugMenu |
| ai-utility-names | AiUtilityCommandInputTests.cpp | AiUtilityCommandInput |
| audio-reinterpret | PresentationCommandInputTests.cpp | PresentationCommandInput |
| basefile-io | BaseFileIOTests.cpp | RawBaseFileIO BaseFile |
| basefile-open-policy | BaseFileOpenPolicyTests.cpp | RawBaseFileOpenPolicy BaseFile |
| basefile-read-key | BaseFileReadKeyTests.cpp | BaseFile |
| basefile-read-policy | BaseFileReadPolicyTests.cpp | RawBaseFileReadPolicy |
| basefile-whole-read | BaseFileWholeReadTests.cpp | RawBaseFileWholeRead |
| budget-refresh | StormPowerRefreshTests.cpp | RawStormPowerRefresh |
| chat-format | ChatFormatTests.cpp | RawChatFormat |
| chat-message | ChatMessageTests.cpp | RawChatMessage |
| chat-packet | ChatPacketTests.cpp | RawChatPacket |
| chat-receive | ChatReceiveTests.cpp | RawChatReceive |
| chat-submit | ChatSubmitTests.cpp | RawChatSubmit |
| detonate-input | DetonateInputTests.cpp | DetonateInput |
| dot-command-dispatch | DotCommandDispatchTests.cpp | RawDotCommandDispatch |
| dot-command-register-v2 | DotCommandRegisterTests.cpp | RawDotCommandRegister |
| dot-command-register | DotCommandRegisterTests.cpp | RawDotCommandRegister |
| dot-command-static-image.hex | ClientDotCommandGlobalsTests.cpp | ClientDotCommandGlobals |
| editor-image | EditorImageTests.cpp | RawImage |
| font-face-enum | PresentationCommandInputTests.cpp | PresentationCommandInput |
| font-face-lookup | PresentationCommandInputTests.cpp | PresentationCommandInput |
| html-input-dialog | HtmlInputDialogTests.cpp | HtmlInputDialog |
| htmlcondition | HtmlConditionEffectTests.cpp | HtmlConditionEffect |
| issue-commands | IssueCommandsTests.cpp | RawIssueCommands |
| make-ai-names | MakeAiCommandInputTests.cpp | MakeAiCommandInput |
| meta-image-list | MetaImageListTests.cpp | MetaImageList |
| meta-image | MetaImageGumpTests.cpp | MetaImageGump |
| options | ClientScriptMenuOptionsTests.cpp LanguageOptionsTests.cpp | LanguageOptions |
| priority-loop | ConfigurationCommandInputTests.cpp | ConfigurationCommandInput |
| renderer-projection | RendererFrameTests.cpp | Renderer |
| scenemusic | ClientAudioTests.cpp | ClientAudio |
| startup-archives | StartupArchiveFilesTests.cpp | StartupArchiveFiles |
| startup-file-names | StartupFileNamesTests.cpp | RawFileName |
| taff-registration | BaseFileSecondaryTests.cpp | BaseFile |
| text-clipboard | TextClipboardTests.cpp | TextClipboard |
| trace-chat | TraceChatTests.cpp | TraceChat |
| xlat-file-read | XlatFileReadTests.cpp | XlatFileRead RawXlat |

## B. 웹 모듈 머리 주석이 "직접 시험이 없다/전용 PE fixture는 없다"고 하지만 cpppj 시험이 있는 단위

`git grep -w 단위이름`으로 cpppj 시험 파일을 찾은 결과다(여러 단위를 함께 쓰는 통합 시험은 대응 단위 이식 때 함께 옮긴다).

| 단위 | cpppj 시험 파일 |
| --- | --- |
| BackgroundGifGump | BackgroundGifGumpTests.cpp |
| ChatNameCompletion | ChatNameCompletionTests.cpp |
| ChatRecipients | ChatRecipientsTests.cpp |
| ChatWindow | ChatWindowClientTests.cpp ChatWindowTestSupport.h ChatWindowTests.cpp HudChatGeometryTests.cpp LanguageRequestTests.cpp TraceChatTests.cpp |
| ClientChatManagerGlobals | CampaignExistingEditorCheckpointTests.cpp ChatPacketTests.cpp ChatSubmitTests.cpp ChatWindowClientTests.cpp ClientChatCoreGlobalsTests.cpp ClientChatCoreOwnersTests.cpp ClientChatRuntimeTests.cpp ProductionInitialContextTests.cpp |
| ClientNsInfoGlobals | ClientStateWorldGlobalsTests.cpp |
| CurrentTypeHitPoints | PriestRegenHitPointCurrentTests.cpp |
| DialogCommandLifetime | DialogCommandLifetimeTests.cpp DialogRuntimeActionTests.cpp |
| DialogExitCommand | CaptureInputTests.cpp DialogConditionFlowTests.cpp DialogExitCommandTests.cpp RecheckCdInputTests.cpp UrlInputTests.cpp |
| FloatingSpGump | FloatingSpGumpTests.cpp RawButtonCursorBoundaryTests.cpp |
| GumpFormBinding | CampaignObjectMenuTests.cpp |
| GumpPointerBoundary | GumpPointerBoundaryTests.cpp |
| HtmlInputDialog | HtmlInputDialogTests.cpp |
| HudChatGeometry | ChatInputTests.cpp HudChatGeometryTests.cpp |
| HudNsInfo | HudNsInfoTests.cpp |
| HudNsInfoCreate | HudNsInfoCreateTests.cpp NsInfoClientTests.cpp |
| ImageGump | ImageGumpTests.cpp InventoryItemGumpTests.cpp RawMenuControlsTests.cpp |
| InventoryAboutMenu | InventoryAboutMenuTests.cpp |
| MenuGump | CampaignObjectMenuTests.cpp GumpDerivedRedrawTests.cpp HtmlWindowFitClientTests.cpp MenuGumpTests.cpp |
| MenuGumpControls | CampaignObjectMenuTests.cpp HtmlWindowFitClientTests.cpp |
| MenuGumpDraw | MenuCheckMarkerTests.cpp |
| MenuGumpInput | GumpDerivedRedrawTests.cpp |
| MenuGumpWidgets | CampaignObjectMenuTests.cpp HtmlWindowFactoryClientTests.cpp HtmlWindowFitClientTests.cpp HtmlWindowMetadataClientTests.cpp |
| MessageBoard | GameTimeSuppressionTests.inc WarningTests.cpp |
| MetaImageGump | EditorIslandLayoutTests.cpp MetaImageGumpTests.cpp MetaImageListTests.cpp RawMenuMetaImageTests.inc |
| MetaImageList | MetaImageListTests.cpp RawMenuImageListTests.inc |
| MinimapGeometry | MinimapBlitTests.cpp MinimapGeometryTests.cpp |
| ProductionHintGump | ProductionHintGumpTests.cpp |
| RawChatFormat | ChatFormatTests.cpp |
| RawChatMessage | ChatMessageTests.cpp ChatPacketTests.cpp ChatWindowClientTests.cpp |
| RawChatPacket | ChatPacketTests.cpp ChatWindowClientTests.cpp |
| RawChatReceive | ChatReceiveTests.cpp ChatWindowClientTests.cpp |
| RawChatSubmit | ChatPacketTests.cpp ChatSubmitTests.cpp ChatWindowClientTests.cpp |
| RawDotCommandDispatch | DotCommandDispatchTests.cpp |
| RawDotCommandRegister | ChatWindowClientTests.cpp DotCommandDispatchTests.cpp DotCommandRegisterTests.cpp |
| RawImage | CampaignEditorImageTests.inc EditorImageTests.cpp EditorIslandLayoutTests.cpp MetaImageGumpTests.cpp MetaImageListTests.cpp RawMenuImageListTests.inc RawMenuMetaImageTests.inc |
| RawIssueCommands | IssueCommandsTests.cpp |
| RawStormPowerRefresh | RecheckCdInputTests.cpp StormPowerRefreshTests.cpp |
| RawTextEntryContext | ChatWindowClientTests.cpp SaveFortNameClipboardTests.inc TextClipboardTests.cpp TextEntryClientTests.cpp |
| RawUserHighlight | CampaignObjectMenuTests.cpp CampaignSessionTests.cpp UserHighlightTests.cpp UserSelectionTests.cpp UtilityCommandInputTests.cpp |
| ScrollGump | GumpDrawTests.cpp |
| SpecialPointerTrace | PointerMenuParentTests.cpp |
| TextClipboard | SaveFortNameClipboardTests.inc TextClipboardTests.cpp |
| TextEntryBuffer | ChatWindowClientTests.cpp ChatWindowTestSupport.h TextEntryBufferTests.cpp TextEntryGumpTests.cpp |
| TextEntryGump | ChatWindowTestSupport.h TextEntryGumpTests.cpp |
| TextEntryStorage | ChatWindowClientTests.cpp SaveFortNameDialogTests.cpp TextClipboardTests.cpp TextEntryClientTests.cpp |
| TextGump | CampaignSessionTests.cpp DialogBodyLayoutTests.cpp InventoryItemGumpTests.cpp WarningTests.cpp |
| TraceChat | ChatWindowClientTests.cpp TraceChatTests.cpp |

## 처리 방법

1. 단위마다 시험 본문을 읽어 같은 입력 열/호출 순서로 웹 모듈을 재생하는 시험을 만들고 manifest 항목(`--write --only`)과 PORT_MAP 메모에 원본 행 수를 적는다.
2. 첫 실행에서 어긋나는 행은 시험 기대값 오류인지 모듈 결함인지 C++ 소스를 다시 읽어 확정한다(E465/E466의 일곱 건은 모두 첫 실행 일치였다).
3. 해당 모듈 머리 주석의 "직접 시험이 없다"는 문구를 연결 사실로 바꾼다(주석 전용 변경).
4. 연결한 단위는 이 표에서 줄을 지우지 말고 단위 문서 링크를 LEFT_JOBS 3.1절 5-B 뒤 "검증 이월" 체크 목록에서 체크한다.
