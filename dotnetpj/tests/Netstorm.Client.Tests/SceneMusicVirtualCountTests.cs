using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Audio;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 현재VT 세 분배·실제 자산 genus·동일 Surface 조립을 검증하는 집중 검사다.
namespace Netstorm.Client.Tests;

/// <summary>가상Count 반환값을 주입하지 않고 실제 raw/해시/현재 TYPE·코드를 실행한다.</summary>
public sealed class SceneMusicVirtualCountTests
{
    /// <summary>시각을 읽지 않는 같은 Client Surface와 명시 raw 입력의 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>현재 same Client 권한/Player/전역이다.</summary>
        public readonly ClientGameplayGlobals Game = new();
        /// <summary>실제 물리·현재 표와 해시를 소유한다.</summary>
        public readonly ClientSurfaceGlobals Surface;
        /// <summary>검사 대상인 실제 세 분배기다.</summary>
        public readonly SceneMusicVirtualCount Counts;

        /// <summary>표/슬롯은 입력으로만 준비하며 Count 결과나 Session 성공 표식은 두지 않는다.</summary>
        public Scene()
        {
            var local = new ClientLocalStateGlobals(Game, new());
            Surface = new(Game, new(local), new(), () => throw new IOException("Count 결선 중 시각 조회 금지"));
            Surface.Storage.Types.AddRange(Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord { FootX = 1, FootY = 1 }));
            Surface.Storage.Frames.AddRange(Enumerable.Range(0, 188).Select(_ => Codes('A')));
            // 실제 클라이언트 할당 뒤 명시 물리 입력을 작성한다.
            for (int sid = 5; sid <= 8; sid++)
            {
                Assert.Equal(sid, Surface.Storage.Pool.Allocate(SidAllocation.Client));
                Surface.Storage.Pool.AllocatedBytes(sid).Clear();
            }
            Counts = new(Surface);
        }

        /// <summary>원문Count 경계에 필요한 프레임 코드 입력을 독립 작성한다.</summary>
        public static RiftTypeFrames Codes(char side) => new(Enumerable.Repeat(new RiftFrameCode((byte)side, (byte)'A', 0, 0), 180));

        /// <summary>현재 VT/TYPE/프레임·레벨을 같은 raw 슬롯에 쓴다.</summary>
        public void Slot(int sid, uint table, byte type, int frame = 0, byte state = 0xa0)
        {
            Span<byte> raw = Surface.Storage.Pool.AllocatedBytes(sid); raw.Clear();
            BinaryPrimitives.WriteUInt32LittleEndian(raw, table); raw[10] = type; raw[11] = state;
            BinaryPrimitives.WriteSingleLittleEndian(raw[14..], 40.25F); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], 41.25F);
            BinaryPrimitives.WriteInt32LittleEndian(raw[36..], frame);
        }
    }

    /// <summary>실제 보호 사본 자산의 genus 후보와169 진단 생성자 주소를 확인한다. 새 PE 실행 자료로 세지 않는다.</summary>
    [Fact]
    public void ActualGenusCandidatesAreDaisAndAuxiliaryFrames()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var storage = new RawSurfaceStorage(); var assets = new HostAssetTables(new HostStartupScene(data), storage);
        int[] candidates = Enumerable.Range(0, storage.Types.Count).Where(type => (storage.Types[type].Flags2 & 0x400000u) != 0).ToArray();
        Assert.Equal(new[] { 165, 169 }, candidates);
        Assert.Equal(165, assets.TypeTable.Find("dais")); Assert.Equal(169, assets.TypeTable.Find("daisExtraFrames"));
        Assert.Equal(0x0044b200u, OriginalConstructors.Address(OriginalEdition.Patch1078, 165));
        Assert.Equal(0x0044a420u, OriginalConstructors.Address(OriginalEdition.Patch1078, 169));
    }

    /// <summary>기존83개 실제생성 VT 범위를 같은 분배기에 통과시키고 공통81개를 Dais 고정으로 바꾸지 않는다.</summary>
    [Fact]
    public void AllConstructedTablesDispatchByCurrentInstanceInsteadOfTypeConstructor()
    {
        var scene = new Scene(); uint[] tables = OriginalConstructors.Recipes.Select(recipe => recipe.Vtable)
            .Append(OriginalConstructors.BaseVtableFor(OriginalEdition.Patch1078)).Distinct().ToArray();
        Assert.Equal(83, tables.Length); int common = 0;
        // TYPE165의 생성자 정보를 무시하고 현재 VT의 몸체만 선택하는 원문 계약이다.
        foreach (uint table in tables)
        {
            scene.Slot(5, table, 165); scene.Surface.Storage.Frames[165] = Scene.Codes('N');
            byte[] before = scene.Surface.Storage.Pool.Slot(5).ToArray();
            uint count = scene.Counts.Count(5);
            if (table == 0x507950) Assert.Equal(1u, count);
            else if (table == 0x5076b8) Assert.Equal(0u, count);
            else { Assert.Equal(5u, count); common++; }
            Assert.Equal(before, scene.Surface.Storage.Pool.Slot(5).ToArray());
        }
        Assert.Equal(81, common);
    }

    /// <summary>실제Dais VT는 현재165/169 코드 교체와 frame경계를 읽고 baseVT에서는 상태BYTE만 읽는다.</summary>
    [Theory]
    [InlineData(165)]
    [InlineData(169)]
    public void DaisBodyUsesCurrentTypeCodesAndFrameButBaseKeepsItsOwnCount(byte type)
    {
        var scene = new Scene(); scene.Slot(5, 0x507950, type, 120);
        scene.Surface.Storage.Frames[type] = Scene.Codes('N'); Assert.Equal(1u, scene.Counts.Count(5));
        scene.Surface.Storage.Frames[type] = Scene.Codes('M'); Assert.Equal(2u, scene.Counts.Count(5));
        scene.Surface.Storage.Frames[type] = Scene.Codes('L'); Assert.Equal(3u, scene.Counts.Count(5));
        scene.Surface.Storage.Frames[type] = Scene.Codes('A'); Assert.Equal(3u, scene.Counts.Count(5));
        BinaryPrimitives.WriteInt32LittleEndian(scene.Surface.Storage.Pool.AllocatedBytes(5)[36..], 119); Assert.Equal(2u, scene.Counts.Count(5));
        BinaryPrimitives.WriteInt32LittleEndian(scene.Surface.Storage.Pool.AllocatedBytes(5)[36..], 63); Assert.Equal(1u, scene.Counts.Count(5));
        BinaryPrimitives.WriteUInt32LittleEndian(scene.Surface.Storage.Pool.AllocatedBytes(5), 0x501dd0);
        scene.Surface.Storage.Pool.AllocatedBytes(5)[11] = 0xe3; Assert.Equal(7u, scene.Counts.Count(5));
    }

    /// <summary>제단은 현재 DaisTYPE의 첫 typed후보를 찾고 그 후보의 현재VT에 다시 분배한다.</summary>
    [Fact]
    public void AltarNestedCountRereadsCurrentDaisTypeAndTargetTable()
    {
        var scene = new Scene(); scene.Slot(5, 0x5076b8, 164); scene.Slot(6, 0x501dd0, 165, state: 0xe0);
        scene.Surface.Storage.Hash.Bucket(1, 40.25F, 41.25F) = 6;
        Assert.Equal(7u, scene.Counts.Count(5));
        BinaryPrimitives.WriteUInt32LittleEndian(scene.Surface.Storage.Pool.AllocatedBytes(6), 0x507950);
        scene.Surface.Storage.Frames[165] = Scene.Codes('M'); Assert.Equal(2u, scene.Counts.Count(5));
        scene.Game.GameAux.DaisTypeBox.Value = 169; Assert.Equal(0u, scene.Counts.Count(5));
        scene.Surface.Storage.Pool.AllocatedBytes(6)[10] = 169; scene.Surface.Storage.Frames[169] = Scene.Codes('L'); Assert.Equal(3u, scene.Counts.Count(5));
    }

    /// <summary>손상된 현재VT/재귀는 정상0으로 숨기지 않으며 실패 뒤 다음 호출의 사슬은 남기지 않는다.</summary>
    [Fact]
    public void UnknownTableAndRecursiveAltarFailWithoutPoisoningFollowingCount()
    {
        var scene = new Scene(); scene.Slot(5, 0x12345678, 165);
        Assert.Throws<InvalidOperationException>(() => scene.Counts.Count(5));
        scene.Slot(5, 0x5076b8, 165); scene.Surface.Storage.Hash.Bucket(1, 40.25F, 41.25F) = 5;
        Assert.Throws<InvalidOperationException>(() => scene.Counts.Count(5));
        BinaryPrimitives.WriteUInt32LittleEndian(scene.Surface.Storage.Pool.AllocatedBytes(5), 0x501dd0);
        Assert.Equal(5u, scene.Counts.Count(5)); Assert.Throws<ArgumentOutOfRangeException>(() => scene.Counts.Count(0));
    }

    /// <summary>월드 조회는 같은 실제 Surface Count를 직접 사용하며 이전Session이나 정답bool을 만들지 않는다.</summary>
    [Fact]
    public void CurrentSurfaceWorldQueryConsumesActualVirtualBody()
    {
        var scene = new Scene(); Assert.Same(scene.Surface, scene.Counts.Surface);
        scene.Slot(5, 0x501dd0, 158, state: 0); scene.Surface.Storage.SidLists.Priests.Append(5);
        scene.Surface.Storage.Types[165].Flags2 = 0x400000; scene.Slot(6, 0x507950, 165, 14, state: 0);
        scene.Surface.Storage.Hash.Bucket(1, 40.25F, 41.25F) = 6;
        var queries = SceneMusicWorldQueries.ForCurrentSurface(scene.Surface, new ClientChallengeGlobals());
        Assert.True(queries.Sacrificing(0));
        BinaryPrimitives.WriteInt32LittleEndian(scene.Surface.Storage.Pool.AllocatedBytes(6)[36..], 13);
        Assert.False(queries.Sacrificing(0));
    }
}
