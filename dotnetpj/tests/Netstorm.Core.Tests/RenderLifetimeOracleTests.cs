using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 원본 관찰을 기대값 계산 없이 같은 공통 표시/버퍼 수명에 적용한다.
namespace Netstorm.Core.Tests;

/// <summary>세 판본 등록/삭제1377행·수집4197행과 기본53비트 삭제영역48행의 대조다.</summary>
public sealed class RenderLifetimeOracleTests
{
    /// <summary>원문 DWORD 입력 목록을 읽는다.</summary>
    private static uint[] Values(string input) => input.Split(',').Select(X86Fixture.UInt).ToArray();
    /// <summary>명령 관찰의 빈 목록 표기를 보존한다.</summary>
    private static string Join(IEnumerable<string> values, char separator) { string text = string.Join(separator, values); return text.Length == 0 ? "-" : text; }
    /// <summary>선택 판본의 실제 슬롯 풀을 만든다.</summary>
    private static RawSidPool Pool(string edition) => new(32768, false, edition: edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
    /// <summary>signed 필드도 하위DWORD로 직렬화한다.</summary>
    private static string Snapshot(RawRenderRegistry registry, RenderProcessFields f) => Join(registry.Slots.ToArray().Select(v => v.ToString()), ':') + "|" + string.Join(':', new uint[]
    { unchecked((uint)f.Form), unchecked((uint)f.Parent), BitConverter.SingleToUInt32Bits(f.Left), BitConverter.SingleToUInt32Bits(f.Top), BitConverter.SingleToUInt32Bits(f.Right), BitConverter.SingleToUInt32Bits(f.Bottom), unchecked((uint)f.Slot), unchecked((uint)f.ZOrder) });

    /// <summary>표시 등록·버퍼 소멸·목록 재사용 전체를 독립 원본459행씩 대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    [InlineData("original1037")]
    public void RegistryAndConstructionDestructionMatchOriginal(string edition)
    {
        string[][] rows = X86Fixture.Read("constructionlifetime", edition); Assert.Equal(459, rows.Length); var pool = Pool(edition);
        int depth = edition == "originals" ? 35 : 33;
        X86Fixture.CheckRows(rows, row =>
        {
            uint[] input = Values(row[2]); var events = new List<string>(); var history = new List<string>();
            pool.RawSlot(0)[depth] = (byte)input[3]; pool.RawSlot(50)[depth] = (byte)input[4]; pool.RawSlot(51)[depth] = 127;
            uint[] initial = Enumerable.Range(0, (int)input[0]).Select(i => (uint)(100 + i)).ToArray();
            if (initial.Length != 0 && input[1] != 0) initial[input[1] == 1 ? 0 : input[1] == 2 ? initial.Length / 2 : initial.Length - 1] = 0;
            var fields = new RenderProcessFields();
            if (row[1] is not ("Construct" or "Sequence"))
            {
                initial[(int)input[8]] = 1; fields.Token = 1; fields.Slot = (int)input[8]; fields.Form = 7; fields.Parent = 50;
                fields.ZOrder = -128; fields.Left = fields.Top = fields.Right = fields.Bottom = BitConverter.UInt32BitsToSingle(0xa5a5a5a5);
            }
            RawRenderRegistry? registry = null;
            var hooks = new RenderRegistryHooks
            {
                Reserve = bytes => { events.Add($"A:{bytes}"); return true; }, ReleaseArray = () => events.Add("F:T"),
                Attach = (f, type, parent, flags) =>
                {
                    Assert.Equal(1u, registry!.Slots[f.Slot]); Assert.Equal(17u, type); Assert.Equal(16u, flags);
                    events.Add($"B:{type}:{parent}:0:{flags}:{f.Slot}:{unchecked((uint)f.ZOrder)}");
                    f.Form = 7; f.Parent = input[5] == 2 ? 51 : parent;
                    if (input[5] == 1) pool.RawSlot(parent)[depth] = 128;
                },
            };
            registry = new(pool, hooks, initial); uint result = 0;
            if (row[1] is "Construct" or "Sequence")
            {
                registry.Construct(fields, 1, (ushort)input[2], 17); result = 1;
                if (row[1] == "Sequence")
                {
                    history.Add(Snapshot(registry, fields)); registry.Destroy(fields); history.Add(Snapshot(registry, fields));
                    registry.Construct(fields, 1, (ushort)input[2], 17); history.Add(Snapshot(registry, fields));
                }
            }
            else if (row[1] == "Render") registry.Destroy(fields);
            else
            {
                var visual = new ConstructionVisualFields();
                if ((input[6] & 1) != 0) visual.Image.Pixels = [17];
                if ((input[6] & 2) != 0) visual.Output = [31];
                if ((input[6] & 4) != 0) visual.Particles = new ConstructionVisualParticle[1];
                RawConstructionLifetime.Destroy(visual, fields, registry, row[1] == "Build" ? 0 : input[7], kind =>
                {
                    Assert.Equal(kind == ConstructionRelease.Object ? 0u : 1u, registry.Slots[fields.Slot]);
                    events.Add("F:" + (kind switch { ConstructionRelease.Source => "S", ConstructionRelease.Output => "O", ConstructionRelease.Particles => "P", _ => "B" }));
                });
                Assert.Empty(visual.Image.Pixels); Assert.Empty(visual.Output); Assert.Empty(visual.Particles);
                if (row[1] != "Build") result = 1;
            }
            string[] snapshot = Snapshot(registry, fields).Split('|');
            Assert.Equal(row[3], Join(events, ';')); Assert.Equal(row[4], snapshot[0]); Assert.Equal(row[5], snapshot[1]);
            Assert.Equal(row[6], Join(history, '~')); Assert.Equal(X86Fixture.UInt(row[7]), result); return true;
        });
    }

    /// <summary>16바이트 공통 출력 항목을 필드 폭대로 직렬화하며 padding을 만들지 않는다.</summary>
    private static byte[] EntryBytes(IEnumerable<RenderCollectionEntry> entries)
    {
        var bytes = new List<byte>();
        // 원문 항목마다 좌표/토큰/깊이/종류/Tail 전체를 같은 순서로 보존한다.
        foreach (RenderCollectionEntry e in entries)
        {
            byte[] raw = new byte[16]; BinaryPrimitives.WriteSingleLittleEndian(raw, e.X); BinaryPrimitives.WriteSingleLittleEndian(raw.AsSpan(4), e.Y);
            BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(8), e.Token); BinaryPrimitives.WriteInt16LittleEndian(raw.AsSpan(12), e.Depth); raw[14] = e.Kind; raw[15] = e.Tail; bytes.AddRange(raw);
        }
        return bytes.ToArray();
    }
    /// <summary>월드 영역의 네float를 원문16바이트로 직렬화한다.</summary>
    private static byte[] BoundsBytes(RenderWorldRect area)
    {
        byte[] raw = new byte[16]; BinaryPrimitives.WriteSingleLittleEndian(raw, area.Left); BinaryPrimitives.WriteSingleLittleEndian(raw.AsSpan(4), area.Top);
        BinaryPrimitives.WriteSingleLittleEndian(raw.AsSpan(8), area.Right); BinaryPrimitives.WriteSingleLittleEndian(raw.AsSpan(12), area.Bottom); return raw;
    }
    /// <summary>fixture가 직접 공급한float 비트만 읽는다.</summary>
    private static float F(uint bits) => BitConverter.UInt32BitsToSingle(bits);

    /// <summary>세 판본의 실제 영역/교차/출력 전체를 대조한다. Renderer의 Order36행씩은 별도 미연결 범위다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    [InlineData("original1037")]
    public void CollectionMatchesOriginalBoundsIntersectionAndOutput(string edition)
    {
        string[][] rows = X86Fixture.Read("rendercollection", edition).Where(row => row[1] != "Order").ToArray(); Assert.Equal(1399, rows.Length);
        var pool = Pool(edition); var state = new ConstructionVisualState();
        X86Fixture.CheckRows(rows, row =>
        {
            uint[] input = Values(row[2]); state.ScaleX = F(input[0]); state.ScaleY = F(input[1]);
            var fields = Enumerable.Range(0, (int)input[10]).Select(_ => new RenderProcessFields()).ToArray(); uint[] slots = new uint[fields.Length];
            if (row[3] != "-")
            {
                // 슬롯과 객체의 관찰 입력을 읽으며 기대 환산/가시성을 계산하지 않는다.
                foreach (string record in row[3].Split(';'))
                {
                    uint[] v = Values(record); int slot = (int)v[0]; var f = fields[slot]; slots[slot] = (uint)(slot + 1);
                    f.Token = slots[slot]; f.Slot = slot; f.Parent = (int)v[1]; f.ZOrder = unchecked((int)v[2]);
                    f.Left = F(v[3]); f.Top = F(v[4]); f.Right = F(v[5]); f.Bottom = F(v[6]);
                    Span<byte> raw = pool.RawSlot(f.Parent); BinaryPrimitives.WriteUInt16LittleEndian(raw[6..], 7); raw[11] = (byte)v[9];
                    BinaryPrimitives.WriteUInt32LittleEndian(raw[14..], v[7]); BinaryPrimitives.WriteUInt32LittleEndian(raw[18..], v[8]);
                }
            }
            var hooks = new RenderRegistryHooks { Reserve = _ => throw new InvalidOperationException("수집 중 확보"), ReleaseArray = () => Assert.Fail("수집 중 반납"), Attach = (_, _, _, _) => Assert.Fail("수집 중 부착") };
            var registry = new RawRenderRegistry(pool, hooks, slots); var collection = new RawRenderCollection(pool, state, registry, token => fields[(int)token - 1]);
            var view = new RenderWorldRect(F(input[6]), F(input[7]), F(input[8]), F(input[9]));
            var output = Enumerable.Repeat(new RenderCollectionEntry(F(0xa5a5a5a5), F(0xa5a5a5a5), 0xa5a5a5a5, unchecked((short)0xa5a5), 0xa5, 0xa5), 8).ToArray();
            if (row[1] == "Bounds") Assert.Equal(X86Fixture.Bytes(row[6]), BoundsBytes(collection.Bounds(fields[0])));
            else if (row[1] == "Intersect")
            {
                Assert.Equal(row[4] != "0", RawRenderCollection.Intersects(view, new(F(input[2]), F(input[3]), F(input[4]), F(input[5])), pool.Edition));
                Assert.Equal(X86Fixture.Int(row[4]), collection.Collect(view, output, 1));
            }
            else Assert.Equal(X86Fixture.Int(row[4]), collection.Collect(view, output, unchecked((int)input[12])));
            if (row[1] != "Intersect") Assert.Equal(X86Fixture.Bytes(row[5]), EntryBytes(output)); return true;
        });
    }

    /// <summary>삭제 영역은 제품 기본0x027f의48관찰만 대조하고0x037f 비교48행은 별도 보존한다.</summary>
    [Fact]
    public void RemovalMatchesOriginalDefaultPrecision()
    {
        string[][] rows = X86Fixture.Read("render-removal"); Assert.Equal(48, rows.Count(row => row[1] == "895")); int count = 0;
        // 확장64비트 x87 제어어의 행은 C# double/제품53비트 판정에 합산하지 않는다.
        foreach (string[] row in rows.Where(row => row[1] == "639"))
        {
            byte[] raw = X86Fixture.Bytes(row[2]), expected = X86Fixture.Bytes(row[3]);
            var fields = new RenderProcessFields { Left = BinaryPrimitives.ReadSingleLittleEndian(raw), Top = BinaryPrimitives.ReadSingleLittleEndian(raw.AsSpan(4)), Right = BinaryPrimitives.ReadSingleLittleEndian(raw.AsSpan(8)), Bottom = BinaryPrimitives.ReadSingleLittleEndian(raw.AsSpan(12)) };
            int[] actual = RenderProcessSystem.RemovalBounds(fields, BinaryPrimitives.ReadSingleLittleEndian(raw.AsSpan(16)), BinaryPrimitives.ReadSingleLittleEndian(raw.AsSpan(20)));
            // 원문 네 signedDWORD와 flags0을 모두 확인한다.
            for (int i = 0; i < 4; i++) Assert.Equal(BinaryPrimitives.ReadInt32LittleEndian(expected.AsSpan(i * 4)), actual[i]);
            Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(expected.AsSpan(16))); count++;
        }
        Assert.Equal(48, count);
    }
}
