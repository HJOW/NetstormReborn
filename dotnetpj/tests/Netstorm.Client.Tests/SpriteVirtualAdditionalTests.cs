using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 객체 가상표에 따른 추가 그림 경계를 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>원본 빈 함수와 선택된 공장·다리·메타 소비자 분배를 검사한다.</summary>
public sealed class SpriteVirtualAdditionalTests
{
    /// <summary>테스트에서 같은 슬롯의 현재 가상표와 타입을 변경한다.</summary>
    private static void Set(RawSidPool pool, int sid, uint table, byte type, int frame = 0)
    {
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw.Clear();
        BinaryPrimitives.WriteUInt32LittleEndian(raw, table);
        raw[10] = type;
        BinaryPrimitives.WriteInt32LittleEndian(raw[36..], frame);
    }

    /// <summary>빈 함수만 자연 반환하고 실제 다리·메타 장치와 미복원 함수는 구별한다.</summary>
    [Fact]
    public void RoutesOnlyKnownAdditionalFunctions()
    {
        var pool = new RawSidPool(24000, server: false);
        int sid = pool.Allocate(SidAllocation.Client);
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        var calls = new List<(string Kind, int Sid, ScreenPoint Point)>();
        var draw = new SpriteVirtualAdditional(pool, frames, _ => throw new InvalidOperationException("예상하지 않은 그림"),
            (id, point) => calls.Add(("bridge", id, point)), (id, point) => calls.Add(("meta", id, point)));
        var point = new ScreenPoint(12, 34);
        Set(pool, sid, 0x00501dd0, 70);
        draw.Draw(sid, point);
        Assert.Empty(calls);
        Set(pool, sid, 0x005034c8, 82);
        draw.Draw(sid, point);
        Set(pool, sid, 0x0050c9f0, 94);
        draw.Draw(sid, point);
        Assert.Equal(new[] { ("bridge", sid, point), ("meta", sid, point) }, calls);
        Set(pool, sid, 0x00509b98, 130);
        Assert.Contains("0045BCD0", Assert.Throws<InvalidOperationException>(() => draw.Draw(sid, point)).Message);
        var missing = new SpriteVirtualAdditional(pool, frames, _ => { });
        Set(pool, sid, 0x005034c8, 82);
        Assert.Throws<InvalidOperationException>(() => missing.Draw(sid, point));
    }

    /// <summary>마나는 그림자 소비자 뒤 현재 프레임을 다시 읽되 처음 선택한 TYPE을 유지한다.</summary>
    [Fact]
    public void ManaAdditionalRereadsFrameAfterShadowAndKeepsSelectedType()
    {
        var pool = new RawSidPool(24000, server: false);
        int sid = pool.Allocate(SidAllocation.Client);
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        var shapes = new List<SpriteShapeRequest>();
        var draw = new SpriteVirtualAdditional(pool, frames, request =>
        {
            shapes.Add(request);
            if (request.Mode != SpriteShapeMode.Shadow) return;
            BinaryPrimitives.WriteUInt32LittleEndian(pool.AllocatedBytes(sid)[36..], 8);
            pool.AllocatedBytes(sid)[10] = 70;
        });
        Set(pool, sid, 0x0050c3b0, 103, 4);
        BinaryPrimitives.WriteUInt16LittleEndian(pool.AllocatedBytes(sid)[12..], 1);
        draw.Draw(sid, new(int.MaxValue, 4));
        Assert.Equal(new SpriteShapeRequest(103, 4, new(unchecked(int.MaxValue + 10), 14), SpriteShapeMode.Shadow), shapes[0]);
        Assert.Equal(new SpriteShapeRequest(103, 8, new(int.MaxValue, 4), SpriteShapeMode.Plain), shapes[1]);
    }

    /// <summary>마나 볼트는 첫 프레임을 두 번 사용하고 고정 추상 출력은 별도 소유자 관문이 없다.</summary>
    [Fact]
    public void ManaBoltAndAbstractAdditionalUseTheirOriginalModes()
    {
        var pool = new RawSidPool(24000, server: false);
        int sid = pool.Allocate(SidAllocation.Client);
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        var shapes = new List<SpriteShapeRequest>();
        var draw = new SpriteVirtualAdditional(pool, frames, request =>
        {
            shapes.Add(request);
            BinaryPrimitives.WriteUInt32LittleEndian(pool.AllocatedBytes(sid)[36..], 9);
        });
        Set(pool, sid, 0x00509810, 105, 3);
        draw.Draw(sid, new(1, 2));
        Assert.Equal(new SpriteShapeRequest(105, 3, new(11, 12), SpriteShapeMode.Shadow), shapes[0]);
        Assert.Equal(new SpriteShapeRequest(105, 3, new(1, 2), SpriteShapeMode.Plain), shapes[1]);
        Set(pool, sid, 0x0050db78, 122, 4);
        draw.Draw(sid, new(5, 6));
        Assert.Equal(new SpriteShapeRequest(122, 4, new(5, 6), SpriteShapeMode.Abstract), shapes[2]);
        Set(pool, sid, 0x00509810, 104, 3);
        Assert.Throws<ArgumentException>(() => draw.Draw(sid, new(1, 2)));
    }

    /// <summary>세 전용 추가 그림이 현재 프레임과 소유자별 상태색을 사용한다.</summary>
    [Theory]
    [InlineData(0x0050da88u, (byte)122)]
    [InlineData(0x0050bad0u, (byte)131)]
    [InlineData(0x00509930u, (byte)72)]
    public void CurrentOverlaysUseActualFrameAndOwner(uint table, byte type)
    {
        var pool = new RawSidPool(24000, server: false);
        int sid = pool.Allocate(SidAllocation.Client);
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        var shapes = new List<SpriteShapeRequest>();
        uint owner = 3;
        var draw = new SpriteVirtualAdditional(pool, frames, shapes.Add, localOwner: () => owner);
        var point = new ScreenPoint(7, 8);
        Set(pool, sid, table, type, 15);
        draw.Draw(sid, point);
        Assert.Equal(new SpriteShapeRequest(type, 15, point, SpriteShapeMode.Plain), Assert.Single(shapes));
        pool.AllocatedBytes(sid)[40] = 0x10;
        pool.AllocatedBytes(sid)[34] = 3;
        draw.Draw(sid, point);
        Assert.Equal(new SpriteShapeRequest(type, 15, point, SpriteShapeMode.Abstract), shapes[1]);
        owner = 4;
        draw.Draw(sid, point);
        Assert.Equal(2, shapes.Count);
        pool.AllocatedBytes(sid)[10] = 70;
        Assert.Throws<ArgumentException>(() => draw.Draw(sid, point));
    }

    /// <summary>훨리베이스의 1·8 상태 비트는 로컬 소유자 조회 전에 추가 그림을 억제한다.</summary>
    [Theory]
    [InlineData((byte)1)]
    [InlineData((byte)8)]
    [InlineData((byte)0x19)]
    public void AviarySuppressesExtraForEitherStateBit(byte flags)
    {
        var pool = new RawSidPool(24000, server: false);
        int sid = pool.Allocate(SidAllocation.Client);
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        var draw = new SpriteVirtualAdditional(pool, frames, _ => throw new InvalidOperationException("예상하지 않은 그림"));
        Set(pool, sid, 0x00509930, 72, 6);
        pool.AllocatedBytes(sid)[40] = flags;
        draw.Draw(sid, new(1, 2));
    }

    /// <summary>제단은 별도 TYPE 169의 현재 프레임 수와 소유자 상태를 사용한다.</summary>
    [Fact]
    public void DaisUsesCurrentExtraShapeFrameCountAndOwner()
    {
        var pool = new RawSidPool(24000, server: false);
        int sid = pool.Allocate(SidAllocation.Client);
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        var assets = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray();
        assets[165].FrameCount = 99;
        assets[169].FrameCount = 4;
        var requests = new List<SpriteShapeRequest>();
        uint owner = 2;
        var draw = new SpriteVirtualAdditional(pool, frames, requests.Add, localOwner: () => owner, shapes: assets);
        Set(pool, sid, 0x00507950, 165, 11);
        draw.Draw(sid, new(3, 4));
        Assert.Equal(new SpriteShapeRequest(169, 3, new(3, 4), SpriteShapeMode.Plain), Assert.Single(requests));
        assets[169].FrameCount = 5;
        pool.AllocatedBytes(sid)[40] = 0x10;
        pool.AllocatedBytes(sid)[34] = 2;
        draw.Draw(sid, new(3, 4));
        Assert.Equal(new SpriteShapeRequest(169, 1, new(3, 4), SpriteShapeMode.Abstract), requests[1]);
        owner = 1;
        draw.Draw(sid, new(3, 4));
        Assert.Equal(2, requests.Count);
        BinaryPrimitives.WriteInt32LittleEndian(pool.AllocatedBytes(sid)[36..], -1);
        Assert.Throws<ArgumentOutOfRangeException>(() => draw.Draw(sid, new(3, 4)));
        assets[169].FrameCount = 0;
        Assert.Throws<InvalidOperationException>(() => draw.Draw(sid, new(3, 4)));
        var missing = new SpriteVirtualAdditional(pool, frames, requests.Add);
        Assert.Throws<InvalidOperationException>(() => missing.Draw(sid, new(3, 4)));
        pool.AllocatedBytes(sid)[10] = 166;
        Assert.Throws<ArgumentException>(() => draw.Draw(sid, new(3, 4)));
    }

    /// <summary>메타 표시의 숨김 비트와 현재 정수·분수 배율을 실제 그림 요청에 전달한다.</summary>
    [Fact]
    public void MetaUsesCurrentFixedScaleAndVisibility()
    {
        var pool = new RawSidPool(24000, server: false);
        int sid = pool.Allocate(SidAllocation.Client);
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        var requests = new List<SpriteShapeRequest>();
        var draw = new SpriteVirtualAdditional(pool, frames, requests.Add);
        Set(pool, sid, 0x0050cc08, 94, 7);
        draw.Draw(sid, new(1, 2));
        Assert.Equal(new SpriteShapeRequest(94, 7, new(1, 2), SpriteShapeMode.Plain)
            { Scaled = true, ScaleX = 0x10000, ScaleY = 0x10000 }, Assert.Single(requests));
        BinaryPrimitives.WriteUInt32LittleEndian(pool.AllocatedBytes(sid)[26..], 2);
        BinaryPrimitives.WriteUInt16LittleEndian(pool.AllocatedBytes(sid)[8..], 0x8000);
        draw.Draw(sid, new(1, 2));
        Assert.Equal(0x28000, requests[1].ScaleX);
        Assert.Equal(requests[1].ScaleX, requests[1].ScaleY);
        BinaryPrimitives.WriteUInt32LittleEndian(pool.AllocatedBytes(sid)[26..], 0x4000);
        draw.Draw(sid, new(1, 2));
        Assert.Equal(2, requests.Count);
    }

    /// <summary>발사기는 실제 포구 좌표와 현재 출력 비트·소유자 상태를 각 그림에서 사용한다.</summary>
    [Fact]
    public void BurningArcherUsesCurrentMuzzleAndOutputState()
    {
        var pool = new RawSidPool(24000, server: false);
        int sid = pool.Allocate(SidAllocation.Client);
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        var assets = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray();
        assets[71].Frames.Add(new SquidDisplayFrame());
        assets[71].Frames.Add(new SquidDisplayFrame { MuzzleX = 3, MuzzleY = -4 });
        var definitions = Enumerable.Range(0, 118).Select(_ => new RiftTypeDefinition()).ToArray();
        definitions[1].SpecialFrames.BaseFrame = 0;
        var requests = new List<SpriteShapeRequest>();
        var draw = new SpriteVirtualAdditional(pool, frames, request =>
        {
            requests.Add(request);
            if (requests.Count == 1) { pool.AllocatedBytes(sid)[40] = 0x10; pool.AllocatedBytes(sid)[34] = 2; }
        }, definitions: definitions, localOwner: () => 2, shapes: assets);
        Set(pool, sid, 0x0050b888, 71, 1);
        draw.Draw(sid, new(10, 20));
        Assert.Equal(new SpriteShapeRequest(71, 1, new(10, 20), SpriteShapeMode.Plain), requests[0]);
        Assert.Equal(new SpriteShapeRequest(83, 0, new(13, 16), SpriteShapeMode.Abstract), requests[1]);
        requests.Clear();
        Set(pool, sid, 0x0050b888, 71, 0);
        draw.Draw(sid, new(10, 20));
        Assert.Empty(requests);
        Set(pool, sid, 0x0050b888, 71, 1);
        pool.AllocatedBytes(sid)[40] = 1;
        draw.Draw(sid, new(10, 20));
        Assert.Single(requests);
        requests.Clear();
        Set(pool, sid, 0x0050b888, 71, 1);
        pool.AllocatedBytes(sid)[12] = 1;
        draw.Draw(sid, new(10, 20));
        Assert.Single(requests);
        Set(pool, sid, 0x0050b888, 70, 1);
        Assert.Throws<ArgumentException>(() => draw.Draw(sid, new(10, 20)));
    }

    /// <summary>세 신전이 현재 프레임과 상승 구간을 사용하고 레인 신전만 조건부 상승음을 요청한다.</summary>
    [Fact]
    public void TempleFramesUseCurrentSlotAndRainAscendSound()
    {
        var pool = new RawSidPool(24000, server: false);
        int sid = pool.Allocate(SidAllocation.Client);
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        frames[90] = new([new((byte)'A', 0, 0, 0), new((byte)'A', 0, 1, 0), new((byte)'C', 0, 0, 0), new((byte)'C', 0, 1, 0)]);
        var definitions = Enumerable.Range(0, 118).Select(_ => new RiftTypeDefinition()).ToArray();
        definitions[20].SpecialFrames.BaseFrame = 1;
        var requests = new List<SpriteShapeRequest>();
        int sounds = 0;
        var draw = new SpriteVirtualAdditional(pool, frames, requests.Add, definitions: definitions, templeAscend: () => sounds++);
        Set(pool, sid, 0x005196f0, 90, 0);
        BinaryPrimitives.WriteUInt16LittleEndian(pool.AllocatedBytes(sid)[12..], 1);
        draw.Draw(sid, new(3, 4));
        Assert.Equal(new uint[] { 0, 0, 2 }, requests.Select(request => request.Frame));
        Assert.Equal(1, sounds);
        requests.Clear();
        Set(pool, sid, 0x00519860, 90, 1);
        BinaryPrimitives.WriteUInt16LittleEndian(pool.AllocatedBytes(sid)[12..], 1);
        draw.Draw(sid, new(3, 4));
        Assert.Equal(new uint[] { 3 }, requests.Select(request => request.Frame));
        requests.Clear();
        Set(pool, sid, 0x00519918, 90, 1);
        BinaryPrimitives.WriteUInt16LittleEndian(pool.AllocatedBytes(sid)[12..], 1);
        draw.Draw(sid, new(3, 4));
        Assert.Equal(new uint[] { 1, 3 }, requests.Select(request => request.Frame));
        Assert.Equal(1, sounds);
        var missingSound = new SpriteVirtualAdditional(pool, frames, _ => { });
        Set(pool, sid, 0x005196f0, 90, 0);
        BinaryPrimitives.WriteUInt16LittleEndian(pool.AllocatedBytes(sid)[12..], 1);
        Assert.Throws<InvalidOperationException>(() => missingSound.Draw(sid, new(3, 4)));
    }

    /// <summary>신전 첫 그림 뒤 변경된 프레임과 소유자를 후속 그림에서 다시 읽는다.</summary>
    [Fact]
    public void TempleRereadsSlotAfterEachFrame()
    {
        var pool = new RawSidPool(24000, server: false);
        int sid = pool.Allocate(SidAllocation.Client);
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        frames[90] = new([new((byte)'A', 0, 0, 0), new((byte)'A', 0, 1, 0), new((byte)'C', 0, 0, 0), new((byte)'C', 0, 1, 0)]);
        var requests = new List<SpriteShapeRequest>();
        var draw = new SpriteVirtualAdditional(pool, frames, request =>
        {
            requests.Add(request);
            if (requests.Count != 1) return;
            BinaryPrimitives.WriteInt32LittleEndian(pool.AllocatedBytes(sid)[36..], 1);
            pool.AllocatedBytes(sid)[40] = 0x10;
            pool.AllocatedBytes(sid)[34] = 2;
        }, localOwner: () => 2);
        Set(pool, sid, 0x005196f0, 90, 0);
        BinaryPrimitives.WriteUInt16LittleEndian(pool.AllocatedBytes(sid)[12..], 1);
        draw.Draw(sid, new(3, 4));
        Assert.Equal(new uint[] { 0, 1, 3 }, requests.Select(request => request.Frame));
        Assert.Equal(new[] { SpriteShapeMode.Plain, SpriteShapeMode.Abstract, SpriteShapeMode.Abstract }, requests.Select(request => request.Mode));
    }

    /// <summary>주문의 도움말 프레임과 추상 소유자 조건을 실제 현재 슬롯으로 분기한다.</summary>
    [Fact]
    public void SpellAdditionalSuppressesHelpAndUsesCurrentOwner()
    {
        var pool = new RawSidPool(24000, server: false);
        int sid = pool.Allocate(SidAllocation.Client);
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        var types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        var definitions = Enumerable.Range(0, 118).Select(_ => new RiftTypeDefinition()).ToArray();
        types[117].Flags2 = TypeFlag2.Bomb;
        definitions[47].SpecialFrames.HelpFrame = 10;
        var shapes = new List<SpriteShapeRequest>();
        uint localOwner = 2;
        var draw = new SpriteVirtualAdditional(pool, frames, shapes.Add, types: types, definitions: definitions, localOwner: () => localOwner);
        Set(pool, sid, 0x00502370, 117, 10);
        draw.Draw(sid, new(7, 8));
        Assert.Empty(shapes);
        Set(pool, sid, 0x00502370, 117, 11);
        draw.Draw(sid, new(7, 8));
        Assert.Equal(new SpriteShapeRequest(117, 11, new(7, 8), SpriteShapeMode.Plain), Assert.Single(shapes));
        pool.AllocatedBytes(sid)[40] = 0x10;
        pool.AllocatedBytes(sid)[34] = 2;
        draw.Draw(sid, new(7, 8));
        Assert.Equal(SpriteShapeMode.Abstract, shapes[1].Mode);
        localOwner = 1;
        draw.Draw(sid, new(7, 8));
        Assert.Equal(2, shapes.Count);
        types[117].Flags2 = 0;
        pool.AllocatedBytes(sid)[40] = 0;
        BinaryPrimitives.WriteInt32LittleEndian(pool.AllocatedBytes(sid)[36..], 10);
        draw.Draw(sid, new(7, 8));
        Assert.Equal(3, shapes.Count);
    }

    /// <summary>공장 본체의 실제 억제와 현재 프레임을 같은 픽셀 그림 소비자에 전달한다.</summary>
    [Fact]
    public void FactoryAdditionalUsesActualCurrentFrameAndSuppression()
    {
        var pool = new RawSidPool(24000, server: false);
        int sid = pool.Allocate(SidAllocation.Client);
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        frames[85] = new([new((byte)'A', 0, 0, 0), new((byte)'A', 0, 1, 0)]);
        var shapes = new List<SpriteShapeRequest>();
        var draw = new SpriteVirtualAdditional(pool, frames, shapes.Add);
        Set(pool, sid, 0x00509148, 85, 1);
        draw.Draw(sid, new(7, 8));
        Assert.Equal(new SpriteShapeRequest(85, 1, new(7, 8), SpriteShapeMode.Plain), Assert.Single(shapes));
        pool.AllocatedBytes(sid)[40] = 1;
        draw.Draw(sid, new(7, 8));
        Assert.Single(shapes);
        Set(pool, sid, 0x00509148, 85, -2);
        Assert.Throws<ArgumentOutOfRangeException>(() => draw.Draw(sid, new(7, 8)));
    }
}
