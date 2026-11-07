// 파생 부착의 진단·선택·슬롯 효과를 실제 기계어 기대값으로 대조한다.
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// 프로세스 부착을 검증하는 테스트 클래스다. 2026-11-07 F2d 프로세스 부착 이식에서 추가됐다.
public sealed class RawProcessAttachTests
{
    /// <summary>구현 전에 고정한 양판본의 진단·공통 자식 인자와 전체 raw 슬롯을 대조한다. 모든 입력에서 서버 0이며 writer·패킷은 실행하지 않는다. 2026-11-07 F2d 프로세스 부착 이식에서 추가됐다.</summary>
    [Fact]
    public void AttachReplaysNativeNonServerInputs()
    {
        string[][] rows = X86Fixture.ReadFile("process-attach-current-local-x86.tsv");
        int count = 0;
        // 모든 입력 행을 같은 풀·훅으로 실행하는 반복문이다. 2026-11-07 F2d 프로세스 부착 이식에서 추가됐다.
        foreach (string[] row in rows)
        {
            Assert.Equal(4, row.Length);
            bool patch = row[0] == "originals";
            Assert.True(patch || row[0] == "originalCD");
            string[] args = row[1].Split(',');
            Assert.Equal(3, args.Length);
            uint stored = uint.Parse(args[0], CultureInfo.InvariantCulture);
            uint flags = uint.Parse(args[1], CultureInfo.InvariantCulture);
            int profile = int.Parse(args[2], CultureInfo.InvariantCulture);
            var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity, true, null, patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
            Span<byte> raw = pool.RawSlot(5);
            // Native와 같은 입력 payload만 생성하는 반복문이다. 2026-11-07 F2d 프로세스 부착 이식에서 추가됐다.
            for (int index = 0; index < raw.Length; ++index)
            {
                raw[index] = unchecked((byte)(index * 29 + 7));
            }

            Put(raw, 16, stored);
            string events = string.Empty;
            void Event(string value)
            {
                if (events.Length > 0)
                {
                    events += ";";
                }

                events += value;
            }

            // 실제 선택된 순서만 직렬화하는 훅이다. 2026-11-07 F2d 프로세스 부착 이식에서 추가됐다.
            var hooks = new ProcessAttachHooks
            {
                Server = () => false,
                Lookup = _ => throw new InvalidOperationException("불러선 안 된다"),
                WriteData = _ => throw new InvalidOperationException("불러선 안 된다"),
                Transmit = _ => throw new InvalidOperationException("불러선 안 된다"),
                Assertion = (expression, file, line) =>
                {
                    Event("D" + expression + "|" + file + "|" + line.ToString(CultureInfo.InvariantCulture));
                    if (profile != 0)
                    {
                        Put(pool.RawSlot(5), 16, 11u);
                    }
                },
                Attach = (form, parent, value) =>
                {
                    Assert.Equal(5, form);
                    Event("A" + parent.ToString(CultureInfo.InvariantCulture) + "," + value.ToString(CultureInfo.InvariantCulture) + "," + Get(pool.RawSlot(5), 16).ToString(CultureInfo.InvariantCulture));
                },
            };
            RawProcessAttach.Attach(pool, 5, 7, flags, hooks);
            Assert.Equal(row[2], events.Length == 0 ? "-" : events);
            // C# 풀 50바이트 전용이라 CD 36바이트 대조는 앞부분으로 자른다(F2c-2 선례)
            string actualHex = BitConverter.ToString(pool.Slot(5).ToArray()).Replace("-", string.Empty).ToLowerInvariant();
            Assert.Equal(row[3], actualHex.Substring(0, row[3].Length));
            ++count;
        }

        Assert.Equal(48, count);
    }

    /// <summary>선택된 실제 장치가 없으면 명시 실패한다. 2026-11-07 F2d 프로세스 부착 이식에서 추가됐다.</summary>
    [Fact]
    public void MissingDeviceThrows()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        var hooks = new ProcessAttachHooks { Server = () => false };
        Assert.Throws<InvalidOperationException>(() => RawProcessAttach.Attach(pool, 5, 7, 0u, hooks));
        Assert.Throws<ArgumentNullException>(() => RawProcessAttach.Attach(null!, 5, 7, 0u, hooks));
    }

    /// <summary>리틀 엔디언 WORD를 읽고 쓰는 정적 보조 함수다. 2026-11-07 F2d 프로세스 부착 이식에서 추가됐다.</summary>
    /// <param name="raw">읽고 쓸 슬롯 바이트다.</param>
    /// <param name="offset">위치다.</param>
    private static ushort Get(Span<byte> raw, int offset) => (ushort)(raw[offset] | (raw[offset + 1] << 8));

    /// <summary>리틀 엔디언 WORD를 쓰는 정적 보조 함수다. 2026-11-07 F2d 프로세스 부착 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 슬롯 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, uint value)
    {
        raw[offset] = unchecked((byte)value);
        raw[offset + 1] = unchecked((byte)(value >> 8));
    }
}
