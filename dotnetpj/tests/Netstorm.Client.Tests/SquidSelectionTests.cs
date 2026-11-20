using System.Buffers.Binary;
using System.Globalization;
using System.Runtime.InteropServices;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 보호 PE의 선택+7c/사정거리/491입구와 실제 Client 정리를 창 없이 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>5600원문 입력의 전체 사건/물리와 실제 표시 연결의 실패 prefix를 대조한다.</summary>
public sealed class SquidSelectionTests
{
    /// <summary>원문 unsigned 십진 DWORD 목록이다.</summary>
    private static uint[] Numbers(string value) => value.Split(',').Select(X86Fixture.UInt).ToArray();
    /// <summary>할당 성공으로 해석하지 않는 독립 raw 입력 주입 뷰다.</summary>
    private static Span<byte> Raw(RawSidPool pool, int sid = 50) => MemoryMarshal.CreateSpan(ref MemoryMarshal.GetReference(pool.Slot(sid)), pool.SlotStride);
    /// <summary>원본83VT/전체BYTE/사정거리/내용물 표시의5600개 모든 필드를 대조한다.</summary>
    [Fact]
    public void EveryNativeObjectSelectionMatches()
    {
        string[][] rows = X86Fixture.Read("squid-selection"); Assert.Equal(5600, rows.Length); var pool = new RawSidPool(24000);
        X86Fixture.CheckRows(rows, row =>
        {
            uint[] input = Numbers(row[0]); var events = new List<string>(); Raw(pool).Clear(); BinaryPrimitives.WriteUInt32LittleEndian(Raw(pool), input[1]); Raw(pool)[10] = (byte)input[7]; Raw(pool)[11] = 0xff; Raw(pool)[40] = (byte)input[2]; uint result = 0;
            // 전체 DWORD 인자를 원문 사건 형식으로 기록한다.
            void Event(string name, params uint[] values) => events.Add(name + (values.Length == 0 ? "" : ":" + string.Join(':', values.Select(value => value.ToString(CultureInfo.InvariantCulture)))));
            if (input[0] == 0)
            {
                new RawSquidSelection(pool, sid =>
                {
                    Assert.Equal(50, sid); Event("R", (uint)sid, Raw(pool)[40], BinaryPrimitives.ReadUInt32LittleEndian(Raw(pool)));
                    if (input[4] == 1) Raw(pool)[40] = 0xa5; else if (input[4] == 2) { Raw(pool)[40] = 0x12; BinaryPrimitives.WriteUInt32LittleEndian(Raw(pool), 0xdeadbeef); } else if (input[4] == 3) Raw(pool)[40] = 0xff;
                }).Mark(50, input[3]);
            }
            else if (input[0] == 1) result = RawSquidSelection.UsesRange(new() { Group = unchecked((int)input[5]), Flags2 = input[6] }) ? 1u : 0u;
            else RawSquidSelection.StartContentPreview(pool, 50, input[8],
                (sid, ev, payload) => { Event("A", 40); if (input[9] != 0) Event("C", ev, (uint)sid, BitConverter.SingleToUInt32Bits(payload)); },
                () => { Event("!:!s.isForm():144"); if (input[4] == 4) Raw(pool)[10] = 158; });
            uint[] current = [BinaryPrimitives.ReadUInt32LittleEndian(Raw(pool)), Raw(pool)[40], Raw(pool)[10]];
            return result == X86Fixture.UInt(row[1]) && (events.Count == 0 ? "-" : string.Join(';', events)) == row[2] && current.SequenceEqual(Numbers(row[3]));
        });
    }
    /// <summary>기본 통지는 표시를 요구하지 않으며 파생/미지원 VT 실패는 bit쓰기 전이다.</summary>
    [Fact]
    public void MissingDerivedDisplayAndUnknownVtableKeepByte()
    {
        var pool = new RawSidPool(24000); Raw(pool).Clear(); BinaryPrimitives.WriteUInt32LittleEndian(Raw(pool), 0x501dd0); Raw(pool)[40] = 0xa1;
        var marks = new RawSquidSelection(pool, null); marks.Mark(50, 1); Assert.Equal(0xa5, Raw(pool)[40]);
        BinaryPrimitives.WriteUInt32LittleEndian(Raw(pool), 0x501ea0); Raw(pool)[40] = 0xa1; Assert.Throws<InvalidOperationException>(() => marks.Mark(50, 1)); Assert.Equal(0xa1, Raw(pool)[40]);
        BinaryPrimitives.WriteUInt32LittleEndian(Raw(pool), 0xdeadbeef); Assert.Equal(0u, RawSquidSelection.SelectionEntry(0xdeadbeef)); Assert.Throws<InvalidOperationException>(() => marks.Mark(50, 1)); Assert.Equal(0xa1, Raw(pool)[40]);
    }
    /// <summary>로딩 관문은 대상/장치를 읽지 않으며 Form 진단 효과는 Regular 실패에서도 보존한다.</summary>
    [Fact]
    public void PreviewGuardAndMissingRegularKeepPrefix()
    {
        var pool = new RawSidPool(24000); RawSquidSelection.StartContentPreview(pool, ushort.MaxValue, 1, null, null);
        Raw(pool).Clear(); Raw(pool)[10] = 6; int diagnostics = 0;
        Assert.Throws<InvalidOperationException>(() => RawSquidSelection.StartContentPreview(pool, 50, 0, null, () => { diagnostics++; Raw(pool)[10] = 158; }));
        Assert.Equal(1, diagnostics); Assert.Equal(158, Raw(pool)[10]);
    }
    /// <summary>같은 권한/보조 owner와188TYPE의 독립 장기 물리를 준비한다.</summary>
    private static RawSurfaceStorage Storage(ClientGameplayGlobals game)
    {
        var storage = new RawSurfaceStorage(authority: game.Authority, gameAux: game.GameAux);
        storage.Types.AddRange(Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord())); return storage;
    }
    /// <summary>첫 메뉴의 음수 개수는 객체/커서 없이 실제 Clear를 거치고 mode0의 TYPE/기억점은 보존한다.</summary>
    [Fact]
    public void FirstMenuClearNeedsNoRuntimeAndPreservesUnselectedFields()
    {
        var game = new ClientGameplayGlobals(); RawSurfaceStorage storage = Storage(game); var owner = game.Selection;
        owner.Enabled.Value = 0x80000000; owner.Selected.Value = 0xfedcba98; owner.MouseType.Value = 0x89abcdef; owner.MultipleCount.Value = -7; owner.Multiple = [0x10037, uint.MaxValue]; owner.MouseX.Value = -2; owner.MouseY.Value = 73; owner.Remembered[0] = 0x7fc00001; owner.Remembered[1] = 0x80000000;
        uint[] physical = owner.Multiple; ClientUserSelectionCleanup.Clear(game, storage);
        Assert.False(game.Borrowed()); Assert.Equal(0u, owner.Selected.Value); Assert.Equal(0, owner.MultipleCount.Value); Assert.Equal(0u, owner.Mode.Value); Assert.Equal(0x89abcdefu, owner.MouseType.Value); Assert.Same(physical, owner.Multiple);
        Assert.Equal(-2, owner.MouseX.Value); Assert.Equal(73, owner.MouseY.Value); Assert.Equal(new uint[] { 0x7fc00001, 0x80000000 }, owner.Remembered);
    }
    /// <summary>비활성은 모든 필드를 유지하고, 실제 커서 누락은 강조 선행 쓰기 뒤 실패한다.</summary>
    [Fact]
    public void DisabledClearAndSelectedCursorKeepExactPrefix()
    {
        var game = new ClientGameplayGlobals(); RawSurfaceStorage storage = Storage(game); var owner = game.Selection; owner.Enabled.Value = 0; owner.Mode.Value = 5; owner.Selected.Value = 0x12345678; owner.MouseType.Value = 71; owner.MultipleCount.Value = -3;
        ClientUserSelectionCleanup.Clear(game, storage, _ => throw new IOException("비활성커서호출")); Assert.Equal(5u, owner.Mode.Value); Assert.Equal(0x12345678u, owner.Selected.Value); Assert.Equal(-3, owner.MultipleCount.Value);
        owner.Enabled.Value = 1; Assert.Throws<InvalidOperationException>(() => ClientUserSelectionCleanup.Clear(game, storage)); Assert.Equal(0u, owner.Mode.Value); Assert.Equal(0u, owner.Selected.Value); Assert.Equal(0u, owner.MouseType.Value); Assert.Equal(0, owner.MultipleCount.Value);
        owner.Mode.Value = 9; uint cursor = 0; ClientUserSelectionCleanup.Clear(game, storage, selected => cursor = selected); Assert.Equal(1u, cursor); Assert.False(game.Borrowed());
    }
    /// <summary>살아 있는 다중 대상은 실제 현재 Session 없이 bit를 지우지 못한다.</summary>
    [Fact]
    public void LiveMultipleRequiresActualCurrentSession()
    {
        var game = new ClientGameplayGlobals(); RawSurfaceStorage storage = Storage(game); var owner = game.Selection; owner.Selected.Value = 99; owner.MultipleCount.Value = 1; owner.Multiple[0] = 50;
        Raw(storage.Pool).Clear(); BinaryPrimitives.WriteUInt32LittleEndian(Raw(storage.Pool), 0x501dd0); Raw(storage.Pool)[40] = 4;
        Assert.Throws<InvalidOperationException>(() => ClientUserSelectionCleanup.Clear(game, storage)); Assert.Equal(4, Raw(storage.Pool)[40]); Assert.Equal(1, owner.MultipleCount.Value); Assert.Equal(99u, owner.Selected.Value);
    }
    /// <summary>실제 현재 Pool의 기본 선택 bit는 먼저 바뀌고 미설치 표시 장치에서 정확히 실패한다.</summary>
    [Fact]
    public void CurrentSessionUsesRealDisplayAndKeepsEarlierBitWrite()
    {
        var game = new ClientGameplayGlobals(); RawSurfaceStorage storage = Storage(game);
        var session = new HostSession(new Kernel(), new GameRandom(1), new CrtRandom(), pool: storage.Pool, sharedGameplay: game);
        using var runtime = HostMissionRuntime.Create(session, storage); game.Selection.MultipleCount.Value = 1; game.Selection.Multiple[0] = 50; game.Selection.Selected.Value = 99;
        Raw(storage.Pool).Clear(); BinaryPrimitives.WriteUInt32LittleEndian(Raw(storage.Pool), 0x501dd0); Raw(storage.Pool)[40] = 4;
        Assert.Throws<InvalidOperationException>(() => ClientUserSelectionCleanup.Clear(game, storage)); Assert.Equal(0, Raw(storage.Pool)[40] & 4); Assert.Equal(1, game.Selection.MultipleCount.Value); Assert.Equal(99u, game.Selection.Selected.Value);
    }
    /// <summary>현재 Session과 다른 Pool은 가상 통지/선택 bit쓰기 전에 거부한다.</summary>
    [Fact]
    public void DifferentCurrentPoolFailsBeforeMark()
    {
        var game = new ClientGameplayGlobals(); RawSurfaceStorage current = Storage(game); RawSurfaceStorage foreign = Storage(game);
        var session = new HostSession(new Kernel(), new GameRandom(1), new CrtRandom(), pool: current.Pool, sharedGameplay: game); using var runtime = HostMissionRuntime.Create(session, current);
        game.Selection.MultipleCount.Value = 1; game.Selection.Multiple[0] = 50; Raw(foreign.Pool).Clear(); Raw(foreign.Pool)[40] = 4;
        Assert.Throws<InvalidOperationException>(() => ClientUserSelectionCleanup.Clear(game, foreign)); Assert.Equal(4, Raw(foreign.Pool)[40]); Assert.Equal(1, game.Selection.MultipleCount.Value);
    }
}
