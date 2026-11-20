using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 메뉴 전 자산 준비와 후속 Session의 동일 물리 표 대여를 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>게임 창 없이 실제 TYPE/SHP 입력과 장기 저장소의 수명 경계를 검증한다.</summary>
public sealed class HostAssetTablesTests
{
    /// <summary>읽기 전용 실제 자료와 시작 설정을 준비한다. 옵션 저장은 호출하지 않는다.</summary>
    private static HostStartupScene Startup()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        return new HostStartupScene(data);
    }

    /// <summary>미션/Player 없이 최초 표와 코드 사본·실패 좌표를 준비하며 풀 슬롯은 보존한다.</summary>
    [Fact]
    public void PreparesActualTablesBeforeAnySession()
    {
        var storage = new RawSurfaceStorage(); byte[] pool = storage.Pool.Bytes.ToArray();
        var assets = new HostAssetTables(Startup(), storage);
        Assert.Same(assets, storage.Source()); Assert.Same(assets.TypeTable.Types, assets.LoadedTypes);
        Assert.Equal(188, storage.Types.Count); Assert.Equal(188, storage.Frames.Count); Assert.Equal(188, storage.Shapes.Count); Assert.Equal(188, storage.Codes.Count);
        Assert.Same(storage.Frames.PhysicalArray, assets.Frames); Assert.Same(storage.Shapes, assets.Shapes);
        Assert.Equal(pool, storage.Pool.Bytes.ToArray()); Assert.Equal(-10000f, storage.BoardTargetState.Invalid.X); Assert.Equal(-10000f, storage.BoardTargetState.Invalid.Y);
        // 모든 실제 자산 프레임은 파일 선언 순서와 같고 현재 TYPE는 최초 TYPE의 독립 값이다.
        for (int index = 0; index < assets.Definitions.Count; index++)
        {
            int type = RiftTypeTable.FirstAssetTypeNumber + index;
            Assert.NotSame(assets.LoadedTypes[type], storage.Types[type]);
            Assert.Equal(assets.Definitions[index].Clusters.Select(cluster => cluster.Code), assets.Frames[type].Codes);
            Assert.Equal(assets.Frames[type].Codes, storage.Codes[type]);
        }
        float cost = assets.LoadedTypes[70].Cost; storage.Types[70].Cost = cost + 37; Assert.Equal(cost, assets.LoadedTypes[70].Cost);
        Assert.Throws<InvalidOperationException>(() => new HostAssetTables(Startup(), storage)); Assert.Equal(cost + 37, storage.Types[70].Cost);
    }

    /// <summary>연속 Session이 같은 배열과 변경된 슬롯을 빌리며 세션 재적재로 장기 표를 덮지 않는다.</summary>
    [Fact]
    public void SequentialSessionsBorrowSameCurrentArray()
    {
        var storage = new RawSurfaceStorage(); var assets = new HostAssetTables(Startup(), storage);
        var first = new HostSession(new Kernel(), new GameRandom(1), new CrtRandom(), sharedWorldFrames: assets.Frames);
        Assert.Same(assets.Frames, first.WorldFrames);
        var replacement = new RiftTypeFrames([new RiftFrameCode((byte)'X', (byte)'Z', 3, 4)]);
        storage.Frames[70] = replacement; Assert.Same(replacement, first.WorldFrames[70]);
        Assert.Throws<InvalidOperationException>(() => first.FillWorldFrames(assets.Definitions)); Assert.Same(replacement, storage.Frames[70]);
        first.ReleasePlayerRecordViews();
        var next = new HostSession(new Kernel(), new GameRandom(2), new CrtRandom(), sharedWorldFrames: assets.Frames);
        Assert.Same(first.WorldFrames, next.WorldFrames); Assert.Same(replacement, next.WorldFrames[70]); next.ReleasePlayerRecordViews();
    }

    /// <summary>잘못된 장기 표 폭/누락은 세션의 Player 초기화보다 먼저 거부한다.</summary>
    [Theory]
    [InlineData(187)]
    [InlineData(188)]
    [InlineData(189)]
    public void RejectsInvalidBorrowedTables(int count)
    {
        var frames = count == 188 ? new RiftTypeFrames[count] : WorldFrameTables.Create([], count);
        Assert.Throws<ArgumentException>(() => new HostSession(new Kernel(), new GameRandom(1), new CrtRandom(), sharedWorldFrames: frames));
    }

    /// <summary>빈 입력도 최초 설치 완료로 구분하고 두 번째 설치를 거부한다.</summary>
    [Fact]
    public void EmptyInitializationCannotBeRepeated()
    {
        var storage = new RawSurfaceStorage(); storage.InitializeAssetTables([], [], []);
        Assert.Throws<InvalidOperationException>(() => storage.InitializeAssetTables([], [], [])); Assert.Null(storage.Source());
    }

    /// <summary>크기 불일치로 실패한 입력은 현재 자산/실패 좌표를 바꾸지 않는다.</summary>
    [Fact]
    public void MismatchFailsBeforeAssetMutation()
    {
        var storage = new RawSurfaceStorage(); RawPathPoint invalid = storage.BoardTargetState.Invalid;
        Assert.Throws<ArgumentException>(() => storage.InitializeAssetTables([new OriginalTypeRecord()], [], []));
        Assert.Empty(storage.Types); Assert.Empty(storage.Frames); Assert.Empty(storage.Shapes); Assert.Empty(storage.Codes); Assert.Null(storage.Source());
        Assert.Equal(invalid.X, storage.BoardTargetState.Invalid.X); Assert.Equal(invalid.Y, storage.BoardTargetState.Invalid.Y);
        storage.InitializeAssetTables([], [], []);
    }

    /// <summary>같은 최초 TYPE 표로 미션을 풀어 기존 판독 결과와 최초 이미지가 유지되는지 확인한다.</summary>
    [Fact]
    public void MissionReaderUsesPreparedOriginalTypeTable()
    {
        HostStartupScene startup = Startup(); var assets = new HostAssetTables(startup, new RawSurfaceStorage());
        MissionFortLoadResult shared = MissionFortLoad.Load(startup, "thewarbegins", assets.TypeTable);
        MissionFortLoadResult independent = MissionFortLoad.Load(startup, "thewarbegins");
        Assert.Equal(independent.FortName, shared.FortName); Assert.Equal(independent.FortPath, shared.FortPath); Assert.Equal(independent.MissionType, shared.MissionType);
        Assert.NotNull(shared.SourceImage); Assert.NotNull(independent.SourceImage); Assert.Equal(independent.SourceImage.Image, shared.SourceImage.Image);
        Assert.Equal(independent.Fort.Flag, shared.Fort.Flag); Assert.Equal(independent.Fort.TrailingBytes, shared.Fort.TrailingBytes);
        Assert.Equal(independent.Fort.TypeHashes, shared.Fort.TypeHashes);
    }
}
