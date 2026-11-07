// 무인도 최초 등록·받침 소유자 전파의 경계·분배를 직접 검사한다.
using System.Globalization;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// RawIslandPostPop의 지도·상태·소유자 전파를 검증하는 테스트 클래스다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public sealed class IslandPostPopTests
{
    /// <summary>검사 풀 용량이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int Capacity = 24000;

    /// <summary>표면 칸 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int Surface = 50;

    /// <summary>무인도 표면 타입 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const uint NoIsland = 157u;

    /// <summary>섬 받침 타입 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const uint Island = 94u;

    /// <summary>섬 종유석 타입 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const uint Stalag = 95u;

    /// <summary>값을 리틀 엔디언으로 쓴다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="raw">대상 슬롯 바이트다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓰는 바이트 수다.</param>
    private static void Put(Span<byte> raw, int offset, uint value, int width)
    {
        // 낮은 바이트부터 폭만큼 쓰는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int i = 0; i < width; ++i)
        {
            raw[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>풀·타입·프레임·지도의 빈 장면을 만든다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <returns>풀·타입·프레임·표면 지도·소유자 지도·연결 전역 묶음이다.</returns>
    private static (RawSidPool Pool, OriginalTypeRecord[] Types, RiftTypeFrames[] Frames, ushort[] SurfaceMap, ushort[] OwnerMap, BridgeConnectState TypeState) Scene()
    {
        RawSidPool pool = new(Capacity, false);
        OriginalTypeRecord[] types = new OriginalTypeRecord[188];
        // 모든 타입 번호에 빈 타입 기록을 채우는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int i = 0; i < types.Length; ++i)
        {
            types[i] = new OriginalTypeRecord();
        }

        RiftTypeFrames[] frames = new RiftTypeFrames[types.Length];
        // 모든 타입 번호마다 빈 프레임 표를 넣는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int i = 0; i < frames.Length; ++i)
        {
            frames[i] = new RiftTypeFrames(Array.Empty<RiftFrameCode>());
        }

        ushort[] surfaceMap = new ushort[65536];
        ushort[] ownerMap = new ushort[65536];
        BridgeConnectState typeState = new()
        {
            IslandType = Island,
            StalagType = Stalag,
            NoIslandType = NoIsland,
        };
        return (pool, types, frames, surfaceMap, ownerMap, typeState);
    }

    /// <summary>원본 지도 조회는 범위 밖·다른 타입을 0으로 돌려준다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void SurfaceLetterGuards()
    {
        var (pool, types, frames, surfaceMap, ownerMap, typeState) = Scene();
        frames[(int)NoIsland] = new RiftTypeFrames(new[] { new RiftFrameCode((byte)'H', (byte)'P', 1, 0) });
        Span<byte> raw = pool.RawSlot(Surface);
        raw.Clear();
        raw[10] = (byte)NoIsland;
        Put(raw, 36, 0u, 4);
        ownerMap[30 * 256 + 29] = Surface;
        RawIslandPostPop surface = new(pool, surfaceMap, types, frames, typeState, new IslandPostPopState(), new IslandPostPopHooks(), ownerMap);
        Assert.Equal(0, surface.SurfaceLetter(-1, 30));
        Assert.Equal(0, surface.SurfaceLetter(30, 256));
        Assert.Equal((byte)'H', surface.SurfaceLetter(29, 30));
        ownerMap[30 * 256 + 29] = 0;
        Assert.Equal(0, surface.SurfaceLetter(29, 30));
        raw[10] = (byte)Island;
        ownerMap[30 * 256 + 29] = Surface;
        Assert.Equal(0, surface.SurfaceLetter(29, 30));
    }

    /// <summary>지도·타입·훅 오류는 생성 때 거부한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void RejectsBadMapsAndHooks()
    {
        var (pool, types, frames, surfaceMap, ownerMap, typeState) = Scene();
        IslandPostPopHooks hooks = new();
        Assert.Throws<ArgumentException>(() => new RawIslandPostPop(pool, Array.Empty<ushort>(), types, frames, typeState, new IslandPostPopState(), hooks));
        Assert.Throws<ArgumentException>(() => new RawIslandPostPop(pool, surfaceMap, types[..100], frames, typeState, new IslandPostPopState(), hooks));
        RawSidPool cd = new(Capacity, false, null, OriginalEdition.Cd1072);
        OriginalTypeRecord[] cdTypes = new OriginalTypeRecord[171];
        // 모든 타입 번호에 빈 타입 기록을 채우는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int i = 0; i < cdTypes.Length; ++i)
        {
            cdTypes[i] = new OriginalTypeRecord();
        }

        RiftTypeFrames[] cdFrames = new RiftTypeFrames[cdTypes.Length];
        // 모든 타입 번호마다 빈 프레임 표를 넣는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int i = 0; i < cdFrames.Length; ++i)
        {
            cdFrames[i] = new RiftTypeFrames(Array.Empty<RiftFrameCode>());
        }

        BridgeConnectState badTypes = new() { IslandType = 500u, NoIslandType = NoIsland, StalagType = Stalag };
        Assert.Throws<ArgumentException>(() => new RawIslandPostPop(cd, surfaceMap, cdTypes, cdFrames, badTypes, new IslandPostPopState(), hooks));
        BridgeConnectState cdState = new() { IslandType = Island, NoIslandType = 100u, StalagType = Stalag };
        Assert.Throws<ArgumentException>(() => new RawIslandPostPop(cd, surfaceMap, cdTypes, cdFrames, cdState, new IslandPostPopState(), hooks));
    }

    /// <summary>첫 비트가 없으면 현재 글자와 무관하게 아무 파생 효과를 수행하지 않는다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void PrefixWithoutFirstBitIsNoOp()
    {
        var (pool, types, frames, surfaceMap, ownerMap, typeState) = Scene();
        RawIslandPostPop surface = new(pool, surfaceMap, types, frames, typeState, new IslandPostPopState(), new IslandPostPopHooks(), ownerMap);
        surface.Prefix(Surface, 0u);
        surface.Prefix(Surface, 4u);
    }

    /// <summary>첫 글자가 받침이면 생성·소유자·Pop·연결 순서로 수행한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void PrefixFirstCreatesIslandSupport()
    {
        var (pool, types, frames, surfaceMap, ownerMap, typeState) = Scene();
        types[(int)Island].FootX = 3;
        types[(int)Island].FootY = 3;
        frames[(int)NoIsland] = new RiftTypeFrames(new[] { new RiftFrameCode((byte)'F', (byte)'P', 1, 0) });
        Span<byte> raw = pool.RawSlot(Surface);
        raw.Clear();
        raw[10] = (byte)NoIsland;
        Put(raw, 14, BitConverter.SingleToUInt32Bits(30f), 4);
        Put(raw, 18, BitConverter.SingleToUInt32Bits(30f), 4);
        raw[34] = 3;
        Put(raw, 36, 0u, 4);
        List<string> events = [];
        IslandPostPopHooks hooks = new()
        {
            Objects = new BridgeConnectHooks
            {
                Create = (type, flags) =>
                {
                    events.Add("C:" + type + ":" + flags);
                    Span<byte> born = pool.RawSlot(60);
                    born.Clear();
                    born[10] = (byte)type;
                    return 60;
                },
                SetOwner = (sid, owner) =>
                {
                    events.Add("O:" + sid + ":" + owner);
                    pool.RawSlot(sid)[34] = (byte)owner;
                },
                Pop = (sid, x, y, flags) => events.Add("P:" + sid + ":" + BitConverter.SingleToUInt32Bits(x) + ":" + BitConverter.SingleToUInt32Bits(y) + ":" + flags),
            },
            Connect = sid => events.Add("N:" + sid),
        };
        new RawIslandPostPop(pool, surfaceMap, types, frames, typeState, new IslandPostPopState(), hooks).Prefix(Surface, 1u);
        Assert.Equal(
        [
            "C:94:2",
            "O:60:3",
            "P:60:" + BitConverter.SingleToUInt32Bits(32f) + ":" + BitConverter.SingleToUInt32Bits(32f) + ":0",
            "N:50",
        ], events);
        Assert.Equal(3, pool.Slot(60)[34]);
    }

    /// <summary>3×3 받침·종유석에 소유자와 표시 갱신을 주고 누락은 진단한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void SetSupportOwnerPropagatesAndReports()
    {
        var (pool, types, frames, surfaceMap, ownerMap, typeState) = Scene();
        int sid = Surface;
        // 3×3 칸의 머리 번호를 심는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int y = 28; y <= 30; ++y)
        {
            // 한 행의 세 칸을 심는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
            for (int x = 28; x <= 30; ++x)
            {
                Span<byte> cell = pool.RawSlot(sid);
                cell.Clear();
                cell[10] = (byte)NoIsland;
                surfaceMap[y * 256 + x] = (ushort)sid;
                ownerMap[y * 256 + x] = (ushort)sid;
                ++sid;
            }
        }

        List<string> events = [];
        int updates = 0;
        IslandPostPopHooks hooks = new()
        {
            Objects = new BridgeConnectHooks
            {
                SetOwner = (target, owner) =>
                {
                    events.Add("O:" + target + ":" + owner);
                    pool.RawSlot(target)[34] = (byte)owner;
                },
            },
            FindTypeAt = (x, y, type) => type == Island ? 70 : 71,
            UpdateDisplay = target =>
            {
                events.Add("U:" + target);
                ++updates;
            },
            IncompleteSupport = (cells, island, stalag) => events.Add("D"),
        };
        RawIslandPostPop surface = new(pool, surfaceMap, types, frames, typeState, new IslandPostPopState(), hooks, ownerMap);
        surface.SetSupportOwner(30f, 30f, 5u);
        Assert.Equal(11, updates);
        Assert.DoesNotContain("D", events);
        // 9칸·받침·종유석의 소유자를 확인하는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int check = Surface; check < Surface + 9; ++check)
        {
            Assert.Equal(5, pool.Slot(check)[34]);
        }

        ownerMap[28 * 256 + 28] = 0;
        events.Clear();
        surface.SetSupportOwner(30f, 30f, 5u);
        Assert.Contains("D", events);
    }

    /// <summary>저장 게임 값을 매 칸에서 다시 읽어 전파를 차단하거나 실행한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void SavedGameGateBlocksHPropagation()
    {
        var (pool, types, frames, surfaceMap, ownerMap, typeState) = Scene();
        frames[(int)NoIsland] = new RiftTypeFrames(new[] { new RiftFrameCode((byte)'H', (byte)'P', 1, 0) });
        Span<byte> raw = pool.RawSlot(Surface);
        raw.Clear();
        raw[10] = (byte)NoIsland;
        Put(raw, 14, BitConverter.SingleToUInt32Bits(30f), 4);
        Put(raw, 18, BitConverter.SingleToUInt32Bits(30f), 4);
        raw[34] = 0;
        Put(raw, 36, 0u, 4);
        surfaceMap[28 * 256 + 28] = Surface;
        surfaceMap[30 * 256 + 30] = Surface;
        bool savedGame = true;
        IslandPostPopState state = new() { SavedGameSource = () => savedGame };
        List<string> events = [];
        IslandPostPopHooks hooks = new()
        {
            Objects = new BridgeConnectHooks(),
            Connect = sid => events.Add("N:" + sid),
            FindTypeAt = (x, y, type) => 0,
            UpdateDisplay = sid => events.Add("U:" + sid),
            IncompleteSupport = (cells, island, stalag) => events.Add("D"),
        };
        RawIslandPostPop surface = new(pool, surfaceMap, types, frames, typeState, state, hooks, ownerMap);
        surface.Prefix(Surface, 1u);
        Assert.Equal(["N:50"], events);
        savedGame = false;
        events.Clear();
        surface.Prefix(Surface, 1u);
        Assert.Contains("D", events);
    }
}
