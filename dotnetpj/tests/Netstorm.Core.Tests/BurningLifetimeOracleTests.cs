using System.Buffers.Binary;
using Netstorm.Core.Display;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 원본 출력/해제/첫색표·프레임gate·발사기그림 요청을 실제공통몸체와 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>그리기579·삭제64·cold12·gate128·발사기88개의 독립 원본 관찰이다.</summary>
public sealed partial class BurningVisualOracleTests
{
    /// <summary>fixture의 현재 입력DWORD 목록만 읽는다.</summary>
    private static uint[] Values(string text) => text.Split(',').Select(X86Fixture.UInt).ToArray();
    /// <summary>현재 의미27DWORD를 원본 순서로 직렬화한다.</summary>
    private static uint[] Words(BurningVisualFields f)
    {
        RenderProcessFields r = f.Registration; ulong last = BitConverter.DoubleToUInt64Bits(f.LastTime), elapsed = BitConverter.DoubleToUInt64Bits(f.Elapsed);
        return [unchecked((uint)r.Form), unchecked((uint)r.Parent), BitConverter.SingleToUInt32Bits(r.Left), BitConverter.SingleToUInt32Bits(r.Top), BitConverter.SingleToUInt32Bits(r.Right), BitConverter.SingleToUInt32Bits(r.Bottom), unchecked((uint)r.Slot), unchecked((uint)r.ZOrder), f.Capacity, f.Total,
            (uint)last, (uint)(last >> 32), (uint)elapsed, (uint)(elapsed >> 32), f.Palette, f.Sound, f.Stage, f.Image.Pixels.Length == 0 ? 0u : 1u, unchecked((uint)f.Image.Width), unchecked((uint)f.Image.Height), unchecked((uint)f.Image.HotspotX), unchecked((uint)f.Image.HotspotY), (uint)f.Victim, f.Mode, f.Rows.Length == 0 ? 0u : 1u, unchecked((uint)f.Selected), f.Off ? 1u : 0u];
    }
    /// <summary>양수 이동입자의 실제5픽셀순서·clip·인접색조회·64×64표면전체를 원본과 대조한다.</summary>
    [Fact]
    public void DrawingMatchesOriginalWritesAndWholeSurface()
    {
        string[][] rows = X86Fixture.Read("burning-lifetime-draw", "D"); Assert.Equal(579, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            uint[] v = Values(row[1]); var fields = new BurningVisualFields { Capacity = v[0], Total = v[1] }; fields.Particles = new BurningParticle[fields.Capacity];
            if (fields.Capacity != 0) fields.Particles[0] = new(F(v[2]), F(v[3]), 0, 0, F(v[4]), v[5] != 0);
            if (v[9] != 0)
                // 원문 별도source/이동/만료입자 입력을 공급하며 출력값은 계산하지 않는다.
                for (int i = 1; i < 4; i++) fields.Particles[i] = new(20.75f + i, 22.5f - i, 0, 0, i == 3 ? -1 : 4, i == 1);
            var state = new BurningFrameState { CameraX = F(v[6]), CameraY = F(v[7]), Viewport = v[8] != 0 ? new(10, 10, 30, 30) : new(0, 0, 64, 64) };
            uint[] colors = [31, 32, 33, 34, 35, 36, 37, 38, 39]; byte[] pixels = Enumerable.Repeat((byte)0xa5, 4096).ToArray(); var writes = new List<string>();
            RawBurningLifetimeDraw.Draw(fields, state, new() { Count = 8, Readable = colors }, (x, y, color) => { int at = y * 64 + x; Assert.InRange(at, 0, pixels.Length - 1); pixels[at] = color; writes.Add($"{at}:{color}"); });
            string actual = $"{X86Fixture.Adler32(pixels)}:{pixels.Sum(b => (int)b)}|{string.Join(';', writes)}"; Assert.Equal(row[2], actual); return true;
        });
    }
    /// <summary>소유버퍼/flags별 소리·등록/base·객체반납 전체 순서를 원본과 대조한다.</summary>
    [Fact]
    public void DestructionMatchesOriginalBuffersSoundAndRegistration()
    {
        string[][] rows = X86Fixture.Read("burning-lifetime-draw", "X"); Assert.Equal(64, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            uint[] v = Values(row[1]); var pool = new RawSidPool(24000, true); var registry = new RawRenderRegistry(pool, new() { Reserve = _ => true, ReleaseArray = () => { }, Attach = (_, _, _, _) => { } }, new uint[] { 111, 777, 222 });
            var fields = new BurningVisualFields { Sound = v[4] != 0 ? 55u : 0 }; RenderProcessFields r = fields.Registration; r.Token = 777; r.Form = 83; r.Parent = 50; r.Slot = 1; r.ZOrder = -12;
            if (v[1] != 0) fields.Image.Pixels = new byte[1]; if (v[2] != 0) fields.Rows = new byte[1]; if (v[3] != 0) fields.Particles = new BurningParticle[1]; var actions = new List<string>();
            string[] labels = ["image", "rows", "particles", "object"];
            RawBurningLifetimeDraw.Destroy(fields, registry, v[0], kind => actions.Add("Free:" + labels[(int)kind]), sound => actions.Add($"Stop:{sound}"));
            uint[] values = [unchecked((uint)r.Form), unchecked((uint)r.Parent), unchecked((uint)r.Slot), unchecked((uint)r.ZOrder)];
            Assert.Equal(row[2], $"{string.Join(',', values)}|{string.Join(',', registry.Slots.ToArray())}|{string.Join(';', actions)}"); Assert.Empty(fields.Image.Pixels); Assert.Empty(fields.Rows); Assert.Empty(fields.Particles); return true;
        });
    }
    /// <summary>cold生成자의 실제6double gradient인수·의미필드·부착/확보를 원본12행에 대조한다.</summary>
    [Fact]
    public void ColdConstructorMatchesOriginalGradientAndAllFields()
    {
        string[][] rows = X86Fixture.Read("burning-lifetime-draw", "C"); Assert.Equal(12, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            uint[] v = Values(row[1]); var pool = new RawSidPool(24000, true); pool.RawSlot(0)[35] = unchecked((byte)-8); pool.RawSlot(50)[35] = unchecked((byte)-11); var actions = new List<string>(); string extra = "";
            var registry = new RawRenderRegistry(pool, new() { Reserve = amount => { actions.Add($"N:{amount}"); return true; }, ReleaseArray = () => { }, Attach = (fields, type, parent, flags) => { actions.Add($"B:{type}:{parent}:0:{flags}"); fields.Form = 83; fields.Parent = parent; } }, new uint[] { 0 });
            var state = new BurningVisualState { Palette = v[0] != 0 ? 0u : 1u }; var visual = new RawBurningVisual(pool, registry, state, new()
            {
                Reserve = amount => { actions.Add($"N:{amount}"); return true; }, CreatePalette = () => 1,
                Gradient = (from, to, count, width) => { ulong[] bits = from.Concat(to).Select(BitConverter.DoubleToUInt64Bits).ToArray(); extra = $"G:{string.Join(',', bits)}:{count}:{BitConverter.DoubleToUInt64Bits(width)}"; },
                BuildPalette = _ => extra += ";Palette",
            });
            var fields = new BurningVisualFields(); visual.Initialize(fields, 777, (ushort)v[1], v[2]); byte[] particles = ParticleBytes(fields);
            string actual = $"{string.Join(',', Words(fields))}|{string.Join(',', registry.Slots.ToArray())}|{string.Join(';', actions)}|{X86Fixture.Adler32(particles)}|{particles.Sum(b => (int)b)}|{(state.Palette != 0 ? "True" : "False")}|{extra}";
            Assert.Equal(row[2], actual); return true;
        });
    }
    /// <summary>signed slot과 현재DWORD프레임을 원본128관찰에 직접 대조한다.</summary>
    [Fact]
    public void CommonFrameGateMatchesOriginal()
    {
        string[][] rows = X86Fixture.Read("render-frame-gate"); Assert.Equal(128, rows.Length);
        X86Fixture.CheckRows(rows, row => { Assert.Equal(row[2] == "1", RawBurningLifetimeDraw.FrameDue(X86Fixture.Int(row[0]), X86Fixture.UInt(row[1]))); return true; });
    }
    /// <summary>발사기의 현재frame/출력WORD/extra와캐논의빈그림을 원본88요청에 대조한다.</summary>
    [Fact]
    public void ArcherCaptureMatchesOriginalCurrentFrameAndMuzzleRequests()
    {
        string[][] rows = X86Fixture.Read("burning-archer-capture"); Assert.Equal(88, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            string[] v = row[0].Split(','); uint type = X86Fixture.UInt(v[0]); byte[] raw = new byte[50]; BinaryPrimitives.WriteUInt32LittleEndian(raw, type == 71 ? 0x50b888u : 0x50b9b8u); raw[10] = (byte)type;
            BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(36), X86Fixture.UInt(v[2])); BinaryPrimitives.WriteUInt16LittleEndian(raw.AsSpan(12), (ushort)X86Fixture.UInt(v[3])); raw[40] = (byte)X86Fixture.UInt(v[4]); var actions = new List<string>();
            int baseFrame = X86Fixture.Int(v[1]); RawBurningLifetimeDraw.DrawArcherCapture(raw, baseFrame, -7, 11, (shape, frame, x, y) => actions.Add($"{shape},{frame},{x},{y}"), 311, 233);
            Assert.Equal(row[1], $"{unchecked((uint)baseFrame)}|{string.Join(';', actions)}"); return true;
        });
    }
}
