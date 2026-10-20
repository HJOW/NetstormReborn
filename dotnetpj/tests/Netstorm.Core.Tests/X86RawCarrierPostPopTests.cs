using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 운반체 후처리(<see cref="RawCarrierPostPop"/>)를 원본 x86 기대값에 대조한다.
/// 기대값은 carrierpostpop-x86.tsv(운반체 직접 호출·Damageable 직접 호출, 판본별 2,448회)다.
/// 각 호출 뒤 효과 순서와 대상 슬롯 50바이트를 비교한다.
/// 열 구성은 cpppj/tests/CarrierPostPopTests.cpp 와 같다.
/// CD 10.72·10.37 행은 기준 판본이 아니므로 읽지 않는다 (W02 Carrier 조각, 2026-10-20).
/// </summary>
public sealed class X86RawCarrierPostPopTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD·10.37 이다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>판본별 호출 수. 전체 7,344행 중 10.78 몫이다.</summary>
    private const int RowsPerEdition = 2448;

    /// <summary>원본 사제 가상 표(10.78). 슬롯 식별 값으로만 쓰며 호스트 함수로 호출하지 않는다.</summary>
    private const uint PriestVtable = 0x0050F210;

    /// <summary>검사 대상 번호. cpppj RawSceneSupport 의 kSource 와 같다.</summary>
    private const int Source = 50;

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 폭 밖의 이웃 필드는 보존한다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>타입 레코드 188개를 만들고 운반체 관련 세 번호에만 원본 플래그를 채운다.</summary>
    /// <param name="flags1">세 번호에 공통으로 넣을 플래그1.</param>
    private static OriginalTypeRecord[] Types(uint flags1)
    {
        var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
        // 쓰지 않는 번호도 슬롯 검사의 범위 안에 들어야 한다
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord();
        }
        // 원본 관찰이 쓰는 82·122(간헐천)·158(사제) 번호만 플래그를 채운다
        foreach (int number in new[] { 82, 122, 158 })
        {
            types[number].Flags1 = flags1;
            types[number].Flags2 = 0x00210000;
        }
        return types;
    }

    /// <summary>
    /// 운반체·Damageable 직접 호출 fixture 의 10.78 몫 2,448회를 재생한다.
    /// 호출마다 효과 순서와 대상 슬롯 50바이트를 대조한다.
    /// </summary>
    [Fact]
    public void CarrierAndDamageableMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("carrierpostpop");
        Assert.Equal(3 * RowsPerEdition, rows.Length);
        int count = 0;
        var failures = new List<string>();
        // 판본 이름이 originals 인 행만 10.78 경로로 재생한다
        foreach (string[] row in rows)
        {
            if (row[1] != PatchEdition)
            {
                continue;
            }
            try
            {
                Assert.Equal(19, row.Length);
                var pool = new RawSidPool(24000, false);
                // 원본 fixture 의 번호 50 을 클라이언트 영역에 확보한다
                for (int id = 5; id <= Source; id++)
                {
                    Assert.Equal(id, pool.Allocate(SidAllocation.Client));
                }
                var types = Types(X86Fixture.UInt(row[8]));
                var state = new CarrierPostPopState
                {
                    Boss = row[3] != "0",
                    LoadingDepth = X86Fixture.UInt(row[4]),
                    GeyserType = X86Fixture.UInt(row[6]),
                };
                int afterType = X86Fixture.Int(row[13]);
                int afterExtra = X86Fixture.Int(row[14]);
                int afterOwner = X86Fixture.Int(row[15]);
                int groundOwner = X86Fixture.Int(row[16]);
                var events = new List<string>();
                int validations = 0;
                Span<byte> raw = pool.AllocatedBytes(Source);
                raw.Clear();
                Put(raw, 0, PriestVtable);
                raw[RawSidPool.TypeOffset] = (byte)X86Fixture.UInt(row[5]);
                raw[RawSidPool.StateOffset] = (byte)X86Fixture.UInt(row[9]);
                raw[40] = (byte)X86Fixture.UInt(row[7]);
                raw[34] = (byte)X86Fixture.UInt(row[10]);
                Put(raw, 12, 0xab84, 2);
                Put(raw, 14, X86Fixture.UInt(row[11]));
                Put(raw, 18, X86Fixture.UInt(row[12]));
                var hooks = new CarrierPostPopHooks
                {
                    CarrierCheck = sid =>
                    {
                        // 직접 호출의 +0xCC 확인은 공통 검사가 끝난 뒤에 와야 한다
                        Assert.Equal(1, validations);
                        events.Add($"C:{sid}");
                        // 후속 변경이 있으면 현재 raw 의 타입·추가·소유자·좌표를 바꾼다
                        if (afterType >= 0)
                        {
                            Span<byte> target = pool.AllocatedBytes(sid);
                            target[RawSidPool.TypeOffset] = (byte)afterType;
                            target[40] = (byte)afterExtra;
                            target[34] = (byte)afterOwner;
                            Put(target, 14, 0x41f20000);
                            Put(target, 18, 0x41fc0000);
                        }
                    },
                    ClaimGround = (x, y, owner) =>
                    {
                        // 지면 갱신도 공통 검사가 끝난 뒤에 와야 한다
                        Assert.Equal(1, validations);
                        events.Add($"G:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}:{owner}");
                        // 후속 소유자가 있으면 대상 슬롯의 소유자를 바꾼다
                        if (groundOwner >= 0)
                        {
                            pool.AllocatedBytes(Source)[34] = (byte)groundOwner;
                        }
                    },
                    ValidateBase = (sid, flags) =>
                    {
                        Assert.Equal(Source, sid);
                        validations++;
                    },
                    Base = (sid, flags) =>
                    {
                        // 공통 몸체는 검사 뒤에 현재 raw 값으로 호출되어야 한다
                        Assert.Equal(1, validations);
                        ReadOnlySpan<byte> slot = pool.Slot(sid);
                        events.Add($"B:{sid}:{flags}:{slot[RawSidPool.TypeOffset]}:{slot[40]}:{slot[34]}");
                    },
                };
                var carrier = new RawCarrierPostPop(pool, types, state, hooks);
                if (row[0] == "Carrier")
                {
                    carrier.PostPop(Source, X86Fixture.UInt(row[2]));
                }
                else
                {
                    Assert.Equal("Damageable", row[0]);
                    carrier.DamageablePostPop(Source, X86Fixture.UInt(row[2]));
                }
                string actual = string.Join(';', events);
                // 효과 순서와 raw 슬롯 전체를 원본과 비교한다
                if (actual != row[17] || Convert.ToHexStringLower(pool.Slot(Source)) != row[18])
                {
                    failures.Add($"{count}: 기대 {row[17]} / 실제 {actual}");
                }
                count++;
            }
            catch (Exception error)
            {
                failures.Add($"{count}: {error.GetType().Name}: {error.Message}");
                count++;
            }
            if (failures.Count >= 6)
            {
                break;
            }
        }
        Assert.True(failures.Count == 0, $"원본 입력 {count}개 중 {failures.Count}개 불일치\n" + string.Join('\n', failures.Take(6)));
        Assert.Equal(RowsPerEdition, count);
    }
}
