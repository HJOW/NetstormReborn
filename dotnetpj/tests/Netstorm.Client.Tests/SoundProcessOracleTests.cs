using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Client.Audio;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 원본 소리 프로세스/이름 표 자료를 실제 Form/Kernel과 같은 논리 객체 필드에 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>원본 PATCH6127입력의 소리 실행·삭제 통지·부착·이름 표 전체 해시를 재생한다.</summary>
public sealed class SoundProcessOracleTests
{
    /// <summary>원본 관측의 같은 부모 SID다.</summary>
    private const int Target = 50;
    /// <summary>원문DWORD 필드를 정확한 폭으로 쓴다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[offset..], value);
    /// <summary>원본16진 double 비트를 산술 변환 없이 복원한다.</summary>
    private static double DoubleHex(string value) => BitConverter.UInt64BitsToDouble(ulong.Parse(value, NumberStyles.HexNumber, CultureInfo.InvariantCulture));
    /// <summary>실제 같은 풀/Host/Kernel·현재 전역과 명시 시계/재생 입력을 소유한다.</summary>
    private sealed class Scene
    {
        /// <summary>원본 fixture의 실제 client 슬롯 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, false);
        /// <summary>원문 소리 객체가 실제로 부착되는 커널이다.</summary>
        public readonly Kernel Kernel = new();
        /// <summary>프로세스와 원문 관측이 공유하는 현재 전역이다.</summary>
        public readonly SoundProcessState State = new();
        /// <summary>실제 소리 부착/삭제 호스트다.</summary>
        public readonly SquidProcessHost Host;
        /// <summary>실제 Add/Run/Destroy를 실행하는 같은 시스템이다.</summary>
        public readonly SoundProcessSystem System;
        /// <summary>발생 순서 그대로 기록한 원문 효과 토큰이다.</summary>
        public readonly List<string> Events = [];
        /// <summary>현재 명시 장치 반환 입력이다.</summary>
        public uint Reply;
        /// <summary>현재 명시 벽시계 입력이다.</summary>
        public double Now;
        /// <summary>원본과 같은 부모/내장 생성자·실제 Form/Kernel을 준비한다.</summary>
        public Scene()
        {
            // 원본 Ctor 입력의 마지막 부모127까지 실제 client 할당을 보존한다.
            for (int sid = 5; sid <= 127; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
            var types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(); types[2].ConstructorAddress = OriginalConstructors.Address(2);
            var factory = new RawSquidFactory(Pool, types); var unpop = new RawUnpop(Pool, new RawSquidHash(), new byte[65536]);
            Host = new SquidProcessHost(Pool, types, Kernel, new RawSquidDestroy(Pool, unpop, types), new SquidProcessState()); Host.BindFormFactory(factory);
            System = new SoundProcessSystem(Host, State, new SoundProcessHooks
            {
                WallSeconds = () => { Events.Add("T"); return Now; },
                PlayLoopAt = (x, y, current, sound) => { Events.Add($"L:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}:{current}:{sound}"); return Reply; },
                PlayOnceAt = (x, y, sound, priority, zero) => { Events.Add($"O:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}:{sound}:{priority}:{zero}"); return Reply; },
                Play = (sound, loop, a, b, priority, c) => { Events.Add($"G:{sound}:{loop}:{a}:{b}:{priority}:{c}"); return Reply; },
                Stop = sound => Events.Add($"S:{sound}"), IsPlaying = sound => { Events.Add($"P:{sound}"); return Reply; }, Report = (_, line) => Events.Add($"!{line}"),
            });
        }
        /// <summary>원본이 읽는 부모 타입/WORD/좌표 입력만 복원한다.</summary>
        public void Parent(uint word, uint x, uint y)
        {
            Span<byte> raw = Pool.AllocatedBytes(Target); raw.Clear(); raw[10] = 82; BinaryPrimitives.WriteUInt16LittleEndian(raw[12..], (ushort)word); Put(raw, 14, x); Put(raw, 18, y);
        }
        /// <summary>현재 논리 필드를 원본 객체의 같은 오프셋과 비교한다. Ctor 자료는+4부터 시작한다.</summary>
        public static void SameFields(SoundProcess process, string hex, int skip = 0)
        {
            byte[] raw = X86Fixture.Bytes(hex);
            Assert.Equal(BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(12 - skip)), process.Current());
            Assert.Equal(BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(16 - skip)), process.Sound());
            Assert.Equal(BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(20 - skip)), process.Alternate());
            Assert.Equal(BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(24 - skip)), process.Flags());
            Assert.Equal(BinaryPrimitives.ReadUInt64LittleEndian(raw.AsSpan(32 - skip)), BitConverter.DoubleToUInt64Bits(process.StartTime()));
        }
        /// <summary>이미 자기 삭제한 경우는 다시 읽지 않고, 살아 있으면 실제 Kill로 정리한다.</summary>
        public void Discard(SoundProcess process)
        {
            if (Kernel.Size() != 0) Host.Kill(process, 0x10);
            Assert.Equal(0, Kernel.Size()); Events.Clear();
        }
        /// <summary>순서 토큰을 원본 빈 표식과 같은 형식으로 만든다.</summary>
        public string EventText() => Events.Count == 0 ? "-" : string.Join(';', Events);
    }

    /// <summary>원본6127행 중Lookup은 별도 실제 표에서 나머지는 같은 실제 소리 객체/Host로 대조한다.</summary>
    [Fact]
    public void OriginalPatch6127MatchesRunDestroyCtorAndLookup()
    {
        string[][] all = X86Fixture.Read("soundprocess"); Assert.Equal(18381, all.Length);
        string[][] rows = all.Where(row => row[0] == "originals").ToArray(); Assert.Equal(6127, rows.Length); var scene = new Scene();
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[1] == "Lookup") { Lookup(row); return true; }
            if (row[1] == "Run")
            {
                Assert.Equal(18, row.Length); scene.Parent(X86Fixture.UInt(row[4]), X86Fixture.UInt(row[11]), X86Fixture.UInt(row[12]));
                scene.State.Frame = X86Fixture.UInt(row[5]); Array.Fill(scene.State.TypeFrames, 0xa5a5a5a5u); scene.State.TypeFrames[82] = X86Fixture.UInt(row[6]);
                scene.State.Debug = new RawCheckingFlag(row[10] == "1"); scene.Reply = X86Fixture.UInt(row[7]); scene.Now = DoubleHex(row[8]);
                SoundProcess process = Assert.IsType<SoundProcess>(scene.System.Add(Target, X86Fixture.UInt(row[13]), X86Fixture.UInt(row[14]), X86Fixture.UInt(row[2])));
                process.SetCurrent(X86Fixture.UInt(row[3])); process.SetStartTime(DoubleHex(row[9])); scene.Events.Clear(); process.RunFrame();
                bool killed = scene.Kernel.Size() == 0; if (killed) scene.Events.Add("K:0");
                Assert.Equal(row[15], scene.EventText()); Assert.Equal(X86Fixture.UInt(row[17]), scene.State.TypeFrames[82]); Assert.Equal(0xa5a5a5a5u, scene.State.TypeFrames[83]);
                if (!killed) Scene.SameFields(process, row[16]); scene.Discard(process); return true;
            }
            if (row[1] == "Destroy")
            {
                Assert.Equal(8, row.Length); scene.Parent(0, 0, 0); scene.Reply = X86Fixture.UInt(row[4]); byte[] dump = X86Fixture.Bytes(row[7]);
                SoundProcess process = Assert.IsType<SoundProcess>(scene.System.Add(Target, BinaryPrimitives.ReadUInt32LittleEndian(dump.AsSpan(16)), BinaryPrimitives.ReadUInt32LittleEndian(dump.AsSpan(20)), X86Fixture.UInt(row[2])));
                process.SetCurrent(X86Fixture.UInt(row[3])); scene.Events.Clear(); process.OnFormDestroy(X86Fixture.UInt(row[5]));
                Assert.Equal(row[6], scene.EventText()); Scene.SameFields(process, row[7]); scene.Discard(process); return true;
            }
            Assert.Equal("Ctor", row[1]); Assert.Equal(9, row.Length); scene.State.ProcessType = X86Fixture.UInt(row[6]);
            SoundProcess created = Assert.IsType<SoundProcess>(scene.System.Add((ushort)X86Fixture.UInt(row[2]), X86Fixture.UInt(row[3]), X86Fixture.UInt(row[4]), X86Fixture.UInt(row[5])));
            int form = created.Form(); bool client = form > 4 && form < RawSidPool.ServerFirst;
            Assert.Equal(row[7], $"B:{scene.Pool.Slot(form)[10]}:{created.Parent()}:0:{(client ? 16 : 0)}"); Scene.SameFields(created, row[8], 4); Assert.Empty(scene.Events);
            scene.State.ProcessType = 47; scene.Discard(created); return true;
        });
    }
    /// <summary>조회 연산 입력만 공급하고 원본 전체0x8000BYTE의Adler와 반환 핸들/보고를 대조한다.</summary>
    private static void Lookup(string[] row)
    {
        Assert.Equal(6, row.Length); var tokens = new List<string>(); var list = new SoundList(OriginalEdition.Patch1078, (_, line) => tokens.Add($"!{line}"));
        // 각 입력은i초기화/z null/p표포인터/hex이름이며 판단은 실제 이름 표가 한다.
        foreach (string op in row[2].Split('|'))
        {
            uint result;
            if (op == "i") { list.Initialize(); result = list.Fallback(); }
            else if (op == "z") result = list.Lookup((string?)null);
            else if (op[0] == 'p') { uint offset = X86Fixture.UInt(op[2..]); result = offset < SoundConstants.ListBytes ? list.LookupPointer(SoundConstants.ListBase + offset) : list.Lookup("edge.wav"); }
            else result = list.Lookup(X86Fixture.Bytes(op[2..]));
            tokens.Add(result != 0 ? unchecked((int)(result - SoundConstants.ListBase)).ToString(CultureInfo.InvariantCulture) : "null");
        }
        Assert.Equal(row[3], string.Join(',', tokens)); Assert.Equal(X86Fixture.UInt(row[4]), list.FirstFree() - SoundConstants.ListBase); Assert.Equal(X86Fixture.UInt(row[5]), X86Fixture.Adler32(list.Raw().Span));
    }
    /// <summary>실제 부착 역할·보고 복귀 뒤 checking·재생 부작용 뒤 TYPE/프레임·double지연을 현재값으로 읽는다.</summary>
    [Fact]
    public void CurrentRoleCheckingFrameStampAndDoubleDelayFollowEffects()
    {
        var scene = new Scene(); scene.Parent(1, 0, 0); uint checking = 0x80000000; var role = new UIntBox { Value = 47 };
        var state = new SoundProcessState { Frame = 100, SharedProcessType = role, Debug = new RawCheckingFlag(() => checking) }; int reports = 0, plays = 0; double now = 2;
        var hooks = new SoundProcessHooks
        {
            WallSeconds = () => now,
            PlayLoopAt = (_, _, _, _) => throw new InvalidOperationException("미선택 위치 반복"), PlayOnceAt = (_, _, _, _, _) => throw new InvalidOperationException("미선택 위치 한번"),
            Play = (sound, loop, volume, pan, priority, limit) =>
            {
                Assert.Equal(456u, sound); Assert.Equal(1u, loop); Assert.Equal(8u, priority); plays++;
                scene.Pool.AllocatedBytes(Target)[10] = 83; state.Frame = 777; return 123;
            },
            Stop = sound => Assert.Equal(123u, sound), IsPlaying = _ => 0,
            Report = (expression, line) => { Assert.Equal("playOnce || !playPriority", expression); Assert.Equal(0x35, line); reports++; checking = 0; },
        };
        var system = new SoundProcessSystem(scene.Host, state, hooks); hooks.Report = null; hooks.Play = null;
        SoundProcess process = Assert.IsType<SoundProcess>(system.Add(Target, 1234, 456, SoundProcess.UseAlternate | SoundProcess.PlayPriority)); Assert.Equal(47, scene.Pool.Slot(process.Form())[10]);
        process.RunFrame(); Assert.Equal(1, reports); Assert.Equal(1, plays); Assert.Equal(0u, state.TypeFrames[82]); Assert.Equal(777u, state.TypeFrames[83]); Assert.Equal(123u, process.Current());
        scene.Host.Kill(process, 0x10); role.Value = 48; SoundProcess next = Assert.IsType<SoundProcess>(system.Add(Target, 0, 0, SoundProcess.PlayOnce)); Assert.Equal(48, scene.Pool.Slot(next.Form())[10]); scene.Host.Kill(next, 0x10);
        var standalone = new SoundProcess(system, 0, 0, 0); now = 16777216; standalone.Delay(0.1f); Assert.Equal(BitConverter.DoubleToUInt64Bits(now + (double)0.1f), BitConverter.DoubleToUInt64Bits(standalone.StartTime()));
        standalone.RunFrame(); Assert.Equal(1, plays); Assert.Equal(0, scene.Kernel.Size());
        Assert.Throws<ArgumentException>(() => new SoundProcessSystem(scene.Host, state, new SoundProcessHooks()));
    }

}
