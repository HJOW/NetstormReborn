// 부착 부모의 호출 순서·부분 효과를 실제 기계어 기대값으로 대조한다.
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// 부착 부모를 검증하는 테스트 클래스다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.
public sealed class RawBaseProcessAttachTests
{
    /// <summary>커널·생성 자식 경계를 포함하는 원문의 최초 호출·필드·슬롯 관측만 기대값으로 읽는다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.</summary>
    [Fact]
    public void AttachReplaysNativeNewLocalInputs()
    {
        string[][] rows = X86Fixture.ReadFile("baseprocess-attach-current-local-x86.tsv");
        int count = 0;
        // 지정 Form·수신·MP 분기는 새로 실행하지 않는 반복문이다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.
        foreach (string[] row in rows)
        {
            Assert.Equal(6, row.Length);
            bool patch = row[0] == "originals";
            Assert.True(patch || row[0] == "originalCD");
            string[] args = row[1].Split(',');
            Assert.Equal(3, args.Length);
            uint state = uint.Parse(args[0], CultureInfo.InvariantCulture);
            uint kind = uint.Parse(args[1], CultureInfo.InvariantCulture);
            int profile = int.Parse(args[2], CultureInfo.InvariantCulture);
            var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity, true, null, patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
            Span<byte> raw = pool.RawSlot(5);
            // Native의 입력 payload만 동일하게 준비하는 반복문이다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.
            for (int index = 0; index < raw.Length; ++index)
            {
                raw[index] = unchecked((byte)(index * 29 + 7));
            }

            Put(raw, 0, 0x10002000u);
            raw[10] = 2;
            Span<byte> parent = pool.RawSlot(7);
            parent[11] = unchecked((byte)state);
            var fields = new BaseProcessAttachFields();
            string events = string.Empty;
            string cause = "returned";
            void Event(string value)
            {
                if (events.Length > 0)
                {
                    events += ";";
                }

                events += value;
            }

            var hooks = new BaseProcessAttachHooks
            {
                TypeEnd = () => patch ? 63u : 62u,
                ProcessFormType = () => 2u,
                Assertion = (expression, file, line) => Event("D" + expression + "|" + file + "|" + line.ToString(CultureInfo.InvariantCulture)),
                RegisterProcess = () =>
                {
                    Event("K");
                    return 7u;
                },
                Create = flags =>
                {
                    Event("C2," + flags.ToString(CultureInfo.InvariantCulture));
                    if (profile != 0)
                    {
                        throw new InvalidOperationException("create");
                    }

                    return 5;
                },
                Attach = (selected, target, flags) =>
                {
                    Assert.Equal(5, selected);
                    Event("A" + target.ToString(CultureInfo.InvariantCulture) + "," + flags.ToString(CultureInfo.InvariantCulture));
                },
                OnAttached = () => Event("N"),
            };
            try
            {
                RawBaseProcessAttach.Attach(pool, fields, kind, 7, 0, 0x50u, hooks);
            }
            catch (Exception error)
            {
                cause = error.Message;
            }

            Assert.Equal(row[2], cause);
            Assert.Equal(row[3], events.Length == 0 ? "-" : events);
            Assert.Equal(row[4], fields.Form.ToString(CultureInfo.InvariantCulture) + "," + fields.Parent.ToString(CultureInfo.InvariantCulture));
            // C# 풀은 10.78 스트라이드(50바이트) 전용이라 CD 장면도 50바이트 풀로 돌리고, CD raw 대조는 앞 36바이트로 자른다(F2c-2 선례)
            string actualHex = BitConverter.ToString(pool.Slot(5).ToArray()).Replace("-", string.Empty).ToLowerInvariant();
            Assert.Equal(row[5], actualHex.Substring(0, row[5].Length));
            ++count;
        }

        Assert.Equal(48, count);
    }

    /// <summary>선택된 실제 장치가 없으면 명시 실패한다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.</summary>
    [Fact]
    public void MissingDeviceThrows()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity, true, null, OriginalEdition.Patch1078);
        var fields = new BaseProcessAttachFields();
        Assert.Throws<InvalidOperationException>(() => RawBaseProcessAttach.Attach(pool, fields, 46u, 7, 0, 0x50u, new BaseProcessAttachHooks()));
        Assert.Throws<ArgumentNullException>(() => RawBaseProcessAttach.Attach(null!, fields, 46u, 7, 0, 0x50u, new BaseProcessAttachHooks()));
    }

    /// <summary>패치 비활성 부모는 필드를 쓴 뒤 거짓으로 돌아간다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.</summary>
    [Fact]
    public void PatchInactiveParentReturnsFalse()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity, true, null, OriginalEdition.Patch1078);
        pool.RawSlot(7)[11] = 3;
        var fields = new BaseProcessAttachFields();
        var hooks = new BaseProcessAttachHooks
        {
            TypeEnd = () => 63u,
            ProcessFormType = () => 2u,
            Assertion = (_, _, _) => throw new InvalidOperationException("불러선 안 된다"),
        };
        Assert.False(RawBaseProcessAttach.Attach(pool, fields, 46u, 7, 0, 0x50u, hooks));
        Assert.Equal(7, fields.Parent);
        Assert.Equal(0, fields.Form);
    }

    /// <summary>리틀 엔디언 DWORD를 낮은 바이트부터 쓰는 정적 보조 함수다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 슬롯 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, uint value)
    {
        // 낮은 바이트부터 지정 폭만 쓰는 반복문이다. 2026-11-07 F2d 부착 부모 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            raw[offset + index] = unchecked((byte)(value >> (index * 8)));
        }
    }
}
