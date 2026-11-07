// 정렬 부착의 링크·Pop·통지·진단을 실제 기계어 기대값으로 대조한다.
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// Content 부착을 검증하는 테스트 클래스다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.
public sealed class RawContentAttachTests
{
    /// <summary>명시 가상 표의 토큰 시작값이다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.</summary>
    private const uint ContentTable = 0x14000000u;

    /// <summary>원문 41행의 정렬·Pop·통지 실패·진단 정상복귀·저장 가상 표·현재 getter를 전체 바이트와 대조한다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.</summary>
    [Fact]
    public void AttachReplaysNativeFrontAndPartialEffects()
    {
        string[][] rows = X86Fixture.ReadFile("content-attach-current-x86.tsv");
        int cases = 0;
        // 제품 구현 전에 고정한 출력만 기대값으로 읽는 반복문이다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.
        foreach (string[] row in rows)
        {
            Assert.Equal(6, row.Length);
            int layout = int.Parse(row[0], CultureInfo.InvariantCulture);
            int state = int.Parse(row[1], CultureInfo.InvariantCulture);
            int profile = int.Parse(row[2], CultureInfo.InvariantCulture);
            var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
            // Native의 초기 입력 범위를 두는 반복문이다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.
            for (int sid = 0; sid < 16; ++sid)
            {
                Span<byte> fill = pool.RawSlot(sid);
                fill.Fill(0);
            }

            uint index = 0;
            // payload까지 서로 같은 입력을 두고 각 가상 표만 구분하는 반복문이다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.
            foreach (int sid in new[] { 5, 7, 9, 11 })
            {
                Span<byte> setup = pool.RawSlot(sid);
                setup.Fill(0x71);
                // 같은 WORD 입력을 두는 반복문이다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.
                foreach (int at in new[] { 4, 6, 14, 16, 20 })
                {
                    PutWord(setup, at, 0);
                }

                PutDword(setup, 0, ContentTable + index++ * 0x100u);
                setup[10] = 6;
                setup[11] = 4;
                PutWord(setup, 18, 85);
                setup[23] = 0;
            }

            Span<byte> own = pool.RawSlot(5);
            Span<byte> first = pool.RawSlot(7);
            Span<byte> parent = pool.RawSlot(9);
            Span<byte> last = pool.RawSlot(11);
            own[11] = unchecked((byte)state);
            if (layout != 0)
            {
                PutWord(parent, 6, 7);
                PutWord(first, 4, 11);
                PutWord(last, 14, 7);
                first[23] = layout == 1 ? (byte)2 : (byte)0;
                last[23] = layout == 1 ? (byte)3 : (byte)1;
                if (layout == 3)
                {
                    first[10] = 1;
                    last[23] = 2;
                }
            }

            if (profile >= 3 && profile <= 5)
            {
                parent[11] = 6;
                own[11] = 6;
            }

            uint content = 6u;
            string trace = string.Empty;
            string cause = "returned";
            var hooks = new ContentAttachHooks
            {
                ContentType = () => content,
                Assertion = (expression, file, line) =>
                {
                    AppendTrace(ref trace, "D" + line.ToString(CultureInfo.InvariantCulture));
                    if (profile == 4 || profile == 5)
                    {
                        pool.RawSlot(line == 0xbe6u ? 9 : 5)[11] = 4;
                        if (profile == 5)
                        {
                            content = 1u;
                            pool.RawSlot(7)[10] = 1;
                            pool.RawSlot(11)[10] = 1;
                        }
                    }
                },
                Previous = (sid, table) =>
                {
                    AppendTrace(ref trace, "G" + sid.ToString(CultureInfo.InvariantCulture) + "," + table.ToString(CultureInfo.InvariantCulture));
                    if (profile == 7)
                    {
                        throw new InvalidOperationException("previous");
                    }

                    if (profile == 6)
                    {
                        PutDword(pool.RawSlot(5), 0, ContentTable + 0x400u);
                    }

                    return GetWord(pool.RawSlot(sid), 14);
                },
                SetPrevious = (sid, table, value) =>
                {
                    AppendTrace(ref trace, "R" + sid.ToString(CultureInfo.InvariantCulture) + "," + table.ToString(CultureInfo.InvariantCulture) + "," + value.ToString(CultureInfo.InvariantCulture));
                    PutWord(pool.RawSlot(sid), 14, value);
                },
                SetParent = (sid, table, value) =>
                {
                    AppendTrace(ref trace, "P" + sid.ToString(CultureInfo.InvariantCulture) + "," + table.ToString(CultureInfo.InvariantCulture) + "," + value.ToString(CultureInfo.InvariantCulture));
                    PutWord(pool.RawSlot(sid), 16, value);
                },
                Pop = (sid, flags) =>
                {
                    Span<byte> current = pool.RawSlot(5);
                    AppendTrace(ref trace, "O" + flags.ToString(CultureInfo.InvariantCulture));
                    if (profile == 1)
                    {
                        throw new InvalidOperationException("pop");
                    }

                    current[11] &= unchecked((byte)~4u);
                },
                Notice = (sid, child, reason) =>
                {
                    AppendTrace(ref trace, "N" + sid.ToString(CultureInfo.InvariantCulture) + "," + child.ToString(CultureInfo.InvariantCulture) + "," + reason.ToString(CultureInfo.InvariantCulture));
                    if (profile == 2)
                    {
                        throw new InvalidOperationException("notice");
                    }
                },
            };
            try
            {
                if (RawContentAttach.Attach(pool, 5, 9, 0x10u, hooks))
                {
                    cause = "attached";
                }
            }
            catch (Exception error)
            {
                cause = error.Message;
            }

            string bytes = string.Empty;
            // 미수정 슬롯·이웃 payload 보존까지 잇는 반복문이다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.
            foreach (int sid in new[] { 0, 5, 7, 9, 11 })
            {
                if (bytes.Length > 0)
                {
                    bytes += ";";
                }

                bytes += BitConverter.ToString(pool.Slot(sid).ToArray()).Replace("-", string.Empty).ToLowerInvariant();
            }

            Assert.Equal(row[3], cause);
            Assert.Equal(row[4], trace.Length == 0 ? "-" : trace);
            Assert.Equal(row[5], bytes);
            ++cases;
        }

        Assert.Equal(41, cases);
    }

    /// <summary>패치가 아니면 명시 실패한다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.</summary>
    [Fact]
    public void RejectsNonPatchEdition()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity, true, null, OriginalEdition.Cd1072);
        Assert.Throws<ArgumentException>(() => RawContentAttach.Attach(pool, 5, 9, 0x10u, new ContentAttachHooks()));
        Assert.Throws<ArgumentNullException>(() => RawContentAttach.Attach(null!, 5, 9, 0x10u, new ContentAttachHooks()));
    }

    /// <summary>선택된 경계 순서만 기록하는 정적 보조 함수다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.</summary>
    /// <param name="trace">이어 붙일 추적 문자열이다.</param>
    /// <param name="item">추가할 사건이다.</param>
    private static void AppendTrace(ref string trace, string item)
    {
        if (trace.Length > 0)
        {
            trace += ";";
        }

        trace += item;
    }

    /// <summary>WORD를 읽는 정적 보조 함수다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.</summary>
    /// <param name="raw">슬롯 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    private static ushort GetWord(ReadOnlySpan<byte> raw, int at) => (ushort)(raw[at] | (raw[at + 1] << 8));

    /// <summary>WORD를 읽는 정적 보조 함수다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.</summary>
    /// <param name="raw">슬롯 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    private static ushort GetWord(Span<byte> raw, int at) => (ushort)(raw[at] | (raw[at + 1] << 8));

    /// <summary>WORD를 쓰는 정적 보조 함수다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 슬롯 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void PutWord(Span<byte> raw, int at, int value)
    {
        raw[at] = unchecked((byte)value);
        raw[at + 1] = unchecked((byte)(value >> 8));
    }

    /// <summary>DWORD를 쓰는 정적 보조 함수다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 슬롯 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void PutDword(Span<byte> raw, int at, uint value)
    {
        // 낮은 바이트부터 지정 폭만 쓰는 반복문이다. 2026-11-07 F2d Content 부착 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            raw[at + index] = unchecked((byte)(value >> (index * 8)));
        }
    }
}
