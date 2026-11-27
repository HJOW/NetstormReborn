using System.Text;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

// 생산 정보 메뉴가 현재 세션과 공통 메뉴의 실제 수명을 공유한다.
namespace Netstorm.Client.Host;

/// <summary>생산 정보 메뉴에 필요한 현재 번역·이름·음향 경계다.</summary>
public sealed class HostProductionAboutDevices
{
    /// <summary>현재 TYPE의 번역된 표시 바이트 설명이다.</summary>
    public required Func<uint, string> Description { get; init; }
    /// <summary>현재 소유자 이름의 표시 바이트다.</summary>
    public required Func<uint, string> OwnerName { get; init; }
    /// <summary>원문 표시 바이트를 현재 언어로 번역한다.</summary>
    public required Func<string, string> Translate { get; init; }
    /// <summary>현재 설정으로 표시 바이트 문구를 치환한다.</summary>
    public required Func<string, string> Substitute { get; init; }
    /// <summary>NULL과 빈 값을 구별하는 현재 도움말 문구다.</summary>
    public required Func<string?> HelpWord { get; init; }
    /// <summary>현재 게임의 전역 음향이다.</summary>
    public required Action<string> Sound { get; init; }
    /// <summary>실제 메뉴 트리 변경을 알린다.</summary>
    public required Action Changed { get; init; }
}

/// <summary>생산 내용물의 설명과 비용을 실제 부착 메뉴에 작성한다.</summary>
public static class HostProductionAbout
{
    /// <summary>같은 세션·메뉴·내용물의 정보 창을 열고 원본 수락 값을 반환한다.</summary>
    public static uint Open(HostSession session, IReadOnlyList<RiftTypeDefinition> definitions, HostHtmlMenus menus,
        InventoryItemNode item, ScreenPoint point, Func<uint, bool> affordable, HostProductionAboutDevices devices)
    {
        ArgumentNullException.ThrowIfNull(session); ArgumentNullException.ThrowIfNull(definitions);
        ArgumentNullException.ThrowIfNull(menus); ArgumentNullException.ThrowIfNull(item);
        ArgumentNullException.ThrowIfNull(affordable); ArgumentNullException.ThrowIfNull(devices);
        uint accepted = 0;
        menus.OpenAttached(() =>
        {
            var info = session.MakeObjectMenuInfo(definitions, new ObjectMenuInfoHooks
            {
                Description = devices.Description, OwnerName = devices.OwnerName, Translate = devices.Translate,
                Substitute = devices.Substitute, Print = PlayerMenuText.Format,
                Title = value => menus.Controls.AddTitle(Encoding.Latin1.GetBytes(value)),
                Header = value => menus.Controls.AddHeader(Encoding.Latin1.GetBytes(value)),
            });
            var roles = session.GameAux;
            var state = new InventoryAboutState
            {
                SharedSuppressedClass = () => roles.GeyserType, SharedAltarType = () => roles.AltarTypeBox.Value,
                SharedDaisType = () => roles.DaisTypeBox.Value, SharedNuggetType = () => roles.NuggetType,
            };
            uint root = 0;
            var hooks = new InventoryAboutHooks
            {
                Begin = () => root = menus.Widgets.BeginFancy(), Root = () => menus.MenuRoot,
                TemplateOwner = () => session.MissionPlayer.TemplateOwner, Editor = () => session.Gameplay.World.Editor != 0,
                Cost = type => session.Reward.TypeCostUnits(type), Affordable = affordable, HelpWord = devices.HelpWord,
                Idle = menus.Widgets.SetIdle, Reference = menus.Widgets.Reference, Describe = info.Describe,
                Translate = devices.Translate, Print = (pattern, value) => PlayerMenuText.Format(pattern, [value]),
                Header = value => menus.Controls.AddHeader(Encoding.Latin1.GetBytes(value)), Sound = devices.Sound,
                Entry = (text, command, parameter, include, enabled, force) => menus.Controls.AddText(
                    Encoding.Latin1.GetBytes(text ?? throw new InvalidOperationException("생산 정보 메뉴의 실제 도움말 문구 없음")),
                    command, parameter, include, enabled, force),
                Attach = (location, parent) => menus.Menu.AttachCurrent(location.X, location.Y, parent),
            };
            accepted = new InventoryAboutMenu(session.Pool, state, hooks).Open(item, point);
            return root;
        }, devices.Changed);
        return accepted;
    }
}
