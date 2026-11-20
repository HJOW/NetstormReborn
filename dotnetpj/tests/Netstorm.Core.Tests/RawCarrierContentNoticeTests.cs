using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 원본 Carrier 통지의 외부 효과·현재 전역과 전체 물리 슬롯을 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>독립 원본1731입력과 저장/로딩의 무접근 접두를 검사한다.</summary>
public sealed class RawCarrierContentNoticeTests
{
    /// <summary>물리 슬롯에 원문이 지정한 폭으로만 입력한다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value, bool word = false)
    {
        if (word) BinaryPrimitives.WriteUInt16LittleEndian(raw[offset..], unchecked((ushort)value));
        else BinaryPrimitives.WriteUInt32LittleEndian(raw[offset..], value);
    }
    /// <summary>효과 인자의 float 비트를 원본DWORD 표기로 보존한다.</summary>
    private static uint Bits(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>원본1731행의 효과 순서·다시 읽는 역할/좌표·전체100바이트를 전수 대조한다.</summary>
    [Fact]
    public void OriginalCarrierNotice1731()
    {
        string[][] rows = X86Fixture.Read("carrier-content-notice"); Assert.Equal(1731, rows.Length);
        // 각 독립 원본 입력은 새 풀과 실제188개 TYPE를 사용한다.
        foreach (string[] row in rows)
        {
            uint[] input = row[0].Split(',').Select(X86Fixture.UInt).ToArray();
            var pool = new RawSidPool(32768, true, null, OriginalEdition.Patch1078);
            OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
            uint saved = input[0], loading = input[1], blocked = input[2], local = input[4];
            types[input[5]].Flags2 = input[6]; var effects = new List<string>();
            pool.RawSlot(50).Fill(0xa5); pool.RawSlot(60).Fill(0x5a);
            Put(pool.RawSlot(50), 0, 0x50f210); pool.RawSlot(50)[10] = 158; pool.RawSlot(50)[34] = unchecked((byte)input[3]);
            Put(pool.RawSlot(50), 14, input[10]); Put(pool.RawSlot(50), 18, input[11]);
            pool.RawSlot(60)[11] = unchecked((byte)input[7]); Put(pool.RawSlot(60), 18, input[5], true); Put(pool.RawSlot(60), 20, input[8], true);
            /// <summary>원본 관측에서 지정한 외부 효과 뒤의 입력 변경만 적용한다.</summary>
            void Emit(string ev, params uint[] values)
            {
                effects.Add(ev + (values.Length == 0 ? "" : "," + string.Join(',', values)));
                if (input[18] == 1 && ev == "B") { Put(pool.RawSlot(60), 18, 158, true); pool.RawSlot(60)[11] = 2; }
                if (input[18] == 2 && ev == "S") { pool.RawSlot(50)[34] = 2; Put(pool.RawSlot(50), 14, Bits(99)); Put(pool.RawSlot(50), 18, Bits(77)); }
                if (input[18] == 3 && ev == "A") { local = 2; Put(pool.RawSlot(60), 18, 123, true); }
                if (input[18] == 4 && ev == "N") loading = 1;
                if (input[18] == 5 && ev == "H") { Put(pool.RawSlot(60), 18, 158, true); Put(pool.RawSlot(50), 14, Bits(25.5f)); Put(pool.RawSlot(50), 18, Bits(30.25f)); }
                if (input[18] == 6 && ev == "G") { Put(pool.RawSlot(50), 14, Bits(55.5f)); Put(pool.RawSlot(50), 18, Bits(66.25f)); }
                if (input[18] == 7 && ev == "P") { pool.RawSlot(60)[11] = 2; loading = 1; }
                if (input[18] == 8 && ev == "B") { blocked = 1; Put(pool.RawSlot(60), 18, 123, true); types[123].Flags2 = 0; }
            }
            var notice = new RawCarrierContentNotice(pool, types, new()
            {
                SavedGame = () => saved != 0, LoadingDepth = () => loading, BombBlocked = () => blocked, LocalOwner = () => local,
                PriestType = () => 158, FreeSoundBlockerType = () => 156, CameraX = () => unchecked((int)input[19]), CameraY = () => unchecked((int)input[20]),
            }, new()
            {
                Center = parent => { Emit("C", (uint)parent); return (BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(parent)[14..])), BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(parent)[18..]))); },
                Bolt = (parent, start, end, seconds) => Emit("B", (uint)parent, unchecked((uint)start.X), unchecked((uint)start.Y), unchecked((uint)end.X), unchecked((uint)end.Y), Bits(seconds)),
                SoundAt = (x, y, name) => { Assert.Equal("thunderCrack.wav", name); Emit("S", Bits(x), Bits(y)); },
                TypeWord = (type, offset) => offset == 0x6c ? type == input[5] ? input[16] : 0x10000000 + type : type == 158 && offset == 0x7c ? input[17] : throw new InvalidOperationException("미등록 TYPE 조회"),
                Allocate = size => { Emit("A", size); return input[12] != 0 ? 1u : 0; },
                Create = (storage, parent, value, other, flags) => { Assert.Equal(1u, storage); Emit("N", 1, (uint)parent, value, other, flags); return input[13] != 0 ? 1u : 0; },
                Duration = (process, seconds) => Emit("T", process, Bits(seconds)), Sound = value => Emit("P", value), CheckCarrier = parent => Emit("H", (uint)parent),
                GenusAt = (x, y, genus) => { Emit("G", Bits(x), Bits(y), genus); return input[14] != 0 ? 60 : 0; },
                TypeAt = (x, y, type) => { Emit("I", Bits(x), Bits(y), type); return input[15] != 0 ? 61 : 0; },
                FreeSound = name => { Assert.Equal("priestFree.wav", name); Emit("F"); },
            });
            Assert.True(notice.Handles(50)); notice.Notice(50, 60, input[9]);
            Assert.Equal(row[1], effects.Count == 0 ? "-" : string.Join(';', effects));
            Assert.Equal(row[2], string.Join(',', new[] { saved, loading, blocked, local, types[123].Flags2, types[158].Flags2, types[168].Flags2, input[19], input[20] }));
            Assert.Equal(row[3], Convert.ToHexString(pool.Slot(50)).ToLowerInvariant()); Assert.Equal(row[4], Convert.ToHexString(pool.Slot(60)).ToLowerInvariant());
        }
    }
    /// <summary>저장/로딩 접두는 잘못된 SID·미연결 후속 장치도 읽지 않고 현재 관문만 조회한다.</summary>
    [Fact]
    public void SavedOrLoadingDoesNotReadPhysicalArgumentsOrUnusedHooks()
    {
        var pool = new RawSidPool(32768, true, null, OriginalEdition.Patch1078);
        OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        bool saved = true; uint loading = 1;
        var state = new CarrierContentNoticeState { SavedGame = () => saved, LoadingDepth = () => loading };
        var notice = new RawCarrierContentNotice(pool, types, state, new());
        notice.Notice(-1, -1, uint.MaxValue); saved = false; notice.Notice(-1, -1, uint.MaxValue);
        loading = 0; Assert.ThrowsAny<Exception>(() => notice.Notice(-1, -1, uint.MaxValue));
        pool.RawSlot(50).Clear(); pool.RawSlot(50)[10] = 158; Put(pool.RawSlot(50), 0, 0x50ee08); Assert.False(notice.Handles(50));
        Put(pool.RawSlot(50), 0, 0x50f210); Assert.True(notice.Handles(50));
    }
}
