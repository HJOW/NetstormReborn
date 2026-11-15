using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 현재 거주 구역/간헐천 부모의 부분 효과와 실제 현재 finder 순서를 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>CPP의 AssetParentsCurrent 계약을 동일한 실제 공통/Carrier/가상 분배에 연결한다.</summary>
public sealed class AssetParentsCurrentTests
{
    /// <summary>부모보다 오래 사는 같은 슬롯·장부·입력과 효과 관찰이다.</summary>
    private sealed class Scene
    {
        /// <summary>명시 원문 번호50/60~63을 쓰는 실제 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, false);
        /// <summary>콜백이 바꾼 현재 TYPE를 실제 본문이 다시 읽는다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord { FootX = 1, FootY = 1 }).ToArray();
        /// <summary>큰 게임 효과 대신 실제 공통 깊이 감소만 선택한다.</summary>
        public readonly RawPostPopState Book = new() { Suppressed = true };
        /// <summary>공유 상태와 목록이다.</summary>
        public readonly AssetPostPopState State = new();
        /// <summary>실제 장치가 호출된 원문 위치의 순서다.</summary>
        public readonly List<string> Events = [];
        /// <summary>같은 실제 공통 후처리다.</summary>
        public readonly RawPostPop Base;
        /// <summary>간헐천이 마지막에 직접 호출하는 실제 Damageable다.</summary>
        public readonly RawCarrierPostPop Damageable;

        /// <summary>실제 입력 번호/물리와 공통 부모를 구성한다.</summary>
        public Scene(bool residence = false)
        {
            // 원본 관측의 번호와 같은 실제 할당 입력이다.
            for (int sid = 5; sid <= 63; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
            Types[70].Cost = 1000; State.Geysers.Entries = new uint[4]; State.Residences.Entries = new uint[4];
            Span<byte> raw = Pool.RawSlot(50); raw.Clear(); BinaryPrimitives.WriteUInt32LittleEndian(raw, residence ? 0x50f540u : 0x50da88u); raw[10] = 70; raw[34] = 1;
            BinaryPrimitives.WriteUInt16LittleEndian(raw[12..], 0xabcd); BinaryPrimitives.WriteSingleLittleEndian(raw[14..], 50); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], 52); BinaryPrimitives.WriteUInt32LittleEndian(raw[36..], 25);
            // 두 실제 후보는 각각 섬 받침/종유석이며 frame0에서 시작한다.
            foreach (int sid in new[] { 60, 61 }) { Pool.RawSlot(sid).Clear(); Pool.RawSlot(sid)[10] = (byte)(sid == 60 ? 94 : 95); Pool.RawSlot(sid)[34] = 2; }
            Base = new(Pool, Types, Book);
            Damageable = new(Pool, Types, new() { Boss = true }, new() { Base = (sid, flags) => { Events.Add("D" + flags); Base.PostPopBase(sid, flags); } });
        }
        /// <summary>원문 선택 위치만 관찰하며 각 예약은 명시 실패 입력이다.</summary>
        public AssetPostPopHooks Hooks() => new()
        {
            Surface = (_, _) => 60, SetOwner = (sid, owner) => { Events.Add("O"); Pool.RawSlot(sid)[34] = (byte)owner; },
            GeyserRegistration = (_, _) => Events.Add("G"), ReserveRegular = () => { Events.Add("A"); return false; },
            NotifySurface = _ => Events.Add("N"), AtPosition = (_, _) => [60, 61],
            Unpop = (sid, _) => Events.Add("U" + sid), Repop = (sid, _) => Events.Add("V" + sid),
        };
        /// <summary>명시 hook 사본을 같은 실제 부모들에 연결한다.</summary>
        public RawAssetPostPop Body(AssetPostPopHooks hooks) => new(Pool, Types, Base, Damageable, State, hooks);
        /// <summary>실제 가상 진입의 깊이 증가와 현재 VT 분배를 실행한다.</summary>
        public void Run(RawAssetPostPop body, uint flags = 1) { Book.Depth++; Base.SetAssetPostPop(body); Base.PostPopVirtual(50, flags); }
        /// <summary>실제 현재 WORD/프레임 값을 읽는다.</summary>
        public uint Read(int sid, int offset, bool word = false) => word ? BinaryPrimitives.ReadUInt16LittleEndian(Pool.Slot(sid)[offset..]) : BinaryPrimitives.ReadUInt32LittleEndian(Pool.Slot(sid)[offset..]);
    }

    /// <summary>최초가 아니면 잘못된 현재 TYPE·연결 안 된 앞단 장치도 읽지 않는다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void NoFirstFlagSkipsUnboundFront(bool residence)
    {
        var scene = new Scene(residence); scene.Pool.RawSlot(50)[10] = 255; scene.State.Temples.Count = 99; scene.Run(scene.Body(new()), 0);
        Assert.Equal(0u, scene.Book.Depth); Assert.True(scene.State.Residences.Items().IsEmpty); Assert.True(scene.State.Geysers.Items().IsEmpty); Assert.Equal(residence ? Array.Empty<string>() : new[] { "D0" }, scene.Events);
    }
    /// <summary>공유 간헐천 목록 추가 뒤 실제 등록 장치 누락에서 멈춘다.</summary>
    [Fact]
    public void MissingRegistrationRetainsSharedListBeforeFailure()
    {
        var scene = new Scene(); var shared = new RawPostPopList { Entries = new uint[4] }; scene.State.SharedGeysers = shared; var hooks = scene.Hooks(); hooks.GeyserRegistration = null;
        Assert.Throws<InvalidOperationException>(() => scene.Run(scene.Body(hooks))); Assert.Equal(1u, shared.Count); Assert.Equal(50u, shared.Entries[0]); Assert.Equal(0u, scene.State.Geysers.Count); Assert.Equal(1u, scene.Book.Depth); Assert.Equal(0xabcdu, scene.Read(50, 12, true));
    }
    /// <summary>signed 개수/용량 관문에서 가득 찬 목록은 물리를 읽지 않고 음수 저장은 그 앞에서 실패한다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void SharedListChecksSignedBoundaryBeforeStorage(bool negative)
    {
        var scene = new Scene(); var list = new RawPostPopList(); uint count = negative ? uint.MaxValue : 4;
        list.BindCurrent(new() { Storage = () => throw new Exception("미도달 목록 물리"), Capacity = () => 4, ReadCount = () => count, WriteCount = value => count = value }); scene.State.SharedGeysers = list;
        var hooks = scene.Hooks(); hooks.AtPosition = (_, _) => [];
        if (negative) { Assert.Throws<ArgumentOutOfRangeException>(() => scene.Run(scene.Body(hooks))); Assert.Empty(scene.Events); Assert.Equal(1u, scene.Book.Depth); }
        else { scene.Run(scene.Body(hooks)); Assert.Equal(new[] { "G", "A", "D1" }, scene.Events); Assert.Equal(0u, scene.Book.Depth); }
        Assert.Equal(negative ? uint.MaxValue : 4, count);
    }
    /// <summary>extra9는 목록/등록/예약/주변 장치를 생략하고 실제 Damageable만 호출한다.</summary>
    [Theory]
    [InlineData(1)]
    [InlineData(8)]
    [InlineData(9)]
    public void GeyserExtraSkipsUnboundFront(byte extra)
    {
        var scene = new Scene(); scene.Pool.RawSlot(50)[40] = extra; scene.Run(scene.Body(new())); Assert.Equal(new[] { "D1" }, scene.Events); Assert.Equal(0u, scene.Book.Depth);
    }
    /// <summary>등록이 바꾼 현재 TYPE 비용을 읽고 확보 실패 뒤에도 다음 실제 장치로 진행한다.</summary>
    [Fact]
    public void RegistrationChangesCurrentTypeCostBeforeWord()
    {
        var scene = new Scene(); var hooks = scene.Hooks(); hooks.GeyserRegistration = (sid, _) => { scene.Events.Add("G"); scene.Pool.RawSlot(sid)[10] = 75; scene.Types[75].Cost = 513.875f; };
        hooks.AtPosition = (_, _) => { scene.Events.Add("P"); return []; }; hooks.AddRegular = null; hooks.Unpop = null; hooks.Repop = null;
        scene.Run(scene.Body(hooks)); Assert.Equal(513u, scene.Read(50, 12, true)); Assert.Equal(new[] { "G", "A", "P", "D1" }, scene.Events); Assert.Equal(0u, scene.Book.Depth);
    }
    /// <summary>등록 뒤 잘못된 TYPE는 그때 실패하고 앞선 목록/등록을 유지한다.</summary>
    [Fact]
    public void BadCurrentTypeFailsAfterRegistration()
    {
        var scene = new Scene(); var hooks = scene.Hooks(); hooks.GeyserRegistration = (sid, _) => { scene.Events.Add("G"); scene.Pool.RawSlot(sid)[10] = 255; };
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Run(scene.Body(hooks))); Assert.Equal(1u, scene.State.Geysers.Count); Assert.Equal(new[] { "G" }, scene.Events); Assert.Equal(1u, scene.Book.Depth);
    }
    /// <summary>이웃 해제가 free로 바꿔도 물리 frame8을 쓴 뒤 미연결 Repop에서 실패한다.</summary>
    [Fact]
    public void MissingRepopKeepsUnpopAndPhysicalFrame()
    {
        var scene = new Scene(); var hooks = scene.Hooks(); hooks.Unpop = (sid, _) => { scene.Events.Add("U" + sid); scene.Pool.RawSlot(sid)[11] = 1; }; hooks.Repop = null;
        Assert.Throws<InvalidOperationException>(() => scene.Run(scene.Body(hooks))); Assert.Equal(8u, scene.Read(60, 36)); Assert.Equal(1, scene.Pool.Slot(60)[11]); Assert.Equal(0u, scene.Read(61, 36)); Assert.Equal("U60", scene.Events[^1]); Assert.Equal(1u, scene.Book.Depth);
    }
    /// <summary>WORD 저장 뒤 현재 다리 통지와 그 효과 뒤 새 좌표를 읽는다.</summary>
    [Fact]
    public void WordSetterReadsCurrentBridgeAndThenCurrentCoordinates()
    {
        var scene = new Scene(); scene.State.SharedBridgeType = () => 70; var hooks = scene.Hooks();
        hooks.NotifySurface = sid => { Assert.Equal(1000u, scene.Read(sid, 12, true)); scene.Events.Add("N"); scene.Pool.RawSlot(sid)[10] = 75; BinaryPrimitives.WriteSingleLittleEndian(scene.Pool.RawSlot(sid)[14..], 40); };
        hooks.AtPosition = (x, y) => { Assert.Equal(40, x); Assert.Equal(52, y); scene.Events.Add("P"); return []; };
        scene.Run(scene.Body(hooks)); Assert.Equal(new[] { "G", "N", "A", "P", "D1" }, scene.Events); Assert.Equal(1000u, scene.Read(50, 12, true));
    }
    /// <summary>선택된 다리 통지가 없으면 WORD까지의 완료 효과를 보존하고 예약 전에 멈춘다.</summary>
    [Fact]
    public void MissingBridgeNoticeRetainsWordAndList()
    {
        var scene = new Scene(); scene.State.BridgeType = 70; var hooks = scene.Hooks(); hooks.NotifySurface = null;
        Assert.Throws<InvalidOperationException>(() => scene.Run(scene.Body(hooks))); Assert.Equal(1000u, scene.Read(50, 12, true)); Assert.Equal(1u, scene.State.Geysers.Count); Assert.Equal(new[] { "G" }, scene.Events);
    }
    /// <summary>실제 finder는 각 효과 뒤 현재 다음 링크를 읽어60→61→63으로 진행한다.</summary>
    [Fact]
    public void LiveFinderAlternatesEffectsAndCurrentNext()
    {
        var scene = new Scene(); var hash = new RawSquidHash(); hash.Cell(1, 25, 26) = 60;
        // 원문 입력과 같은 좌표/발자국과 초기60→61→62 체인이다.
        foreach (int sid in new[] { 60, 61, 62, 63 })
        {
            Span<byte> raw = scene.Pool.RawSlot(sid); raw.Clear(); raw[10] = (byte)(sid == 60 ? 94 : 95); BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], (ushort)(sid == 60 ? 61 : sid == 61 ? 62 : 0)); BinaryPrimitives.WriteSingleLittleEndian(raw[14..], 50); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], 52);
        }
        var hooks = scene.Hooks(); hooks.VisitAtPosition = RawAssetPostPop.MakeAssetPostPopFinder(scene.Pool, hash, scene.Types); hooks.AtPosition = null;
        hooks.Unpop = (sid, _) => { scene.Events.Add("U" + sid); if (sid == 60) BinaryPrimitives.WriteUInt16LittleEndian(scene.Pool.RawSlot(61)[4..], 63); };
        scene.Run(scene.Body(hooks)); Assert.Equal(new[] { "G", "A", "U60", "V60", "U61", "V61", "U63", "V63", "D1" }, scene.Events); Assert.Equal(0u, scene.Read(62, 36)); Assert.Equal(0u, scene.Book.Depth);
    }
    /// <summary>Native 변환만 비유한/범위 밖을 원문 indefinite low DWORD0으로 저장한다.</summary>
    [Theory]
    [InlineData(float.NaN)]
    [InlineData(float.PositiveInfinity)]
    [InlineData(float.NegativeInfinity)]
    [InlineData(float.MaxValue)]
    public void NativeCostIndefinitePreservesZeroLowWord(float cost)
    {
        var scene = new Scene(); scene.Types[70].Cost = cost; var hooks = scene.Hooks(); hooks.AtPosition = (_, _) => []; var body = scene.Body(hooks); scene.Book.Depth = 1; body.GeyserPostPop(50, 1);
        Assert.Equal(0u, scene.Read(50, 12, true)); Assert.Equal(0u, scene.Book.Depth); Assert.Equal(new[] { "G", "A", "D1" }, scene.Events);
    }
    /// <summary>거주 구역은 extra9/다른 목록과 무관하게 세 독립 확보 실패 뒤 현재 통지/프레임을 처리한다.</summary>
    [Fact]
    public void ResidenceIndependentAllocationFailuresKeepCurrentTail()
    {
        var scene = new Scene(true); scene.Pool.RawSlot(50)[40] = 9; scene.State.Temples.Count = 99; scene.State.ResidenceAnimation = true; var hooks = scene.Hooks(); hooks.Surface = (_, _) => 0; hooks.SetOwner = null; hooks.AddRegular = null;
        hooks.NotifySurface = sid => { scene.Events.Add("N"); scene.Pool.RawSlot(sid)[11] = 9; BinaryPrimitives.WriteUInt32LittleEndian(scene.Pool.RawSlot(sid)[36..], 17); };
        scene.Run(scene.Body(hooks)); Assert.Equal(new[] { "A", "A", "A", "N" }, scene.Events); Assert.Equal(4u, scene.Read(50, 42)); Assert.Equal(1u, scene.State.Residences.Count); Assert.Equal(0u, scene.Book.Depth);
    }
}
