using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 공통 공장 초기화의 실제 슬롯·진단 복귀·현재 HP 부모를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>PostCreate/PostTake의 양판본 원본576입력과 선택된 HP 진단 부분 효과다.</summary>
public sealed class FactoryInitializationCurrentTests
{
    /// <summary>원본 입력을 한 번씩 재생하고 슬롯 전체·진단 시점·현재 전역·풀 수량을 대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    public void InitializationMatchesOriginalSlotsAndDiagnosticReturn(string editionName)
    {
        string[][] rows = X86Fixture.Read("factory-initialization-current", editionName); Assert.Equal(288, rows.Length);
        // 호출 종류·서버·contained·HP 모드·복귀 프로필을 원본 행대로 독립 공급한다.
        foreach (string[] row in rows)
        {
            string[] input = row[1].Split(','); Assert.Equal(6, input.Length);
            bool patch = editionName == "originals", server = X86Fixture.UInt(input[3]) != 0;
            int nativeBytes = patch ? 50 : 36;
            Assert.Equal(nativeBytes * 2, row[2].Length);
            int mode = X86Fixture.Int(input[4]), profile = X86Fixture.Int(input[5]);
            var quarter = new BoolBox(); var priest = new UIntBox { Value = 158 }; var dais = new UIntBox { Value = 165 };
            var pool = new RawSidPool(32768, true, () => server, patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
            int sid = pool.Allocate(SidAllocation.Client); Span<byte> raw = pool.RawSlot(sid);
            // 관리 풀의50BYTE 슬롯을 채우고 CD의 실제36BYTE 밖 패딩도 그대로 보존하는지 검사한다.
            for (int index = 0; index < raw.Length; index++) raw[index] = unchecked((byte)(index * 17 + 23));
            byte[] padding = raw[nativeBytes..].ToArray();
            raw[10] = unchecked((byte)X86Fixture.UInt(input[1])); raw[11] = unchecked((byte)X86Fixture.UInt(input[2]));
            raw[patch ? 40 : 35] = mode == 2 ? (byte)1 : (byte)0;
            // 기존 C# 공장의 CD padded188 API를 유지하며 실제 사용 칸/슬롯 폭은 원본과 같다.
            var types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
            // 원본 장치가 읽는 네 타입의 입력만 채운다.
            foreach (int number in new[] { 3, 74, 75, 76 })
            {
                types[number].MaxHitPoints = number == 74 ? 0x12345678 : -37;
                types[number].Flags1 = mode == 0 ? 16u : 0; types[number].Flags2 = mode != 0 ? 0x400000u : 0;
                types[number].ZOrder = 0x123 + number;
            }
            var factory = new RawSquidFactory(pool, types); factory.BindWeakenedMana(quarter); factory.BindHitPointTypes(priest, dais);
            var diagnostics = new List<string>();
            factory.BindDiagnostics(new()
            {
                Types = types, Assertion = (expression, file, line) =>
                {
                    Span<byte> current = pool.RawSlot(sid);
                    diagnostics.Add($"{expression}|{file}|{line}|{Convert.ToHexString(current[..nativeBytes]).ToLowerInvariant()}");
                    if (profile == 1)
                    {
                        current[10] = 75; types[75].MaxHitPoints = -37; types[75].ZOrder = 0x345;
                        quarter.Value = true; priest.Value = 75;
                    }
                    else if (profile == 2) { current[10] = 76; server = false; current[11] &= 0xf7; }
                },
            });
            uint free = pool.FreeCount;
            if (input[0] == "create") factory.PostCreate(sid); else { Assert.Equal("take", input[0]); factory.PostTake(sid); }
            Assert.Equal(row[2], Convert.ToHexString(pool.Slot(sid)[..nativeBytes]).ToLowerInvariant());
            Assert.Equal(padding, pool.Slot(sid)[nativeBytes..].ToArray());
            Assert.Equal(row[3], diagnostics.Count == 0 ? "-" : string.Join(';', diagnostics));
            Assert.Equal(X86Fixture.UInt(row[4]) != 0, server); Assert.Equal(X86Fixture.UInt(row[5]) != 0, quarter.Value);
            Assert.Equal(X86Fixture.UInt(row[6]), priest.Value); Assert.Equal(free, pool.FreeCount);
        }
    }

    /// <summary>선택된 HP 진단의 재연결에서도 기록한 owner/zOrder와 아직 쓰지 않은 HP를 구별한다.</summary>
    [Fact]
    public void MissingHpDiagnosticKeepsEarlierWritesAndFactoryUsesRebasedSelectedRecord()
    {
        var pool = new RawSidPool(24000, true);
        var types = Enumerable.Range(0, 189).Select(_ => new OriginalTypeRecord()).ToArray();
        types[74].Flags2 = 0x400000; types[74].ZOrder = 17; types[74].MaxHitPoints = 29;
        var origin = new ArraySegment<OriginalTypeRecord>(types, 0, 188);
        var factory = new RawSquidFactory(pool, origin); var quarter = new BoolBox();
        var priest = new UIntBox { Value = 158 }; var dais = new UIntBox { Value = 165 };
        factory.BindWeakenedMana(quarter); factory.BindHitPointTypes(priest, dais);
        int sid = pool.Allocate(SidAllocation.Client); pool.RawSlot(sid).Clear();
        pool.RawSlot(sid)[10] = 74; pool.RawSlot(sid)[40] = 1; pool.RawSlot(sid)[34] = 9;
        System.Buffers.Binary.BinaryPrimitives.WriteInt32LittleEndian(pool.RawSlot(sid)[26..], 0x12345678);
        factory.BindDiagnostics(new() { Types = origin, Checking = new(true) });
        Assert.Throws<InvalidOperationException>(() => factory.PostCreate(sid));
        Assert.Equal(17, pool.Slot(sid)[35]); Assert.Equal(0, pool.Slot(sid)[34]);
        Assert.Equal(0x12345678, System.Buffers.Binary.BinaryPrimitives.ReadInt32LittleEndian(pool.Slot(sid)[26..]));
        factory.BindDiagnostics(new()
        {
            Types = origin, Checking = new(true), Assertion = (_, _, _) =>
            {
                quarter.Value = true; priest.Value = 73; types[74].MaxHitPoints = -37; pool.RawSlot(sid)[10] = 75;
                factory.BindDiagnostics(new() { Types = new ArraySegment<OriginalTypeRecord>(types, 1, 188) });
            },
        });
        factory.PostCreate(sid);
        Assert.Equal(-9, System.Buffers.Binary.BinaryPrimitives.ReadInt32LittleEndian(pool.Slot(sid)[26..]));
        Assert.Equal(75, pool.Slot(sid)[10]); Assert.Equal(17, pool.Slot(sid)[35]);
    }
}
