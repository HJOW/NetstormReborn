using System.Buffers.Binary;
using System.Globalization;
using System.Runtime.InteropServices;
using Netstorm.Client.Input;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 원본 반환/사건/현재 필드를 제품 계산으로 만들지 않고 독립 관찰과 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>실제 Pool/장기 전역의 선택·해제·ESC·다중 선택 순서와 실패 지점을 검증한다.</summary>
public sealed class UserSelectionTests
{
    /// <summary>원문 십진 DWORD 목록이다.</summary>
    private static uint[] Numbers(string text) => text.Split(',').Select(X86Fixture.UInt).ToArray();
    /// <summary>동일 물리 입력과 명시 callback 변경을 각 행에 준비한다.</summary>
    private sealed class Scene
    {
        /// <summary>원본과 같은24000개50BYTE 물리 슬롯이다.</summary>
        public readonly RawSidPool Pool = new(24000, true);
        /// <summary>각 callback 뒤 현재 조회할188TYPE다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>선택과 강조의 동일 장기 필드다.</summary>
        public readonly ClientUserSelectionGlobals Owner = new();
        /// <summary>현재 독립 명시 입력이다.</summary>
        public uint[] Row = [];
        /// <summary>외부 경계 호출 순서와 인자다.</summary>
        public readonly List<string> Events = [];
        /// <summary>일반 선택의 현재 차단 DWORD다.</summary>
        private uint _blocked;
        /// <summary>다중 관측의 첫 이동/표시 callback을 구분한다.</summary>
        private int _moves, _refreshes;
        /// <summary>예약/반납 상태도 포함한 원문 독립 raw 입력을 같은 물리에 주입한다. 게임 할당 성공으로 해석하지 않는다.</summary>
        public Span<byte> Raw(int sid) => MemoryMarshal.CreateSpan(ref MemoryMarshal.GetReference(Pool.Slot(sid)), Pool.SlotStride);
        /// <summary>원문 폭의 BYTE/WORD/DWORD를 쓴다.</summary>
        public void Write(int sid, int offset, uint value, int width = 4)
        {
            // 낮은 바이트부터 지정한 폭만 갱신한다.
            for (int index = 0; index < width; index++) Raw(sid)[offset + index] = (byte)(value >> (index * 8));
        }
        /// <summary>원본의 세미콜론 사건 셀을 unsigned 십진수로 기록한다.</summary>
        public void Event(string name, params uint[] values) => Events.Add(name + (values.Length == 0 ? "" : ":" + string.Join(':', values.Select(value => value.ToString(CultureInfo.InvariantCulture)))));
        /// <summary>관찰이 명시한 경계 변경만 제공하며 부모의 다음 읽기는 제품이 수행한다.</summary>
        private void Mutate(uint boundary)
        {
            if (Row[27] != boundary) return;
            switch (boundary)
            {
                case 1: Owner.MultipleCount.Value = 1; Owner.MouseType.Value = 77; break;
                case 2: Owner.Selected.Value = 99; break;
                case 3: Write(50, 10, 101, 1); break; case 4: Write(50, 10, 102, 1); break;
                case 5: Write(50, 40, 1, 1); break; case 6: Owner.Selected.Value = 0; break;
                case 7: Owner.Remembered[0] = Row[23]; Owner.Remembered[1] = Row[24]; break;
                case 8: Owner.Selected.Value = 202; Owner.MouseType.Value = 77; break;
                case 9: Owner.Enabled.Value = 0; _blocked = 1; break;
                case 10: Owner.Selected.Value = 99; Owner.MouseType.Value = 77; break;
                case 11: Write(60, 4, 65, 2); break; case 12: Owner.MultipleCount.Value = 5; break;
            }
        }
        /// <summary>원문31DWORD의 현재 필드와 슬롯 입력을 이전 실행과 분리해 준비한다.</summary>
        public void Prepare(uint[] values)
        {
            Row = values; Events.Clear(); Owner.Selected.Value = Row[4]; Owner.MouseType.Value = Row[20]; Owner.Mode.Value = Row[19]; Owner.Enabled.Value = Row[6]; _blocked = Row[7];
            Owner.Remembered[0] = Row[21]; Owner.Remembered[1] = Row[22]; Owner.MultipleCount.Value = unchecked((int)Row[25]); Owner.MouseX.Value = 320; Owner.MouseY.Value = 240;
            Owner.Multiple = Row[26] != 0 ? [0, 24000, 55, uint.MaxValue, 56] : [55, 56, 57, 58, 59];
            // 예약1과 물리 마지막23999도 생성 성공을 합성하지 않고 raw 입력으로만 준비한다.
            foreach (int sid in new[] { 1, 50, 51, 52, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 99, 202, 23999 })
            {
                Raw(sid).Clear(); Write(sid, 10, 100, 1); Write(sid, 34, Row[12], 1); Write(sid, 14, Row[23]); Write(sid, 18, Row[24]);
            }
            Write(50, 11, Row[8], 1); Write(50, 40, Row[9], 1); if (Row[18] != 0) Write(50, 10, 165, 1); Write(51, 6, 60, 2);
            // 자산/내장 Form 필터와 효과 전 next 보관을 검사할 실제 종속 체인이다.
            foreach ((int sid, uint type) in new (int, uint)[] { (60, 50), (61, 51), (62, 49), (63, 100), (64, 6), (65, 5) }) { Write(sid, 10, type, 1); Write(sid, 4, sid < 65 ? (uint)sid + 1 : 0, 2); }
            Write(57, 11, 1, 1); Write(58, 11, 2, 1); Write(59, 11, 8, 1);
            // callback이 슬롯 TYPE를 바꾸면 이 현재 속성을 다시 읽는다.
            foreach (int type in new[] { 100, 101, 102, 165 }) { Types[type].Flags1 = type == 101 ? 0x10000000u : Row[10]; Types[type].Flags2 = type == 102 ? 0 : Row[11]; }
        }
        /// <summary>같은 강조 본체의 실제 외부 효과만 관측한다.</summary>
        public RawUserHighlight Highlight() => new(Owner.HighlightState(), new()
        {
            SelectCursor = value => Event("K", value),
            Project = (x, y) => { Event("J", unchecked((uint)x), unchecked((uint)y)); Mutate(8); return (0x40f00000u, 0x41140000u); },
            Move = (sid, x, y, force) => Event("H", sid, x, y, force),
        });
        /// <summary>큰 실제 하위의 인자/반환만 제공하며 선택 부모와 기억 비교를 대신하지 않는다.</summary>
        public UserSelectionHooks Hooks(RawUserHighlight highlight) => new()
        {
            Paused = () => { Event("P"); return Row[5]; }, Enabled = () => Owner.Enabled.Value, Blocked = () => { Event("B"); return _blocked; },
            LocalOwner = () => Row[14], Editor = () => Row[13], Dais = () => 165, ManaProcess = () => 50, RangeProcess = () => 51,
            SelectNotice = (sid, on) => { Event("N", (uint)sid, on, Owner.Selected.Value); Mutate(sid == 55 ? 1u : sid == 56 ? 12u : sid == 51 ? 2u : on != 0 ? 6u : 0u); },
            Refresh = sid => { Event("R", (uint)sid, Owner.Selected.Value); Mutate(sid != 51 ? 7u : 0u); },
            ProcessNotice = (sid, ev, info, value) => { Event("F", (uint)sid); Event("Q", ev, info, value); Mutate(11); },
            Dump = sid => { Event("D", (uint)sid); Mutate(3); }, ShowMana = (sid, flag) => Event("T", (uint)sid, flag),
            RangeType = type => { Event("V", type); Mutate(4); return Row[15]; }, ShowRange = (sid, flag) => { Event("C", (uint)sid, flag); Mutate(5); },
            Content = sid => { Event("I", (uint)sid, 1, 0, 0); return (ushort)Row[16]; }, Altar = sid => { Event("A", (uint)sid); Mutate(9); return (ushort)Row[18]; },
            Moving = sid => { Event("M", (uint)sid); return Row[17]; }, MovementPreview = sid => Event("E", (uint)sid), Highlight = highlight.Highlight,
            Assertion = () => Event("!:selectedSid:3841"), Sound = (x, y, name, flag) => { Events.Add($"S:{x}:{y}:{name}:{flag}"); Mutate(10); },
        };
        /// <summary>원문 관찰과 같은10칸의 현재 전역/물리를 읽는다.</summary>
        public uint[] Current() => [Owner.Selected.Value, Owner.MouseType.Value, Owner.Remembered[0], Owner.Remembered[1], unchecked((uint)Owner.MultipleCount.Value), Owner.Mode.Value, Owner.Enabled.Value, Raw(50)[10], Raw(50)[40], BinaryPrimitives.ReadUInt16LittleEndian(Raw(60)[4..])];
        /// <summary>원문13DWORD의 다중 선택 입력을 준비한다.</summary>
        public void PrepareMultiple(uint[] input)
        {
            Row = input; Events.Clear(); _moves = _refreshes = 0; Owner.ResetForGeneralWorld(); Owner.Selected.Value = Row[5]; Owner.Mode.Value = 5; Owner.Enabled.Value = Row[7]; Owner.MultipleCount.Value = unchecked((int)Row[6]); Owner.Multiple[0] = 52; Owner.Multiple[1] = 53; Owner.Multiple[2] = 52;
            if (Row[9] == 2) { Owner.Multiple[0] = 0; Owner.Multiple[1] = 24000; }
            // 다섯 물리 대상의 상태/선택 비트만 원문 입력으로 쓴다.
            for (int sid = 50; sid < 55; sid++)
            {
                Raw(sid).Clear(); Write(sid, 10, 100, 1); Write(sid, 34, 1, 1); Write(sid, 40, sid == 50 ? Row[4] : 4, 1); Write(sid, 14, 0x41280000); Write(sid, 18, 0xc0500000);
                if (sid == 50) Write(sid, 11, Row[3], 1); if (Row[9] == 1 && (sid == 52 || sid == 53)) Write(sid, 11, sid == 52 ? 1u : 2u, 1);
            }
            Types[100].Flags2 = 0x10000;
        }
        /// <summary>토글·목록·전송 부모는 실제 본체를 사용하고 관찰의 명시 효과만 제공한다.</summary>
        public UserSelectionHooks MultipleHooks() => new()
        {
            Paused = () => 0, Enabled = () => Owner.Enabled.Value, Blocked = () => { Event("B"); if (Row[10] == 8) { Owner.MultipleCount.Value = 1; Owner.Multiple[0] = 53; } return Row[8]; },
            SelectNotice = (sid, on) => { Event("N", (uint)sid, on); if (Row[10] == 1) Owner.Selected.Value = 51; if (Row[10] == 2) Owner.MultipleCount.Value = 1; Raw(sid)[40] = (byte)((Raw(sid)[40] & ~4u) | ((on & 1) << 2)); },
            Refresh = sid => { Event("R", (uint)sid); if (Row[10] == 3 && ++_refreshes == 1) { Owner.Multiple[1] = 54; Owner.MultipleCount.Value = 3; } },
            AppendMultiple = sid => { Event("A", sid); Owner.AppendMultiple(sid); }, RemoveMultiple = sid => { Event("D", sid); Owner.RemoveMultiple(sid); }, WriteEnabled = value => Owner.Enabled.Value = value,
            Sound = (x, y, name, flag) => { Assert.Equal("select.wav", name); Event("S", x, y, flag); if (Row[10] == 4) Owner.Selected.Value = 52; }, Highlight = mode => { Event("H", mode); Owner.Mode.Value = mode; },
            LocalOwner = () => 1, Editor = () => 0, Dais = () => 165, Dump = _ => { }, RangeType = _ => 0, Content = _ => 0, Moving = _ => 0, MovementPreview = _ => { },
        };
        /// <summary>첫 전송 뒤 관찰이 명시한 현재 목록/활성 변경만 수행한다.</summary>
        public void Move(uint sid, uint x, uint y)
        {
            Event("M", sid, x, y); if (++_moves != 1) return;
            if (Row[10] == 5) Owner.MultipleCount.Value = 1; if (Row[10] == 6) Owner.Multiple[1] = 54; if (Row[10] == 7) Owner.Enabled.Value = 7;
        }
        /// <summary>현재 선택/모드/활성/개수·선택 비트5칸·물리 배열30칸이다.</summary>
        public uint[] MultipleCurrent() => new uint[] { Owner.Selected.Value, Owner.Mode.Value, Owner.Enabled.Value, unchecked((uint)Owner.MultipleCount.Value), Raw(50)[40], Raw(51)[40], Raw(52)[40], Raw(53)[40], Raw(54)[40] }.Concat(Owner.Multiple).ToArray();
        /// <summary>사건이 없을 때도 원문 빈 셀 표기를 유지한다.</summary>
        public string Trace() => Events.Count == 0 ? "-" : string.Join(';', Events);
    }

    /// <summary>단일 선택1074·Clear87·ESC154입력의 모든 반환/사건/현재 필드를 대조한다.</summary>
    [Theory]
    [InlineData("user-selection", 1074, 0)]
    [InlineData("user-selection-clear", 87, 1)]
    [InlineData("escape-selection", 154, 2)]
    public void EveryNativeSingleSelectionMatches(string fixture, int count, int mode)
    {
        string[][] rows = X86Fixture.Read(fixture); Assert.Equal(count, rows.Length); var scene = new Scene();
        X86Fixture.CheckRows(rows, row =>
        {
            scene.Prepare(Numbers(row[0])); var selection = new RawUserSelection(scene.Pool, scene.Types, scene.Owner.SelectionState(), scene.Hooks(scene.Highlight())); uint result = 0;
            if (mode == 1) selection.Clear(); else if (mode == 2) _ = selection.Select(0, 0, 0);
            else if (scene.Row[0] == 0) result = selection.Select(scene.Row[1], scene.Row[2], scene.Row[3]);
            else if (scene.Row[0] == 1) selection.ClearMultiple(); else if (scene.Row[0] == 2) selection.Remember(scene.Row[23], scene.Row[24], 100);
            else
            {
                var input = new InputEvent(0x20000, unchecked((int)scene.Row[29]), unchecked((int)scene.Row[30]));
                // 공통 메뉴는 관찰의 명시 인자/반환만 제공한다.
                uint Common(int sid, InputEvent ev) { scene.Event("O", (uint)sid, ev.Code, unchecked((uint)ev.X), unchecked((uint)ev.Y)); return scene.Row[28]; }
                result = scene.Row[0] == 4 ? selection.ArcherMenu(50, input, Common) : selection.CarrierMenu(50, input, Common);
            }
            return (mode == 2 || result == X86Fixture.UInt(row[1])) && scene.Trace() == row[2] && scene.Current().SequenceEqual(Numbers(row[3]));
        });
    }
    /// <summary>다중 토글/전송601입력의 모든 사건과 전체39DWORD를 대조한다.</summary>
    [Fact]
    public void EveryNativeMultipleSequenceMatches()
    {
        string[][] rows = X86Fixture.Read("multiple-selection"); Assert.Equal(601, rows.Length); var scene = new Scene();
        X86Fixture.CheckRows(rows, row =>
        {
            scene.PrepareMultiple(Numbers(row[0])); var selection = new RawUserSelection(scene.Pool, scene.Types, scene.Owner.SelectionState(), scene.MultipleHooks());
            if (scene.Row[0] == 0) selection.ToggleMultiple(scene.Row[1], scene.Row[2]); else selection.MoveMultiple(scene.Row[11], scene.Row[12], scene.Move);
            return scene.Trace() == row[1] && scene.MultipleCurrent().SequenceEqual(Numbers(row[2]));
        });
    }
    /// <summary>정지 관문은 장치를 요구하지 않고 선택 해제의 누락은 앞선 쓰기를 보존한다.</summary>
    [Fact]
    public void LazyGuardsAndClearFailurePreservePrefix()
    {
        var scene = new Scene(); var owner = scene.Owner; owner.Selected.Value = 51; owner.MultipleCount.Value = 2;
        var disabled = new RawUserSelection(scene.Pool, scene.Types, owner.SelectionState(), new() { Enabled = () => 0 }); disabled.Clear(); Assert.Equal(51u, owner.Selected.Value); Assert.Equal(2, owner.MultipleCount.Value);
        var paused = new RawUserSelection(scene.Pool, scene.Types, owner.SelectionState(), new() { Paused = () => 1 }); Assert.Equal(0u, paused.Select(50, 0, 0));
        owner.MultipleCount.Value = 0; var missing = new RawUserSelection(scene.Pool, scene.Types, owner.SelectionState(), new() { Enabled = () => 1 });
        Assert.Throws<InvalidOperationException>(missing.Clear); Assert.Equal(0u, owner.Selected.Value); Assert.Equal(0, owner.MultipleCount.Value);
    }
    /// <summary>통지의 재할당/개수 변경을 다음 반복에서 다시 읽고 이전 배열은 유지한다.</summary>
    [Fact]
    public void ClearMultipleRereadsReallocatedArray()
    {
        var scene = new Scene(); var owner = scene.Owner; owner.Multiple = [55, 56]; uint[] old = owner.Multiple; owner.MultipleCount.Value = 2; scene.Raw(55).Clear(); scene.Raw(56).Clear(); scene.Raw(57).Clear();
        var notices = new List<int>(); var refreshes = new List<int>();
        var selection = new RawUserSelection(scene.Pool, scene.Types, owner.SelectionState(), new()
        {
            SelectNotice = (sid, on) => { Assert.Equal(0u, on); notices.Add(sid); if (sid == 55) { owner.Multiple = [99, 57, 56]; owner.MultipleCount.Value = 3; } else owner.MultipleCount.Value = 1; }, Refresh = refreshes.Add,
        });
        selection.ClearMultiple(); Assert.Equal(new[] { 55, 57 }, notices); Assert.Equal(notices, refreshes); Assert.Equal(0, owner.MultipleCount.Value); Assert.Equal(new uint[] { 55, 56 }, old);
    }
    /// <summary>Shift 추가/제외와 주선택 승격이 같은 실제 선택/목록을 사용한다.</summary>
    [Fact]
    public void ExtendSelectionPromotesCurrentMultiple()
    {
        var scene = new Scene(); scene.PrepareMultiple([0, 50, uint.MaxValue, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0]); scene.Raw(51)[40] = 0;
        var selection = new RawUserSelection(scene.Pool, scene.Types, scene.Owner.SelectionState(), scene.MultipleHooks());
        selection.ExtendSelection(50, 0); Assert.Equal(50u, scene.Owner.Selected.Value); selection.ExtendSelection(51, 0); Assert.Equal(1, scene.Owner.MultipleCount.Value);
        selection.ExtendSelection(51, 0); Assert.Equal(0, scene.Owner.MultipleCount.Value); selection.ExtendSelection(51, 0); selection.ToggleMultiple(50, uint.MaxValue);
        Assert.Equal(51u, scene.Owner.Selected.Value); Assert.Equal(0, scene.Owner.MultipleCount.Value); Assert.Equal(0, scene.Raw(50)[40] & 4); Assert.Equal(4, scene.Raw(51)[40] & 4);
    }
    /// <summary>전송 실패 때 활성0/기존 목록을 보존하고 재시도 정상 복귀 뒤에만 정리한다.</summary>
    [Fact]
    public void FailedMovementKeepsDisabledPrefix()
    {
        var scene = new Scene(); scene.PrepareMultiple([1, 50, 0, 0, 4, 50, 1, 1, 0, 0, 0, 0x41280000, 0xc0500000]); var owner = scene.Owner;
        var selection = new RawUserSelection(scene.Pool, scene.Types, owner.SelectionState(), scene.MultipleHooks());
        Assert.Throws<InvalidOperationException>(() => selection.MoveMultiple(1, 2, (sid, x, y) => { Assert.Equal(0u, owner.Enabled.Value); Assert.Equal(0u, selection.Select(0, 0, 1)); throw new InvalidOperationException("명시전송실패"); }));
        Assert.Equal(0u, owner.Enabled.Value); Assert.Equal(50u, owner.Selected.Value); Assert.Equal(1, owner.MultipleCount.Value);
        selection.MoveMultiple(1, 2, (sid, x, y) => Assert.Equal(52u, sid)); Assert.Equal(1u, owner.Enabled.Value); Assert.Equal(0, owner.MultipleCount.Value); Assert.Equal(50u, owner.Selected.Value);
    }
    /// <summary>검색 callback의 변경과 Select 거부 뒤에도 원래 found의 현재 extra를 검사한다.</summary>
    [Fact]
    public void RecheckUsesOriginalFoundAfterPausedSelection()
    {
        var scene = new Scene(); var owner = scene.Owner; owner.MouseType.Value = 74; owner.Remembered[0] = 0x41280000; owner.Remembered[1] = 0x41540000; scene.Raw(50).Clear();
        var selection = new RawUserSelection(scene.Pool, scene.Types, owner.SelectionState(), new() { Paused = () => 1 });
        selection.RecheckMouse((x, y, type) => { Assert.Equal(0x41280000u, x); Assert.Equal(0x41540000u, y); Assert.Equal(74u, type); return 50; }); Assert.Equal(0u, owner.MouseType.Value); Assert.Equal(0u, owner.Selected.Value);
        owner.MouseType.Value = 74; selection.RecheckMouse((x, y, type) => { owner.MouseType.Value = 75; return 0; }); Assert.Equal(75u, owner.MouseType.Value);
    }
}
