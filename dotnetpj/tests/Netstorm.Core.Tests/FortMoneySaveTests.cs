using System.Globalization;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// Fort Money 저장(<see cref="RawFortMoneySave"/>)을 독립 원본 입력과 대조한다.
/// cpppj <c>FortMoneySaveTests</c>의 전수 1105행과 경계 직접 2종을 옮겼다. (F2c-4, 2026-11-03)
/// </summary>
public sealed class FortMoneySaveTests
{
    /// <summary>독립 PE 두 stream 토큰이다.</summary>
    private static readonly uint[] MoneyStreams = [0x14000100u, 0x14000120u];

    /// <summary>독립 PE 두 물리 64바이트 버퍼 토큰이다.</summary>
    private static readonly uint[] MoneyBuffers = [0x14001000u, 0x14001100u];

    /// <summary>마지막 빈 칸도 보존해 독립 입력과 실측 열을 정밀히 나눈다.</summary>
    /// <param name="source">분리할 원문이다.</param>
    /// <param name="separator">구분자다.</param>
    private static string[] MoneyFields(string source, char separator)
    {
        List<string> result = [];
        int start = 0;
        // 다음 구분자의 위치를 찾아 원문 그대로 복사한다
        for (; ; )
        {
            int at = source.IndexOf(separator, start);
            result.Add(at < 0 ? source[start..] : source.Substring(start, at - start));
            if (at < 0)
            {
                return [.. result];
            }
            start = at + 1;
        }
    }

    /// <summary>현재 raw DWORD를 little endian으로 읽는다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    private static uint MoneyRead(ReadOnlySpan<byte> bytes, int at = 0)
    {
        uint value = 0;
        // 지정한 네 바이트만 합친다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)bytes[at + index] << (index * 8);
        }
        return value;
    }

    /// <summary>명시 효과가 선택한 현재 stream 필드만 바꾼다.</summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void MoneyPut(Span<byte> bytes, int at, uint value)
    {
        // 직접 필드만 보존한다
        for (int index = 0; index < 4; index++)
        {
            bytes[at + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>실제 바이트 열을 독립 fixture와 같은 소문자 hex로 관측한다.</summary>
    /// <param name="bytes">표시할 바이트다.</param>
    private static string MoneyHex(ReadOnlySpan<byte> bytes)
    {
        const string digits = "0123456789abcdef";
        StringBuilder result = new();
        // 출력 형식만 정하며 기대값을 만들지 않는다
        foreach (byte value in bytes)
        {
            result.Append(digits[value >> 4]);
            result.Append(digits[value & 15]);
        }
        return result.ToString();
    }

    /// <summary>원본 명시 중단과 실제 실패를 구별한다.</summary>
    internal sealed class MoneyStop : Exception;

    /// <summary>명시 Create·진단과 같은 실제 일반 stream writer만 공급하며 Money 본체는 제외하지 않는다.</summary>
    private sealed class MoneyDevices
    {
        /// <summary>공유 난수다. 부분 중단·진단·공유 난수·실행 결과를 모두 기록한다.</summary>
        public NetstormRandom Random = new(0);

        /// <summary>고정 this 슬롯이다.</summary>
        public int Slot;

        /// <summary>현재 manager다.</summary>
        public uint Manager;

        /// <summary>명시 Create 반환 번호다.</summary>
        public uint Selected;

        /// <summary>실제 writer가 빌려주는 두 현재 stream 기록이다.</summary>
        public byte[][] Fields = [new byte[16], new byte[16]];

        /// <summary>두 물리 출력과 주변 64바이트다.</summary>
        public byte[][] Buffers = [new byte[64], new byte[64]];

        /// <summary>독립 입력 효과와 실제 호출 순서다.</summary>
        public Dictionary<string, int> Calls = [];

        /// <summary>독립 입력 효과 목록이다.</summary>
        public List<string[]> Effects = [];

        /// <summary>실제 호출 trace다.</summary>
        public string Trace = "";

        /// <summary>실제 장치를 먼저 기록하고 선택된 외부 효과를 입력 순서대로 적용한다.</summary>
        /// <param name="kind">기록 종류다.</param>
        /// <param name="text">기록 본문이다.</param>
        public void Boundary(string kind, string text)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }
            Trace += text;
            int count = Calls.GetValueOrDefault(kind) + 1;
            Calls[kind] = count;
            string full = kind + count.ToString(CultureInfo.InvariantCulture);
            // 같은 경계 여러 효과를 입력 순서대로 적용한다
            foreach (string[] effect in Effects)
            {
                // 같은 경계 이름만 골라 적용한다
                if (effect[0] != full)
                {
                    continue;
                }
                uint value = uint.Parse(effect[2], CultureInfo.InvariantCulture);
                if (effect[1] == "stop")
                {
                    Assert.Equal(1u, value);
                    throw new MoneyStop();
                }
                else if (effect[1] == "random")
                {
                    Random.SetState(value);
                }
                else if (effect[1] == "manager")
                {
                    Manager = value;
                }
                else
                {
                    string[] target = MoneyFields(effect[1], '.');
                    int index = target[0] == "a" ? 0 : 1;
                    int offset = target[1] == "data" ? 0 : target[1] == "pos" ? 4 : target[1] == "end" ? 8 : 12;
                    MoneyPut(Fields[index], offset, value);
                }
            }
        }

        /// <summary>열 1개만 입력으로 받고 나머지는 원본 결과를 장치 분기에 사용한다.</summary>
        /// <param name="row">입력 행이다.</param>
        public void Initialize(string[] row)
        {
            Random.SetState(uint.Parse(row[2], CultureInfo.InvariantCulture));
            Slot = int.Parse(row[3], CultureInfo.InvariantCulture);
            Manager = uint.Parse(row[5], CultureInfo.InvariantCulture);
            Selected = uint.Parse(row[6], CultureInfo.InvariantCulture);
            for (int index = 0; index < Fields.Length; index++)
            {
                // 실제 stream과 물리 출력 버퍼를 독립 입력과 같게 초기화한다
                MoneyPut(Fields[index], 0, MoneyBuffers[index]);
                MoneyPut(Fields[index], 4, uint.Parse(row[7], CultureInfo.InvariantCulture));
                MoneyPut(Fields[index], 8, uint.Parse(row[8], CultureInfo.InvariantCulture));
                MoneyPut(Fields[index], 12, uint.Parse(row[9], CultureInfo.InvariantCulture));
                for (int address = 0; address < Buffers[index].Length; address++)
                {
                    Buffers[index][address] = (byte)address;
                }
            }
            if (row[10] != "-")
            {
                foreach (string effect in MoneyFields(row[10], ';'))
                {
                    Effects.Add(MoneyFields(effect, '/'));
                }
            }
        }

        /// <summary>부모 manager·Create와 실제 일반 stream writer를 각각 실제 호출 순서에 공급한다.</summary>
        public FortMoneySaveHooks Hooks()
        {
            return new FortMoneySaveHooks
            {
                Manager = selectedSlot =>
                {
                    Assert.Equal(Slot, selectedSlot);
                    uint captured = Manager;
                    Boundary("manager", "manager:" + selectedSlot + ':' + captured);
                    return captured;
                },
                Create = (captured, name) =>
                {
                    Assert.Equal("Money", name);
                    uint stream = MoneyStreams[(int)Selected];
                    Boundary("create", "create:" + captured + ":Money:" + stream + ':' + Random.State);
                    return stream;
                },
                Write = (stream, source) =>
                {
                    Assert.Equal(4, source.Length);
                    Boundary("write", "write:" + stream + ':' + MoneyRead(source) + ':' + Random.State);
                    int index = stream == MoneyStreams[0] ? 0 : 1;
                    Assert.Equal(MoneyStreams[index], stream);
                    MemStreamHooks memory = new()
                    {
                        Buffer = token =>
                        {
                            uint buffer = token == MoneyBuffers[0] ? 0u : 1u;
                            Assert.Equal(MoneyBuffers[buffer], token);
                            return Buffers[buffer];
                        },
                        Assertion = kind =>
                        {
                            Assert.Equal(MemStreamAssertion.WriteCapacity, kind);
                            Boundary("assert", "assert:capacity:" + Random.State);
                        },
                    };
                    RawMemStream.Write(Fields[index], source, 4, memory);
                },
            };
        }
    }

    /// <summary>전달된 경계 누락이 원본 선행 효과를 보존한 채 명시 실패하는지 확인한다.</summary>
    /// <param name="function">실행할 동작이다.</param>
    private static bool MoneyRejects(Action function)
    {
        try
        {
            function();
        }
        catch (InvalidOperationException)
        {
            return true;
        }
        return false;
    }

    /// <summary>실제 1010개 float 비트·공유 난수·부모 현재 manager·일반 쓰기·부분 중단 105개를 대조한다.</summary>
    [Fact]
    public void IndependentNativeFloatAndStream()
    {
        string[][] rows = X86Fixture.Read("fort-money-save");
        int compared = 0;
        // 독립 입력과 실제 PE 관측열을 구분해 실행 전후로 나눈다
        foreach (string[] row in rows)
        {
            Assert.Equal(21, row.Length);
            MoneyDevices devices = new();
            devices.Initialize(row);
            FortMoneySaveHooks hooks = devices.Hooks();
            bool completed = true;
            try
            {
                if (row[1] == "money")
                {
                    RawFortMoneySave.SaveFortMoney(MoneyStreams[(int)devices.Selected], devices.Random, hooks.Write!);
                }
                else
                {
                    Assert.Equal("parent", row[1]);
                    RawFortMoneySave.SnapshotFortMoney(devices.Slot, devices.Random, hooks);
                }
            }
            catch (MoneyStop)
            {
                completed = false;
            }
            Assert.Equal(completed, row[11] == "1");
            Assert.Equal(uint.Parse(row[12], CultureInfo.InvariantCulture), devices.Random.State);
            Assert.Equal(uint.Parse(row[13], CultureInfo.InvariantCulture), devices.Manager);
            Assert.Equal(row[14], MoneyHex(devices.Fields[0]));
            Assert.Equal(row[15], MoneyHex(devices.Fields[1]));
            Assert.Equal(row[16], MoneyHex(devices.Buffers[0]));
            Assert.Equal(row[17], MoneyHex(devices.Buffers[1]));
            Assert.Equal(devices.Trace.Length == 0 ? "-" : devices.Trace, row[18]);
            if (completed)
            {
                Assert.Equal(0, int.Parse(row[19], CultureInfo.InvariantCulture));
            }
            else
            {
                Assert.True(int.Parse(row[19], CultureInfo.InvariantCulture) < 0);
            }
            Assert.True(uint.Parse(row[20], CultureInfo.InvariantCulture) > 0);
            compared++;
        }
        Assert.Equal(1105, compared);
    }

    /// <summary>선택 장치 누락이 이전 Create·난수 효과를 되돌리지 않으며 배치 밖 slot도 임의 추진하지 않는다.</summary>
    [Fact]
    public void LazyEffectsAndUnclampedSlot()
    {
        NetstormRandom random = new(1);
        Assert.True(MoneyRejects(() => RawFortMoneySave.SaveFortMoney(77, random, null!)));
        Assert.Equal(0x10006u, random.State);
        random.SetState(1);
        FortMoneySaveHooks hooks = new();
        Assert.True(MoneyRejects(() => RawFortMoneySave.SnapshotFortMoney(-2, random, hooks)));
        Assert.Equal(1u, random.State);
        int managers = 0;
        hooks.Manager = slot =>
        {
            Assert.Equal(-2, slot);
            managers++;
            return 0u;
        };
        Assert.True(MoneyRejects(() => RawFortMoneySave.SnapshotFortMoney(-2, random, hooks)));
        Assert.Equal(1u, random.State);
        Assert.Equal(1, managers);
        int creates = 0;
        hooks.Create = (manager, name) =>
        {
            Assert.Equal(0u, manager);
            Assert.Equal("Money", name);
            creates++;
            random.SetState(1);
            return 77u;
        };
        Assert.True(MoneyRejects(() => RawFortMoneySave.SnapshotFortMoney(-2, random, hooks)));
        Assert.Equal(0x10006u, random.State);
        Assert.Equal(1, creates);
        int writes = 0;
        hooks.Write = (stream, bytes) =>
        {
            Assert.Equal(77u, stream);
            Assert.Equal(4, bytes.Length);
            Assert.Equal(0x43a70000u, MoneyRead(bytes));
            writes++;
        };
        RawFortMoneySave.SnapshotFortMoney(-2, random, hooks);
        Assert.Equal(0x10006u, random.State);
        Assert.Equal(3, managers);
        Assert.Equal(2, creates);
        Assert.Equal(1, writes);
    }

    /// <summary>SP 분산 저장소가 공유하는 같은 난수만 전진하고 SP 초기값·배열 쓰기 함수는 건드리지 않는다.</summary>
    [Fact]
    public void SharedRandomWithoutSpStoreWrites()
    {
        NetstormRandom random = new(1);
        int reseeds = 0;
        RawSpStore store = new(random, () => { reseeds++; return 0u; }, 0x87654321u);
        uint[] pools = store.Pools.ToArray();
        byte[] saved = new byte[4];
        RawFortMoneySave.SaveFortMoney(77, random, (stream, bytes) =>
        {
            Assert.Equal(77u, stream);
            Assert.Equal(saved.Length, bytes.Length);
            bytes.CopyTo(saved);
        });
        Assert.Equal(0x10006u, random.State);
        Assert.Equal(0x43a70000u, MoneyRead(saved));
        Assert.Equal(0, reseeds);
        Assert.False(store.Initialized);
        Assert.True(store.Pools.SequenceEqual(pools));
    }
}
