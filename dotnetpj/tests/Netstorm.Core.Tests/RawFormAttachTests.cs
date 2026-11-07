// 공통 부착의 최초 관측을 실제 기계어 기대값으로 대조한다.
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// Form 부착을 검증하는 테스트 클래스다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.
public sealed class RawFormAttachTests
{
    /// <summary>원문 관측기의 가상 setter 토큰 시작값이다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.</summary>
    private const uint AttachVtable = 0x14000000u;

    /// <summary>현재 가상 장치·저장 head·부모·진단 복귀와 네 raw 슬롯 전체를 원문에 대조한다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.</summary>
    [Fact]
    public void AttachReplaysNativeParentFixture()
    {
        string[][] rows = X86Fixture.ReadFile("form-attach-current-x86.tsv");
        int patchRows = 0;
        int cdRows = 0;
        // 모든 최초 관측 입력을 독립 풀에서 실행하는 반복문이다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.
        foreach (string[] row in rows)
        {
            Assert.Equal(5, row.Length);
            bool patch = row[0] == "originals";
            Assert.True(patch || row[0] == "originalCD");
            if (patch)
            {
                ++patchRows;
            }
            else
            {
                ++cdRows;
            }

            string[] args = row[1].Split(',');
            Assert.Equal(6, args.Length);
            int parent = int.Parse(args[0], CultureInfo.InvariantCulture);
            byte number = byte.Parse(args[1], CultureInfo.InvariantCulture);
            uint genus = uint.Parse(args[2], CultureInfo.InvariantCulture);
            int head = int.Parse(args[3], CultureInfo.InvariantCulture);
            int profile = int.Parse(args[4], CultureInfo.InvariantCulture);
            uint flags = uint.Parse(args[5], CultureInfo.InvariantCulture);
            var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity, true, null, patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
            // 합성 payload만 준비하고 결과는 계산하지 않는 반복문이다. 원문 슬롯 크기(패치 50·CD 36)로 자릿수를 맞춘다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.
            int stride = patch ? 50 : 36;
            for (uint sid = 0; sid < 16; ++sid)
            {
                Span<byte> setup = pool.RawSlot((int)sid);
                for (int offset = 0; offset < setup.Length; ++offset)
                {
                    setup[offset] = unchecked((byte)(((sid * (uint)stride + (uint)offset) * 37u + 19u) % 256u));
                }
            }

            // 명시 세 가상 장치의 초기 토큰을 두는 반복문이다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.
            for (int index = 0; index < 3; ++index)
            {
                PutDword(pool.RawSlot(5 + index * 2), 0, AttachVtable + (uint)index * 0x100u);
            }

            pool.RawSlot(5)[10] = number;
            pool.RawSlot(5)[11] = 4;
            PutWord(pool.RawSlot(parent), 6, head);
            uint content = 6u;
            string events = string.Empty;
            var hooks = new FormAttachHooks
            {
                Genus = current =>
                {
                    AppendEvent(ref events, "G" + current.ToString(CultureInfo.InvariantCulture));
                    return genus;
                },
                ContentType = () => content,
                Assertion = (expression, file, line) =>
                {
                    AppendEvent(ref events, "D" + expression + "|" + file + "|" + line.ToString(CultureInfo.InvariantCulture));
                    if (profile == 1)
                    {
                        Span<byte> raw = pool.RawSlot(5);
                        PutDword(raw, 0, AttachVtable + 0x100u);
                        raw[10] = 69;
                    }

                    if (profile == 3 && expression == "n != getSid()")
                    {
                        PutWord(pool.RawSlot(parent), 6, 7);
                        PutWord(pool.RawSlot(5), 16, 11);
                    }

                    if (profile == 5 && expression == "getType()!=contentFormType")
                    {
                        pool.RawSlot(5)[10] = 71;
                    }
                },
                SetParent = (sid, table, value) =>
                {
                    AppendEvent(ref events, "P" + sid.ToString(CultureInfo.InvariantCulture) + "," + table.ToString(CultureInfo.InvariantCulture) + "," + value.ToString(CultureInfo.InvariantCulture));
                    PutWord(pool.RawSlot(sid), 16, value);
                    if (profile == 2)
                    {
                        Span<byte> raw = pool.RawSlot(5);
                        PutDword(raw, 0, AttachVtable + 0x200u);
                        PutWord(pool.RawSlot(parent), 6, 7);
                        PutWord(raw, 16, 11);
                    }
                },
                SetPrevious = (sid, table, value) =>
                {
                    AppendEvent(ref events, "R" + sid.ToString(CultureInfo.InvariantCulture) + "," + table.ToString(CultureInfo.InvariantCulture) + "," + value.ToString(CultureInfo.InvariantCulture));
                    PutWord(pool.RawSlot(sid), 14, value);
                    if (profile == 4 && value == 5)
                    {
                        Span<byte> raw = pool.RawSlot(5);
                        PutDword(raw, 0, AttachVtable + 0x100u);
                        PutWord(raw, 16, 11);
                        PutWord(pool.RawSlot(parent), 6, 13);
                    }
                },
                Pop = (sid, value) =>
                {
                    Span<byte> raw = pool.RawSlot(5);
                    AppendEvent(ref events, "O" + value.ToString(CultureInfo.InvariantCulture) + "," + raw[11].ToString(CultureInfo.InvariantCulture) + "," + GetWord(raw, 4).ToString(CultureInfo.InvariantCulture) + "," + GetWord(raw, 14).ToString(CultureInfo.InvariantCulture) + "," + GetWord(raw, 16).ToString(CultureInfo.InvariantCulture) + "," + GetWord(pool.RawSlot(parent), 6).ToString(CultureInfo.InvariantCulture));
                    raw[11] &= unchecked((byte)~4u);
                    if (profile == 5)
                    {
                        raw[10] = 70;
                        content = 70u;
                    }
                },
            };
            RawFormAttach.Attach(pool, 5, parent, flags, hooks);
            Assert.Equal(row[2], events);
            Assert.Equal(content.ToString(CultureInfo.InvariantCulture), row[3]);
            string[] expectedSlots = row[4].Split(';');
            Assert.Equal(4, expectedSlots.Length);
            // C# 풀 50바이트 전용이라 CD 슬롯 대조는 앞부분으로 자른다(F2c-2 선례)
            string HexSlot(int sid)
            {
                string hex = BitConverter.ToString(pool.Slot(sid).ToArray()).Replace("-", string.Empty).ToLowerInvariant();
                return hex.Substring(0, expectedSlots[0].Length);
            }

            Assert.Equal(expectedSlots[0], HexSlot(0));
            Assert.Equal(expectedSlots[1], HexSlot(5));
            Assert.Equal(expectedSlots[2], HexSlot(7));
            Assert.Equal(expectedSlots[3], HexSlot(9));
        }

        Assert.Equal(432, patchRows);
        Assert.Equal(432, cdRows);
    }

    /// <summary>부모 0은 패치에서 조용히 돌아가고 CD에서 진단한다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.</summary>
    [Fact]
    public void ZeroParentReturnsOrDiagnoses()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        int calls = 0;
        var hooks = new FormAttachHooks
        {
            Genus = _ => 0u,
            Assertion = (_, _, _) => ++calls,
        };
        RawFormAttach.Attach(pool, 5, 0, 0u, hooks);
        Assert.Equal(0, calls);
        var cdPool = new RawSidPool(RawSurfaceStorage.PoolCapacity, true, null, OriginalEdition.Cd1072);
        Assert.Throws<InvalidOperationException>(() => RawFormAttach.Attach(cdPool, 5, 0, 0u, new FormAttachHooks { Genus = _ => 0u }));
    }

    /// <summary>실제 호출 순서만 직렬화하는 정적 보조 함수다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.</summary>
    /// <param name="events">이어 붙일 사건 문자열이다.</param>
    /// <param name="item">추가할 사건이다.</param>
    private static void AppendEvent(ref string events, string item)
    {
        if (events.Length > 0)
        {
            events += ";";
        }

        events += item;
    }

    /// <summary>WORD를 읽는 정적 보조 함수다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.</summary>
    /// <param name="raw">슬롯 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    private static ushort GetWord(ReadOnlySpan<byte> raw, int offset) => (ushort)(raw[offset] | (raw[offset + 1] << 8));

    /// <summary>WORD를 읽는 정적 보조 함수다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.</summary>
    /// <param name="raw">슬롯 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    private static ushort GetWord(Span<byte> raw, int offset) => (ushort)(raw[offset] | (raw[offset + 1] << 8));

    /// <summary>WORD를 쓰는 정적 보조 함수다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 슬롯 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void PutWord(Span<byte> raw, int offset, int value)
    {
        raw[offset] = unchecked((byte)value);
        raw[offset + 1] = unchecked((byte)(value >> 8));
    }

    /// <summary>DWORD를 쓰는 정적 보조 함수다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 슬롯 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void PutDword(Span<byte> raw, int offset, uint value)
    {
        // 낮은 바이트부터 지정 폭만 쓰는 반복문이다. 2026-11-07 F2d Form 부착 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            raw[offset + index] = unchecked((byte)(value >> (index * 8)));
        }
    }
}
