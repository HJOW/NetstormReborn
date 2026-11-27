using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;
using static Netstorm.Client.Tests.InventoryRawSupport;

// 독립 원본 객체 정보의 현재 바이트와 외부 효과 순서를 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>객체 정보의 원본 입력 전체와 짧은 관문을 검증한다.</summary>
public sealed class RawObjectMenuInfoTests
{
    /// <summary>동일 물리 슬롯과 현재 TYPE·외부 효과의 한 검사 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>현재 객체의 실제 슬롯 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, true);
        /// <summary>현재 TYPE 표다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>현재 클래스 표다.</summary>
        public readonly string[] Classes = Enumerable.Repeat("", 188).ToArray();
        /// <summary>현재 정보 전역이다.</summary>
        public readonly ObjectMenuInfoState State = new();
        /// <summary>현재 동맹 표다.</summary>
        private readonly uint[] _alliances = new uint[81];
        /// <summary>독립 원본 입력 20개다.</summary>
        public uint[] Row = [];
        /// <summary>현재 로컬·편집·미션 값이다.</summary>
        private uint _local, _editor, _mission;
        /// <summary>제목과 정보 머리 호출 횟수다.</summary>
        private int _titles, _headers;
        /// <summary>원본 순서의 실제 외부 효과다.</summary>
        public readonly List<string> Events = [];

        /// <summary>현재 모드 조회를 같은 상태 필드에 연결한다.</summary>
        public Scene() { State.LocalOwner = () => _local; State.Editor = () => _editor; State.MissionPost = () => _mission; }
        /// <summary>원본 입력만 다시 설치하고 기대 효과를 만들지 않는다.</summary>
        public void Prepare(uint[] values)
        {
            Row = values; Assert.Equal(20, Row.Length); Events.Clear(); _local = Row[7]; _editor = Row[8]; _mission = Row[12]; _titles = _headers = 0;
            State.Invisible = 117; State.Paralyzed = 118;
            Span<byte> raw = Raw(Pool, 50); raw.Clear(); Put(raw, 0, 0x12200000); raw[10] = (byte)Row[1]; raw[34] = (byte)Row[2]; raw[40] = (byte)Row[10]; Put(raw, 14, Row[16]); Put(raw, 18, Row[17]);
            Array.Fill(Classes, "");
            // 각 입력의 TYPE 인스턴스를 새로 만들어 이전 행의 쓰기를 지운다.
            for (int index = 0; index < Types.Length; index++) Types[index] = new OriginalTypeRecord();
            Types[Row[1]].Flags2 = Row[5]; Types[Row[1]].Theme = unchecked((int)Row[6]); Classes[Row[1]] = Row[4] != 0 ? "Class % value" : "";
            Array.Clear(_alliances); _alliances[Row[2] * 9 + Row[7]] = Row[14];
        }
        /// <summary>현재 슬롯의 비정렬 DWORD를 입력 비트로 기록한다.</summary>
        private static void Put(Span<byte> raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[offset..], value);
        /// <summary>원본 경계마다 지정된 현재 값 변경만 실행한다.</summary>
        private void Mutate(string boundary)
        {
            Span<byte> raw = Raw(Pool, 50); uint kind = Row[1];
            if (Row[15] == 1 && boundary == "count") { raw[10] = 101; raw[34] = 2; raw[40] = 0x30; }
            if (Row[15] == 2 && boundary == "description") Classes[kind] = "";
            if (Row[15] == 3 && boundary == "firstTitle") { Classes[kind] = "Changed"; Types[kind].Flags2 = 0x10000; Types[kind].Theme = 4; }
            if (Row[15] == 4 && boundary == "relation") { _editor = 1; Put(raw, 14, BitConverter.SingleToUInt32Bits(-3.75f)); Put(raw, 18, BitConverter.SingleToUInt32Bits(4.5f)); }
            if (Row[15] == 5 && boundary == "subId") { Put(raw, 14, BitConverter.SingleToUInt32Bits(-17.5f)); Put(raw, 18, BitConverter.SingleToUInt32Bits(21.5f)); }
            if (Row[15] == 6 && boundary == "name") { _local = 2; _alliances[Row[2] * 9 + 2] = 1; }
            if (Row[15] == 7 && boundary == "invisibleHeader") raw[40] = 0;
            if (Row[15] == 8 && boundary == "invisibleTranslation") { State.Invisible = 99; raw[40] = 0x20; }
            if (Row[15] == 9 && boundary == "ground") { _mission = 1; raw[34] = 2; }
            if (Row[15] == 10 && boundary == "alignment") Types[kind].Theme = 5;
            if (Row[15] == 11 && boundary == "class") Classes[kind] = "Replacement % token";
            if (Row[15] == 12 && boundary == "firstHeader") Classes[kind] = "";
            if (Row[15] == 13 && boundary == "lastHeader") raw[40] = 0x30;
        }
        /// <summary>실제 외부 효과의 종류와 인자를 순서대로 남긴다.</summary>
        private void Event(string kind, string value) => Events.Add(kind + ":" + value);
        /// <summary>선택된 번역의 원본 입력을 관찰하고 현재 변경을 반영한다.</summary>
        public string Translate(string text)
        {
            Event("L", Hex(text));
            if (text is "Owner" or "Ally" or "Enemy") Mutate("relation");
            if (text == "~y~[I%d.*]Invisible!~.\n") Mutate("invisibleTranslation");
            if (text == "\nAlignment: ~i%s~.") Mutate("alignment");
            if (text == "\nClass: ~i%s~.") Mutate("class");
            return (Row[18] != 0 ? "é:" : "") + text;
        }
        /// <summary>원본 검사 경계의 %s·%d·%% 서식만 순서대로 실행한다.</summary>
        private string Format(string pattern, IReadOnlyList<PriestMenuArgument> values)
        {
            string result = PlayerMenuText.Format(pattern, values); Event("F", Hex(result)); return result;
        }
        /// <summary>실제 본체가 선택한 조회와 출력 효과만 제공한다.</summary>
        public ObjectMenuInfoHooks Hooks() => new()
        {
            Count = sid => { Event("C", sid.ToString(CultureInfo.InvariantCulture)); Mutate("count"); return Row[3]; },
            Description = kind => { Event("D", kind.ToString(CultureInfo.InvariantCulture)); Mutate("description"); return "Type" + kind; },
            OwnerName = owner => { Event("P", owner.ToString(CultureInfo.InvariantCulture)); Mutate("name"); return Row[19] != 0 ? "Player" + owner : "UNKNOWN"; },
            Translate = Translate, Substitute = value => { Event("S", Hex(value)); return "[" + value + "]"; }, Print = Format,
            Title = text => { Event("T", Hex(text)); if (++_titles == 1) Mutate("firstTitle"); },
            Header = text => { Event("H", Hex(text)); if (++_headers == 1) Mutate("firstHeader"); if (text.StartsWith("\nSubID:", StringComparison.Ordinal)) Mutate("subId"); if (text.Contains("Invisible!", StringComparison.Ordinal)) Mutate("invisibleHeader"); if (text == "\n") Mutate("lastHeader"); },
            Alliance = (owner, local) => _alliances[owner * 9 + local],
            GroundImmobile = sid => { Event("G", sid.ToString(CultureInfo.InvariantCulture)); Mutate("ground"); return Row[11] != 0; },
            OwnerStatus = owner => owner < 9 ? (byte)Row[13] : throw new ArgumentOutOfRangeException(nameof(owner)),
        };
        /// <summary>호출 뒤 실제 슬롯과 현재 전역·TYPE만 직렬화한다.</summary>
        public string Current() => Convert.ToHexStringLower(Pool.Slot(50)) + $"|{_local},{_editor},{_mission},{State.Invisible},{State.Paralyzed}|{Hex(Classes[Row[1]])}|{Types[Row[1]].Flags2},{unchecked((uint)Types[Row[1]].Theme)}";
    }

    /// <summary>독립 원본의 1,434행에 대해 외부 효과·반환·현재 슬롯을 전부 비교한다.</summary>
    [Fact]
    public void NativeAllObjectInformationEventsAndCurrentState()
    {
        var scene = new Scene(); int count = 0;
        // 각 행의 명시 입력만 준비하고 원본 관측값과 실제 출력을 비교한다.
        foreach (string[] row in Rows("object-menu-info"))
        {
            scene.Prepare(Inputs(row[0])); var info = new RawObjectMenuInfo(scene.Pool, scene.Types, scene.Classes, scene.State, scene.Hooks()); string result = "-";
            if (scene.Row[0] == 0) info.Describe(scene.Row[1], scene.Row[3], scene.Row[2], scene.Row[9]);
            else if (scene.Row[0] == 1) info.Summary(50);
            else if (scene.Row[0] == 2) info.CaptureStatus(50);
            else if (scene.Row[0] == 3) result = Hex(RawObjectMenuInfo.LevelName(scene.Row[3]));
            else result = Hex(scene.Translate(RawObjectMenuInfo.ThemeName(unchecked((int)scene.Row[6]))));
            string events = scene.Events.Count == 0 ? "-" : string.Join(';', scene.Events);
            Assert.True(events == row[1], $"입력 {row[0]}\n실제 {events}\n원본 {row[1]}");
            Assert.Equal(row[2], result); Assert.Equal(row[3], scene.Current()); count++;
        }
        Assert.Equal(1434, count);
    }
}
