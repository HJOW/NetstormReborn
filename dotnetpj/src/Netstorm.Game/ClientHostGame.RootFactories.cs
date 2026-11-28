using System.Text;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

// RU-3b Root와 생산과 채팅과 메시지 공장을 실제 Root가 만들고 소유한다. 0.987에서 추가됐다.
public sealed partial class ClientHostGame
{
    /// <summary>같은 미션 Root가 소유하는 NSInfo와 메시지 공장 집합이다. 진입할 때 만든다.</summary>
    private HostRootFactorySet? _rootFactories;
    /// <summary>같은 미션 Root가 기록한 12BYTE 상태 송신 outbox다. R11 수송이 비운다.</summary>
    private readonly List<byte[]> _rootStatusOutbox = [];
    /// <summary>R11 수송이 비우는 같은 미션 Root의 상태 송신 기록이다.</summary>
    public IReadOnlyList<byte[]> RootStatusOutbox => _rootStatusOutbox;
    /// <summary>같은 미션 Root가 기록한 87BYTE 옵션 송신 outbox다. R11 수송이 비운다.</summary>
    private readonly List<(uint Battle, byte[] Options, uint Kind)> _rootOptionOutbox = [];
    /// <summary>R11 수송이 비우는 같은 미션 Root의 옵션 송신 기록이다.</summary>
    public IReadOnlyList<(uint Battle, byte[] Options, uint Kind)> RootOptionOutbox => _rootOptionOutbox;

    /// <summary>같은 메뉴 주인과 저장소로 Root와 메시지 공장을 만들고 실제 Root에 묶는다.</summary>
    /// <param name="storage">같은 실제 풀과 자산 저장소다.</param>
    /// <remarks>2026-11-25 0.987에서 RU-3b용으로 추가했다. 근거: cpppj RawMenuControls.cpp의 BindNs와 UberGump.cpp 2200행의 바깥 읽기. 생산 공장은 세션 장치(0.825~0.837·BuildProductionWorldDevices), 채팅 공장은 메뉴 경로(_nativeChatWindows)로 이미 소유하므로 여기서는 Root와 메시지 공장만 만든다.</remarks>
    private HostRootFactorySet EnsureRootFactories(RawSurfaceStorage storage)
    {
        var menu = _mainMenu ?? throw new InvalidOperationException("Root 공장의 실제 메뉴 owner 없음");
        var text = _htmlText ?? throw new InvalidOperationException("Root 공장의 실제 본문 owner 없음");
        if (_rootFactories is { } current)
        {
            if (!current.Borrows(menu.Tree, text, _nsInfoGlobals, _gameplay, storage))
                throw new InvalidOperationException("미션 Root 공장 재사용의 같은 장기 소유자 필요");
            return current;
        }
        var menus = _htmlMenus ?? throw new InvalidOperationException("Root 공장의 실제 메뉴 열 owner 없음");
        var startup = _startup ?? throw new InvalidOperationException("미션 진입에 시작 화면 없음");
        var audio = _missionAudio ?? throw new InvalidOperationException("Root 음향의 실제 미션 음향 없음");
        // 같은 문자열 저장소를 바깥 UI 장치가 공유한다. VFS 고리와 수명을 분리하지 않는다.
        var strings = new Dictionary<uint, string>();
        uint nextString = 0x2000000u;
        uint StoreString(string value)
        {
            uint token = nextString;
            nextString++;
            strings.Add(token, value);
            return token;
        }

        // 원본 xlat 표의 같은 현재 언어 번역이다. 없으면 원문을 보존한다.
        string Translate(string value)
        {
            byte[]? bytes = startup.Files.TryReadAllBytes("d/xlat." + (startup.Configuration.Configuration.Get("currentLanguage") ?? "english"));
            var table = bytes is null ? null : Netstorm.Assets.XlatTable.FromFileBytes(bytes);
            return table?.Translate(value) ?? value;
        }

        // 원문 바깥 전역 읽기다. UberGump.cpp 2200행의 같은 주소 분기를 따른다.
        uint ReadOuter(uint address)
        {
            if (HostNsInfoExternalRead.TryRead(address, menu.BackgroundType, _driver.Screen.Colors(), out uint value)) return value;
            return address switch
            {
            // 전체 부모가 준비한 뒤에는 같은 생산 전역을 현재 조회한다. 부모가 없는 기존 직접 진입에서만 종전 메뉴 준비값을 쓴다.
            0x5ca920u => _productionGlobals.BackgroundType != 0 ? _productionGlobals.BackgroundType : menu.BackgroundType,
            0x5ca924u => _productionGlobals.BackgroundType != 0 ? _productionGlobals.BackgroundFrame : 1u,
            // 0x5c85c8은 원본 MetadataMode다. 시험의 Basic[5] 대신 실제 전역을 읽는다.
            0x5c85c8u => _localState.MetadataMode.Value,
            0x542714u => _nsInfoGlobals.LeaveMenuCommand,
            0x542960u => 0x0200009bu,
            0x542790u => 0x0200009bu,
            0x54278cu => 0x02000026u,
            0x5429dcu => _nsInfoGlobals.KickoffCommand,
            0x542b80u => _nsInfoGlobals.SortOptionsCommand,
            0x542b7cu => _nsInfoGlobals.SetOptionCommand,
            0x5429e0u => _nsInfoGlobals.ExcludeTypeCommand,
            0x5429e4u => _nsInfoGlobals.RestoreTypeCommand,
            0x5427a8u => 0x0200002cu,
            _ => throw new InvalidOperationException("Root 바깥 UI 읽기 확장:" + address.ToString("x")),
            };
        }
        var baseDevices = new HostNsInfoDevices
        {
            Read = ReadOuter,
            Translate = Translate,
            StoreText = StoreString,
            Text = token => strings[token],
            FormatZone = (format, server, zone) => format.Replace("%s", server, StringComparison.Ordinal).Replace("%d", zone.ToString(System.Globalization.CultureInfo.InvariantCulture), StringComparison.Ordinal),
            Changed = () => _menuDirty = true,
            ObjectOwner = sid =>
            {
                if (sid > 65535 || sid >= (uint)storage.Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(sid), "NSInfo 참가자 SID의 실제 Pool 범위");
                return storage.Pool.Slot(unchecked((int)sid))[0x22];
            },
            TechWeight = core => ClientMetaCoreWeight.Weight(core.Span, () => storage.Types.Count, index => (storage.Types[index].TechBitIndex, storage.Types[index].Level)),
            RankPicture = RankPicture,
            Describe = type => DescribeType(startup, storage, type, Translate),
            Assertion = line => ErrorLog.Note("NSInfo 원문 진단 줄: " + line),
            PlayerSubscriber = () => _subscriberGlobals.MySubscriber.Value,
            PlayerAuxiliary = value => _gameplay.StateAuxFC4.Value = value,
            PlayerData = (battle, options, kind) => _rootOptionOutbox.Add((battle, options.ToArray(), kind)),
            BattleData = (battle, options, kind) => _rootOptionOutbox.Add((battle, options.ToArray(), kind)),
        };
        HostNsInfoWindows? created = null;
        var rootHooks = new HostNsInfoRootDevicesHooks
        {
            SendStatus = packet => _rootStatusOutbox.Add(packet.ToArray()),
            KickoffSound = () => audio.PlayGlobal("chalKickedoff.wav"),
            BattleGuySound = () => audio.PlayGlobal("guy.wav"),
            ParticipantMenu = (_, _) => throw new InvalidOperationException("참가자 메뉴는 R5 팝업 연결에서 잇는다"),
            CenterOnSid = _ => throw new InvalidOperationException("중앙 카메라는 R5 카메라 연결에서 잇는다"),
            MousePoint = () => new ScreenPoint(_previousMouse.X, _previousMouse.Y),
            BaseInput = (_, _, _) => 0u,
            MetaRootLayout = token => (created ?? throw new InvalidOperationException("NSInfo 배치의 실제 창 없음")).LayoutMetaRoot(token, _challenge),
            MetaChildLayout = (kind, token) => LayoutMetaChild(created ?? throw new InvalidOperationException("NSInfo 배치의 실제 창 없음"), kind, token),
            PlayerMenuBegin = () => menus.Widgets.BeginVertical(0),
            PlayerMenuAttach = () => { _ = menus.Menu.Current(); },
            PlayerMenuTitle = title => menus.Controls.AddTitle(Bytes(title)),
            PlayerMenuItem = (label, code, parameter) => menus.Controls.AddText(Bytes(label), code, parameter, true, true, false),
            BattleSortMenu = battle => NativeBattleInfoMenus.BuildBattleSortMenu(battle, BattleMenuHooks(menus, ReadOuter, Translate)),
            BattleOptionsMenu = (_, _) => throw new InvalidOperationException("전투 옵션 메뉴는 R5 메뉴 연결에서 잇는다"),
            BattleExcludeMenu = (_, _) => throw new InvalidOperationException("제외 추가 메뉴는 R5 메뉴 연결에서 잇는다"),
            BattleOptionMenu = (_, _, _) => throw new InvalidOperationException("포함 선택값 메뉴는 R5 메뉴 연결에서 잇는다"),
            BattleRemoveMenu = _ => throw new InvalidOperationException("제외 제거 메뉴는 R5 메뉴 연결에서 잇는다"),
        };
        var messageHooks = new UberMessageProbeHooks
        {
            BackgroundName = NativeHudBackgroundName,
            StoredText = token => strings[token],
            StoreText = StoreString,
            TryRead = startup.Files.TryReadAllBytes,
            LogProbe = text => ErrorLog.Note(text),
            ChangePalette = _ => throw new InvalidOperationException("메시지 팔레트 전체 부모는 R5 표시 연결에서 잇는다"),
            Construct = (_, _) => throw new InvalidOperationException("메시지 배경 후단 생성은 R5 표시 연결에서 잇는다"),
            MessageEvent = (_, _, _, _) => throw new InvalidOperationException("메시지 배경 사건은 R5 표시 연결에서 잇는다"),
        };
        var set = new HostRootFactorySet(menu.Tree, text, _nsInfoGlobals, _gameplay, storage, _staticStrings.FileNames(), new() { BaseDevices = baseDevices, Root = rootHooks, Message = messageHooks });
        created = set.Primary;
        _rootFactories = set;
        return set;
    }

    /// <summary>같은 순위 아이콘 자산을 현재 TYPE 프레임에서 읽는다.</summary>
    /// <param name="rank">표시할 순위다.</param>
    private string RankPicture(int rank)
    {
        byte[] output = new byte[256];
        byte[] temporary = new byte[256];
        var hooks = new ChatManagerPrefixHooks
        {
            MinorCount = () => AssetTables.IconFrameCount((byte)'B'),
            MaximumRank = () =>
            {
                int minor = AssetTables.IconFrameCount((byte)'B');
                int major = AssetTables.IconFrameCount((byte)'C');
                return unchecked(minor * major - 1);
            },
            Assertion = line => ErrorLog.Note("NSInfo 랭크 원문 진단 줄: " + line),
        };
        var result = RawChatManagerPrefix.ChatPrefixIcons(output, temporary, rank, hooks);
        int end = result.IndexOf((byte)0);
        return Encoding.Latin1.GetString(end < 0 ? result : result[..end]);
    }

    /// <summary>같은 TYPE 설명을 번역과 설정 치환으로 만든다. 80BYTE를 넘기면 거부한다.</summary>
    /// <param name="startup">같은 실제 설정 owner다.</param>
    /// <param name="storage">같은 실제 자산 저장소다.</param>
    /// <param name="type">설명할 TYPE 번호다.</param>
    /// <param name="translate">같은 현재 언어 번역이다.</param>
    private static string DescribeType(HostStartupScene startup, RawSurfaceStorage storage, uint type, Func<string, string> translate)
    {
        if (type >= (uint)storage.Types.Count) throw new ArgumentOutOfRangeException(nameof(type), "NSInfo 설명의 실제 TYPE 범위");
        string text = translate(storage.Types[unchecked((int)type)].Description);
        if (text.Length >= 80) throw new ArgumentOutOfRangeException(nameof(type), "메타 제외 TYPE 설명의 실제80BYTE 복사 범위");
        string value = startup.Configuration.SubstituteLocal(text);
        if (value.Length >= 80) throw new ArgumentOutOfRangeException(nameof(type), "메타 제외 TYPE 설명의 실제80BYTE 치환 범위");
        return value;
    }

    /// <summary>메타 자식 종류별로 같은 실제 플레이어와 배틀 본문을 배치한다.</summary>
    private void LayoutMetaChild(HostNsInfoWindows windows, HudNsInfoChild kind, uint token)
    {
        if (kind == HudNsInfoChild.Player) windows.LayoutMetaPlayer(token, _challenge);
        else windows.LayoutMetaBattle(token, _challenge);
    }

    /// <summary>원문 글꼴 바이트에 끝 NUL을 덧붙인다.</summary>
    private static byte[] Bytes(string text) => Encoding.Latin1.GetBytes(text + '\0');

    /// <summary>정렬 메뉴의 같은 fancy 작성 장치다. 메뉴 열 infra를 빌린다.</summary>
    private static NativeBattleInfoMenuHooks BattleMenuHooks(HostHtmlMenus menus, Func<uint, uint> read, Func<string, string> translate) => new()
    {
        BeginFancy = () => menus.Widgets.BeginVertical(0),
        Title = title => menus.Controls.AddTitle(Bytes(title)),
        Read = read,
        Item = (text, command, parameter, enabled) => menus.Controls.AddText(Bytes(text), command, parameter, true, enabled, false),
        Attach = () => { _ = menus.Menu.Current(); },
        Translate = translate,
    };
}
