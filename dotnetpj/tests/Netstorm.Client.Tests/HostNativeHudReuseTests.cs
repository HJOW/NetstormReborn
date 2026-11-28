using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

// 실제 네 패널 공장과 동일 자산/트리를 사용하는 Native HUD 재진입을 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>전체 State 부모와 미션 적재를 실행하지 않고 실제 HUD의 교체·소유자·도킹 재사용을 검증한다.</summary>
public sealed class HostNativeHudReuseTests
{
    /// <summary>창과 음향 없이 원본 자산·실제 HTML/생산/미니맵/NSInfo/Chat 공장을 한 트리에 조립한다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>같은 실제 초기 자산과 설정 소유자다.</summary>
        public readonly HostStartupScene Startup;
        /// <summary>동일 파일 팔레트와8비트 논리 화면이다.</summary>
        public readonly Screen Screen = new(1024, 768);
        /// <summary>모든 패널이 공유하는 실제 장기 게임 전역이다.</summary>
        public readonly ClientGameplayGlobals Game = new();
        /// <summary>같은 TYPE/프레임/SHP/권한의 실제 장기 물리다.</summary>
        public readonly RawSurfaceStorage Storage;
        /// <summary>한 번만 적재해 모든 그림 공장이 빌리는 실제 자산이다.</summary>
        public readonly HostAssetTables Assets;
        /// <summary>같은 실제 SHP 헤더 판독기다.</summary>
        private readonly Netstorm.Client.Display.ShapeDatabase _shapes;
        /// <summary>HTML/Console/메뉴가 공유하는 실제 입력 큐다.</summary>
        private readonly InputQueue _queue = new();
        /// <summary>같은 실제 공통 트리의 장기 메뉴 소유자다.</summary>
        public readonly HostMainMenu Menu;
        /// <summary>같은 실제 HTML 창 수명과 파서다.</summary>
        public readonly HostHtmlWindows Html;
        /// <summary>동일 문자/스타일/조각/그림 자산이다.</summary>
        public readonly HostHtmlText Text;
        /// <summary>동일 Console 편집 버퍼 소유자다.</summary>
        private readonly HostTextEntry _entry;
        /// <summary>같은 controlled 메뉴 스택과 팝업이다.</summary>
        public readonly HostHtmlMenus Menus;
        /// <summary>실제 공통 수집/정렬/픽셀 출력이다.</summary>
        public readonly HostGumpDrawing Drawing;
        /// <summary>다음 노드 세대도 보존하는 실제 미니맵 bitmap/갱신 큐다.</summary>
        public readonly MetadisplayDirtyState MinimapState = new();
        /// <summary>실제 미니맵 현재 토큰/serial/색 전역이다.</summary>
        public readonly RawMinimapGlobals MinimapGlobals = new();
        /// <summary>같은 장기 물리의 실제 미니맵 공장이다.</summary>
        public readonly RawMinimapControls Minimap;
        /// <summary>실제 공유 생산 배경/예산serial 전역이다.</summary>
        public readonly ProductionMaintenanceGlobals ProductionGlobals = new();
        /// <summary>미션 없는 Client에서도 같은 실제 분산 SP를 읽는다.</summary>
        private readonly ClientSpGlobals _sp = new(new GameRandom(17), new() { Value = 23 }, new(), () => 1234567);
        /// <summary>노드 세대마다 새로 만들고 이전 출력 대여를 먼저 반납하는 생산 공장이다.</summary>
        public HostProductionWindows? Production;
        /// <summary>장기 NSInfo current/zone/서버 전역이다.</summary>
        public readonly ClientNsInfoGlobals NsInfoGlobals = new();
        /// <summary>같은 원문 파일 이름 고리다.</summary>
        private readonly RawFileName _fileNames = new();
        /// <summary>첫 실제 지식 패널 생성 때 한 번 준비하는 장기 공장 집합이다.</summary>
        private HostRootFactorySet? _factories;
        /// <summary>실제 장기 공장 집합을 요구하며 검사에서 대체 공장을 만들지 않는다.</summary>
        public HostRootFactorySet Factories => _factories ?? throw new InvalidOperationException("실제 Root 공장 생성 전 조회");
        /// <summary>같은128줄 로그/문자열/현재창 전역이다.</summary>
        public readonly ClientChatWindowGlobals ChatGlobals = new();
        /// <summary>같은 실제 Chat 명령 등록표다.</summary>
        private readonly ClientDotCommandGlobals _commands = new();
        /// <summary>기술 갱신도 같은 실제 장기 manager 관문을 사용한다.</summary>
        private readonly ClientChatManagerGlobals _managers = new();
        /// <summary>랭크 접두 갱신이 빌리는 실제 구독자 전역이다.</summary>
        private readonly ClientSubscriberGlobals _subscribers = new();
        /// <summary>같은 실제 Chat 표시/로그/Console 공장이다.</summary>
        public readonly HostChatWindows Chat;
        /// <summary>동일 공통 지도와 미니맵 갱신 DWORD를 빌리는 실제 도킹 전역이다.</summary>
        public readonly HudDockGlobals Dock;
        /// <summary>같은 네 공장을 생성/부착하는 실제 전체 부모다.</summary>
        public readonly RawUberRootControls Uber;
        /// <summary>네 공장이 실제로 선택된 횟수다. 반복 Ensure가 새 노드를 만들지 않는지 관측한다.</summary>
        public readonly int[] FactoryCalls = new int[4];
        /// <summary>외부 화면 변경 요청 횟수다. 장치 경계의 호출만 기록한다.</summary>
        private int _changed;
        /// <summary>모든 실제 패널이 사용하는 공통 트리를 반환한다.</summary>
        public GumpButtonTree Tree => Menu.Tree;

        /// <summary>동일 최초 자산을 한 번만 적재하고 각 실제 공장의 선택된 장치만 연결한다.</summary>
        public Scene()
        {
            string? data = GameDataLocator.FindDataDirectory();
            Assert.NotNull(data);
            Startup = new(data);
            Assert.True(Screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet));
            Startup.Draw(Screen);
            Menu = new(Startup, Screen, new()
            {
                Now = () => 1, Poll = _ => default, Sound = () => throw new InvalidOperationException("미선택 버튼 음향"),
                Redraw = (_, _) => _changed++, HandleBase = (_, _, _, _) => 0,
            });
            Html = new(Menu, _queue, () => 1, _ => throw new InvalidOperationException("미선택 HTML 명령"));
            Html.BindParser(Startup, _ => _changed++);
            Text = new(Menu, Html, Screen, _ => default, (_, _) => _changed++, _ => _changed++, _ => _changed++);
            Text.Bind(Html);
            _entry = new(Menu, Html, Text, Screen, () => 1, (_, _) => _changed++);
            Menus = new(Menu, Html, Text, Startup, Screen, (_, _) => _changed++);
            Menus.BindInput(_queue, _ => throw new InvalidOperationException("미선택 메뉴 음향"), (_, _) => _changed++);
            Storage = new(OriginalEdition.Patch1078, Game.Authority, Game.GameAux);
            Assets = new(Startup, Storage);
            _shapes = new(Assets.ShapeImage);
            byte[] pictureColors = SpriteColorMapBuilder.BuildSpriteColorMaps(Screen.LogicalPalette).Shadow;
            Text.BindPictures(new(Assets.TypeTable, Assets.Definitions, _shapes), () => pictureColors, Startup.Configuration);
            Drawing = new(Menu, Text, Menus, Screen, _entry);
            Dock = new(Tree.Create, Tree.TreeState, new() { Value = 1 }, Game.MapRevision);
            Minimap = new(Tree, Storage, MinimapState, MinimapGlobals, Game.MapRevision, new()
            {
                View = () => new(BitConverter.SingleToUInt32Bits(176), BitConverter.SingleToUInt32Bits(120), Dock.MapRect),
                Center = (_, _) => throw new InvalidOperationException("미선택 미니맵 카메라"),
                Move = (_, _) => throw new InvalidOperationException("미선택 미니맵 명령"),
                InputMode = () => throw new InvalidOperationException("미선택 미니맵 입력 모드"),
                Poll = () => throw new InvalidOperationException("미선택 미니맵 폴링"),
                BaseHandle = (_, _, _, _) => 0, Invalidate = (_, _) => _changed++, Changed = () => _changed++,
            });
            var runtime = new ClientChatWindowRuntime(ChatGlobals, new(OriginalEdition.Patch1078, () => 100), value => value);
            var geometry = new HudChatGeometry(new()
            {
                Chat = token => Tree.Node(token) as HudChatNode, Node = Tree.Node, BaseAttach = Tree.Tree.Attach,
                Push = Tree.Tree.Push, Attach = (token, x, y, parent, mode) => Tree.TreeHooks.Attach!(token, x, y, parent, mode),
                BottomIndex = token => Text.Text.AtBottom(token) ? 1u : 0,
                RequestBottom = (token, extra) => Text.Text.RequestBottom(token, unchecked((int)extra)),
                Challenge = () => Storage.OwnerMode.Challenge ? 1u : 0u, Current = () => ChatGlobals.Current.Value,
            });
            var help = new GumpHelp(Tree.Create, new(), new() { Node = Tree.Node });
            Chat = new(Menu, Html, Text, _entry, Menus, Screen, ChatGlobals, _commands, runtime, geometry, new()
            {
                Read = address => address switch
                {
                    0x541348 => unchecked((uint)Storage.Types.Count), 0x548714 => _managers.Current.Value,
                    0x594fc8 => Storage.OwnerMode.Challenge ? 1u : 0u, 0x594fbc => Storage.OwnerMode.Battle ? 1u : 0u,
                    0x5cae94 or 0x5cae98 => 300, 0x563e38 => Tree.Create.Entries[0],
                    _ => throw new InvalidOperationException("미선택 Chat 전역:" + address.ToString("x")),
                },
                Text = token => throw new InvalidOperationException("미선택 Chat 문자열:" + token), Translate = value => value,
                FrameCount = type => type >= 70 && type - 70 < Assets.Definitions.Count ? Assets.Definitions[(int)type - 70].Clusters.Count : 0,
                FrameHeight = (type, frame) => BinaryPrimitives.ReadInt16LittleEndian(_shapes.SquidHeader((int)type - 70, (int)frame).AsSpan(0x1a, 2)),
                Recipient = value => value is "" or "allies", Config = Startup.Configuration.GetInt, BaseHandle = help.Handle,
            });
            Uber = HostUberRoot.Assemble(Tree, Menus.State, Dock, new()
            {
                CreateMinimap = () => { FactoryCalls[0]++; return Minimap.Construct(); },
                CreateProduction = CreateProduction,
                CreateKnowledge = CreateKnowledge,
                CreateChat = () => { FactoryCalls[3]++; return Chat.Create(); },
            }, new()
            {
                Read = ReadUber, Write = WriteUber, Type = type => type < Storage.Types.Count ? type : throw new ArgumentOutOfRangeException(nameof(type)),
                Translate = ChatGlobals.Store, Help = (token, value) => Tree.SetHelpText(token, ChatGlobals.Strings[value]), HelpWord = Tree.SetHelpWord,
                RefreshTechnology = RefreshRank,
                Poll = _ => new(InputCode.Release, 0, 0), Cursor = _ => throw new InvalidOperationException("미선택 하드웨어 커서"),
                BaseHandle = (_, _, _, _) => 0, ClosePopups = () => Menus.RequestClosePopups(() => _changed++),
                Replaced = _ => Menu.ForgetReleasedControls(), Changed = () => _changed++,
                DockInvalidate = (_, _, _, _, _) => _changed++, DockFinishResize = () => _changed++,
                DockChatVisible = visible => Chat.Geometry.SetHelperVisible(Chat.CurrentRoot, visible ? 1u : 0u),
                DockChatInput = visible => Chat.Geometry.SetInputVisible(Chat.CurrentRoot, visible ? 1u : 0u),
            });
        }

        /// <summary>전체 부모가 읽는 실제 화면/배경/TYPE/미니맵 전역만 공급한다.</summary>
        private uint ReadUber(uint address) => address switch
        {
            0x540451 => 43, 0x5411c8 => Menu.BackgroundType, 0x542664 => 1,
            0x5c78f0 or 0x531860 => unchecked((uint)Screen.Width), 0x5c78f4 or 0x531864 => unchecked((uint)Screen.Height),
            0x5949d0 => MinimapGlobals.Current, 0x5ca920 => ProductionGlobals.BackgroundType,
            0x5ca924 => ProductionGlobals.BackgroundFrame, 0x5ca928 => ProductionGlobals.PanelFrame,
            0x565e70 => (Html.Parser ?? throw new InvalidOperationException("실제 HTML 파서 없음")).Globals.Background[0],
            _ => throw new InvalidOperationException("미선택 전체 HUD 전역:" + address.ToString("x")),
        };

        /// <summary>원문 생성자가 쓴 세 생산 전역을 같은 장기 객체에 기록한다.</summary>
        private void WriteUber(uint address, uint value)
        {
            switch (address)
            {
                case 0x5ca920: ProductionGlobals.BackgroundType = value; return;
                case 0x5ca924: ProductionGlobals.BackgroundFrame = value; return;
                case 0x5ca928: ProductionGlobals.PanelFrame = value; return;
                default: throw new InvalidOperationException("미선택 전체 HUD 쓰기:" + address.ToString("x"));
            }
        }

        /// <summary>같은 실제 장기 manager/Core 관문과 적재된 아이콘 프레임으로 기술 접두를 갱신한다.</summary>
        private void RefreshRank() => ClientChatRuntime.RefreshClientChatRankPrefix(_managers, _subscribers, Game.CoreData.ActiveBox,
            () => throw new InvalidOperationException("미선택 활성 Core"), () => Assets.IconFrameCount((byte)'B'),
            () => unchecked(Assets.IconFrameCount((byte)'B') * Assets.IconFrameCount((byte)'C') - 1),
            _ => throw new InvalidOperationException("미선택 랭크 진단"));

        /// <summary>이전 노드가 실제 반납된 생산 공장만 교체하고 제품과 같이 루트 Idle을 꺼 둔다.</summary>
        private uint CreateProduction()
        {
            FactoryCalls[1]++;
            if (Production is { RootToken: not 0 }) throw new InvalidOperationException("실제 생산 부모 중복 생성");
            Production?.Dispose();
            Production = new(Tree, Text, new()
            {
                Storage = Storage, Gameplay = Game, Players = Game.PlayerRecords, Options = Game.BattleOptions, Sp = _sp,
            }, ProductionGlobals, new()
            {
                Now = () => 1, Sound = _ => throw new InvalidOperationException("미선택 생산 음향"),
                Tell = _ => throw new InvalidOperationException("미선택 생산 알림"), ConfigInt = (_, fallback) => fallback,
                Translate = value => value, Changed = () => _changed++, BaseHandle = (_, _, _, _) => 0,
                Redraw = (_, _) => _changed++, ClosePopups = () => Menus.RequestClosePopups(() => _changed++),
                MenuInstances = () => Menus.State.Instances,
            });
            uint token = Production.Construct();
            Tree.Lifetime.SetIdle(token, false);
            Production.SetPalette(Screen.LogicalPalette);
            return token;
        }

        /// <summary>첫 실제 NSInfo 창 생성 이후에는 같은 장기 공장의 실제 재생성 관문을 사용한다.</summary>
        private uint CreateKnowledge()
        {
            FactoryCalls[2]++;
            if (_factories is not null) return _factories.RecreatePrimary();
            _factories = new(Tree, Text, NsInfoGlobals, Game, Storage, _fileNames, new()
            {
                BaseDevices = new()
                {
                    Read = address => HostNsInfoExternalRead.TryRead(address, Menu.BackgroundType, Screen.Colors(), out uint value) ? value : address switch
                    {
                        0x5ca920 => ProductionGlobals.BackgroundType, 0x5ca924 => ProductionGlobals.BackgroundFrame, 0x5c85c8 => 0,
                        _ => throw new InvalidOperationException("미선택 NSInfo 전역:" + address.ToString("x")),
                    },
                    Translate = value => value, StoreText = ChatGlobals.Store, Text = token => ChatGlobals.Strings[token],
                    FormatZone = (format, server, zone) => format.Replace("%s", server, StringComparison.Ordinal).Replace("%d", zone.ToString(System.Globalization.CultureInfo.InvariantCulture), StringComparison.Ordinal),
                    Changed = () => _changed++,
                },
                Root = new()
                {
                    SendStatus = _ => throw new InvalidOperationException("미선택 NSInfo 송신"),
                    KickoffSound = () => throw new InvalidOperationException("미선택 추방 음향"), BattleGuySound = () => throw new InvalidOperationException("미선택 참가자 음향"),
                    ParticipantMenu = (_, _) => throw new InvalidOperationException("미선택 참가자 메뉴"), CenterOnSid = _ => throw new InvalidOperationException("미선택 참가자 카메라"),
                    MousePoint = () => default, BaseInput = (_, _, _) => 0,
                    MetaRootLayout = _ => throw new InvalidOperationException("미선택 메타 Root 배치"), MetaChildLayout = (_, _) => throw new InvalidOperationException("미선택 메타 자식 배치"),
                    PlayerMenuBegin = () => throw new InvalidOperationException("미선택 Player 메뉴"), PlayerMenuAttach = () => throw new InvalidOperationException("미선택 Player 부착"),
                    PlayerMenuTitle = _ => throw new InvalidOperationException("미선택 Player 제목"), PlayerMenuItem = (_, _, _) => throw new InvalidOperationException("미선택 Player 항목"),
                    BattleSortMenu = _ => throw new InvalidOperationException("미선택 전투 정렬"), BattleOptionsMenu = (_, _) => throw new InvalidOperationException("미선택 전투 옵션"),
                    BattleExcludeMenu = (_, _) => throw new InvalidOperationException("미선택 제외 메뉴"), BattleOptionMenu = (_, _, _) => throw new InvalidOperationException("미선택 개별 옵션"),
                    BattleRemoveMenu = _ => throw new InvalidOperationException("미선택 제외 해제"),
                },
                Message = new()
                {
                    BackgroundName = () => HtmlTextParser.HtmlBackgroundName(Html.Parser!.Globals), StoredText = token => ChatGlobals.Strings[token],
                    StoreText = ChatGlobals.Store, TryRead = _ => throw new InvalidOperationException("미선택 배경 파일"),
                    LogProbe = _ => throw new InvalidOperationException("미선택 배경 로그"), ChangePalette = _ => throw new InvalidOperationException("미선택 배경 팔레트"),
                    Construct = (_, _) => throw new InvalidOperationException("미선택 배경 GIF"), MessageEvent = (_, _, _, _) => throw new InvalidOperationException("미선택 GIF 사건"),
                },
            });
            return NsInfoGlobals.Current.Value;
        }

        /// <summary>제품 Native Ensure와 같은 실제 owner 검증 뒤 팔레트/예산/미니맵/채팅 숨김만 갱신한다.</summary>
        public UberRootNode Reuse()
        {
            var root = HostUberRoot.CurrentRoot(Tree, Uber) ?? throw new InvalidOperationException("현재 실제 Native Root 없음");
            var production = Production ?? throw new InvalidOperationException("현재 실제 생산 공장 없음");
            Assert.True(Factories.Borrows(Tree, Text, NsInfoGlobals, Game, Storage));
            var mini = HostUberRoot.RequireChild(Tree, root, root.Minimap, Minimap.RootToken, Minimap);
            Assert.Same(Minimap.Node(root.Minimap), mini);
            var panel = Assert.IsType<ProductionRootNode>(HostUberRoot.RequireChild(Tree, root, root.Production, production.RootToken, production));
            HostUberRoot.RequireChild(Tree, root, root.Knowledge, root.Knowledge, Factories.Primary);
            Assert.True(Factories.OwnsToken(root.Knowledge));
            Assert.IsType<HudChatNode>(HostUberRoot.RequireChild(Tree, root, root.Chat, Chat.CurrentRoot, Chat));
            Minimap.SetPalette(Screen.Colors());
            production.SetPalette(Screen.LogicalPalette);
            production.Idle(panel.Budget);
            Minimap.Refresh();
            Uber.Handle(Tree.Create.Entries[0], 0x02000088, 0, 0);
            return root;
        }

        /// <summary>실제 공통 State Kill/Cleanup만 호출하고 등록0의 부모는 Replace에 남긴다.</summary>
        public void CleanupStateNodes()
        {
            Tree.KillStateGumps();
            Tree.Lifetime.Cleanup();
            Menu.ForgetReleasedControls();
        }

        /// <summary>원문 전체 공장 교체 뒤 실제 mode3 도킹을 선택한다.</summary>
        public UberRootNode Replace()
        {
            Uber.Replace();
            var root = HostUberRoot.CurrentRoot(Tree, Uber) ?? throw new InvalidOperationException("새 실제 Native Root 없음");
            Uber.Handle(Tree.Create.Entries[0], 0x0200003d, 3, 0);
            return root;
        }

        /// <summary>실제 자식 우선 수명으로 닫고 장기 공유 문자/로그 대여를 마지막에 반납한다.</summary>
        public void Dispose()
        {
            Uber.Dispose();
            Production?.Dispose();
            Minimap.Dispose();
            Chat.Dispose();
            _factories?.Dispose();
            Html.CloseAll();
            _entry.Dispose();
        }
    }

    /// <summary>같은 실제 부모의 일곱 HUD 직계 자식 토큰을 읽는다.</summary>
    private static uint[] Children(UberRootNode root) => [root.Vertical, root.MinimapDivider, root.ChatDivider, root.Minimap, root.Production, root.Knowledge, root.Chat];

    /// <summary>실제 도킹의 위치/기본 크기/현재 크기/부모를 값으로 기록한다.</summary>
    private static (uint Token, float X, float Y, float Width, float Height, float BaseWidth, float BaseHeight, uint Parent)[] Geometry(Scene scene, UberRootNode root)
        => Children(root).Select(token => { var node = scene.Tree.Node(token)!; return (token, node.X, node.Y, node.Width, node.Height, node.BaseWidth, node.BaseHeight, node.Parent); }).ToArray();

    /// <summary>세 실제 구분선의 유효한 원문 serial/관계 슬롯만 순서대로 기록한다.</summary>
    private static (uint Master, uint Serial, uint Flags)[] Slaves(Scene scene, UberRootNode root)
        => new[] { root.Vertical, root.MinimapDivider, root.ChatDivider }
            .SelectMany(token => Assert.IsType<HudDividerNode>(scene.Tree.Node(token)).Slaves.Where(slot => slot.Serial != 0).Select(slot => (token, slot.Serial, slot.Flags))).ToArray();

    /// <summary>실제 네 공장과 shared bitmap/로그를 두 State 세대에서 재사용하며 같은 세대의 Ensure는 도킹을 덮지 않는다.</summary>
    [Fact]
    public void ActualNativeFactoriesSurviveStateCleanupAndReuseWithoutDuplicatingDocking()
    {
        using var scene = new Scene();
        Assert.Null(HostUberRoot.CurrentRoot(scene.Tree, scene.Uber));
        scene.CleanupStateNodes();
        UberRootNode root = scene.Replace();
        scene.Reuse();
        var factories = scene.Factories;
        var primary = factories.Primary;
        var message = factories.MessageProbe;
        byte[] bitmap = scene.MinimapState.Bitmap;
        var log = scene.ChatGlobals.Log;
        var pictureTypes = scene.Storage.Source();
        uint nodeCount = scene.Tree.Create.Count;

        // 두 실제 노드 세대 모두 같은 수명/도킹 계약을 지키며 세 번째 표시를 임의 생성하지 않는다.
        for (int generation = 0; generation < 2; generation++)
        {
            scene.Tree.Tree.Move(root.Vertical, 110, 0, 0);
            scene.Tree.Tree.Move(root.MinimapDivider, 0, scene.Screen.Height - 110, 0);
            scene.Reuse();
            var geometry = Geometry(scene, root);
            var slaves = Slaves(scene, root);
            int[] calls = scene.FactoryCalls.ToArray();
            Assert.Equal(10, slaves.Length);
            Assert.Equal(4, scene.Tree.OwnedNodes(scene.Uber).Length);
            Assert.Equal(nodeCount, scene.Tree.Create.Count);
            scene.Reuse();
            scene.Reuse();
            Assert.Equal(geometry, Geometry(scene, root));
            Assert.Equal(slaves, Slaves(scene, root));
            Assert.Equal(calls, scene.FactoryCalls);
            Assert.Equal(nodeCount, scene.Tree.Create.Count);
            Assert.Same(factories, scene.Factories);
            Assert.Same(primary, scene.Factories.Primary);
            Assert.Same(message, scene.Factories.MessageProbe);
            Assert.Same(bitmap, scene.MinimapState.Bitmap);
            Assert.Same(log, scene.ChatGlobals.Log);
            Assert.Same(log, scene.Text.Buffers.Binding(0x548758).State);
            Assert.Same(pictureTypes, scene.Storage.Source());
            Assert.True(scene.Text.SharesPictureTypes(pictureTypes!));
            Assert.Equal(new[] { generation + 1, generation + 1, generation + 1, generation + 1 }, scene.FactoryCalls);
            scene.Drawing.Draw();

            if (generation == 1) break;
            uint oldRoot = scene.Tree.Create.Entries[0];
            uint[] oldChildren = Children(root);
            var previousProduction = scene.Production!;
            scene.CleanupStateNodes();
            Assert.Same(root, scene.Tree.Node(oldRoot));
            // 등록0은 원문 State Kill에서 남고, 네 패널과 구분선은 실제로 반납된다.
            foreach (uint child in oldChildren) Assert.Null(scene.Tree.Node(child));
            Assert.Equal(0u, scene.Minimap.RootToken);
            Assert.Equal(0u, previousProduction.RootToken);
            Assert.Equal(0u, scene.Chat.CurrentRoot);
            Assert.Equal(0u, primary.Current());
            Assert.Throws<InvalidOperationException>(() => HostUberRoot.CurrentRoot(scene.Tree, scene.Uber));
            root = scene.Replace();
            Assert.Null(scene.Tree.Node(oldRoot));
            Assert.NotSame(previousProduction, scene.Production);
            Assert.False(previousProduction.HasNodes);
            scene.Reuse();
        }
    }

    /// <summary>추가 NSInfo가 공유 current를 바꾸어도 실제 첫 지식 패널의 소유자는 그대로 유지한다.</summary>
    [Fact]
    public void ExtraNsInfoCurrentDoesNotReplaceActualPrimaryHudOwner()
    {
        using var scene = new Scene();
        scene.CleanupStateNodes();
        UberRootNode root = scene.Replace();
        scene.Reuse();
        var primary = scene.Factories.Primary;
        uint token = root.Knowledge;
        scene.Factories.CreateExtra();
        Assert.NotEqual(token, scene.NsInfoGlobals.Current.Value);
        Assert.Equal(0u, primary.Current());
        Assert.Same(primary, scene.Tree.NodeOwner(token));
        int[] calls = scene.FactoryCalls.ToArray();
        scene.Reuse();
        Assert.Equal(token, root.Knowledge);
        Assert.Equal(calls, scene.FactoryCalls);
        Assert.False(scene.Factories.Borrows(scene.Tree, scene.Text, new(), scene.Game, scene.Storage));
        Assert.True(scene.Factories.Borrows(scene.Tree, scene.Text, scene.NsInfoGlobals, scene.Game, scene.Storage));
    }

    /// <summary>현재 Native Root와 실제 자식 identity가 어긋나면 수동 HUD나 다른 공장으로 조용히 대체하지 않는다.</summary>
    [Fact]
    public void ActualNativeReuseRejectsForeignRootOwnerAndChangedChildIdentity()
    {
        using var scene = new Scene();
        scene.CleanupStateNodes();
        UberRootNode root = scene.Replace();
        scene.Reuse();
        Assert.Throws<InvalidOperationException>(() => HostUberRoot.CurrentRoot(scene.Tree, null));
        using var foreign = new Scene();
        Assert.Throws<InvalidOperationException>(() => HostUberRoot.CurrentRoot(scene.Tree, foreign.Uber));
        Assert.Throws<InvalidOperationException>(() => HostUberRoot.RequireChild(scene.Tree, root, root.Minimap, scene.Production!.RootToken, scene.Minimap));
        Assert.Throws<InvalidOperationException>(() => HostUberRoot.RequireChild(scene.Tree, root, root.Minimap, scene.Minimap.RootToken, scene.Chat));
        uint vertical = root.Vertical;
        try
        {
            root.Vertical = root.Minimap;
            Assert.Throws<InvalidOperationException>(() => HostUberRoot.CurrentRoot(scene.Tree, scene.Uber));
        }
        finally { root.Vertical = vertical; }
        uint production = root.Production;
        try
        {
            root.Production = root.Minimap;
            Assert.Throws<InvalidOperationException>(() => scene.Reuse());
        }
        finally { root.Production = production; }
        var panel = scene.Production!.Node(production)!;
        uint flags = panel.Flags;
        try
        {
            panel.Flags |= 0x10000;
            Assert.Throws<InvalidOperationException>(() => HostUberRoot.CurrentRoot(scene.Tree, scene.Uber));
            Assert.Throws<InvalidOperationException>(() => HostUberRoot.RequireChild(scene.Tree, root, production, production, scene.Production));
        }
        finally { panel.Flags = flags; }
        var mini = scene.Minimap.Node(root.Minimap)!;
        uint parent = mini.Parent;
        try
        {
            mini.Parent = 0;
            Assert.Throws<InvalidOperationException>(() => HostUberRoot.RequireChild(scene.Tree, root, root.Minimap, scene.Minimap.RootToken, scene.Minimap));
        }
        finally { mini.Parent = parent; }
        scene.Reuse();
    }
}
