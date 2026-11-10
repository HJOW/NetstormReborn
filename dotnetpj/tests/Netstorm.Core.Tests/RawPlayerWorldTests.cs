// 실제 Player wrapper의 현재 전역 재조회와 파생 후처리의 직접 기록·예약 순서를 검증한다.
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// RawPlayerWorld의 PATCH 본체를 검증하는 테스트 클래스다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class RawPlayerWorldTests
{
    /// <summary>검사 대상 번호다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int Source = 5;

    /// <summary>실제 Player152 생성자가 쓰는 가상표다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const uint PlayerVtable = 0x50ee08u;

    /// <summary>값을 리틀 엔디언으로 쓴다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="bytes">대상 바이트다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓰는 바이트 수다.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 폭만큼 쓰는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>값을 리틀 엔디언으로 읽는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="offset">읽기 시작 위치다.</param>
    /// <returns>조합한 값이다.</returns>
    private static uint Get(ReadOnlySpan<byte> bytes, int offset)
    {
        uint value = 0u;
        // 낮은 바이트부터 모으는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (int i = 0; i < 4; i++)
        {
            value |= (uint)bytes[offset + i] << (i * 8);
        }

        return value;
    }

    /// <summary>번호 5를 클라이언트 영역에 확보한 풀을 만든다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <returns>준비된 풀이다.</returns>
    private static RawSidPool Pool()
    {
        var pool = new RawSidPool(24000, false);
        // 원본 관찰의 첫 번호가 5번이 되도록 클라이언트 영역을 확보한다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (int id = 5; id <= 127; id++)
        {
            Assert.Equal(id, pool.Allocate(SidAllocation.Client));
        }

        return pool;
    }

    /// <summary>타입 레코드 188개를 만든다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <returns>타입 표다.</returns>
    private static OriginalTypeRecord[] Types()
    {
        var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
        // 쓰지 않는 번호도 슬롯 검사의 범위 안에 들어야 한다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord();
        }

        return types;
    }

    /// <summary>기본 슬롯(Player 가상표·74번·소유자 1번)을 채운다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="pool">슬롯 풀이다.</param>
    /// <param name="vtable">가상표 값이다.</param>
    private static void Fill(RawSidPool pool, uint vtable)
    {
        Span<byte> raw = pool.AllocatedBytes(Source);
        raw.Clear();
        Put(raw, 0, vtable);
        raw[RawSidPool.TypeOffset] = 74;
        raw[RawSidPool.StateOffset] = 0;
        raw[OwnerOffset] = 1;
        Put(raw, FrameOffset, 0xdeadbeefu);
    }

    /// <summary>슬롯 안 소유자 바이트의 위치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int OwnerOffset = 34;

    /// <summary>슬롯 안 직접 프레임 위치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int FrameOffset = 36;

    /// <summary>현재 9기록(9×0xb4)을 만든다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <returns>9기록 바이트다.</returns>
    private static byte[] Records() => new byte[9 * 0xb4];

    /// <summary>억제한 공통 장부와 몸체를 묶는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="pool">슬롯 풀이다.</param>
    /// <param name="types">타입 표다.</param>
    /// <param name="book">공통 장부다.</param>
    /// <returns>공통 몸체다.</returns>
    private static RawPostPop Base(RawSidPool pool, OriginalTypeRecord[] types, RawPostPopState book)
    {
        return new RawPostPop(pool, types, book);
    }

    /// <summary>생성자는 같은 풀·9기록·PATCH 판본을 요구한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void ConstructorRejectsMismatchedPoolAndRecords()
    {
        var pool = Pool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true };
        var inner = Base(pool, types, book);
        var state = new PlayerWorldState();
        _ = new RawPlayerWorld(pool, inner, Records(), state, new PlayerWorldHooks());
        // 짧은 9기록은 받지 않는다. 2026-11-07 F2d Player 이식에서 추가됐다.
        Assert.ThrowsAny<Exception>(() => new RawPlayerWorld(pool, inner, new byte[8 * 0xb4], state, new PlayerWorldHooks()));
        // 다른 풀의 공통 몸체와는 묶지 않는다. 2026-11-07 F2d Player 이식에서 추가됐다.
        var other = new RawSidPool(24000, false);
        var otherBase = new RawPostPop(other, types, new RawPostPopState());
        Assert.ThrowsAny<Exception>(() => new RawPlayerWorld(pool, otherBase, Records(), state, new PlayerWorldHooks()));
    }

    /// <summary>실제 Player 가상표만 처리 대상으로 인정한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void HandlesChecksPatchVtable()
    {
        var pool = Pool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true };
        var inner = Base(pool, types, book);
        var body = new RawPlayerWorld(pool, inner, Records(), new PlayerWorldState(), new PlayerWorldHooks());
        Fill(pool, 0x11111111u);
        Assert.False(body.Handles(Source));
        Fill(pool, PlayerVtable);
        Assert.True(body.Handles(Source));
    }

    /// <summary>미복원 표와 9기록 밖 소유자는 공간 변경 전에 거부한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void ValidateRejectsUnknownVtableAndBadOwnerBeforeBase()
    {
        var pool = Pool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true, Depth = 1u };
        var inner = Base(pool, types, book);
        var records = Records();
        var body = new RawPlayerWorld(pool, inner, records, new PlayerWorldState(), new PlayerWorldHooks());
        Fill(pool, 0x11111111u);
        Assert.ThrowsAny<Exception>(() => body.Validate(Source, 0u));
        Fill(pool, PlayerVtable);
        pool.AllocatedBytes(Source)[OwnerOffset] = 9;
        byte[] before = (byte[])records.Clone();
        Assert.ThrowsAny<Exception>(() => body.Validate(Source, 0u));
        Assert.True(records.AsSpan().SequenceEqual(before));
        Assert.Equal(1u, book.Depth);
    }

    /// <summary>아직 번호가 알려지지 않았으면 현재 소유자를 읽지 않고 0을 반환한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void CurrentTechnologySkipsUnknownInvalidOwner()
    {
        var pool = Pool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true };
        var inner = Base(pool, types, book);
        var state = new PlayerWorldState { BattleOwner = 65535u };
        var body = new RawPlayerWorld(pool, inner, Records(), state, new PlayerWorldHooks());
        Assert.Equal(0u, body.CurrentTechnology());
        state.PlayerKnown = 1u;
        Assert.ThrowsAny<Exception>(() => body.CurrentTechnology());
    }

    /// <summary>진단 뒤 갱신한 명시 기록을 읽으며 현재 소유자로 바꾸지 않는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void TechnologyDiagnosticReturnsCurrentExplicitRecord()
    {
        var pool = Pool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true };
        var inner = Base(pool, types, book);
        var records = Records();
        var state = new PlayerWorldState();
        var events = new List<string>();
        var hooks = new PlayerWorldHooks
        {
            Assertion = kind =>
            {
                Assert.Equal(PlayerWorldAssertion.PlayerIdKnown, kind);
                events.Add("A");
                Put(records.AsSpan(), 8 * 0xb4 + 0x9c, 0x12345678u);
                state.BattleOwner = 1u;
            },
        };
        var body = new RawPlayerWorld(pool, inner, records, state, hooks);
        Assert.Equal(0x12345678u, body.Technology(8));
        Assert.Single(events);
        Assert.Equal(0u, body.CurrentTechnology());
    }

    /// <summary>최초 비트가 없는 입력은 예약 장치 없이 현재 기록과 공통 부모를 갱신한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void PostPopWithoutFirstBitNeedsNoReservation()
    {
        var pool = Pool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true, Depth = 1u };
        var inner = Base(pool, types, book);
        var records = Records();
        var body = new RawPlayerWorld(pool, inner, records, new PlayerWorldState(), new PlayerWorldHooks());
        Fill(pool, PlayerVtable);
        body.PostPop(Source, 0u);
        Assert.Equal((uint)Source, Get(records, 0xb4 + 0x9c));
        Assert.Equal(0xdeadbeefu, Get(pool.AllocatedBytes(Source), FrameOffset));
        Assert.Equal(0u, book.Depth);
    }

    /// <summary>예약·특수 프레임 조건이 모두 맞으면 순서대로 기록한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void PostPopReservesRegularAndWritesSpecialFrame()
    {
        var pool = Pool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true, Depth = 1u };
        var inner = Base(pool, types, book);
        var records = Records();
        var state = new PlayerWorldState { Special = 1u };
        var events = new List<string>();
        var hooks = new PlayerWorldHooks
        {
            ReserveRegular = () => { events.Add("N"); return true; },
            AddRegular = (sid, ev, payload) => { Assert.Equal(Source, sid); Assert.Equal(1u, ev); Assert.Equal(5.0f, payload); events.Add("R"); },
        };
        var body = new RawPlayerWorld(pool, inner, records, state, hooks);
        Fill(pool, PlayerVtable);
        body.PostPop(Source, 1u);
        Assert.Equal(new[] { "N", "R" }, events);
        Assert.Equal((uint)Source, Get(records, 0xb4 + 0x9c));
        Assert.Equal(1u, Get(pool.AllocatedBytes(Source), FrameOffset));
        Assert.Equal(0u, book.Depth);
    }

    /// <summary>도달한 확보 실패는 먼저 쓴 Technology를 보존하고 특수 프레임에 도달하지 않는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void MissingReserveKeepsTechnologyBeforeFrame()
    {
        var pool = Pool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true, Depth = 1u };
        var inner = Base(pool, types, book);
        var records = Records();
        var state = new PlayerWorldState { Special = 1u };
        var body = new RawPlayerWorld(pool, inner, records, state, new PlayerWorldHooks());
        Fill(pool, PlayerVtable);
        Assert.ThrowsAny<Exception>(() => body.PostPop(Source, 1u));
        Assert.Equal((uint)Source, Get(records, 0xb4 + 0x9c));
        Assert.Equal(0xdeadbeefu, Get(pool.AllocatedBytes(Source), FrameOffset));
        Assert.Equal(1u, book.Depth);
    }

    /// <summary>실제 9기록 밖 소유자는 물리 저장 직전에 거부한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void InvalidPhysicalOwnerFailsBeforeReservation()
    {
        var pool = Pool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true, Depth = 1u };
        var inner = Base(pool, types, book);
        var records = Records();
        var state = new PlayerWorldState();
        var events = new List<string>();
        var hooks = new PlayerWorldHooks
        {
            ReserveRegular = () => { events.Add("N"); return true; },
            AddRegular = (sid, ev, payload) => events.Add("R"),
        };
        var body = new RawPlayerWorld(pool, inner, records, state, hooks);
        Fill(pool, PlayerVtable);
        pool.AllocatedBytes(Source)[OwnerOffset] = 9;
        byte[] before = (byte[])records.Clone();
        Assert.ThrowsAny<Exception>(() => body.PostPop(Source, 1u));
        Assert.True(records.AsSpan().SequenceEqual(before));
        Assert.Empty(events);
        Assert.Equal(1u, book.Depth);
    }

    /// <summary>진단 복귀 뒤 현재 타입을 다시 조회하고 소유자 효과 뒤 물리 프레임 0을 쓴다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void CreateUsesCurrentTypeAfterDiagnosticsAndPhysicalFrame()
    {
        var pool = Pool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true };
        var inner = Base(pool, types, book);
        var records = Records();
        var state = new PlayerWorldState();
        var events = new List<string>();
        var hooks = new PlayerWorldHooks();
        hooks.Assertion = kind =>
        {
            events.Add(kind == PlayerWorldAssertion.PlayerAlreadyExists ? "A2" : "A1");
            if (kind == PlayerWorldAssertion.PlayerAlreadyExists)
            {
                state.PlayerType = 76u;
            }
            else
            {
                state.PlayerType = 74u;
                state.Special = 1u;
            }
        };
        hooks.Find = (x, y, type) =>
        {
            Assert.Equal(18.0f, x);
            Assert.Equal(10.0f, y);
            Assert.Equal(74u, type);
            events.Add("F");
            state.PlayerType = 75u;
            state.Special = 0u;
            return 6;
        };
        hooks.Create = (type, flags) =>
        {
            Assert.Equal(76u, type);
            Assert.Equal(2u, flags);
            events.Add("C");
            return Source;
        };
        hooks.SetOwner = (sid, owner) =>
        {
            Assert.Equal(Source, sid);
            Assert.Equal(8u, owner);
            events.Add("O");
            pool.AllocatedBytes(sid)[OwnerOffset] = (byte)owner;
        };
        hooks.Pop = (sid, x, y, flags) =>
        {
            Assert.Equal(Source, sid);
            Assert.Equal(18.0f, x);
            Assert.Equal(10.0f, y);
            Assert.Equal(0u, flags);
            Assert.Equal(0u, Get(pool.Slot(sid), FrameOffset));
            events.Add("P");
        };
        // 슬롯 5의 겹친 쓰기를 피하려고 생성 전에 비운다. 2026-11-07 F2d Player 이식에서 추가됐다.
        pool.AllocatedBytes(Source).Clear();
        var body = new RawPlayerWorld(pool, inner, records, state, hooks);
        Assert.Equal(Source, body.Create(8));
        Assert.Equal(new[] { "A1", "F", "A2", "C", "O", "P" }, events);
    }

    /// <summary>실제 생성·소유자 뒤 미연결 Pop에 도달하기 전에 직접 프레임 0이 남는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void CreateMissingPopKeepsOwnerAndFrameStore()
    {
        var pool = Pool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true };
        var inner = Base(pool, types, book);
        var records = Records();
        var state = new PlayerWorldState { PlayerKnown = 1u };
        var hooks = new PlayerWorldHooks
        {
            Find = (x, y, type) => 0,
            Create = (type, flags) => Source,
            SetOwner = (sid, owner) =>
            {
                pool.AllocatedBytes(sid)[OwnerOffset] = (byte)owner;
            },
        };
        pool.AllocatedBytes(Source).Clear();
        var body = new RawPlayerWorld(pool, inner, records, state, hooks);
        Assert.ThrowsAny<Exception>(() => body.Create(8));
        Assert.Equal(8, pool.Slot(Source)[OwnerOffset]);
        Assert.Equal(0u, Get(pool.Slot(Source), FrameOffset));
    }

    /// <summary>CD 풀을 만든다. 2026-11-10 F3 미뤄 둔 전수 재생(0.298)에서 추가됐다.</summary>
    /// <returns>준비된 CD 풀이다.</returns>
    private static RawSidPool CdPool()
    {
        var pool = new RawSidPool(24000, false, null, OriginalEdition.Cd1072);
        // 원본 관찰의 첫 번호가 5번이 되도록 클라이언트 영역을 확보한다.
        for (int id = 5; id <= 127; id++)
        {
            Assert.Equal(id, pool.Allocate(SidAllocation.Client));
        }

        return pool;
    }

    /// <summary>CD의 실제 Player 가상표다. 2026-11-10 F3 미뤄 둔 전수 재생(0.298)에서 추가됐다.</summary>
    private const uint CdPlayerVtable = 0x5002a8u;

    /// <summary>슬롯 안 CD 소유자 바이트의 위치다. 2026-11-10 F3 미뤄 둔 전수 재생(0.298)에서 추가됐다.</summary>
    private const int CdOwnerOffset = 32;

    /// <summary>슬롯 안 CD 직접 프레임 위치다. 2026-11-10 F3 미뤄 둔 전수 재생(0.298)에서 추가됐다.</summary>
    private const int CdFrameOffset = 34;

    /// <summary>현재 9기록(9×0xac)을 만든다. 2026-11-10 F3 미뤄 둔 전수 재생(0.298)에서 추가됐다.</summary>
    /// <returns>9기록 바이트다.</returns>
    private static byte[] CdRecords() => new byte[9 * 0xac];

    /// <summary>CD 기본 슬롯(CD 가상표·74번·소유자 1번)을 채운다. 2026-11-10 F3 미뤄 둔 전수 재생(0.298)에서 추가됐다.</summary>
    /// <param name="pool">슬롯 풀이다.</param>
    /// <param name="vtable">가상표 값이다.</param>
    private static void FillCd(RawSidPool pool, uint vtable)
    {
        Span<byte> raw = pool.AllocatedBytes(Source);
        raw.Clear();
        Put(raw, 0, vtable);
        raw[RawSidPool.TypeOffset] = 74;
        raw[RawSidPool.StateOffset] = 0;
        raw[CdOwnerOffset] = 1;
        Put(raw, CdFrameOffset, 0xdeadbeefu);
    }

    /// <summary>CD 필수 효과 7종을 묶는다. 2026-11-10 F3 미뤄 둔 전수 재생(0.298)에서 추가됐다.</summary>
    /// <returns>효과 묶음이다.</returns>
    private static PlayerWorldHooks FullHooks() => new()
    {
        Assertion = _ => { },
        Find = (x, y, type) => 0,
        Create = (type, flags) => Source,
        SetOwner = (sid, owner) => { },
        Pop = (sid, x, y, flags) => { },
        ReserveRegular = () => false,
        AddRegular = (sid, ev, payload) => { },
    };

    /// <summary>CD 생성자는 0xac 9기록과 7개 효과를 요구한다. 2026-11-10 F3 미뤄 둔 전수 재생(0.298)에서 추가됐다.</summary>
    [Fact]
    public void ConstructorAcceptsCdRecordsAndRequiresHooks()
    {
        var pool = CdPool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true };
        var inner = Base(pool, types, book);
        var state = new PlayerWorldState();
        _ = new RawPlayerWorld(pool, inner, CdRecords(), state, FullHooks());
        // PATCH 크기 9기록은 CD에서 받지 않는다.
        Assert.ThrowsAny<Exception>(() => new RawPlayerWorld(pool, inner, Records(), state, FullHooks()));
        // CD는 빠진 효과가 하나라도 있으면 묶지 않는다.
        var missing = FullHooks();
        missing.Pop = null;
        Assert.ThrowsAny<Exception>(() => new RawPlayerWorld(pool, inner, CdRecords(), state, missing));
    }

    /// <summary>CD 실제 Player 가상표만 처리 대상으로 인정한다. 2026-11-10 F3 미뤄 둔 전수 재생(0.298)에서 추가됐다.</summary>
    [Fact]
    public void HandlesChecksCdVtable()
    {
        var pool = CdPool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true };
        var inner = Base(pool, types, book);
        var body = new RawPlayerWorld(pool, inner, CdRecords(), new PlayerWorldState(), FullHooks());
        FillCd(pool, 0x11111111u);
        Assert.False(body.Handles(Source));
        FillCd(pool, CdPlayerVtable);
        Assert.True(body.Handles(Source));
    }

    /// <summary>CD 0xac 기록의 0x94 위치를 읽는다. 2026-11-10 F3 미뤄 둔 전수 재생(0.298)에서 추가됐다.</summary>
    [Fact]
    public void TechnologyReadsCdOffsets()
    {
        var pool = CdPool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true };
        var inner = Base(pool, types, book);
        var records = CdRecords();
        var state = new PlayerWorldState { PlayerKnown = 1u, BattleOwner = 8u };
        var body = new RawPlayerWorld(pool, inner, records, state, FullHooks());
        Put(records.AsSpan(), 8 * 0xac + 0x94, 0x12345678u);
        Assert.Equal(0x12345678u, body.Technology(8));
        Assert.Equal(0x12345678u, body.CurrentTechnology());
    }

    /// <summary>CD 후처리는 검증 뒤 기록·1바이트 프레임을 쓴다. 2026-11-10 F3 미뤄 둔 전수 재생(0.298)에서 추가됐다.</summary>
    [Fact]
    public void PostPopWritesCdTechnologyAndFrameByte()
    {
        var pool = CdPool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true, Depth = 1u };
        var inner = Base(pool, types, book);
        var records = CdRecords();
        var state = new PlayerWorldState { Special = 1u };
        var events = new List<string>();
        var hooks = FullHooks();
        hooks.ReserveRegular = () => { events.Add("N"); return true; };
        hooks.AddRegular = (sid, ev, payload) => { Assert.Equal(Source, sid); Assert.Equal(1u, ev); Assert.Equal(5.0f, payload); events.Add("R"); };
        var body = new RawPlayerWorld(pool, inner, records, state, hooks);
        FillCd(pool, CdPlayerVtable);
        body.PostPop(Source, 1u);
        Assert.Equal(new[] { "N", "R" }, events);
        Assert.Equal((uint)Source, Get(records, 0xac + 0x94));
        // 1바이트 쓰기이므로 34번만 바뀌고 35번은 그대로다.
        Assert.Equal(1, pool.AllocatedBytes(Source)[CdFrameOffset]);
        Assert.Equal(0xbe, pool.AllocatedBytes(Source)[CdFrameOffset + 1]);
        Assert.Equal(0u, book.Depth);
    }

    /// <summary>CD 9기록 밖 소유자는 물리 저장 직전에 거부한다. 2026-11-10 F3 미뤄 둔 전수 재생(0.298)에서 추가됐다.</summary>
    [Fact]
    public void ValidateRejectsCdBadOwner()
    {
        var pool = CdPool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true, Depth = 1u };
        var inner = Base(pool, types, book);
        var records = CdRecords();
        var body = new RawPlayerWorld(pool, inner, records, new PlayerWorldState(), FullHooks());
        FillCd(pool, CdPlayerVtable);
        pool.AllocatedBytes(Source)[CdOwnerOffset] = 9;
        byte[] before = (byte[])records.Clone();
        Assert.ThrowsAny<Exception>(() => body.Validate(Source, 0u));
        Assert.True(records.AsSpan().SequenceEqual(before));
        Assert.Equal(1u, book.Depth);
    }

    /// <summary>CD 생성은 소유자 바이트 32 뒤 1바이트 프레임 0을 쓴다. 2026-11-10 F3 미뤄 둔 전수 재생(0.298)에서 추가됐다.</summary>
    [Fact]
    public void CreateWritesCdFrameByte()
    {
        var pool = CdPool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true };
        var inner = Base(pool, types, book);
        var records = CdRecords();
        var state = new PlayerWorldState { PlayerKnown = 1u };
        var hooks = new PlayerWorldHooks
        {
            Assertion = _ => { },
            Find = (x, y, type) => 0,
            Create = (type, flags) => Source,
            SetOwner = (sid, owner) =>
            {
                pool.AllocatedBytes(sid)[CdOwnerOffset] = (byte)owner;
            },
            // CD 생성자는 Pop 효과도 요구하므로 호출 때 미연결로 멈춘다.
            Pop = (sid, x, y, flags) => throw new InvalidOperationException("실제 공간 Pop 미연결"),
            ReserveRegular = () => false,
            AddRegular = (sid, ev, payload) => { },
        };
        pool.AllocatedBytes(Source).Clear();
        var body = new RawPlayerWorld(pool, inner, records, state, hooks);
        Assert.ThrowsAny<Exception>(() => body.Create(8));
        Assert.Equal(8, pool.Slot(Source)[CdOwnerOffset]);
        Assert.Equal(0, pool.Slot(Source)[CdFrameOffset]);
    }

    /// <summary>확보 효과 뒤 바뀐 special을 읽고 직접 프레임 1을 쓴다. 2026-11-10 F3 미뤄 둔 전수 재생(0.299)에서 추가됐다.</summary>
    [Fact]
    public void PostPopFailedReserveUsesCurrentSpecialAndPhysicalFrame()
    {
        // cpp `PlayerParentCurrent_FailedReserveUsesCurrentSpecialAndPhysicalFrame`의 순서 계약이다.
        var pool = Pool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true, Depth = 1u };
        var inner = Base(pool, types, book);
        var records = Records();
        var state = new PlayerWorldState();
        var events = new List<string>();
        var hooks = new PlayerWorldHooks
        {
            ReserveRegular = () =>
            {
                events.Add("N");
                // 먼저 쓴 Technology를 확인한 뒤 예약을 거절하고 special을 바꾼다.
                Assert.Equal((uint)Source, Get(records, 0xb4 + 0x9c));
                state.Special = 256u;
                state.BattleOwner = 8u;
                return false;
            },
            AddRegular = (sid, ev, payload) => events.Add("R"),
        };
        var body = new RawPlayerWorld(pool, inner, records, state, hooks);
        Fill(pool, PlayerVtable);
        body.PostPop(Source, 1u);
        Assert.Equal(["N"], events);
        Assert.Equal((uint)Source, Get(records, 0xb4 + 0x9c));
        Assert.Equal(0u, Get(records, 8 * 0xb4 + 0x9c));
        Assert.Equal(1u, Get(pool.AllocatedBytes(Source), FrameOffset));
        Assert.Equal(0u, book.Depth);
    }

    /// <summary>성공한 확보 뒤 생성자 장치 실패는 기록·확보를 남긴다. 2026-11-10 F3 미뤄 둔 전수 재생(0.299)에서 추가됐다.</summary>
    [Fact]
    public void PostPopMissingAddRegularKeepsSuccessfulReservation()
    {
        // cpp `PlayerParentCurrent_MissingConstructorKeepsSuccessfulReservation`의 순서 계약이다.
        var pool = Pool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true, Depth = 1u };
        var inner = Base(pool, types, book);
        var records = Records();
        var state = new PlayerWorldState();
        var events = new List<string>();
        var hooks = new PlayerWorldHooks
        {
            ReserveRegular = () => { events.Add("N"); return true; },
            AddRegular = null,
        };
        var body = new RawPlayerWorld(pool, inner, records, state, hooks);
        Fill(pool, PlayerVtable);
        Assert.ThrowsAny<Exception>(() => body.PostPop(Source, 1u));
        Assert.Equal(["N"], events);
        Assert.Equal((uint)Source, Get(records, 0xb4 + 0x9c));
        Assert.Equal(1u, book.Depth);
    }

    /// <summary>생성 효과 뒤 special을 다시 읽으며 처음 관문을 재검사하지 않는다. 2026-11-10 F3 미뤄 둔 전수 재생(0.299)에서 추가됐다.</summary>
    [Fact]
    public void PostPopRegularMutationCanDisableLaterSpecialFrame()
    {
        // cpp `PlayerParentCurrent_RegularMutationCanDisableLaterSpecialFrame`의 순서 계약이다.
        var pool = Pool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true, Depth = 1u };
        var inner = Base(pool, types, book);
        var records = Records();
        var state = new PlayerWorldState { Special = 1u };
        var events = new List<string>();
        var hooks = new PlayerWorldHooks
        {
            ReserveRegular = () => { events.Add("N"); return true; },
            AddRegular = (sid, ev, payload) =>
            {
                events.Add("R");
                state.Special = 0u;
                state.FortMode = true;
            },
        };
        var body = new RawPlayerWorld(pool, inner, records, state, hooks);
        Fill(pool, PlayerVtable);
        body.PostPop(Source, 1u);
        Assert.Equal(["N", "R"], events);
        Assert.Equal(0xdeadbeefu, Get(pool.AllocatedBytes(Source), FrameOffset));
        Assert.Equal((uint)Source, Get(records, 0xb4 + 0x9c));
        Assert.Equal(0u, Get(records, 0x9c));
        Assert.Equal(0u, book.Depth);
    }

    /// <summary>바이트 소유자 1과 현재 DWORD 257은 다르지만 special 프레임은 독립해서 실행한다. 2026-11-10 F3 미뤄 둔 전수 재생(0.299)에서 추가됐다.</summary>
    [Fact]
    public void PostPopFullLocalOwnerDwordDoesNotSelectReservation()
    {
        // cpp `PlayerParentCurrent_FullLocalOwnerDwordDoesNotSelectReservation`의 순서 계약이다.
        var pool = Pool();
        var types = Types();
        var book = new RawPostPopState { Suppressed = true, Depth = 1u };
        var inner = Base(pool, types, book);
        var records = Records();
        var state = new PlayerWorldState { BattleOwner = 257u, Special = 1u };
        var body = new RawPlayerWorld(pool, inner, records, state, new PlayerWorldHooks());
        Fill(pool, PlayerVtable);
        body.PostPop(Source, 1u);
        Assert.Equal(1u, Get(pool.AllocatedBytes(Source), FrameOffset));
        Assert.Equal((uint)Source, Get(records, 0xb4 + 0x9c));
        Assert.Equal(0u, book.Depth);
    }
}
