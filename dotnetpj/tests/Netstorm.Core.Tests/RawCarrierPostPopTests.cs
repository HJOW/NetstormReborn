using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 운반체 후처리(<see cref="RawCarrierPostPop"/>)의 보호 경로와 연결 지점 계약을 확인한다.
/// 원본 기대값 재생(X86RawCarrierPostPopTests)은 직접 호출의 효과 순서·결과를 대조하고,
/// 이 검사들은 cpppj/tests/CarrierPostPopTests.cpp 의 보호 검사만 10.78 몫으로 옮겼다.
/// 사제 접두·프로세스 호스트와 엮는 결합 검사는 사제 후처리(W08)가 없어서 하지 않는다 (W02 Carrier 조각, 2026-10-20).
/// </summary>
public sealed class RawCarrierPostPopTests
{
    /// <summary>원본 사제 가상 표(10.78). 슬롯 식별 값으로만 쓰며 호스트 함수로 호출하지 않는다.</summary>
    private const uint PriestVtable = 0x0050F210;

    /// <summary>원본 사제 타입 번호.</summary>
    private const int PriestType = 158;

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
    private static OriginalTypeRecord[] Types(uint flags1 = 0x00069012)
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

    /// <summary>번호 50 을 클라이언트 영역에 확보하고 기본 사제 슬롯을 채운 풀을 만든다.</summary>
    private static RawSidPool Pool()
    {
        var pool = new RawSidPool(24000, false);
        // 원본 fixture 의 번호 50 을 클라이언트 영역에 확보한다
        for (int id = 5; id <= Source; id++)
        {
            Assert.Equal(id, pool.Allocate(SidAllocation.Client));
        }
        return pool;
    }

    /// <summary>기본 사제 슬롯을 채운다. 타입·상태·추가·소유자·좌표를 지정한다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="type">타입 번호.</param>
    /// <param name="state">상태 바이트.</param>
    /// <param name="extra">추가 비트 바이트.</param>
    /// <param name="owner">소유자 바이트.</param>
    private static void Fill(RawSidPool pool, byte type = PriestType, byte state = 0, byte extra = 0, byte owner = 0)
    {
        Span<byte> raw = pool.AllocatedBytes(Source);
        raw.Clear();
        Put(raw, 0, PriestVtable);
        raw[RawSidPool.TypeOffset] = type;
        raw[RawSidPool.StateOffset] = state;
        raw[40] = extra;
        raw[34] = owner;
        Put(raw, 12, 0xab84, 2);
        Put(raw, 14, 0x42480000);
        Put(raw, 18, 0x42500000);
    }

    /// <summary>효과 순서와 공통 검사 호출 수를 기록하는 hooks 를 만든다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="events">효과 순서 기록.</param>
    /// <param name="validations">공통 검사 호출 수(길이 1).</param>
    private static CarrierPostPopHooks Hooks(RawSidPool pool, List<string> events, int[] validations) => new()
    {
        CarrierCheck = sid => { events.Add($"C:{sid}"); },
        ClaimGround = (x, y, owner) => { events.Add($"G:{owner}"); },
        ValidateBase = (sid, flags) => { validations[0]++; },
        Base = (sid, flags) => { events.Add($"B:{sid}:{flags}"); },
    };

    /// <summary>
    /// 잘못된 raw·타입 누락과 공통 검사의 거부는 풀·효과를 바꾸기 전에 실패한다.
    /// 빈 번호·범위 밖 번호·자산 밖 타입·내용물 상태·공통 검사 거부를 확인한다.
    /// </summary>
    [Fact]
    public void RejectsInvalidInputsBeforeExternalEffects()
    {
        var pool = Pool();
        var types = Types();
        var state = new CarrierPostPopState();
        var events = new List<string>();
        var validations = new int[1];
        Fill(pool);
        // 효과가 하나라도 없으면 생성하지 않는다
        Assert.ThrowsAny<Exception>(() => new RawCarrierPostPop(pool, types, state, new CarrierPostPopHooks()));
        Assert.ThrowsAny<Exception>(() => new RawCarrierPostPop(pool, types.Take(100).ToList(), state, Hooks(pool, events, validations)));
        var broken = Hooks(pool, events, validations);
        broken.ValidateBase = null;
        Assert.ThrowsAny<Exception>(() => new RawCarrierPostPop(pool, types, state, broken));
        var carrier = new RawCarrierPostPop(pool, types, state, Hooks(pool, events, validations));
        string before = Convert.ToHexStringLower(pool.Slot(Source));
        // 빈 번호·할당되지 않은 번호는 슬롯을 읽기 전에 거부한다
        Assert.ThrowsAny<Exception>(() => carrier.PostPop(0, 0));
        Assert.ThrowsAny<Exception>(() => carrier.PostPop(51, 0));
        Assert.Equal(before, Convert.ToHexStringLower(pool.Slot(Source)));
        Assert.Empty(events);
        Assert.Equal(0, validations[0]);
        // 자산 밖 타입·내용물 상태도 공통 검사 전에 거부한다
        pool.AllocatedBytes(Source)[RawSidPool.TypeOffset] = 255;
        Assert.ThrowsAny<Exception>(() => carrier.PostPop(Source, 0));
        pool.AllocatedBytes(Source)[RawSidPool.TypeOffset] = PriestType;
        pool.AllocatedBytes(Source)[RawSidPool.StateOffset] = 8;
        Assert.ThrowsAny<Exception>(() => carrier.PostPop(Source, 0));
        pool.AllocatedBytes(Source)[RawSidPool.StateOffset] = 0;
        Assert.Equal(before, Convert.ToHexStringLower(pool.Slot(Source)));
        Assert.Empty(events);
        Assert.Equal(0, validations[0]);
        // 공통 검사가 거부해도 효과는 나가지 않는다
        var rejecting = Hooks(pool, events, validations);
        rejecting.ValidateBase = (sid, flags) => throw new InvalidOperationException("공통 경계 거부 입력");
        var rejected = new RawCarrierPostPop(pool, types, state, rejecting);
        Assert.ThrowsAny<Exception>(() => rejected.PostPop(Source, 0));
        Assert.Equal(before, Convert.ToHexStringLower(pool.Slot(Source)));
        Assert.Empty(events);
    }

    /// <summary>
    /// 직접 공통 호출은 기존 장부를 쓰고 가상 +0xCC 확인의 가드 범위를 지킨다.
    /// 로딩 중 지면 갱신·비용 누적·로컬/전체 통계·AI 부착 거부·억제 경로를 확인한다.
    /// </summary>
    [Fact]
    public void DirectBasePreservesVirtualGuardsAndBookkeeping()
    {
        string[][] rows = X86Fixture.Read("carrierpostpop");
        string[] template = rows.First(row => row[1] == "originals");
        Assert.Equal(19, template.Length);
        var pool = Pool();
        var types = Types(431122);
        types[PriestType].Cost = 123.75f;
        var carrierState = new CarrierPostPopState
        {
            Boss = true,
            LoadingDepth = 1,
            GeyserType = 122,
        };
        Span<byte> raw = pool.AllocatedBytes(Source);
        raw.Clear();
        Put(raw, 0, PriestVtable);
        raw[RawSidPool.TypeOffset] = PriestType;
        raw[RawSidPool.StateOffset] = 0;
        raw[40] = 0;
        raw[34] = 1;
        Put(raw, 12, 0xab84, 2);
        Put(raw, 14, X86Fixture.UInt(template[11]));
        Put(raw, 18, X86Fixture.UInt(template[12]));
        var book = new RawPostPopState
        {
            LocalOwner = 1,
            TotalCost = 10,
            Depth = 1,
        };
        var @base = new RawPostPop(pool, types, book);
        // 사제 가상 표는 기본 몸체가 지원하지 않으므로 직접 공통 호출이 먼저 거부된다
        Assert.ThrowsAny<Exception>(() => @base.Validate(Source, 1, default));
        Assert.ThrowsAny<Exception>(() => @base.Activate(Source, 1));
        Assert.ThrowsAny<Exception>(() => @base.PostPop(Source, 1));
        Assert.Equal(1u, book.Depth);
        Assert.Equal(10, book.TotalCost);
        int checks = 0;
        int claims = 0;
        uint xbits = 0;
        uint ybits = 0;
        CarrierPostPopHooks hooks = RawCarrierPostPop.MakeCarrierPostPopHooks(
            pool,
            @base,
            sid => { Assert.Equal(Source, sid); checks++; },
            (x, y, owner) =>
            {
                claims++;
                xbits = BitConverter.SingleToUInt32Bits(x);
                ybits = BitConverter.SingleToUInt32Bits(y);
                Assert.Equal(1, owner);
            });
        var carrier = new RawCarrierPostPop(pool, types, carrierState, hooks);
        carrier.PostPop(Source, 1);
        // 로딩 중 지면 갱신이 현재 좌표 비트로 한 번 나가고 공통 장부가 쌓인다
        Assert.Equal(1, claims);
        Assert.Equal(X86Fixture.UInt(template[11]), xbits);
        Assert.Equal(X86Fixture.UInt(template[12]), ybits);
        Assert.Equal(0, checks);
        Assert.Equal(0u, book.Depth);
        Assert.Equal(133, book.TotalCost);
        Assert.Equal(1u, book.LocalCounts[PriestType]);
        Assert.Equal(1u, book.LocalSecondaryCounts[PriestType]);
        Assert.Equal(1u, book.GlobalCounts[PriestType]);
        Assert.Equal(0u, book.Providers.Count);
        Assert.Equal(0u, book.Factories.Count);
        // AI 가 붙은 소유자의 최초 등록 통지는 미복원이므로 거부하고 장부를 그대로 둔다
        book.AiAttached[1] = true;
        book.Depth = 1;
        Assert.ThrowsAny<Exception>(() => carrier.PostPop(Source, 1));
        Assert.Equal(1, claims);
        Assert.Equal(1u, book.Depth);
        Assert.Equal(133, book.TotalCost);
        Assert.Equal(1u, book.GlobalCounts[PriestType]);
        // 억제 경로도 지면 갱신 뒤 공통 깊이만 1 줄이고 통계를 건드리지 않는다
        book.Suppressed = true;
        book.Depth = 8;
        carrier.DamageablePostPop(Source, 1);
        Assert.Equal(2, claims);
        Assert.Equal(7u, book.Depth);
        Assert.Equal(133, book.TotalCost);
        // 다른 풀의 기본 몸체로는 묶음을 만들지 않는다
        var other = new RawSidPool(24000, false);
        Assert.ThrowsAny<Exception>(() => RawCarrierPostPop.MakeCarrierPostPopHooks(other, @base, _ => { }, (_, _, _) => { }));
    }
}
