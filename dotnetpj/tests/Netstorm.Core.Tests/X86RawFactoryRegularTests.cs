using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// Factory의 독립 원문656입력과 현재 조회/0범위 예외 선행 난수를 대조한다. 2026-11-19 0.628에서 R4-2 공장 프레임·정기 집계 후속용으로 추가했다.
// 장면은 cpppj/tests/FactoryRegularCurrentTests.cpp 의 FactoryRegularScene 과 같다. 원본656행 전수 재생과 조기 반환·판본 경계 검사를 수행한다.
// 근거: cpppj o/RawFactoryRegular.h/.cpp 와 factory-regular-current-x86.tsv(656행)다.
public sealed class X86RawFactoryRegularTests
{
    /// <summary>실제 풀의 슬롯 수(원본 관찰과 같다). 2026-11-19 0.628에서 추가했다.</summary>
    private const int PoolCapacity = 24000;

    /// <summary>정기 처리 대상 부모 번호다. 2026-11-19 0.628에서 추가했다.</summary>
    private const int Source = 50;

    /// <summary>슬롯 전체를 소문자로 만든다. 2026-11-19 0.628에서 추가했다.</summary>
    /// <param name="span">슬롯 바이트다.</param>
    private static string Hex(ReadOnlySpan<byte> span) => Convert.ToHexStringLower(span.ToArray());

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 2026-11-19 0.628에서 추가했다.</summary>
    /// <param name="bytes">대상 슬롯 바이트다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 낮은 바이트부터 폭 4바이트로 쓴다.
        for (int i = 0; i < 4; ++i)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>한 행의 실제 풀·난수와 명시 입력·선택 장치 관찰을 소유하는 장면. 2026-11-19 0.628에서 추가했다.</summary>
    private sealed class Scene
    {
        /// <summary>원본 관측의 부모50을 포함한 물리 풀이다. 2026-11-19 0.628에서 추가했다.</summary>
        public readonly RawSidPool Pool = new(PoolCapacity, false, null, OriginalEdition.Patch1078);

        /// <summary>원문과 같은 실제 LCG다. 2026-11-19 0.628에서 추가했다.</summary>
        public readonly GameRandom Random = new(1);

        /// <summary>8개 DWORD 명시 입력이다. 2026-11-19 0.628에서 추가했다.</summary>
        public readonly uint[] Row = new uint[8];

        /// <summary>실제로 선택한 외부 경계의 순서다. 2026-11-19 0.628에서 추가했다.</summary>
        public string Trace = string.Empty;

        /// <summary>관찰 번호를 빠짐없이 확보한다. 2026-11-19 0.628에서 추가했다.</summary>
        public Scene()
        {
            // 관찰 번호를 빠짐없이 확보하는 반복문이다.
            for (ushort sid = 5; sid <= 61; ++sid)
            {
                int got = Pool.Allocate(SidAllocation.Client);
                if (got != sid)
                {
                    throw new InvalidOperationException("공장 정기 실제 SID 준비 오류");
                }
            }
        }

        /// <summary>같은 입력 바이트·seed를 재배치한다. 2026-11-19 0.628에서 추가했다.</summary>
        /// <param name="input">쉼표로 나눈 8개 입력이다.</param>
        public void Prepare(string input)
        {
            string[] parts = input.Split(',');
            if (parts.Length != 8)
            {
                throw new ArgumentException("공장 정기 입력 8개 오류", nameof(input));
            }

            // 8개 입력을 순서대로 읽는 반복문이다.
            for (int i = 0; i < 8; ++i)
            {
                Row[i] = uint.Parse(parts[i], CultureInfo.InvariantCulture);
            }

            Span<byte> raw = Pool.AllocatedBytes(Source);
            raw.Fill(0xa5);
            Put(raw, 0, 0x11010000u);
            raw[10] = 100;
            raw[11] = (byte)((Row[3] << 5) | 4);
            Random.SetState(Row[6]);
            Trace = string.Empty;
        }

        /// <summary>원문이 선택한 효과만 서식화한다. 2026-11-19 0.628에서 추가했다.</summary>
        /// <param name="text">추가할 사건 표기다.</param>
        private void Event(string text)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }

            Trace += text;
        }

        /// <summary>큰 업그레이드·표시·진행의 원본 관측과 같은 명시 경계다. 2026-11-19 0.628에서 추가했다.</summary>
        public FactoryRegularHooks Hooks()
        {
            return new FactoryRegularHooks
            {
                Level = parent =>
                {
                    if (parent != Source)
                    {
                        throw new InvalidOperationException("공장 정기 부모 번호 불일치");
                    }

                    int level = Pool.Slot(Source)[11] >> 5;
                    Event("L:" + level.ToString(CultureInfo.InvariantCulture));
                    return level;
                },
                FirstFrame = (parent, level) =>
                {
                    if (parent != Source)
                    {
                        throw new InvalidOperationException("공장 정기 부모 번호 불일치");
                    }

                    byte type = Pool.Slot(Source)[10];
                    Event("T:" + type.ToString(CultureInfo.InvariantCulture));
                    int first = unchecked((int)(Row[4] + (type == 101 ? 100u : 0u) + unchecked((uint)level) * 10u));
                    if (Row[7] != 0)
                    {
                        Span<byte> raw = Pool.AllocatedBytes(Source);
                        raw[10] = 101;
                        raw[11] = (byte)((((Row[3] + 1u) & 7u) << 5) | 4u);
                    }

                    return first;
                },
                FrameCount = (parent, _) =>
                {
                    if (parent != Source)
                    {
                        throw new InvalidOperationException("공장 정기 부모 번호 불일치");
                    }

                    byte type = Pool.Slot(Source)[10];
                    Event("T:" + type.ToString(CultureInfo.InvariantCulture));
                    return unchecked((int)(Row[5] + (type == 101 ? 1u : 0u)));
                },
                Random = limit =>
                {
                    Event("R:" + limit.ToString(CultureInfo.InvariantCulture));
                    if (limit == 0)
                    {
                        _ = Random.Next(1);
                        throw new ArgumentOutOfRangeException(nameof(limit), "원문 난수 0범위");
                    }

                    return Random.Next(limit);
                },
                Frame = (parent, value, flags) =>
                {
                    if (parent != Source)
                    {
                        throw new InvalidOperationException("공장 정기 부모 번호 불일치");
                    }

                    Event("S:" + unchecked((uint)value).ToString(CultureInfo.InvariantCulture) + ":" + flags.ToString(CultureInfo.InvariantCulture));
                },
                Advance = (parent, value, flags) =>
                {
                    if (parent != Source)
                    {
                        throw new InvalidOperationException("공장 정기 부모 번호 불일치");
                    }

                    Event("A:" + value.ToString(CultureInfo.InvariantCulture) + ":" + flags.ToString(CultureInfo.InvariantCulture));
                },
                Begin = (parent, payload) =>
                {
                    if (parent != Source)
                    {
                        throw new InvalidOperationException("공장 정기 부모 번호 불일치");
                    }

                    Event("U:" + BitConverter.SingleToUInt32Bits(payload).ToString(CultureInfo.InvariantCulture));
                },
                Finish = parent =>
                {
                    if (parent != Source)
                    {
                        throw new InvalidOperationException("공장 정기 부모 번호 불일치");
                    }

                    Event("F");
                    return -1.0f;
                },
                Flare = (parent, number, code) => Event("P:" + parent.ToString(CultureInfo.InvariantCulture) + ":" + number.ToString(CultureInfo.InvariantCulture) + ":" + code.ToString(CultureInfo.InvariantCulture)),
                Damageable = (parent, @event, count, payload) =>
                {
                    if (parent != Source)
                    {
                        throw new InvalidOperationException("공장 정기 부모 번호 불일치");
                    }

                    Event("B:" + @event.ToString(CultureInfo.InvariantCulture) + ":" + count.ToString(CultureInfo.InvariantCulture) + ":" + BitConverter.SingleToUInt32Bits(payload).ToString(CultureInfo.InvariantCulture));
                    return 0.375f;
                },
            };
        }

        /// <summary>현재 슬롯 전체를 소문자로 읽는다. 2026-11-19 0.628에서 추가했다.</summary>
        public string RawHex() => Hex(Pool.Slot(Source).ToArray());
    }

    /// <summary>현재 두 조회·실제 난수·예외와 최종 raw를 독립 원문 결과에 대조한다. 2026-11-19 0.628에서 추가했다.</summary>
    [Fact]
    public void ReplaysOriginal656Inputs()
    {
        string[][] rows = X86Fixture.Read("factory-regular-current");
        var scene = new Scene();
        int count = 0;
        int faults = 0;
        int diagnostics = 0;
        // 원래 기대 반환·순서·seed·50BYTE는 수정하지 않는다.
        foreach (string[] fields in rows)
        {
            Assert.Equal(5, fields.Length);
            scene.Prepare(fields[0]);
            string result;
            try
            {
                float value = RawFactoryRegular.Run(scene.Pool, Source, scene.Row[0], scene.Row[1], BitConverter.UInt32BitsToSingle(scene.Row[2]), scene.Hooks());
                result = BitConverter.SingleToUInt32Bits(value).ToString(CultureInfo.InvariantCulture);
            }
            catch (ArgumentOutOfRangeException)
            {
                result = "fault";
                ++faults;
            }

            string trace = scene.Trace.Length == 0 ? "-" : scene.Trace;
            if ((trace != fields[2] || result != fields[1]) && diagnostics < 5)
            {
                ++diagnostics;
            }

            Assert.True(result == fields[1], $"공장 정기 입력 {fields[0]} 반환 불일치: 출력 {result} / 기대 {fields[1]} trace {trace}");
            Assert.True(trace == fields[2], $"공장 정기 입력 {fields[0]} 순서 불일치: 출력 {trace} / 기대 {fields[2]}");
            Assert.True(scene.Random.State() == X86Fixture.UInt(fields[3]), $"공장 정기 입력 {fields[0]} 난수 불일치");
            Assert.True(scene.RawHex() == fields[4], $"공장 정기 입력 {fields[0]} raw 불일치");
            ++count;
        }

        Assert.Equal(656, count);
        Assert.Equal(128, faults);
    }

    /// <summary>조기 반환은 관련 없는 조회·기본·표시 장치를 요구하지 않는다. 2026-11-19 0.628에서 추가했다.</summary>
    [Fact]
    public void LazySelectedEffectsAndEdition()
    {
        var pool = new RawSidPool(PoolCapacity, false, null, OriginalEdition.Patch1078);
        var hooks = new FactoryRegularHooks();
        Assert.Equal(0.0f, RawFactoryRegular.Run(pool, 0, 4, 1, 9.0f, hooks));
        hooks.Finish = _ => 0.125f;
        Assert.Equal(0.125f, RawFactoryRegular.Run(pool, 0, 3, 1, 9.0f, hooks));
        Assert.Throws<InvalidOperationException>(() => RawFactoryRegular.Run(pool, 0, 3, 0, 9.0f, hooks));
        var cd = new RawSidPool(PoolCapacity, false, null, OriginalEdition.Cd1072);
        Assert.Throws<ArgumentException>(() => RawFactoryRegular.Run(cd, 0, 4, 1, 9.0f, hooks));
    }
}
