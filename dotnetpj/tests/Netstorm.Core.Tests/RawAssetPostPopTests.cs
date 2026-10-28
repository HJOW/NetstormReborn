using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 자산 후처리(<see cref="RawAssetPostPop"/>)의 보호 경로와 연결 지점 계약을 확인한다.
/// 원본 기대값 재생(X86RawAssetPostPopTests)은 전용 몸체의 효과 순서·결과를 대조하고,
/// 이 검사들은 생성·지원 표·필수 효과·비용 범위의 보호 계약만 확인한다 (W02 Asset 조각, 2026-10-20).
/// </summary>
public sealed class RawAssetPostPopTests
{
    /// <summary>원본 사제 가상 표(10.78). 슬롯 식별 값으로만 쓰며 호스트 함수로 호출하지 않는다.</summary>
    private const uint PriestVtable = 0x0050F210;

    /// <summary>검사 대상 번호.</summary>
    private const int Source = 50;

    /// <summary>값을 리틀 엔디언으로 쓴다. 폭 밖의 이웃 필드는 보존한다.</summary>
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

    /// <summary>번호 50 을 클라이언트 영역에 확보한 풀을 만든다.</summary>
    private static RawSidPool Pool()
    {
        var pool = new RawSidPool(24000, false);
        // 원본 관찰의 번호 범위(5~127번)를 클라이언트 영역에 확보한다
        for (int id = 5; id <= 127; id++)
        {
            Assert.Equal(id, pool.Allocate(SidAllocation.Client));
        }
        return pool;
    }

    /// <summary>타입 레코드 188개를 만들고 70번만 채운다.</summary>
    private static OriginalTypeRecord[] Types(float cost = 1000f)
    {
        var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
        // 쓰지 않는 번호도 슬롯 검사의 범위 안에 들어야 한다
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord();
        }
        Types70(types, cost);
        return types;
    }

    /// <summary>자산 관찰이 쓰는 70번 타입을 채운다.</summary>
    /// <param name="types">타입 표.</param>
    /// <param name="cost">비용.</param>
    private static void Types70(OriginalTypeRecord[] types, float cost)
    {
        types[70].Flags1 = 4096;
        types[70].Flags2 = 0x200;
        types[70].Cost = cost;
    }

    /// <summary>기본 슬롯(사제 가상 표·70번·소유자 1번)을 채운다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="vtable">가상 표 값.</param>
    /// <param name="extra">추가 비트 바이트.</param>
    private static void Fill(RawSidPool pool, uint vtable, byte extra = 0)
    {
        Span<byte> raw = pool.AllocatedBytes(Source);
        raw.Clear();
        Put(raw, 0, vtable);
        raw[RawSidPool.TypeOffset] = 70;
        raw[RawSidPool.StateOffset] = 0;
        raw[34] = 1;
        raw[40] = extra;
        Put(raw, 12, 0xabcd, 2);
        Put(raw, 14, 0x42480000);
        Put(raw, 18, 0x42500000);
        Put(raw, 36, 25);
    }

    /// <summary>공통 장부(억제 켜짐)와 기본·운반체 몸체를 묶는다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="types">타입 표.</param>
    /// <param name="book">공통 장부.</param>
    /// <param name="carrierState">운반체 상태.</param>
    private static (RawPostPop @base, RawCarrierPostPop damageable, AssetPostPopState state) Hub(
        RawSidPool pool, OriginalTypeRecord[] types, RawPostPopState book, CarrierPostPopState carrierState)
    {
        var @base = new RawPostPop(pool, types, book);
        var damageable = new RawCarrierPostPop(pool, types, carrierState, new CarrierPostPopHooks
        {
            CarrierCheck = sid => throw new InvalidOperationException("확인 미연결"),
            ClaimGround = (x, y, owner) => throw new InvalidOperationException("지면 미연결"),
            ValidateBase = (sid, flags) => @base.ValidateBase(sid, flags),
            Base = (sid, flags) => @base.PostPopBase(sid, flags),
        });
        return (@base, damageable, new AssetPostPopState());
    }

    /// <summary>
    /// 생성자는 같은 풀·10.78 타입 표를 요구한다.
    /// 다른 풀의 기본·Damageable 몸체와 짧은 타입 표는 거부한다.
    /// </summary>
    [Fact]
    public void ConstructorRejectsMismatchedPoolAndTypes()
    {
        var pool = Pool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true };
        var carrierState = new CarrierPostPopState();
        var (@base, damageable, state) = Hub(pool, types, book, carrierState);
        _ = new RawAssetPostPop(pool, types, @base, damageable, state, new AssetPostPopHooks());
        // 다른 풀의 기본 몸체와는 묶지 않는다
        var other = new RawSidPool(24000, false);
        var otherBase = new RawPostPop(other, types, new RawPostPopState());
        Assert.ThrowsAny<Exception>(() => new RawAssetPostPop(pool, types, otherBase, damageable, state, new AssetPostPopHooks()));
        // 짧은 타입 표(CD 171개)는 받지 않는다
        Assert.ThrowsAny<Exception>(() => new RawAssetPostPop(pool, types.Take(100).ToList(), @base, damageable, state, new AssetPostPopHooks()));
    }

    /// <summary>
    /// 지원 표에 없는 가상 표는 처리하지 않고, 복원한 표는 처리한다.
    /// 타입 번호가 표를 벗어나도 거부한다.
    /// </summary>
    [Fact]
    public void HandlesOnlyRestoredVtables()
    {
        var pool = Pool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true };
        var carrierState = new CarrierPostPopState();
        var (@base, damageable, state) = Hub(pool, types, book, carrierState);
        var assets = new RawAssetPostPop(pool, types, @base, damageable, state, new AssetPostPopHooks());
        // 복원하지 않은 표는 처리하지 않는다
        Fill(pool, 0x11111111);
        Assert.False(assets.Handles(Source));
        Assert.ThrowsAny<Exception>(() => assets.Validate(Source, 1));
        // 비최초 호출의 미지원 표도 쓰기 전에 거부한다(종류 검사가 먼저다)
        Assert.ThrowsAny<Exception>(() => assets.Validate(Source, 0));
        // 복원한 제네레이터 표는 처리한다
        Fill(pool, 0x00501ea0);
        Assert.True(assets.Handles(Source));
        // 타입 번호가 표를 벗어나면 거부한다
        pool.AllocatedBytes(Source)[RawSidPool.TypeOffset] = 255;
        Assert.ThrowsAny<Exception>(() => assets.Validate(Source, 1));
    }

    /// <summary>
    /// Gun 분기의 필수 효과가 없으면 공간 쓰기 전에 거부한다.
    /// 각도 Gun 의 프레임·그래프 연결이 빠지면 실패한다.
    /// </summary>
    [Fact]
    public void ValidateRejectsMissingGunEffectsBeforeMutation()
    {
        var pool = Pool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true };
        var carrierState = new CarrierPostPopState();
        var (@base, damageable, state) = Hub(pool, types, book, carrierState);
        Fill(pool, 0x0050b6e8);
        string before = Convert.ToHexStringLower(pool.Slot(Source));
        // 각도 Gun 인데 프레임·그래프 연결이 없으면 거부한다
        var assets = new RawAssetPostPop(pool, types, @base, damageable, state, new AssetPostPopHooks());
        Assert.ThrowsAny<Exception>(() => assets.Validate(Source, 7));
        Assert.Equal(before, Convert.ToHexStringLower(pool.Slot(Source)));
    }

    /// <summary>
    /// 간헐천 비용이 CRT 범위를 벗어나면 슬롯 쓰기 전에 거부한다.
    /// 비유한 비용은 목록·통계에 닿기 전에 실패한다.
    /// </summary>
    [Fact]
    public void GeyserRejectsNonFiniteCostBeforeSlotWrite()
    {
        var pool = Pool();
        var types = Types(float.NaN);
        var book = new RawPostPopState { Suppressed = true };
        var carrierState = new CarrierPostPopState();
        var (@base, damageable, state) = Hub(pool, types, book, carrierState);
        Fill(pool, 0x0050da88);
        var hooks = new AssetPostPopHooks
        {
            ReserveRegular = () => false,
            AddRegular = (sid, @event, delay) => { },
            GeyserRegistration = (sid, value) => { },
            AtPosition = (x, y) => [],
            Unpop = (sid, flags) => { },
            Repop = (sid, flags) => { },
        };
        var assets = new RawAssetPostPop(pool, types, @base, damageable, state, hooks);
        string before = Convert.ToHexStringLower(pool.Slot(Source));
        // 비유한 비용의 절삭이 실패하므로 슬롯 바이트는 그대로 둔다
        Assert.ThrowsAny<Exception>(() => assets.PostPop(Source, 1));
        Assert.Equal(before, Convert.ToHexStringLower(pool.Slot(Source)));
    }

    /// <summary>
    /// 전용 몸체 4종(훨리베이스·맨 오 워 풀·썬 바리케이드·너겟)은 각자 효과로 분배한다.
    /// fixture 에 없는 경로라 단위 검사로 분기·필수 효과를 고정한다.
    /// </summary>
    [Fact]
    public void SpecialBodiesDispatchToDedicatedEffects()
    {
        (uint vtable, Action<AssetPostPopHooks> clear, Func<AssetPostPopHooks, bool> missing)[] cases =
        [
            (0x00509930, hooks => hooks.SunAviaryPostPop = null, hooks => hooks.SunAviaryPostPop is null),
            (0x005099e8, hooks => hooks.RainAviaryPostPop = null, hooks => hooks.RainAviaryPostPop is null),
            (0x00509470, hooks => hooks.SunFencePostPop = null, hooks => hooks.SunFencePostPop is null),
            (0x0050dc30, hooks => hooks.NuggetPostPop = null, hooks => hooks.NuggetPostPop is null),
        ];
        // 자산 후처리 가상 표 경우를 하나씩 꺼내, 경우마다 새 풀과 타입 표를 만든다
        foreach ((uint vtable, Action<AssetPostPopHooks> clear, Func<AssetPostPopHooks, bool> missing) in cases)
        {
            var pool = Pool();
            var types = Types();
            var book = new RawPostPopState { Suppressed = true };
            var carrierState = new CarrierPostPopState();
            var (@base, damageable, state) = Hub(pool, types, book, carrierState);
            Fill(pool, vtable);
            var calls = new List<string>();
            var hooks = new AssetPostPopHooks
            {
                SunAviaryPostPop = (sid, flags) => calls.Add($"A:{sid}:{flags}"),
                RainAviaryPostPop = (sid, flags) => calls.Add($"A:{sid}:{flags}"),
                SunFencePostPop = (sid, flags) => calls.Add($"A:{sid}:{flags}"),
                NuggetPostPop = (sid, flags) => calls.Add($"A:{sid}:{flags}"),
            };
            var assets = new RawAssetPostPop(pool, types, @base, damageable, state, hooks);
            // 각자 효과로 분배하고 공통 깊이를 건드리지 않는다
            Assert.True(assets.Handles(Source));
            assets.PostPop(Source, 1);
            Assert.Equal(new[] { $"A:{Source}:1" }, calls);
            // 필수 효과가 없으면 쓰기 전에 거부한다
            clear(hooks);
            Assert.True(missing(hooks));
            string specialBefore = Convert.ToHexStringLower(pool.Slot(Source));
            Assert.ThrowsAny<Exception>(() => assets.PostPop(Source, 1));
            Assert.Equal(specialBefore, Convert.ToHexStringLower(pool.Slot(Source)));
        }
    }

    /// <summary>
    /// 보스 신전은 같은 영역의 다른 소용돌이만 가상 삭제한다.
    /// 영역 번호가 다르면 삭제하지 않는다.
    /// </summary>
    [Fact]
    public void VortexBossDestroysOnlySameRegion()
    {
        // 검사할 섬 번호(99 는 다른 섬, 3 은 같은 섬)를 차례로 돌며 같은 시나리오를 되풀이한다
        foreach (int region in new[] { 99, 3 })
        {
            var pool = Pool();
            var types = Types();
            var book = new RawPostPopState { Suppressed = true };
            var carrierState = new CarrierPostPopState();
            var (@base, damageable, state) = Hub(pool, types, book, carrierState);
            Fill(pool, 0x005196f0);
            state.Boss = true;
            state.LoadingDepth = 1;
            // 61번을 같은 영역 후보(신전 부류·비추가)로 둔다
            Span<byte> candidate = pool.AllocatedBytes(61);
            candidate.Clear();
            candidate[RawSidPool.TypeOffset] = 70;
            var destroyed = new List<int>();
            var hooks = new AssetPostPopHooks
            {
                ReserveRegular = () => false,
                AddRegular = (sid, @event, delay) => { },
                TypeDword = (sid, offset) => 7u,
                PlayImpactSound = value => { },
                SetOwner = (sid, owner) => { },
                IslandNumber = (x, y) => 3,
                IslandExists = island => true,
                RegisterTemple = (island, sid) => { },
                RegionNumber = (x, y) => region,
                Destroy = (sid, flags) => { destroyed.Add(sid); pool.AllocatedBytes(sid)[RawSidPool.StateOffset] |= 2; },
                HasContent = (sid, slot) => true,
                CreateSlot = (sid, slot) => { },
                RefreshProduction = () => { },
                ClearPlacement = () => { },
            };
            var assets = new RawAssetPostPop(pool, types, @base, damageable, state, hooks);
            assets.PostPop(Source, 1);
            // 섬 번호와 같은 영역일 때만 61번을 삭제한다
            Assert.Equal(region == 3 ? new[] { 61 } : [], destroyed);
            Assert.Equal(region == 3, (pool.Slot(61)[RawSidPool.StateOffset] & 2) != 0);
        }
    }

    /// <summary>
    /// 새 전용 몸체 2종(데빌메이커·성장형 아이스)은 각자 효과로 분배한다.
    /// fixture 에 없는 경로라 단위 검사로 분기·필수 효과를 고정한다.
    /// 근거: cpppj/src/o/RawAssetPostPop.cpp Validate·PostPop (R0-4 재대조, 2026-10-28).
    /// </summary>
    [Fact]
    public void NewHookBodiesDispatchToDedicatedEffects()
    {
        (uint vtable, Action<AssetPostPopHooks> clear, bool logicError)[] cases =
        [
            (0x00509aa0, hooks => hooks.WindAviaryPostPop = null, true),
            (0x0050e888, hooks => hooks.GrowingIcePostPop = null, false),
        ];
        // 자산 후처리 가상 표 경우를 하나씩 꺼내, 경우마다 새 풀과 타입 표를 만든다
        foreach ((uint vtable, Action<AssetPostPopHooks> clear, bool logicError) in cases)
        {
            var pool = Pool();
            var types = Types();
            var book = new RawPostPopState { Suppressed = true };
            var carrierState = new CarrierPostPopState();
            var (@base, damageable, state) = Hub(pool, types, book, carrierState);
            Fill(pool, vtable);
            var calls = new List<string>();
            var hooks = new AssetPostPopHooks
            {
                WindAviaryPostPop = (sid, flags) => calls.Add($"W:{sid}:{flags}"),
                GrowingIcePostPop = (sid, flags) => calls.Add($"G:{sid}:{flags}"),
            };
            var assets = new RawAssetPostPop(pool, types, @base, damageable, state, hooks);
            // 각자 효과로 분배한다
            Assert.True(assets.Handles(Source));
            assets.PostPop(Source, 1);
            Assert.Single(calls);
            // 필수 효과가 없으면 쓰기 전에 거부한다
            clear(hooks);
            string before = Convert.ToHexStringLower(pool.Slot(Source));
            if (logicError)
            {
                Assert.Throws<InvalidOperationException>(() => assets.PostPop(Source, 1));
            }
            else
            {
                Assert.Throws<ArgumentException>(() => assets.PostPop(Source, 1));
            }
            Assert.Equal(before, Convert.ToHexStringLower(pool.Slot(Source)));
        }
    }

    /// <summary>
    /// 스파이어 2종(아크·애시드)은 최초에만 사건 0을 예약하고 항상 같은 Damageable 로 이어진다.
    /// 아크는 1초, 애시드는 0.3초 뒤로 예약한다.
    /// 근거: cpppj/src/o/RawArcSpire.cpp·RawAcidFence.cpp PostPop (R0-4 재대조, 2026-10-28).
    /// </summary>
    [Fact]
    public void SpireBodiesReserveFirstEventThenDamageable()
    {
        (uint vtable, float delay)[] cases = [(0x0050e668, 1.0f), (0x0050e738, 0.3f)];
        // 스파이어 가상 표 경우를 하나씩 꺼내, 경우마다 새 풀과 타입 표를 만든다
        foreach ((uint vtable, float delay) in cases)
        {
            var pool = Pool();
            var types = Types();
            var book = new RawPostPopState { Suppressed = true };
            var carrierState = new CarrierPostPopState();
            var (@base, damageable, state) = Hub(pool, types, book, carrierState);
            Fill(pool, vtable);
            var calls = new List<string>();
            var hooks = new AssetPostPopHooks
            {
                ReserveRegular = () => { calls.Add("R"); return true; },
                AddRegular = (sid, @event, value) => calls.Add($"A:{sid}:{@event}:{value}"),
            };
            var assets = new RawAssetPostPop(pool, types, @base, damageable, state, hooks);
            // 최초 호출은 예약 뒤 Damageable 까지 간다
            Assert.True(assets.Handles(Source));
            assets.PostPop(Source, 1);
            Assert.Equal(new[] { "R", $"A:{Source}:0:{delay}" }, calls);
            // 비최초 호출은 예약 없이 Damageable 로 간다
            calls.Clear();
            assets.PostPop(Source, 0);
            Assert.Empty(calls);
        }
    }

    /// <summary>
    /// 스파이어 2종은 추가·매립 비트가 모두 꺼진 최초 호출에서만 Regular 예약을 요구한다.
    /// 비트가 켜지거나 비최초이면 예약 없이 통과한다.
    /// </summary>
    [Fact]
    public void SpireBodiesRequireRegularOnlyOnFirstPostPop()
    {
        // 스파이어 가상 표 경우를 하나씩 꺼내, 경우마다 새 풀과 타입 표를 만든다
        foreach (uint vtable in new[] { 0x0050e668u, 0x0050e738u })
        {
            var pool = Pool();
            var types = Types();
            var book = new RawPostPopState { Suppressed = true };
            var carrierState = new CarrierPostPopState();
            var (@base, damageable, state) = Hub(pool, types, book, carrierState);
            Fill(pool, vtable);
            var assets = new RawAssetPostPop(pool, types, @base, damageable, state, new AssetPostPopHooks());
            // 최초 호출인데 예약이 없으면 쓰기 전에 거부한다
            string before = Convert.ToHexStringLower(pool.Slot(Source));
            Assert.Throws<ArgumentException>(() => assets.PostPop(Source, 1));
            Assert.Equal(before, Convert.ToHexStringLower(pool.Slot(Source)));
            // 추가 비트가 켜지면 예약 없이 통과한다
            Fill(pool, vtable, extra: 8);
            assets.PostPop(Source, 1);
            // 비최초 호출도 예약 없이 통과한다
            Fill(pool, vtable);
            assets.PostPop(Source, 0);
        }
    }

    /// <summary>
    /// 외부 효과가 슬롯 타입을 바꾼 뒤에도 호출 시점의 현재 타입을 다시 읽는다.
    /// 표를 벗어난 타입은 명시적으로 거부한다.
    /// 근거: cpppj/src/o/RawAssetPostPop.cpp Type(sid) (R0-4 재대조, 2026-10-28).
    /// </summary>
    [Fact]
    public void GunRereadsCurrentSlotTypeAfterExternalEffects()
    {
        var pool = Pool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true };
        var carrierState = new CarrierPostPopState();
        var (@base, damageable, state) = Hub(pool, types, book, carrierState);
        state.InBattle = true;
        state.LoadingDepth = 1;
        Fill(pool, 0x0050b9b8);
        var hooks = new AssetPostPopHooks
        {
            // 외부 효과가 슬롯 타입을 표 밖으로 바꾼다
            CreateGunProcess = sid => pool.AllocatedBytes(sid)[RawSidPool.TypeOffset] = 200,
        };
        var assets = new RawAssetPostPop(pool, types, @base, damageable, state, hooks);
        // 옛 코드는 표 직접 색인이라 IndexOutOfRange 였으나 이제 명시적 범위로 거부한다
        Assert.Throws<ArgumentOutOfRangeException>(() => assets.PostPop(Source, 1));
    }

    /// <summary>
    /// 기술 비트 번호는 없으면 -1이며 복사본에 보존된다.
    /// 원본 타입 구조체 PATCH +0x1ec·CD +0x1cc 의 techBitIndex 다.
    /// 근거: cpppj/src/o/RiftType.h (R0-4 재대조, 2026-10-28).
    /// </summary>
    [Fact]
    public void TechBitIndexDefaultsToMinusOneAndClonePreserves()
    {
        var record = new OriginalTypeRecord();
        Assert.Equal(-1, record.TechBitIndex);
        record.TechBitIndex = 5;
        Assert.Equal(5, record.Clone().TechBitIndex);
    }
}
