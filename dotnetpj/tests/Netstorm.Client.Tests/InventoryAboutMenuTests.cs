using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;
using Netstorm.Tests;
using static Netstorm.Client.Tests.InventoryRawSupport;

namespace Netstorm.Client.Tests; // 실제 Affordable와 현재 별칭을 사용해 정보 메뉴 전체를 대조한다.

/// <summary>원본1296+2592행과 효과 뒤 현재 TYPE/클래스 getter를 검사한다.</summary>
public sealed class InventoryAboutMenuTests
{
    /// <summary>원본이 공급한 안정된 메뉴/부모 주소다.</summary>
    private const uint Menu = 0x12402000;
    /// <summary>실제 입력 바이트와 명시 메뉴 작성 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 내용물 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, true);
        /// <summary>같은 실제 입력 항목이다.</summary>
        public readonly InventoryItemNode Item = new() { Content = 50, Object = 314159 };
        /// <summary>같은 실제 명령/현재 별칭이다.</summary>
        public readonly InventoryAboutState State = new();
        /// <summary>현재 실제 편집 관문이다.</summary>
        public bool Editor;
        /// <summary>현재 실제 정수 비용이다.</summary>
        public int Cost;
        /// <summary>현재 실제 float SP다.</summary>
        public float Money;
        /// <summary>NULL/빈 문자열/Help 명시 선택이다.</summary>
        public uint Help;
        /// <summary>원문 전체 효과 순서다.</summary>
        public string Trace = "";
        /// <summary>메뉴 비용 조회의 명시 변이 경계다.</summary>
        public Action? CostEffect;
        /// <summary>편집 관문 조회의 명시 변이 경계다.</summary>
        public Action? EditorEffect;
        /// <summary>실제 작성한 회수 라벨이다.</summary>
        public readonly List<string> Salvage = [];
        /// <summary>같은 실제 정보 메뉴 본체다.</summary>
        public readonly InventoryAboutMenu About;
        /// <summary>같은 실제 현재4af010과 원문 입력/효과만 연결한다.</summary>
        public Scene(uint[] input, bool withClass)
        {
            int shift = withClass ? 1 : 0; var raw = Raw(Pool, 50); raw.Clear(); raw[10] = withClass ? (byte)input[0] : (byte)0; BinaryPrimitives.WriteUInt16LittleEndian(raw[18..], (ushort)input[shift]); BinaryPrimitives.WriteUInt16LittleEndian(raw[20..], (ushort)input[shift + 1]); Editor = input[shift + 2] != 0; Cost = unchecked((int)input[shift + 3]); Money = F(input[shift + 4]); Help = input[shift + 5];
            var inventory = new InventoryItemGump(Pool, new() { LocalOwner = () => 1, Editor = () => Editor, StockEnabled = () => 0, InputDisabled = () => 0, LastType = new(), BridgeType = () => 82, PixelsX = () => 16, PixelsY = () => 11, BackgroundType = () => 0, BackgroundFrame = () => 0 }, new() { CostUnits = _ => Cost, Money = owner => { Event("Money," + owner); return Money; } });
            About = new(Pool, State, new()
            {
                Begin = () => { Event("Begin"); return Menu; }, Root = () => Menu, TemplateOwner = () => 2, Editor = () => { EditorEffect?.Invoke(); return Editor; }, Cost = _ => { CostEffect?.Invoke(); return Cost; }, Affordable = content => inventory.Affordable(content) != 0,
                HelpWord = () => Help == 0 ? null : Help == 2 ? "Help" : "", Idle = (menu, on) => { Assert.Equal(Menu, menu); Event("Idle," + (on ? 1 : 0)); }, Reference = (menu, sid) => { Assert.Equal(Menu, menu); Event("Object," + sid); },
                Describe = (kind, count, owner, sid) => Event("Describe," + kind + ',' + count + ',' + owner + ',' + sid), Translate = text => { Event("Translate," + Hex(text)); return text; }, Print = (pattern, value) => pattern.Replace("%d", value.ToString(CultureInfo.InvariantCulture), StringComparison.Ordinal), Header = text => Event("Header," + Hex(text)),
                Entry = (text, command, parameter, include, enabled, force) => { Event("Entry," + (text is null ? "-" : Hex(text)) + ',' + command + ',' + parameter + ',' + (include ? 1 : 0) + ',' + (enabled ? 1 : 0) + ',' + (force ? 1 : 0)); if (command == State.SalvageCommand) Salvage.Add(text!); },
                Attach = (point, parent) => Event("Attach," + unchecked((uint)point.X).ToString(CultureInfo.InvariantCulture) + ',' + unchecked((uint)point.Y).ToString(CultureInfo.InvariantCulture) + ',' + parent), Sound = name => Event("Sound," + Hex(name) + ",0,0,0,0,0"),
            });
        }
        /// <summary>실제 외부 효과의 순서만 기록한다.</summary>
        private void Event(string text) { if (Trace.Length != 0) Trace += ';'; Trace += text; }
    }
    /// <summary>입력 전체를 실제 본체에 넣고 풀/항목 불변과 모든 원문 효과를 비교한다.</summary>
    [Theory]
    [InlineData("inventory-about", false, 1296)]
    [InlineData("inventory-about-class", true, 2592)]
    public void NativeAllClassCostHelpAndOrderedEffects(string name, bool withClass, int expected)
    {
        int count = 0;
        // 기대 출력은 재생성하지 않고 원문 전체 행을 읽는다.
        foreach (var row in Rows(name)) { Assert.Equal(2, row.Length); var scene = new Scene(Inputs(row[0]), withClass); byte[] before = All(scene.Pool).ToArray(); string item = GumpRawSupport.Encode(scene.Item); Assert.Equal(1u, scene.About.Open(scene.Item, new(17, -9))); Assert.Equal(row[1], scene.Trace); Assert.True(before.AsSpan().SequenceEqual(All(scene.Pool))); Assert.Equal(item, GumpRawSupport.Encode(scene.Item)); count++; }
        Assert.Equal(expected, count);
    }
    /// <summary>비용/편집 장치 뒤의 현재 제단/다이스/너겟/제외 클래스를 조회한다.</summary>
    [Fact]
    public void CurrentAliasesReadAfterCostAndEditorAndOverrideIndependentCopies()
    {
        var scene = new Scene([6, 70, 32, 0, 1000, B(2000), 2], true); uint altar = 164, dais = 165, nugget = 154; int costs = 0;
        scene.State.SharedAltarType = () => altar; scene.State.SharedDaisType = () => dais; scene.State.SharedNuggetType = () => nugget; scene.CostEffect = () => { if (++costs == 2) nugget = 70; };
        scene.About.Open(scene.Item, new(17, -9)); altar = 70; scene.About.Open(scene.Item, new(17, -9)); altar = 164; dais = 70; scene.About.Open(scene.Item, new(17, -9)); dais = 165; nugget = 154; scene.About.Open(scene.Item, new(17, -9));
        Assert.Equal(8, costs); Assert.Equal(new[] { "Salvage for 1000~[IfortGump.3]", "Salvage for 0~[IfortGump.3]", "Salvage for 0~[IfortGump.3]", "Salvage for 250~[IfortGump.3]" }, scene.Salvage);
        var other = new Scene([6, 70, 32, 0, 1000, B(2000), 2], true); uint suppressed = 122; other.State.SharedSuppressedClass = () => suppressed; other.EditorEffect = () => suppressed = 6; other.About.Open(other.Item, new(17, -9)); Assert.Empty(other.Salvage); other.EditorEffect = null; suppressed = 122; other.State.SuppressedClass = 6; other.About.Open(other.Item, new(17, -9)); Assert.Single(other.Salvage);
    }
}
