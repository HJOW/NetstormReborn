using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 기존독립PE기대값을계산하지않고전체필드/입자/난수/요청/픽셀로대조한다.
namespace Netstorm.Core.Tests;

/// <summary>파편980프레임/생성관측과216수명관측의직접소비자다.</summary>
public sealed class X86RawShrapnelTests
{
    /// <summary>현재rawDWORD를littleendian으로읽는다.</summary>
    private static uint U(byte[] raw, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset, 4));
    /// <summary>rawfloat비트를그대로복원한다.</summary>
    private static float F(byte[] raw, int offset) => BitConverter.UInt32BitsToSingle(U(raw, offset));
    /// <summary>rawdouble비트를그대로복원한다.</summary>
    private static double D(byte[] raw, int offset) => BitConverter.UInt64BitsToDouble(BinaryPrimitives.ReadUInt64LittleEndian(raw.AsSpan(offset, 8)));
    /// <summary>주어진폭의낮은바이트부터직렬화한다.</summary>
    private static void Append(List<byte> bytes, ulong value, int width = 4)
    {
        // DWORD/QWORD의원래byte순서를보존한다.
        for (int index = 0; index < width; ++index) bytes.Add((byte)(value >> (index * 8)));
    }
    /// <summary>요청이름과DWORD인수의전체순서를기록한다.</summary>
    private static void Event(List<byte> bytes, char name, params uint[] values)
    {
        bytes.Add((byte)name);
        // 요청인수마다원본DWORD폭을유지한다.
        foreach (uint value in values) Append(bytes, value);
    }
    /// <summary>float의원본비트다.</summary>
    private static uint Bits(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>전체용량의미사용0까지24BYTE입자순서로직렬화한다.</summary>
    private static byte[] Particles(ShrapnelFrameFields fields)
    {
        List<byte> bytes = [];
        // 모든입자의다섯float와색토큰을차례대로쓴다.
        foreach (ShrapnelParticle particle in fields.Particles)
        {
            Append(bytes, Bits(particle.X)); Append(bytes, Bits(particle.Y)); Append(bytes, Bits(particle.VelocityX)); Append(bytes, Bits(particle.VelocityY)); Append(bytes, Bits(particle.Life)); Append(bytes, particle.Pixel);
        }
        return bytes.ToArray();
    }
    /// <summary>76BYTE프레임또는92BYTE수명필드를직렬화한다.</summary>
    private static byte[] Fields(ShrapnelFrameFields fields, bool lifetime = false)
    {
        List<byte> bytes = []; RenderProcessFields rect = fields.Registration;
        if (lifetime) { Append(bytes, unchecked((uint)rect.Form)); Append(bytes, unchecked((uint)rect.Parent)); }
        Append(bytes, Bits(rect.Left)); Append(bytes, Bits(rect.Top)); Append(bytes, Bits(rect.Right)); Append(bytes, Bits(rect.Bottom));
        if (lifetime) Append(bytes, unchecked((uint)rect.Slot));
        Append(bytes, unchecked((uint)rect.ZOrder)); Append(bytes, (uint)fields.Particles.Length); Append(bytes, unchecked((uint)fields.Total));
        Append(bytes, BitConverter.DoubleToUInt64Bits(fields.LastTime), 8); Append(bytes, BitConverter.DoubleToUInt64Bits(fields.Elapsed), 8);
        Append(bytes, Bits(fields.Delta)); Append(bytes, Bits(fields.ScaledDelta)); Append(bytes, fields.First ? 1u : 0);
        if (lifetime) Append(bytes, fields.Image.Pixels.Length != 0 ? 1u : 0);
        Append(bytes, unchecked((uint)fields.Image.Width)); Append(bytes, unchecked((uint)fields.Image.Height)); Append(bytes, unchecked((uint)fields.Image.HotspotX)); Append(bytes, unchecked((uint)fields.Image.HotspotY)); Append(bytes, Bits(fields.Gravity));
        return bytes.ToArray();
    }
    /// <summary>행번호와첫차이를보고해다른입력의회귀를구별한다.</summary>
    private static void Equal(byte[] actual, byte[] expected, int row, string field)
    {
        int difference = 0;
        // 원문기대값을재계산하지않고첫다른byte를찾는다.
        while (difference < Math.Min(actual.Length, expected.Length) && actual[difference] == expected[difference]) ++difference;
        Assert.True(actual.AsSpan().SequenceEqual(expected), $"파편 행{row} {field} byte{difference} 폭실제{actual.Length}/원본{expected.Length}");
    }
    /// <summary>명시SID까지실제client번호를확보한다.</summary>
    private static RawSidPool Pool(int last = 61)
    {
        RawSidPool pool = new(32768, false);
        // 원본입력의source50/dummy60/대체61까지순서대로확보한다.
        for (int sid = 5; sid <= last; ++sid) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
        return pool;
    }
    /// <summary>원문입력76BYTE와용량전체를의미필드로설치한다.</summary>
    private static ShrapnelFrameFields Input(byte[] before, byte[] image, byte[] particles)
    {
        ShrapnelFrameFields fields = new()
        {
            Particles = new ShrapnelParticle[U(before, 20)], Total = unchecked((int)U(before, 24)), LastTime = D(before, 28), Elapsed = D(before, 36), Delta = F(before, 44), ScaledDelta = F(before, 48), First = U(before, 52) != 0,
            Image = new() { Width = unchecked((int)U(before, 56)), Height = unchecked((int)U(before, 60)), HotspotX = unchecked((int)U(before, 64)), HotspotY = unchecked((int)U(before, 68)), Pixels = image }, Gravity = F(before, 72),
        };
        RenderProcessFields rect = fields.Registration; rect.Parent = 50; rect.Left = F(before, 0); rect.Top = F(before, 4); rect.Right = F(before, 8); rect.Bottom = F(before, 12); rect.ZOrder = unchecked((int)U(before, 16));
        // 색주소는같은bitmap의offset+1토큰으로만입력한다.
        for (int index = 0; index < fields.Particles.Length; ++index)
        {
            int offset = index * 24; fields.Particles[index] = new() { X = F(particles, offset), Y = F(particles, offset + 4), VelocityX = F(particles, offset + 8), VelocityY = F(particles, offset + 12), Life = F(particles, offset + 16), Pixel = U(particles, offset + 20) };
        }
        return fields;
    }
    /// <summary>생성또는Frame/Draw의모든독립관측을재현한다.</summary>
    private static void Replay(uint kind)
    {
        RawSidPool pool = Pool(); int count = 0, index = 0;
        // 원문기대필드는결과비교에만사용한다.
        foreach (string[] row in X86Fixture.Read("shrapnel-frame"))
        {
            int current = index++; Assert.Equal(15, row.Length); if (X86Fixture.UInt(row[0]) != kind) continue;
            byte[] before = X86Fixture.Bytes(row[2]), image = X86Fixture.Bytes(row[3]), particles = X86Fixture.Bytes(row[4]), parent = X86Fixture.Bytes(row[5]), globals = X86Fixture.Bytes(row[6]);
            Assert.Equal(76, before.Length); Assert.Equal(50, parent.Length); Assert.Equal(28, globals.Length); Assert.Equal(U(before, 20) * 24, (uint)particles.Length);
            parent.CopyTo(pool.AllocatedBytes(50)); GameRandom game = new(X86Fixture.UInt(row[7])); CrtRandom crt = new(X86Fixture.UInt(row[8]));
            ShrapnelFrameState state = new() { Now = D(globals, 0), PixelScaleX = F(globals, 8), PixelScaleY = F(globals, 12), Rate = F(globals, 16), CameraX = F(globals, 20), CameraY = F(globals, 24) };
            ShrapnelFrameFields fields = Input(before, image, particles); List<byte> events = []; byte[] canvas = Enumerable.Repeat((byte)199, 68 * 48).ToArray(); bool killed = false;
            ShrapnelFrameHooks hooks = new()
            {
                Reserve = bytes => { Event(events, 'N', bytes); return true; }, Release = scratch => Event(events, 'F', scratch ? 1u : 2),
                Invalidate = rect => Event(events, 'D', unchecked((uint)rect.Left), unchecked((uint)rect.Top), unchecked((uint)rect.Right), unchecked((uint)rect.Bottom), 0),
                RemoveForm = () => { Event(events, 'K', 0); killed = true; },
                Pixel = pixel => { Event(events, 'P', unchecked((uint)pixel.X), unchecked((uint)pixel.Y), pixel.Color); if (pixel.X >= 1 && pixel.X < 63 && pixel.Y >= 2 && pixel.Y < 46) canvas[pixel.Y * 68 + pixel.X] = (byte)pixel.Color; },
            };
            RawShrapnelFrame logic = new(pool, game, crt, state, hooks);
            if (kind == 0) logic.Prepare(fields, (ShrapnelKind)X86Fixture.UInt(row[1])); else { logic.Step(fields); if (!killed) logic.Draw(fields); }
            Equal(Fields(fields), X86Fixture.Bytes(row[9]), current, "76BYTE"); Equal(Particles(fields), X86Fixture.Bytes(row[10]), current, "전체입자");
            Assert.Equal(X86Fixture.UInt(row[11]), game.State()); Assert.Equal(X86Fixture.UInt(row[12]), crt.State());
            Equal(events.ToArray(), X86Fixture.Bytes(row[13]), current, "요청"); Equal(canvas, X86Fixture.Bytes(row[14]), current, "clip/pitch"); Assert.Equal(image, fields.Image.Pixels); ++count;
        }
        Assert.Equal(kind == 0 ? 198 : 782, count);
    }
    /// <summary>세생성기·역순게임난수·매조건CRT·가장자리/투명픽셀198관측이다.</summary>
    [Fact]
    public void PreparationMatchesAll198NativeObservations() => Replay(0);
    /// <summary>간격/중력저장·진입수명/소진·정수화·실제clip/padding782관측이다.</summary>
    [Fact]
    public void FrameAndDrawMatchAll782NativeObservations() => Replay(1);
    /// <summary>검사raw슬롯에DWORD입력을쓴다.</summary>
    private static void Put(RawSidPool pool, int sid, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(pool.AllocatedBytes(sid).Slice(offset, 4), value);
    /// <summary>원문목록전체를빈칸포함직렬화한다.</summary>
    private static string Slots(RawRenderRegistry registry) => registry.Slots.Length == 0 ? "-" : string.Join(',', registry.Slots.ToArray());

    /// <summary>부모0목록·callback현재값·실제준비·선택객체반납216관측이다.</summary>
    [Fact]
    public void CreationAndReleaseMatchAll216NativeObservations()
    {
        RawSidPool pool = Pool(); int count = 0;
        // 각행의명시입력만수명몸체에공급한다.
        foreach (string[] row in X86Fixture.Read("shrapnel-lifetime"))
        {
            Assert.Equal(15, row.Length); ShrapnelKind kind = (ShrapnelKind)X86Fixture.UInt(row[0]); int capacity = X86Fixture.Int(row[1]), hole = X86Fixture.Int(row[2]); uint mutation = X86Fixture.UInt(row[4]), flags = X86Fixture.UInt(row[5]);
            pool.RawSlot(0)[35] = 255;
            // source/실제더미/대체부모의현재raw입력을설치한다.
            foreach (int sid in new[] { 50, 60, 61 })
            {
                Span<byte> raw = pool.AllocatedBytes(sid); raw.Clear(); Put(pool, sid, 0, 0x12900000); raw[10] = sid == 50 ? (byte)144 : (byte)110; raw[35] = sid == 50 ? (byte)X86Fixture.UInt(row[3]) : sid == 60 ? (byte)0 : (byte)127;
                Put(pool, sid, 14, Bits(sid == 50 ? 2.125f : sid == 60 ? 9.75f : -2.75f)); Put(pool, sid, 18, Bits(sid == 50 ? 3.25f : sid == 60 ? 8.5f : 7.125f));
            }
            uint[] slots = Enumerable.Range(0, capacity).Select(index => index == hole ? 0u : 100u + (uint)index).ToArray(); List<byte> events = [];
            RawRenderRegistry registry = new(pool, new() { Reserve = bytes => { Event(events, 'N', bytes); return true; }, ReleaseArray = () => Event(events, 'F', 1), Attach = (_, _, _, _) => throw new InvalidOperationException("부모0의 예상 밖 부착") }, slots);
            GameRandom game = new(0x12345678); UIntBox crtState = new() { Value = 1 }; CrtRandom crt = CrtRandom.Borrow(crtState); ShrapnelFrameState state = new() { Now = 1.125 };
            ShrapnelFrameFields fields = new() { Delta = BitConverter.UInt32BitsToSingle(0xa5a5a5a5), ScaledDelta = BitConverter.UInt32BitsToSingle(0xa5a5a5a5) };
            RawShrapnelFrame frame = new(pool, game, crt, state, new()
            {
                Reserve = bytes => { Event(events, 'N', bytes); return true; }, Release = scratch => Event(events, 'F', scratch ? 5u : 3), Invalidate = _ => throw new InvalidOperationException("준비의 예상 밖 dirty"), Pixel = _ => throw new InvalidOperationException("준비의 예상 밖 픽셀"), RemoveForm = () => throw new InvalidOperationException("준비의 예상 밖 소진"),
            });
            ShrapnelLifetimeHooks hooks = new()
            {
                Reserve = bytes => { Event(events, 'N', bytes); return true; },
                Create = (type, value) => { Event(events, 'C', type, value); Assert.Equal(110u, type); Assert.Equal(2u, value); if (mutation == 1) { Put(pool, 50, 14, Bits(-7.75f)); Put(pool, 50, 18, Bits(8.25f)); pool.AllocatedBytes(50)[35] = 128; } return 60; },
                Pop = (sid, x, y, value) => { Event(events, 'P', (uint)sid, Bits(x), Bits(y), value); Put(pool, sid, 14, Bits(x)); Put(pool, sid, 18, Bits(y)); },
                Attach = (rect, type, dummy, value) => { Event(events, 'A', type, (uint)dummy, 0, value, (uint)rect.Slot, unchecked((uint)rect.ZOrder)); rect.Form = 80; rect.Parent = mutation == 2 ? 61 : dummy; },
                Depend = (form, dummy) => Event(events, 'D', (uint)form, (uint)dummy),
                Capture = source => { Event(events, 'I', (uint)source); ConstructionVisualImage image = new() { Width = 3, Height = 4, HotspotX = 1, HotspotY = 3, Pixels = Enumerable.Range(0, 12).Select(index => (byte)((index * 13 + 7) % 255)).ToArray() }; if (mutation == 3) pool.AllocatedBytes(50)[35] = 255; return image; },
                Prepare = frame.Prepare,
            };
            RawShrapnelLifetime lifetime = new(pool, registry, hooks); Assert.True(lifetime.ReserveObject()); int dummy = 0; lifetime.Initialize(fields, 1, 50, kind, ref dummy); Assert.Equal(60, dummy);
            Equal(Fields(fields, true), X86Fixture.Bytes(row[6]), count, "생성92"); Equal(Particles(fields), X86Fixture.Bytes(row[7]), count, "전체입자"); Assert.Equal(X86Fixture.UInt(row[8]), game.State()); Assert.Equal(X86Fixture.UInt(row[9]), crt.State()); Assert.Equal(crtState.Value, crt.State()); Assert.Equal(row[10], Slots(registry)); Equal(events.ToArray(), X86Fixture.Bytes(row[11]), count, "생성요청");
            events.Clear(); lifetime.Destroy(fields, flags, release => { Event(events, 'F', release == ShrapnelRelease.Source ? 2u : release == ShrapnelRelease.Particles ? 3u : 4u); Assert.Equal(release == ShrapnelRelease.Object ? 0u : 1u, registry.Slots[fields.Registration.Slot]); });
            byte[] actual = Fields(fields, true), expected = X86Fixture.Bytes(row[12]);
            // 반납뒤raw포인터/용량은dangling값이지만관리배열은실제반납하므로그두표현만제외한다.
            Equal(actual[..32], expected[..32], count, "삭제등록"); Equal(actual[36..68], expected[36..68], count, "삭제시간/꼬리"); Equal(actual[72..], expected[72..], count, "삭제메타");
            Assert.Equal(row[13], Slots(registry)); Equal(events.ToArray(), X86Fixture.Bytes(row[14]), count, "반납요청"); Assert.Empty(fields.Particles); Assert.Empty(fields.Image.Pixels); ++count;
        }
        Assert.Equal(216, count);
    }
}
