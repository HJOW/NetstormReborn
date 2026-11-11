using System.Globalization;
using System.Text;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>
/// 구름 갱신·dirty·행 조회·표시(<see cref="RawKencloudRender"/>)를 고정 Native 944관측과 대조한다.
/// 전체 물리(FNV 해시)·clip·속도·반환·호출 순서를 모두 비교한다.
/// cpppj <c>KencloudRenderTests.cpp</c>의 944행 재생과 자연 생략·물리 접두 검사를 옮겼다. (0.381, 2026-11-11)
/// </summary>
public sealed class RawKencloudRenderTests
{
    /// <summary>독립 원문의 명시 생성 주소다.</summary>
    private const uint First = 0x12001000;

    /// <summary>독립 원문의 명시 변경 주소다.</summary>
    private const uint Second = 0x12051000;

    /// <summary>명시 경계 중단을 정상 return과 구별한다.</summary>
    private sealed class Stop : Exception
    {
        /// <summary>중단한 경계 이름이다.</summary>
        public string Cause { get; }

        /// <summary>경계 이름으로 만든다.</summary>
        /// <param name="cause">경계 이름이다.</param>
        public Stop(string cause) => Cause = cause;
    }

    /// <summary>명시 하위 효과만 구현하고 부모 계산은 제품 몸체를 호출하는 장치다(cpppj 시험의 <c>Device</c>).</summary>
    private sealed class Device
    {
        /// <summary>같은 packed 물리다.</summary>
        public readonly uint[] State = new uint[2054];

        /// <summary>기존 clip 네 DWORD다.</summary>
        public uint[] Clip = [0xfffffff7, 0xfffffff9, 900, 700];

        /// <summary>현재 속도다.</summary>
        public uint Speed;

        /// <summary>경계 순번 계수다.</summary>
        public uint Steps, Clips, Blits;

        /// <summary>하위 효과 변형 이름이다.</summary>
        public string Mutation = "";

        /// <summary>호출 순서 기록이다.</summary>
        public readonly StringBuilder Trace = new();

        /// <summary>진입 순서만 기록한다.</summary>
        /// <param name="value">기록할 값이다.</param>
        private void Event(string value)
        {
            if (Trace.Length != 0) Trace.Append(';');
            Trace.Append(value);
        }

        /// <summary>원문 도구와 같은 명시 하위 입력을 경계마다 적용한다.</summary>
        /// <param name="label">경계 이름이다.</param>
        private void Mutate(string label)
        {
            if (Mutation == label + "-stop") throw new Stop(label);
            if (Mutation == label + "-speed2") Speed = 2;
            else if (Mutation == label + "-speed0") Speed = 0;
            else if (Mutation == label + "-cache22") State[515] = Second;
            else if (Mutation == label + "-all0") State[2051] = 0;
            else if (Mutation == label + "-all1") State[2051] = 1;
        }

        /// <summary>명시 하위 장치 묶음이다. 실제 잠금/복사 장치는 별도 실제 Screen 검사로 대조한다.</summary>
        public KencloudRenderHooks Hooks() => new()
        {
            Build = () => { Event("BUILD"); State[515] = First; Mutate("build"); },
            Step = () => { Steps++; Event("STEP"); Mutate("step" + Steps); },
            Clip = () => Clip,
            SetClip = value => { Clips++; Event("CLIP:" + Numbers(value)); Clip = value; Mutate("clip" + Clips); },
            Lock = alternate => { Event("LOCK:" + (alternate ? 1 : 0)); Mutate("lock"); },
            Unlock = alternate => { Event("UNLOCK:" + (alternate ? 1 : 0)); Mutate("unlock"); },
            Blit = args => { Blits++; Event("BLIT:" + Numbers(args)); Mutate("blit" + Blits); },
        };
    }

    /// <summary>원시 unsigned DWORD의 십진 입력이다.</summary>
    /// <param name="values">값 배열이다.</param>
    private static string Numbers(uint[] values) => string.Join(',', values);

    /// <summary>같은 little-endian 2054 DWORD 전체를 FNV-1a 64로 식별한다.</summary>
    /// <param name="state">전체 물리다.</param>
    private static string Hash(uint[] state)
    {
        ulong value = 14695981039346656037UL;
        // 모든 DWORD의 네 바이트를 낮은 쪽부터 섞는다.
        foreach (uint word in state)
        {
            for (int shift = 0; shift < 32; shift += 8)
            {
                value ^= (byte)(word >> shift);
                value *= 1099511628211UL;
            }
        }
        return value.ToString("x16", CultureInfo.InvariantCulture);
    }

    /// <summary>양 x87 모드의 944행을 그대로 읽고 부분 중단까지 같은 상태/순서를 대조한다.</summary>
    [Fact]
    public void Original944Observations()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "kencloud-render-x86.tsv");
        int rows = 0;
        // 고정 fixture 행을 정규화하거나 중복 제거하지 않는다.
        foreach (string raw in File.ReadLines(path))
        {
            string line = raw.EndsWith('\r') ? raw[..^1] : raw;
            if (line.Length == 0 || line[0] == '#') continue;
            string[] fields = line.Split('\t'), args = fields[1].Split(',');
            Assert.True(fields.Length == 8 && args.Length == 13);
            var device = new Device();
            // 같은 현재 물리 초기 입력이다.
            for (int index = 0; index < device.State.Length; index++) device.State[index] = 0x81000000U + (uint)index;
            uint Number(int index) => unchecked((uint)long.Parse(args[index], CultureInfo.InvariantCulture));
            uint cache = Number(1) != 0 ? First : 0, pattern = Number(3);
            device.Speed = Number(2);
            device.Mutation = args[12];
            // 같은 이전/현재 행 dirty 표를 만든다.
            for (int index = 0; index < 512; index++)
            {
                device.State[index] = pattern == 2 || (pattern == 1 && (index == 0 || index == 8 || index == 511)) ? 1U : 0U;
                device.State[516 + index] = pattern == 2 || (pattern == 1 && (index == 1 || index == 8 || index == 511)) ? 1U : 0U;
            }
            device.State[512] = unchecked(cache + 262144);
            device.State[513] = unchecked(cache + 261632);
            device.State[514] = unchecked(cache + 512);
            device.State[515] = cache;
            device.State[2051] = Number(4);
            device.State[2052] = Number(5);
            device.State[2053] = 9;
            uint[] area = new uint[6];
            // 같은 인자 DWORD를 보존한다.
            for (int index = 0; index < 6; index++) area[index] = Number(index + 6);
            KencloudRenderHooks hooks = device.Hooks();
            var globals = new KencloudRenderGlobals(device.State, () => device.Speed);
            uint result = 0;
            string completion = "return";
            try
            {
                if (args[0] == "update") RawKencloudRender.UpdateKencloud(globals, hooks);
                else if (args[0] == "mark") RawKencloudRender.MarkKencloudDirty(device.State);
                else if (args[0] == "clean") RawKencloudRender.CleanKencloudDirty(device.State);
                else if (args[0] == "query") result = RawKencloudRender.KencloudRowDirty(device.State, area[0], area[1]);
                else RawKencloudRender.DrawKencloud(globals, area, hooks, args[0] == "alternate");
            }
            catch (Stop stop) { completion = stop.Cause; }
            string actual = fields[0] + '\t' + fields[1] + '\t' + completion + '\t' + result + '\t' + Hash(device.State) + '\t' + Numbers(device.Clip) + '\t' + device.Speed + '\t' + device.Trace;
            Assert.True(actual == line, $"구름 표시 최초 차이 {fields[1]}\n관측 {actual}\n원문 {line}");
            rows++;
        }
        Assert.Equal(944, rows);
    }

    /// <summary>자연 생략은 하위를 요구하지 않으며 실제 순서의 물리 오류는 앞선 효과를 유지한다.</summary>
    [Fact]
    public void NaturalSkipAndPhysicalPrefixes()
    {
        uint speed = 0;
        RawKencloudRender.DrawKencloud(new KencloudRenderGlobals([], () => speed), [0, 0, 65536, 65536, 0, 0], new KencloudRenderHooks(), true);
        uint[] cache = new uint[516];
        cache[515] = 1;
        RawKencloudRender.UpdateKencloud(new KencloudRenderGlobals(cache, () => speed), new KencloudRenderHooks());
        uint[] shortState = new uint[2052];
        Assert.Throws<ArgumentOutOfRangeException>(() => RawKencloudRender.MarkKencloudDirty(shortState));
        Assert.Equal(1U, shortState[2051]);
        var device = new Device();
        device.State[515] = 7;
        Assert.Equal(1U, RawKencloudRender.KencloudRowDirty(device.State, 0xffffffffU, 0)); // 행 -1은 같은 cache DWORD의 인접 alias다.
        device.State[515] = 0;
        Assert.Equal(0U, RawKencloudRender.KencloudRowDirty(device.State, 0xffffffffU, 0));
    }
}
