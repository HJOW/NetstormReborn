using System.Text;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host; // 실제 NSInfo 전체 생성을 같은 공통 메뉴/문자/수명에 연결한다.

/// <summary>RawNsInfoContext의 현재 주소/번역/공유 문자열과 선택된 실제 메타 후속이다.</summary>
public sealed class HostNsInfoDevices
{
    /// <summary>같은 현재 배경/TYPE/색 등 바깥 Client 전역이다.</summary>
    public required Func<uint, uint> Read { get; init; }
    /// <summary>같은 원문 글꼴 바이트의 현재 번역이다.</summary>
    public required Func<string, string> Translate { get; init; }
    /// <summary>원문 공용 형식 문자열 고리에 실제 문자열을 넣는다.</summary>
    public required Func<string, uint> StoreText { get; init; }
    /// <summary>같은 공용 고리의 현재 토큰을 해석한다.</summary>
    public required Func<uint, string> Text { get; init; }
    /// <summary>캡처한 printf format/server/zone의 실제 외부 형식화다.</summary>
    public required Func<string, string, int, string> FormatZone { get; init; }
    /// <summary>현재 NSInfo의 실제 메타맵 배치 전체다.</summary>
    public Action<uint>? MetaRootLayout { get; init; }
    /// <summary>현재 Player/Battle의 실제 메타맵 배치 전체다.</summary>
    public Action<HudNsInfoChild, uint>? MetaChildLayout { get; init; }
    /// <summary>현재 NSInfo/Player/Battle의 실제 원문 입력 분배다.</summary>
    public Func<uint, uint, uint, uint, uint>? Handle { get; init; }
    /// <summary>현재 실제 특수 버튼의 원문 그림이다.</summary>
    public Action<uint, GumpPixelTarget, GumpGlyphColorState, GumpBackgroundDraw>? DrawPlayerButton { get; init; }
    /// <summary>같은 Pool의 실제 SID+22 BYTE다. 추가 active 필터는 없다. 0.921에서 추가됐다.</summary>
    public Func<uint, uint>? ObjectOwner { get; init; }
    /// <summary>0042b210의 현재 TYPE 기술 합계다. 0.921에서 추가됐다.</summary>
    public Func<ReadOnlyMemory<byte>, uint>? TechWeight { get; init; }
    /// <summary>0042f480의 같은 현재 icon B/C와 전체 256BYTE 랭크 그림이다. 아이콘 틀 개수는 자산표에서 공급하며 0.922에서 추가됐다.</summary>
    public Func<int, string>? RankPicture { get; init; }
    /// <summary>0049a840→0049af80의 현재 TYPE 설명/번역/80BYTE 설정 치환이다. 0.923에서 추가됐다.</summary>
    public Func<uint, string>? Describe { get; init; }
    /// <summary>같은 실제 원시 마우스 좌표를 소비 시점에 읽는다. 루트 입력 결선에서 사용하며 0.925에서 추가됐다.</summary>
    public Func<ScreenPoint>? MousePoint { get; init; }
    /// <summary>같은 실제 참가자 메뉴 공장이다. 루트 입력 결선에서 사용하며 0.925에서 추가됐다.</summary>
    public Action<uint, ScreenPoint>? ParticipantMenu { get; init; }
    /// <summary>같은 실제 참가자 SID 카메라 중심 이동이다. 루트 입력 결선에서 사용하며 0.925에서 추가됐다.</summary>
    public Action<uint>? CenterOnSid { get; init; }
    /// <summary>같은 실제 1fh/12BYTE 헤더 뒤 부분효과 송신이다. 루트 입력 결선에서 사용하며 0.925에서 추가됐다.</summary>
    public Action<Action<Memory<byte>>>? StatusSend { get; init; }
    /// <summary>같은 실제 추방 확인 음향이다. 루트 입력 결선에서 사용하며 0.925에서 추가됐다.</summary>
    public Action? KickoffSound { get; init; }
    /// <summary>처리하지 않은 원래 세 DWORD의 같은 기본 입력 처리다. 루트 입력 결선에서 사용하며 0.925에서 추가됐다.</summary>
    public Func<uint, uint, uint, uint>? BaseInput { get; init; }
    /// <summary>같은 실제 540c74 구독자다. 플레이어 입력 결선에서 사용하며 0.926에서 추가됐다.</summary>
    public Func<uint>? PlayerSubscriber { get; init; }
    /// <summary>자기 특수 슬롯 변경 때만 쓰는 실제594fc4 DWORD다. 플레이어 입력 결선에서 사용하며 0.926에서 추가됐다.</summary>
    public Action<uint>? PlayerAuxiliary { get; init; }
    /// <summary>실제 현재 옵션87BYTE 송신이다. 플레이어 입력 결선에서 사용하며 0.926에서 추가됐다.</summary>
    public Action<uint, ReadOnlyMemory<byte>, uint>? PlayerData { get; init; }
    /// <summary>같은 Fancy controlled 메뉴 생성이다. 플레이어 입력 결선에서 사용하며 0.926에서 추가됐다.</summary>
    public Action? PlayerMenuBegin { get; init; }
    /// <summary>같은 Fancy 메뉴 부착이다. 플레이어 입력 결선에서 사용하며 0.926에서 추가됐다.</summary>
    public Action? PlayerMenuAttach { get; init; }
    /// <summary>같은 실제 메뉴 제목 추가다. 플레이어 입력 결선에서 사용하며 0.926에서 추가됐다.</summary>
    public Action<string>? PlayerMenuTitle { get; init; }
    /// <summary>같은 실제 메뉴 항목 작성이다. 플레이어 입력 결선에서 사용하며 0.926에서 추가됐다.</summary>
    public Action<string, uint, uint>? PlayerMenuItem { get; init; }
    /// <summary>같은 실제 guy.wav(0,0,0,0,0) 음향이다. 배틀 입력 결선에서 사용하며 0.927에서 추가됐다.</summary>
    public Action? BattleGuySound { get; init; }
    /// <summary>원문 Sort Game Options By 전체 Fancy 메뉴 자식이다. 배틀 입력 결선에서 사용하며 0.927에서 추가됐다.</summary>
    public Action<HudBattleInfoNode>? BattleSortMenu { get; init; }
    /// <summary>전체 활성 옵션 하위 메뉴와 현재 TYPE 분류/정렬의 제외 추가 메뉴다. 배틀 입력 결선에서 사용하며 0.927에서 추가됐다.</summary>
    public Action<HudBattleInfoNode, ReadOnlyMemory<byte>>? BattleOptionsMenu { get; init; }
    /// <summary>제외 추가 메뉴다. 배틀 입력 결선에서 사용하며 0.927에서 추가됐다.</summary>
    public Action<HudBattleInfoNode, ReadOnlyMemory<byte>>? BattleExcludeMenu { get; init; }
    /// <summary>선택 옵션 하나의 모든 포함 선택값 메뉴다. 배틀 입력 결선에서 사용하며 0.927에서 추가됐다.</summary>
    public Action<HudBattleInfoNode, ReadOnlyMemory<byte>, int>? BattleOptionMenu { get; init; }
    /// <summary>현재 TYPE 설명을 쓰는 제외 제거 확인 메뉴다. 배틀 입력 결선에서 사용하며 0.927에서 추가됐다.</summary>
    public Action<uint>? BattleRemoveMenu { get; init; }
    /// <summary>같은 현재 BattleOptions87BYTE 송신이다. 배틀 입력 결선에서 사용하며 0.927에서 추가됐다.</summary>
    public Action<uint, ReadOnlyMemory<byte>, uint>? BattleData { get; init; }
    /// <summary>복귀하면 계속하는 실제 원문 줄 번호 진단이다.</summary>
    public Action<uint>? Assertion { get; init; }
    /// <summary>실제 공통 구조 변경을 알린다.</summary>
    public required Action Changed { get; init; }
}

/// <summary>Source RawMenuControls의 AllocateNs/BindNs/CreateNsInfo/CloseNsInfo를 기존 실제 하위에 조립한다.
/// 일반 관문 뒤 선택하는 메타 배치·입력·특수 그림은 실제 장치를 요구하며 빈 성공으로 바꾸지 않는다.</summary>
public sealed class HostNsInfoWindows : IDisposable
{
    /// <summary>같은 실제 노드/등록/입력/수명/그림 소유자다.</summary>
    private readonly GumpButtonTree _tree;
    /// <summary>동일 스크롤/스타일/한 줄 문자 본체다.</summary>
    private readonly HostHtmlText _text;
    /// <summary>같은 현재 NSInfo/zone/서버 목록 owner다.</summary>
    private readonly ClientNsInfoGlobals _globals;
    /// <summary>같은 실제 모드/TYPE 물리다.</summary>
    private readonly RawSurfaceStorage _storage;
    /// <summary>같은 현재 PlayerKnown owner다.</summary>
    private readonly ClientGameplayGlobals _gameplay;
    /// <summary>같은 현재 외부 장치다.</summary>
    private readonly HostNsInfoDevices _devices;
    /// <summary>새 생성 도중 원문 첫0xd8 할당만 구별하는 호스트 문맥이다.</summary>
    private bool _allocatingRoot;
    /// <summary>현재 실제 NSInfo 파생의 소유 토큰이다.</summary>
    private readonly HashSet<uint> _roots = [];
    /// <summary>현재 실제 PlayerInfo 파생의 소유 토큰이다.</summary>
    private readonly HashSet<uint> _players = [];
    /// <summary>현재 실제 BattleInfo 파생의 소유 토큰이다.</summary>
    private readonly HashSet<uint> _battles = [];
    /// <summary>현재 실제 특수 버튼의 소유 토큰이다.</summary>
    private readonly HashSet<uint> _buttons = [];
    /// <summary>원문 주소별 현재 번역 바이트다. 형식 고리의 소유권을 가져오지 않는다.</summary>
    private readonly Dictionary<uint, string> _translated = [];
    /// <summary>같은 전체 부모 생성기다.</summary>
    private readonly HudNsInfoCreate _create;
    /// <summary>같은 일반 부착/현재 모드 관문이다.</summary>
    private readonly HudNsInfo _geometry;
    /// <summary>기본 물리 반납의 이전 장치다.</summary>
    private readonly Action<uint> _releaseBase;
    /// <summary>자기 특수 그림의 출력 수명 번호다.</summary>
    private readonly uint _drawing;
    /// <summary>수명 대여 반납 상태다.</summary>
    private bool _disposed;
    /// <summary>현재 선택된 실제 후속만 도달 위치에서 요구한다.</summary>
    private static T Need<T>(T? hook) where T : Delegate => hook ?? throw new InvalidOperationException("NSInfo 실제 현재 후속 장치 미연결");
    /// <summary>같은 실제 트리/문자/Client owner를 검증하고 실제 가상 분배만 먼저 설치한다. 생성은 Create까지 미룬다.</summary>
    public HostNsInfoWindows(GumpButtonTree tree, HostHtmlText text, ClientNsInfoGlobals globals, ClientGameplayGlobals gameplay, RawSurfaceStorage storage, HostNsInfoDevices devices)
    {
        ArgumentNullException.ThrowIfNull(tree); ArgumentNullException.ThrowIfNull(text); ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(gameplay); ArgumentNullException.ThrowIfNull(storage); ArgumentNullException.ThrowIfNull(devices);
        if (!ReferenceEquals(tree, text.Tree) || tree.Create.Collection.Edition != OriginalEdition.Patch1078 || storage.Pool.Edition != OriginalEdition.Patch1078 || storage.Types.Count == 0 || storage.Source() is not { } assets || !text.SharesPictureTypes(assets) || !ReferenceEquals(storage.Authority(), gameplay.Authority) || !ReferenceEquals(storage.GameAux(), gameplay.GameAux)) throw new ArgumentException("NSInfo 실제 공통 트리/10.78 물리 연결 불일치");
        if (devices.Read is null || devices.Translate is null || devices.StoreText is null || devices.Text is null || devices.FormatZone is null || devices.Changed is null) throw new ArgumentException("NSInfo 실제 현재 주소/번역/문자열 연결 누락", nameof(devices));
        _tree = tree; _text = text; _globals = globals; _gameplay = gameplay; _storage = storage; _devices = devices; _releaseBase = Need(tree.LifetimeHooks.Release);
        var hooks = new HudNsInfoCreateHooks
        {
            Node = Root, Player = token => _players.Contains(token) ? tree.Node(token) as HudPlayerInfoNode : null, Battle = token => _battles.Contains(token) ? tree.Node(token) as HudBattleInfoNode : null,
            Button = token => _buttons.Contains(token) ? tree.Node(token) as HudPlayerButtonNode : null, Base = tree.Node,
            AddTextFlags = (token, bits) => Scroll(token).TextFlags |= bits, AddButtonFlags = (token, bits) => Button(token).ButtonFlags |= bits,
            Read = Read, Allocate = Allocate, BaseConstruct = (token, command, width, height, flags) => tree.Factory.Construct(token, command, width, height, flags), Background = tree.Backgrounds.Construct,
            Scroll = text.Widgets.Construct, Text = text.Single.Construct, ButtonBase = (token, width, height, flags) => tree.Buttons.ConstructBase(token, width, height, flags),
            Stone = (token, width, height, label, color, context) => { if (context != 0) throw new InvalidOperationException("NSInfo 돌 버튼 실제 null 문맥 필요"); return tree.Stones.Construct(token, width, height, [.. Bytes(Text(label)), 0], color); },
            Scrollable = text.Widgets.SetScrollable, Slider = text.Widgets.SetSlider, Wrap = (token, value) => text.Widgets.SetWrap(token, unchecked((int)value)),
            ParseScroll = (token, value) => text.Parser.Parse(token, Bytes(Text(value))), ParseText = (token, value) => { var node = Plain(token); text.Style.Parse(ref node.Head, Bytes(Text(value)), node.TextContext); },
            FitText = (token, width, height) => text.Single.Fit(token, unchecked((int)width), unchecked((int)height)), FitStone = tree.Stones.FitToText,
            Attach = (token, x, y, parent, mode) => Need(tree.TreeHooks.Attach)(token, x, y, parent, mode), BaseAttach = tree.Tree.Attach,
            Translate = token => { _translated[token] = devices.Translate(Text(token, true)); return token; },
            Type = type => type < assets.LoadedTypes.Count ? 0x100000 + type : throw new ArgumentOutOfRangeException(nameof(type), "NSInfo 실제 최초 TYPE 범위"),
            ServerName = () => { globals.ResolveServerName(value => Bytes(devices.Translate(value))); return 0x594970; },
            FormatZone = (format, server, zone) => devices.StoreText(devices.FormatZone(Text(format), Text(server), zone)),
            Assertion = line => Need(devices.Assertion)(line), MetamapLayout = (kind, token) => Need(devices.MetaChildLayout)(kind, token),
        };
        _create = new(globals.Current, hooks);
        _geometry = new(new() { Metamap = () => Read(0x594fc8), PlayerKnown = () => Read(0x595344) }, new()
        {
            Node = Root, Base = tree.Node, BaseAttach = tree.Tree.Attach, Push = tree.Tree.Push,
            Attach = (token, x, y, parent, mode) => Need(tree.TreeHooks.Attach)(token, x, y, parent, mode), MetamapLayout = token => Need(devices.MetaRootLayout)(token),
        });
        var weak = new WeakReference<HostNsInfoWindows>(this); Func<uint, HostNsInfoWindows?> resolve = token => weak.TryGetTarget(out var owner) && !owner._disposed && owner.OwnsParent(token) ? owner : null;
        Bind(resolve, weak); ButtonGump.BindButtonGump(tree.EventHooks, tree.InputHooks, tree.FrameHooks, token => weak.TryGetTarget(out var owner) && !owner._disposed && owner._buttons.Contains(token) ? tree.Buttons : null);
        _drawing = tree.ExternalDrawing.RegisterExternalGumps(new() { Owns = token => weak.TryGetTarget(out var owner) && !owner._disposed && owner._buttons.Contains(token), Draw = (token, target, colors, background) => { if (!weak.TryGetTarget(out var owner)) throw new InvalidOperationException("NSInfo 실제 그림 owner 반납됨"); Need(owner._devices.DrawPlayerButton)(token, target, colors, background); } });
    }
    /// <summary>현재 같은 실제 파생 부모에만 가상 함수를 분배한다.</summary>
    private bool OwnsParent(uint token) => _roots.Contains(token) || _players.Contains(token) || _battles.Contains(token);
    /// <summary>현재 살아 있는 같은 NSInfo 주소다.</summary>
    private HudNsInfoNode? Root(uint token) => _roots.Contains(token) ? _tree.Node(token) as HudNsInfoNode : null;
    /// <summary>실제 같은 스크롤 주소만 요구한다.</summary>
    private ScrollingTextGumpNode Scroll(uint token) => _tree.Node(token) as ScrollingTextGumpNode ?? throw new InvalidOperationException("NSInfo 실제 스크롤 없음");
    /// <summary>실제 같은 특수 버튼 주소만 요구한다.</summary>
    private HudPlayerButtonNode Button(uint token) => _buttons.Contains(token) && _tree.Node(token) is HudPlayerButtonNode node ? node : throw new InvalidOperationException("NSInfo 실제 특수 버튼 없음");
    /// <summary>실제 같은 한 줄 문자 주소만 요구한다.</summary>
    private TextGumpNode Plain(uint token) => _tree.Node(token) as TextGumpNode ?? throw new InvalidOperationException("NSInfo 실제 한 줄 문자 없음");
    /// <summary>첫NUL 앞의 원문 글꼴 바이트만 실제 파서에 제공한다.</summary>
    private static byte[] Bytes(string text) { int end = text.IndexOf('\0'); return Encoding.Latin1.GetBytes(end < 0 ? text : text[..end]); }
    /// <summary>원문 정적 포인터/현재 번역/서버 버퍼/공유 형식 고리를 구별한다.</summary>
    private string Text(uint token, bool original = false)
    {
        if (!original && _translated.TryGetValue(token, out string? value)) return value;
        return token switch { 0x50c894 => "~w~E%s\nPlayers in Zone %d\n", 0x50cd3c => "~r~EBATTLEMASTER", 0x50cd2c => "Start Battle", 0x594970 => Encoding.Latin1.GetString(_globals.ServerName.AsSpan(0, Array.IndexOf(_globals.ServerName, (byte)0) is int end && end >= 0 ? end : _globals.ServerName.Length)), _ => _devices.Text(token) };
    }
    /// <summary>현재 owner의 같은 전역을 사용 위치에서 읽고 바깥 배경/TYPE/색만 실제 장치에 위임한다.</summary>
    private uint Read(uint address) => address switch
    {
        0x540421 => _globals.NsKind, 0x540440 => _globals.PlayerKind, 0x540457 => _globals.BattleKind, 0x5467b8 => _globals.Zone.Value, 0x540bac => _globals.ZoneFormat, 0x593150 => _globals.Current.Value,
        0x54db4c => unchecked((uint)_globals.ServerSelection.Value), 0x594fc8 => _storage.OwnerMode.Challenge ? 1U : 0U, 0x595344 => _gameplay.Player.PlayerKnown,
        _ => _devices.Read(address),
    };
    /// <summary>NUL 끝을 붙인 원문 글꼴 바이트로 본문 추가 길이를 센다. 0.921에서 추가됐다.</summary>
    private static byte[] AppendBytes(string text) => Encoding.Latin1.GetBytes(text + '\0');
    /// <summary>소유 문자열의 기존 반납/새 토큰/현재 노드 저장 순서를 도움말과 메타 창이 공유한다. 공통 트리 소유 표를 사용한다. 0.921에서 추가됐다.</summary>
    private void SetNodeHelp(uint token, string text, bool word)
    {
        // 원문 도움말 실제 노드 없음은 그 위치에서 거부한다.
        if (_tree.Node(token) is null)
        {
            throw new InvalidOperationException("NSInfo 도움말 실제 노드 없음");
        }

        if (word)
        {
            _tree.SetHelpWord(token, text);
        }
        else
        {
            _tree.SetHelpText(token, text);
        }
    }
    private string MetaHeader(bool battle)
    {
        string format = Need(_devices.Translate)(battle ? "~w~E%s\nPlayers in Battle (Zone %d)\n" : Text(_globals.ZoneFormat, true));
        int zone = unchecked((int)_globals.Zone.Value);
        string server = Encoding.Latin1.GetString(_globals.ResolveServerName(value => Bytes(Need(_devices.Translate)(value))).Span);
        string text = Need(_devices.FormatZone)(format, server, zone);
        if (text.Length >= 256)
        {
            throw new ArgumentOutOfRangeException(nameof(battle), "NSInfo 메타 제목의 실제256BYTE 범위");
        }

        return text;
    }
    /// <summary>같은 실제 CS와 현재 NSInfo/zone/서버/문자 저장소로 전체 메타 루트 본문을 배치한다. 0.921에서 추가됐다.</summary>
    /// <param name="token">같은 0xd8 NSInfo 노드 토큰이다.</param>
    /// <param name="challenge">같은 실제 CS 참가자40·전투10 기록이다.</param>
    public void LayoutMetaRoot(uint token, ClientChallengeGlobals challenge)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        ArgumentNullException.ThrowIfNull(challenge);
        var globals = new HudNsInfoMetaGlobals { Challenge = challenge, Info = _globals, Metamap = () => Read(0x594fc8), Known = () => Read(0x595344), Owner = () => _gameplay.Player.BattleOwner };
            var hooks = new HudNsInfoMetaHooks
            {
                Root = Root,
                Player = player => _players.Contains(player) ? _tree.Node(player) as HudPlayerInfoNode : null,
                Battle = battle => _battles.Contains(battle) ? _tree.Node(battle) as HudBattleInfoNode : null,
                Node = _tree.Node,
                Push = _tree.Tree.Push,
                FitHeight = target => _text.Text.FitHeight(target),
                Attach = (target, x, y, parent, mode) => Need(_tree.TreeHooks.Attach)(target, x, y, parent, mode),
                Replace = (target, text) => _text.Text.ReplaceText(target, AppendBytes(text), 0),
                Parse = (target, text) => _text.Parser.Parse(target, Bytes(text)),
                Wrap = (target, value) => _text.Widgets.SetWrap(target, unchecked((int)value)),
                Read = Read,
                ObjectOwner = sid => Need(_devices.ObjectOwner)(sid),
                Weight = core => Need(_devices.TechWeight)(core),
                Append = (target, text, context, metadata) => _text.Text.AppendText(target, AppendBytes(text), context, -1, metadata),
                Header = MetaHeader,
                Translate = text => Need(_devices.Translate)(text),
                HelpText = (target, text) => SetNodeHelp(target, text, false),
                HelpWord = (target, text) => SetNodeHelp(target, text, true),
            };
            HudNsInfoMetaLayout.Layout(token, globals, hooks);
    }
    /// <summary>같은 실제 스크롤 노드의 현재 플래그 상자를 빌린다. 배치는 마친 뒤 노드에 되돌린다. 0.922에서 추가됐다.</summary>
    private readonly Dictionary<uint, UIntBox> _textFlagBoxes = [];
    /// <summary>같은 실제 스크롤 노드의 현재 플래그 상자를 빌린다. 0.922에서 추가됐다.</summary>
    private UIntBox TextFlags(uint token)
    {
        ScrollingTextGumpNode node = Scroll(token);
        if (!_textFlagBoxes.TryGetValue(token, out UIntBox? box))
        {
            box = new UIntBox { Value = node.TextFlags };
            _textFlagBoxes.Add(token, box);
        }

        return box;
    }
    /// <summary>같은 실제 CS와 정렬표로 루트 원문 입력을 메뉴·카메라·송신에 연결한다. 0.925에서 추가됐다.</summary>
    /// <param name="event">원문 입력 event DWORD다.</param>
    /// <param name="first">원문 입력 첫 DWORD다.</param>
    /// <param name="second">원문 입력 둘째 DWORD다.</param>
    /// <param name="challenge">같은 실제 CS 참가자40·전투10 기록이다.</param>
    public uint HandleRootInput(uint @event, uint first, uint second, ClientChallengeGlobals challenge)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        ArgumentNullException.ThrowIfNull(challenge);
        var hooks = new NativeNsInfoInputHooks
        {
            Command = address => Read(address),
            Roster = row => row < (uint)_globals.RosterOrder.Length ? _globals.RosterOrder[row] : throw new ArgumentOutOfRangeException(nameof(second), "NSInfo 루트 입력의 실제 정렬표 범위"),
            Participant = index => challenge.Participant(index),
            Mouse = () => Need(_devices.MousePoint)(),
            Menu = (index, point) => Need(_devices.ParticipantMenu)(index, point),
            Center = sid => Need(_devices.CenterOnSid)(sid),
            Status = fill => Need(_devices.StatusSend)(fill),
            KickoffSound = () => Need(_devices.KickoffSound)(),
            Base = (@event, first, second) => Need(_devices.BaseInput)(@event, first, second),
        };
        return NativeNsInfoInput.Handle(@event, first, second, hooks);
    }
    /// <summary>같은 실제 CS와 파생 노드로 플레이어 원문 입력을 준비·슬롯·추방 확인에 연결한다. 0.926에서 추가됐다.</summary>
    /// <param name="token">같은 0xbc PlayerInfo 노드 토큰이다.</param>
    /// <param name="event">원문 입력 event DWORD다.</param>
    /// <param name="first">원문 입력 첫 DWORD다.</param>
    /// <param name="second">원문 입력 둘째 DWORD다.</param>
    /// <param name="challenge">같은 실제 CS 참가자40·전투10 기록이다.</param>
    public uint HandlePlayerInput(uint token, uint @event, uint first, uint second, ClientChallengeGlobals challenge)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        ArgumentNullException.ThrowIfNull(challenge);
        HudPlayerInfoNode node = _tree.Node(token) as HudPlayerInfoNode ?? throw new InvalidOperationException("NSInfo 실제 플레이어 노드 없음");
        var hooks = new NativePlayerInfoInputHooks
        {
            Command = address => Read(address),
            Serial = child => _tree.Node(child)?.Serial ?? throw new InvalidOperationException("PlayerInfo 입력의 실제 자식 없음"),
            ButtonValue = button => (_tree.Node(button) as ButtonGumpNode)?.Value ?? throw new InvalidOperationException("PlayerInfo 입력의 실제 버튼 없음"),
            Owner = () => _gameplay.Player.BattleOwner,
            Subscriber = () => Need(_devices.PlayerSubscriber)(),
            Participant = index => challenge.Participant(index),
            Battle = index => challenge.Battle(index),
            Auxiliary = value => Need(_devices.PlayerAuxiliary)(value),
            Status = fill => Need(_devices.StatusSend)(fill),
            Data = (battle, options, kind) => Need(_devices.PlayerData)(battle, options, kind),
            Translate = text => Need(_devices.Translate)(text),
            BeginMenu = () => Need(_devices.PlayerMenuBegin)(),
            AttachMenu = () => Need(_devices.PlayerMenuAttach)(),
            Title = text => Need(_devices.PlayerMenuTitle)(text),
            Item = (text, command, parameter) => Need(_devices.PlayerMenuItem)(text, command, parameter),
        };
        return NativePlayerInfoInput.Handle(node, @event, first, second, hooks);
    }
    /// <summary>같은 실제 CS와 파생 노드로 배틀 원문 입력을 순환·모드·설정·메뉴에 연결한다. 0.927에서 추가됐다.</summary>
    /// <param name="token">같은 0x16c BattleInfo 노드 토큰이다.</param>
    /// <param name="event">원문 입력 event DWORD다.</param>
    /// <param name="first">원문 입력 첫 DWORD다.</param>
    /// <param name="second">원문 입력 둘째 DWORD다.</param>
    /// <param name="challenge">같은 실제 CS 참가자40·전투10 기록이다.</param>
    public uint HandleBattleInput(uint token, uint @event, uint first, uint second, ClientChallengeGlobals challenge)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        ArgumentNullException.ThrowIfNull(challenge);
        HudBattleInfoNode node = _tree.Node(token) as HudBattleInfoNode ?? throw new InvalidOperationException("NSInfo 실제 배틀 노드 없음");
        var hooks = new NativeBattleInfoInputHooks
        {
            Metamap = () => Read(0x594fc8),
            Battle = index => challenge.Battle(index),
            Command = address => Read(address),
            Serial = child => _tree.Node(child)?.Serial ?? throw new InvalidOperationException("BattleInfo 입력의 실제 자식 없음"),
            GuySound = () => Need(_devices.BattleGuySound)(),
            SortMenu = battle => Need(_devices.BattleSortMenu)(battle),
            OptionsMenu = (battle, options) => Need(_devices.BattleOptionsMenu)(battle, options),
            ExcludeMenu = (battle, options) => Need(_devices.BattleExcludeMenu)(battle, options),
            OptionMenu = (battle, options, index) => Need(_devices.BattleOptionMenu)(battle, options, index),
            RemoveMenu = type => Need(_devices.BattleRemoveMenu)(type),
            Push = push => _tree.Tree.Push(push),
            Attach = (target, x, y, parent, mode) => Need(_tree.TreeHooks.Attach)(target, x, y, parent, mode),
            Data = (battle, options, kind) => Need(_devices.BattleData)(battle, options, kind),
        };
        return NativeBattleInfoInput.Handle(token, node, @event, first, second, hooks);
    }
    /// <summary>같은 실제 CS와 현재 NSInfo 전역으로 전체 메타 플레이어 본문을 배치한다. 0.922에서 추가됐다.</summary>
    /// <param name="token">같은 0xbc PlayerInfo 노드 토큰이다.</param>
    /// <param name="challenge">같은 실제 CS 참가자40·전투10 기록이다.</param>
    public void LayoutMetaPlayer(uint token, ClientChallengeGlobals challenge)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        ArgumentNullException.ThrowIfNull(challenge);
        foreach ((uint entry, UIntBox box) in _textFlagBoxes)
        {
            if (_tree.Node(entry) is ScrollingTextGumpNode node)
            {
                box.Value = node.TextFlags;
            }
        }

        try
        {
            var globals = new HudNsInfoMetaGlobals { Challenge = challenge, Info = _globals, Metamap = () => Read(0x594fc8), Known = () => Read(0x595344), Owner = () => _gameplay.Player.BattleOwner };
            var hooks = new HudNsInfoMetaHooks
            {
                Player = player => _players.Contains(player) ? _tree.Node(player) as HudPlayerInfoNode : null,
                Node = _tree.Node,
                Push = _tree.Tree.Push,
                TextFlags = TextFlags,
                Read = Read,
                Replace = (target, text) => _text.Text.ReplaceText(target, AppendBytes(text), 0),
                Parse = (target, text) => _text.Parser.Parse(target, Bytes(text)),
                Append = (target, text, context, metadata) => _text.Text.AppendText(target, AppendBytes(text), context, -1, metadata),
                AppendDefault = (target, text) => _text.Text.AppendText(target, AppendBytes(text), -1, 0),
                Enabled = (target, enabled) => _tree.Buttons.SetEnabled(target, enabled),
                Value = (target, value) => _tree.Buttons.SetValue(target, value),
                ReadyButton = button => _buttons.Contains(button) ? _tree.Node(button) as HudPlayerButtonNode : null,
                SlotRank = (options, slot) => BattleOptionSlots.BattleSlotRank(options.Span, slot),
                RankPicture = rank => Need(_devices.RankPicture)(rank),
                RequestTop = target => _text.Text.RequestScroll(target, 0),
                FitContent = target => _text.Text.FitContent(target, 0, 0),
                Translate = text => Need(_devices.Translate)(text),
                Attach = (target, x, y, parent, mode) => Need(_tree.TreeHooks.Attach)(target, x, y, parent, mode),
            };
            HudNsInfoMetaPlayer.Layout(token, globals, hooks);
        }
        finally
        {
            foreach ((uint entry, UIntBox box) in _textFlagBoxes)
            {
                if (_tree.Node(entry) is ScrollingTextGumpNode node)
                {
                    node.TextFlags = box.Value;
                }
            }
        }
    }
    /// <summary>같은 실제 CS와 현재 NSInfo 전역으로 전체 메타 배틀 본문을 배치한다. 0.923에서 추가됐다.</summary>
    /// <param name="token">같은 0x16c BattleInfo 노드 토큰이다.</param>
    /// <param name="challenge">같은 실제 CS 참가자40·전투10 기록이다.</param>
    public void LayoutMetaBattle(uint token, ClientChallengeGlobals challenge)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        ArgumentNullException.ThrowIfNull(challenge);
        foreach ((uint entry, UIntBox box) in _textFlagBoxes)
        {
            if (_tree.Node(entry) is ScrollingTextGumpNode node)
            {
                box.Value = node.TextFlags;
            }
        }

        try
        {
            var globals = new HudNsInfoMetaGlobals { Challenge = challenge, Info = _globals, Metamap = () => Read(0x594fc8), Known = () => Read(0x595344), Owner = () => _gameplay.Player.BattleOwner };
            var hooks = new HudNsInfoMetaHooks
            {
                Battle = battle => _battles.Contains(battle) ? _tree.Node(battle) as HudBattleInfoNode : null,
                Node = _tree.Node,
                Push = _tree.Tree.Push,
                TextFlags = TextFlags,
                Scrollable = (target, value) => _text.Widgets.SetScrollable(target, value),
                Slider = (target, value) => _text.Widgets.SetSlider(target, value),
                Replace = (target, text) => _text.Text.ReplaceText(target, AppendBytes(text), 0),
                Parse = (target, text) => _text.Parser.Parse(target, Bytes(text)),
                Read = Read,
                OptionValueText = (options, index) => BattleOptionTexts.BattleOptionValueText(options.Span, unchecked((uint)index), Need(_devices.Translate)),
                OptionNameText = index => BattleOptionTexts.BattleOptionNameText(unchecked((uint)index), Need(_devices.Translate)),
                Describe = type => Need(_devices.Describe)(type),
                Translate = text => Need(_devices.Translate)(text),
                Attach = (target, x, y, parent, mode) => Need(_tree.TreeHooks.Attach)(target, x, y, parent, mode),
                RequestTop = target => _text.Text.RequestScroll(target, 0),
            };
            HudNsInfoMetaBattle.Layout(token, globals, hooks);
        }
        finally
        {
            foreach ((uint entry, UIntBox box) in _textFlagBoxes)
            {
                if (_tree.Node(entry) is ScrollingTextGumpNode node)
                {
                    node.TextFlags = box.Value;
                }
            }
        }
    }
    /// <summary>원문 new 크기와 첫0xd8 문맥의 실제 파생을 같은 공통 저장소에 할당한다.</summary>
    private uint Allocate(int bytes)
    {
        GumpCreateNode node;
        if (bytes == 0xd8 && _allocatingRoot) { _allocatingRoot = false; node = new HudNsInfoNode(); }
        else node = bytes switch { 0xd8 => new ScrollingTextGumpNode(), 0xb8 => new GumpBackgroundNode(), 0x170 => new StoneButtonGumpNode(), 0xbc => new HudPlayerInfoNode(), 0x16c => new HudBattleInfoNode(), 0xc8 => new HudPlayerButtonNode(), 0xa4 => new TextGumpNode(), _ => throw new ArgumentOutOfRangeException(nameof(bytes), "NSInfo 실제 파생 할당 크기") };
        uint token = _tree.AllocateNode(node); if (node is HudNsInfoNode) _roots.Add(token); else if (node is HudPlayerInfoNode) _players.Add(token); else if (node is HudBattleInfoNode) _battles.Add(token); else if (node is HudPlayerButtonNode) _buttons.Add(token); return token;
    }
    /// <summary>원문 부착/입력과 물리 반납을 현재 자기 파생에만 분배한다.</summary>
    private void Bind(Func<uint, HostNsInfoWindows?> resolve, WeakReference<HostNsInfoWindows> weak)
    {
        var attach = _tree.TreeHooks.Attach; var events = _tree.EventHooks.Handle; var input = _tree.InputHooks.Handle; var release = _releaseBase;
        _tree.TreeHooks.Attach = (token, x, y, parent, mode) => { if (resolve(token) is { } owner) { if (owner._roots.Contains(token)) owner._geometry.Attach(token, x, y, parent, mode); else if (owner._players.Contains(token)) owner._create.AttachPlayer(token, x, y, parent, mode); else owner._create.AttachBattle(token, x, y, parent, mode); } else Need(attach)(token, x, y, parent, mode); };
        _tree.EventHooks.Handle = (token, code, first, second) => resolve(token) is { } owner ? Need(owner._devices.Handle)(token, code, first, second) : Need(events)(token, code, first, second);
        _tree.InputHooks.Handle = (token, code, first, second) => resolve(token) is { } owner ? Need(owner._devices.Handle)(token, code, first, second) : Need(input)(token, code, first, second);
        _tree.LifetimeHooks.Release = token => { if (weak.TryGetTarget(out var owner)) { owner._roots.Remove(token); owner._players.Remove(token); owner._battles.Remove(token); owner._buttons.Remove(token); } release(token); };
    }
    /// <summary>같은 실제 공통 owner의 전체 자식을 만들고 wrapper가 숨긴 현재 루트를 반환한다.</summary>
    public uint Create()
    {
        ObjectDisposedException.ThrowIf(_disposed, this); return _tree.WithNodeOwner(this, () => { _allocatingRoot = true; try { return _create.CreateHidden(); } finally { _allocatingRoot = false; } });
    }
    /// <summary>같은 현재 전역이 가리키는 실제 살아 있는 자기 부모만 반환한다.</summary>
    public uint Current() => Root(_globals.Current.Value) is { } node && (node.Flags & 0x10000) == 0 ? _globals.Current.Value : 0;
    /// <summary>공통 자식 우선 소멸과 미등록 부분 할당을 자기 owner 범위에서만 반납한다. 원문 current DWORD는 보존한다.</summary>
    public void Close()
    {
        // 현재 등록표 순서로 자기 노드만 Kill하고 이미 죽은 자식은 다시 요청하지 않는다.
        foreach (uint token in _tree.Create.Entries.Skip(1).ToArray()) if (token != 0 && ReferenceEquals(_tree.NodeOwner(token), this) && _tree.Node(token) is { } node && (node.Flags & 0x10000) == 0) _tree.Lifetime.Kill(token);
        _tree.Lifetime.Cleanup();
        // new 성공 뒤 기본 생성 전 실패한 자기 물리만 Source처럼 마지막에 반납한다.
        foreach (uint token in _tree.OwnedNodes(this)) { if (_tree.Create.Entries.Contains(token)) throw new InvalidOperationException("NSInfo 정리 뒤 실제 등록 객체 남음"); _roots.Remove(token); _players.Remove(token); _battles.Remove(token); _buttons.Remove(token); _releaseBase(token); }
        _devices.Changed();
    }
    /// <summary>자기 전체 노드 뒤 자기 특수 그림 대여만 반납하고 다른 UI/공유 문자는 유지한다.</summary>
    public void Dispose() { if (_disposed) return; Close(); _tree.ExternalDrawing.UnregisterExternalGumps(_drawing); _disposed = true; }
}
