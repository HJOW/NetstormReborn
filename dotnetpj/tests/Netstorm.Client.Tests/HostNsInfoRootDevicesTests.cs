using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;

namespace Netstorm.Tests; // 세션 Root의 실제 외부 장치 공급을 대조한다.

// cpppj RawMenuControls와 Native 입력의 RU-3a 실제 장치 계약이다. 0.986에서 추가됐다.
public sealed class HostNsInfoRootDevicesTests
{
    /// <summary>같은 기본 장치 묶음을 만든다.</summary>
    private static HostNsInfoDevices BaseDevices()
    {
        return new HostNsInfoDevices
        {
            Read = _ => 0u,
            Translate = value => value,
            StoreText = _ => 1u,
            Text = _ => "",
            FormatZone = (format, _, _) => format,
            Changed = () => { },
        };
    }

    /// <summary>같은 Root 실장치 갈고리를 만든다.</summary>
    private static HostNsInfoRootDevicesHooks Hooks(
        Action<ReadOnlyMemory<byte>>? send = null,
        List<ReadOnlyMemory<byte>>? sent = null,
        Action? kickoff = null,
        Action? guy = null)
    {
        return new HostNsInfoRootDevicesHooks
        {
            SendStatus = send ?? (packet => sent?.Add(packet)),
            KickoffSound = kickoff ?? (() => { }),
            BattleGuySound = guy ?? (() => { }),
            ParticipantMenu = (_, _) => { },
            CenterOnSid = _ => { },
            MousePoint = () => new ScreenPoint(1, 2),
            BaseInput = (_, _, _) => 7u,
            MetaRootLayout = _ => { },
            MetaChildLayout = (_, _) => { },
            PlayerMenuBegin = () => { },
            PlayerMenuAttach = () => { },
            PlayerMenuTitle = _ => { },
            PlayerMenuItem = (_, _, _) => { },
            BattleSortMenu = _ => { },
            BattleOptionsMenu = (_, _) => { },
            BattleExcludeMenu = (_, _) => { },
            BattleOptionMenu = (_, _, _) => { },
            BattleRemoveMenu = _ => { },
        };
    }

    /// <summary>상태 패킷은 12BYTE를 만들고 채운 뒤 전송한다.</summary>
    [Fact]
    public void StatusPacketFillsTwelveBytesThenSends()
    {
        List<ReadOnlyMemory<byte>> sent = [];
        HostNsInfoRootDevices.SendStatusPacket(packet => { packet.Span[4] = 9; packet.Span[8] = 8; }, sent.Add);
        Assert.Single(sent);
        Assert.Equal(12, sent[0].Length);
        Assert.Equal(9, sent[0].Span[4]);
        Assert.Equal(8, sent[0].Span[8]);
    }

    /// <summary>묶음은 상태 송신을 12BYTE 구성으로 연결한다.</summary>
    [Fact]
    public void BuildWiresStatusSendThroughTwelveBytes()
    {
        List<ReadOnlyMemory<byte>> sent = [];
        HostNsInfoDevices devices = HostNsInfoRootDevices.Build(Hooks(sent: sent), BaseDevices());
        devices.StatusSend!(packet => packet.Span[0] = 0xab);
        Assert.Single(sent);
        Assert.Equal(0xab, sent[0].Span[0]);
    }

    /// <summary>묶음은 음향과 메뉴와 카메라와 입력과 배치를 같은 Root 실장치로 둔다.</summary>
    [Fact]
    public void BuildWiresSoundMenuCameraInputLayout()
    {
        List<(uint Index, ScreenPoint Point)> menus = [];
        List<uint> centers = [];
        ScreenPoint mouse = new(11, 22);
        List<uint> layouts = [];
        var hooks = new HostNsInfoRootDevicesHooks
        {
            SendStatus = _ => { },
            KickoffSound = () => { },
            BattleGuySound = () => { },
            ParticipantMenu = (index, point) => menus.Add((index, point)),
            CenterOnSid = sid => centers.Add(sid),
            MousePoint = () => mouse,
            BaseInput = (_, _, _) => 7u,
            MetaRootLayout = token => layouts.Add(token),
            MetaChildLayout = (_, _) => { },
            PlayerMenuBegin = () => { },
            PlayerMenuAttach = () => { },
            PlayerMenuTitle = _ => { },
            PlayerMenuItem = (_, _, _) => { },
            BattleSortMenu = _ => { },
            BattleOptionsMenu = (_, _) => { },
            BattleExcludeMenu = (_, _) => { },
            BattleOptionMenu = (_, _, _) => { },
            BattleRemoveMenu = _ => { },
        };
        HostNsInfoDevices devices = HostNsInfoRootDevices.Build(hooks, BaseDevices());
        devices.ParticipantMenu!(3u, mouse);
        devices.CenterOnSid!(9u);
        devices.MetaRootLayout!(5u);
        Assert.Equal((3u, mouse), menus[0]);
        Assert.Equal(9u, centers[0]);
        Assert.Equal(5u, layouts[0]);
        Assert.Equal(mouse, devices.MousePoint!());
        Assert.Equal(7u, devices.BaseInput!(1u, 2u, 3u));
    }

    /// <summary>묶음은 메뉴 갈고리를 같은 Root 실장치로 둔다.</summary>
    [Fact]
    public void BuildWiresPlayerAndBattleMenus()
    {
        List<string> titles = [];
        List<string> items = [];
        int sorts = 0;
        int removes = 0;
        var hooks = new HostNsInfoRootDevicesHooks
        {
            SendStatus = _ => { },
            KickoffSound = () => { },
            BattleGuySound = () => { },
            ParticipantMenu = (_, _) => { },
            CenterOnSid = _ => { },
            MousePoint = () => new ScreenPoint(0, 0),
            BaseInput = (_, _, _) => 0u,
            MetaRootLayout = _ => { },
            MetaChildLayout = (_, _) => { },
            PlayerMenuBegin = () => titles.Add("begin"),
            PlayerMenuAttach = () => titles.Add("attach"),
            PlayerMenuTitle = title => titles.Add(title),
            PlayerMenuItem = (text, _, _) => items.Add(text),
            BattleSortMenu = _ => sorts++,
            BattleOptionsMenu = (_, _) => { },
            BattleExcludeMenu = (_, _) => { },
            BattleOptionMenu = (_, _, _) => { },
            BattleRemoveMenu = _ => removes++,
        };
        HostNsInfoDevices devices = HostNsInfoRootDevices.Build(hooks, BaseDevices());
        devices.PlayerMenuBegin!();
        devices.PlayerMenuAttach!();
        devices.PlayerMenuTitle!("title");
        devices.PlayerMenuItem!("item", 1u, 2u);
        Assert.Equal(["begin", "attach", "title"], titles);
        Assert.Equal(["item"], items);
        Assert.Equal(0, sorts);
        devices.BattleSortMenu!(null!);
        Assert.Equal(1, sorts);
        devices.BattleRemoveMenu!(4u);
        Assert.Equal(1, removes);
    }

    /// <summary>기본 읽기과 번역과 문자열과 변경은 그대로 유지한다.</summary>
    [Fact]
    public void BuildKeepsBaseReadTranslateTextChanged()
    {
        HostNsInfoDevices baseDevices = BaseDevices();
        HostNsInfoDevices devices = HostNsInfoRootDevices.Build(Hooks(), baseDevices);
        Assert.Same(baseDevices.Read, devices.Read);
        Assert.Same(baseDevices.Translate, devices.Translate);
        Assert.Same(baseDevices.StoreText, devices.StoreText);
        Assert.Same(baseDevices.Text, devices.Text);
        Assert.Same(baseDevices.FormatZone, devices.FormatZone);
        Assert.Same(baseDevices.Changed, devices.Changed);
    }
}
