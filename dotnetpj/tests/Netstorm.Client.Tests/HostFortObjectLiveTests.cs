using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 호스트 세션의 요새 객체 적재 장치가 실제 FortTemplate을 풀에 만드는지 검사한다.
public sealed class HostFortObjectLiveTests
{
    /// <summary>실제 자료로 객체 장면을 만든다. 타입 표·정의·SHP·프레임은 실파일 값이다.</summary>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) LoadRealObjectScene(uint seed = 1u)
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var session = new HostSession(new Kernel(), new GameRandom(seed), new CrtRandom());
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        session.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        byte[] shpRaw = startup.Files.ReadAllBytes("d/_shapes.shp");
        var shpDb = new ShapeDatabase(shpRaw);
        List<SquidDisplayShape> shapes = TerrainDisplayShapes.Build(definitions, shpDb);
        session.BuildTerrainPopDevices(table.Types, shapes);
        session.BuildFortObjectDevices(table.Types, session.WorldFrames);
        return (session, table.Types);
    }

    /// <summary>사용한 슬롯 수를 센다. 빈 칸은 건너뛴다.</summary>
    /// <param name="session">셀 세션이다.</param>
    private static int UsedSlots(HostSession session)
    {
        int count = 0;
        // 일반 번호부터 빈 칸을 건너뛰고 실제 슬롯을 세는 반복문이다.
        for (int sid = RawSidPool.FirstOrdinary; sid < session.Pool.Capacity; ++sid)
        {
            if ((session.Pool.Slot(sid)[RawSidPool.StateOffset] & RawSidPool.FreeBit) != 0) continue;
            ++count;
        }
        return count;
    }

    /// <summary>장치 인자 오류를 연결 전에 거부하는지 확인한다.</summary>
    [Fact]
    public void FortDevicesRejectBadArgs()
    {
        var (session, types) = LoadRealObjectScene();
        Assert.Throws<ArgumentNullException>(() => session.BuildFortObjectDevices(null!, session.WorldFrames));
        Assert.Throws<ArgumentNullException>(() => session.BuildFortObjectDevices(types, null!));
        Assert.Throws<ArgumentException>(() => session.BuildFortObjectDevices(types, []));
        // 지형 Pop 장치보다 먼저 연결하면 공유 공간이 없어 거부한다.
        var bare = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        Assert.Throws<InvalidOperationException>(() => bare.BuildFortObjectDevices(types, bare.WorldFrames));
    }

    /// <summary>장치 미연결 적재를 거부하는지 확인한다.</summary>
    [Fact]
    public void LoadFortObjectsRequiresDevices()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        MissionFortLoadResult fort = MissionFortLoad.Load(startup, "thewarbegins");
        Assert.Throws<InvalidOperationException>(() => session.LoadFortObjects(fort.Fort, 1u));
    }

    /// <summary>실제 요새의 객체 적재 장치를 만든다. live 시작 흐름과 같은 순서(지형 PlaceAll·Generate 뒤 Objects)용이다.</summary>
    private static void BuildLiveTerrainFirst(HostSession session, FortTemplate fort, IReadOnlyList<OriginalTypeRecord> types)
    {
        // live와 같은 120바이트 Territory 읽기와 같다. 스트림 위치·진단은 FortWorldLoad 쪽 검사에서 다룬다.
        Assert.True(fort.Territory.Length >= 120);
        byte[] records = new byte[120];
        Array.Copy(fort.Territory, 0, records, 0, 120);
        var islands = session.BuildIslands(new IslandRuntimeHooks
        {
            SetResidenceOwner = (_, _) => { },
            Assertion = _ => { },
        });
        var islandState = new RawIslandBuilderState();
        var builder = new RawIslandBuilder(
            session.World, session.World, islands, session.WorldFrames,
            () => session.GameAux.PuzzlePieceType, islandState, _ => { });
        builder.PlaceAll(records, 0, 20);
        // live와 같은 전체 지도 생성이다. 전투 옵션이 고른 레이아웃·간격을 그대로 쓴다.
        var diagnostics = new NormalTerrainDiagnostics
        {
            Context = _ => { },
            Stamp = _ => { },
            Region = _ => { },
            Farm = _ => { },
            Postpass = _ => { },
        };
        var binding = session.BuildTerrainBinding(types, diagnostics, new IslandRuntimeHooks
        {
            SetResidenceOwner = (_, _) => { },
            Assertion = _ => { },
        });
        binding.GenerateOriginal(
            islandState.Owner, islandState.Bounds, session.TerrainGlobals.NoRegion,
            session.Game, session.TerrainCrt, session.BuildTerrainStampHooks(types), (_, _, _) => { });
    }

    /// <summary>실제 미션 요새를 적재하고 구조·결정성을 확인한다.</summary>
    [Fact]
    public void LoadRealFortStructuralAndDeterministic()
    {
        var (first, firstTypes) = LoadRealObjectScene();
        var (second, _) = LoadRealObjectScene();
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        MissionFortLoadResult fort = MissionFortLoad.Load(startup, "thewarbegins");
        BuildLiveTerrainFirst(first, fort.Fort, firstTypes);
        BuildLiveTerrainFirst(second, fort.Fort, firstTypes);
        int before = UsedSlots(first);
        first.LoadFortObjects(fort.Fort, 1u);
        second.LoadFortObjects(fort.Fort, 1u);
        int firstUsed = UsedSlots(first);
        int secondUsed = UsedSlots(second);
        // 객체 슬롯이 새로 생긴다(지형 슬롯만 있던 상태에서 증가).
        Assert.True(firstUsed > before);
        Assert.Equal(firstUsed, secondUsed);
        // 내용물 보유 타입(123·124·158)이 실제로 적재돼야 한다(부분집합만 돌면 여기서 걸린다).
        foreach (uint container in new uint[] { 123u, 124u, 158u })
        {
            bool found = false;
            // 일반 번호 슬롯을 순서대로 훑는 반복문이다.
            for (int sid = RawSidPool.FirstOrdinary; sid < first.Pool.Capacity; ++sid)
            {
                ReadOnlySpan<byte> raw = first.Pool.Slot(sid);
                if ((raw[RawSidPool.StateOffset] & RawSidPool.FreeBit) == 0 && raw[RawSidPool.TypeOffset] == container)
                {
                    found = true;
                    break;
                }
            }
            Assert.True(found);
        }
        // 내용물 부착 경로가 실제로 실행돼야 한다(진단 복귀 뒤 계속 진행한다).
        Assert.NotEmpty(first.FortAttachDiagnostics);
        // 같은 시드·같은 요새면 난수·슬롯 바이트가 완전히 같다.
        Assert.Equal(first.Game.State(), second.Game.State());
        Assert.Equal(first.Pool.Capacity, second.Pool.Capacity);
        // 일반 번호 슬롯을 순서대로 비교하는 반복문이다.
        for (int sid = RawSidPool.FirstOrdinary; sid < first.Pool.Capacity; ++sid)
        {
            byte[] a = first.Pool.Slot(sid).ToArray();
            byte[] b = second.Pool.Slot(sid).ToArray();
            Assert.Equal(a, b);
        }
    }

    /// <summary>실제 요새의 객체 타입별 Pop 지원 여부를 분류한다. 지원표 밖 가상 표(71·75·78·89·122 등)가 있어 공간 등록은 R6 postPop 장치와 함께 잇는다.</summary>
    [Fact]
    public void ProbeFortObjectPopSupport()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        MissionFortLoadResult fort = MissionFortLoad.Load(startup, "thewarbegins");
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        var factory = new RawSquidFactory(session.Pool, table.Types);
        var unsupported = new SortedDictionary<uint, uint>();
        // Load와 같은 순서(20개 Terr+Chaff)로 타입을 훑는 반복문이다.
        for (int territory = 0; territory < 20; ++territory)
        {
            foreach (Netstorm.Core.Original.FortChunk chunk in fort.Fort.Territories[territory].Chunks)
            {
                foreach (Netstorm.Core.Original.FortObject @object in chunk.Objects)
                {
                    uint type = @object.StoredType >= 0xf6 ? 82u : unchecked((uint)@object.Type);
                    if (type == 0 || type == 94u || !unsupported.TryAdd(type, 0u))
                    {
                        continue;
                    }
                    int sid = factory.Create(type, Netstorm.Core.Simulation.SidAllocation.Server);
                    ReadOnlySpan<byte> raw = session.Pool.Slot(sid);
                    uint vtable = unchecked((uint)(raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)));
                    if (!OriginalPopDisplay.Supports(session.Pool.Edition, vtable, 9u))
                    {
                        unsupported[type] = vtable;
                    }
                    else
                    {
                        unsupported.Remove(type);
                    }
                }
            }
        }
        Assert.NotEmpty(unsupported);
    }

    /// <summary>실제 요새의 내용물 보유 객체를 전수한다. thewarbegins 고정 구성(123×1·124×1·158×2)을 문서화하며 Attach live 결선의 근거다.</summary>
    [Fact]
    public void ProbeFortObjectContents()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        MissionFortLoadResult fort = MissionFortLoad.Load(startup, "thewarbegins");
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        var found = new SortedDictionary<uint, int>();
        int total = 0;
        // Load와 같은 순서(20개 Terr+Chaff)로 훑는 반복문이다.
        for (int territory = 0; territory < 20; ++territory)
        {
            foreach (Netstorm.Core.Original.FortChunk chunk in fort.Fort.Territories[territory].Chunks)
            {
                foreach (Netstorm.Core.Original.FortObject @object in chunk.Objects)
                {
                    uint type = @object.StoredType >= 0xf6 ? 82u : unchecked((uint)@object.Type);
                    if (type == 0 || type == 94u || type >= (uint)table.Types.Count)
                    {
                        continue;
                    }
                    ++total;
                    if ((table.Types[(int)type].Flags1 & 0x20000u) != 0)
                    {
                        found[type] = found.TryGetValue(type, out int count) ? count + 1 : 1;
                    }
                }
            }
        }
        Assert.True(total > 0);
        Assert.Equal(new SortedDictionary<uint, int> { [123u] = 1, [124u] = 1, [158u] = 2 }, found);
    }

    /// <summary>실제 요새의 적재 순서를 덤프 생성 순서와 대조한다. 영역별 순서가 어긋나면 적재 순서 결함이다.</summary>
    [Fact]
    public void ProbeFortLoadVsDumpOrder()
    {
        var (session, _) = LoadRealObjectScene();
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        MissionFortLoadResult fort = MissionFortLoad.Load(startup, "thewarbegins");
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        BuildLiveTerrainFirst(session, fort.Fort, table.Types);
        session.LoadFortObjects(fort.Fort, 1u);
        var islands = session.BuildIslands(new IslandRuntimeHooks
        {
            SetResidenceOwner = (_, _) => { },
            Assertion = _ => { },
        });
        var expectedServer = new List<uint>();
        var expectedClient = new List<uint>();
        var expectedPredictable = new List<uint>();
        // Load와 같은 순서·건너뜀·다리 매핑으로 훑는 반복문이다.
        var orderState = new FortObjectLoadState();
        for (int territory = 0; territory < 20; ++territory)
        {
            CollectFortSection(fort.Fort.Territories[territory], 1u, territory, orderState, session, islands,
                table.Types, expectedServer, expectedClient, expectedPredictable);
        }
        CollectFortSection(fort.Fort.Chaff, 1u, -1, orderState, session, islands,
            table.Types, expectedServer, expectedClient, expectedPredictable);
        var actualServer = new List<uint>();
        var actualClient = new List<uint>();
        var actualPredictable = new List<uint>();
        // 풀 전체를 번호 순서대로 읽는 반복문이다.
        for (int sid = RawSidPool.FirstOrdinary; sid < session.Pool.Capacity; ++sid)
        {
            ReadOnlySpan<byte> raw = session.Pool.Slot(sid);
            if ((raw[RawSidPool.StateOffset] & RawSidPool.FreeBit) != 0) continue;
            if (raw[RawSidPool.TypeOffset] != 6) continue;
            if (sid >= RawSidPool.PredictableFirst) actualPredictable.Add(raw[RawSidPool.TypeOffset]);
            else if (sid >= RawSidPool.ServerFirst) actualServer.Add(raw[RawSidPool.TypeOffset]);
            else actualClient.Add(raw[RawSidPool.TypeOffset]);
        }
        // 서버 영역의 내용물 자식은 6번뿐이므로 서버 슬롯 전체 순서와 대조한다.
        var actualServerAll = new List<uint>();
        for (int sid = RawSidPool.ServerFirst; sid < RawSidPool.PredictableFirst; ++sid)
        {
            ReadOnlySpan<byte> raw = session.Pool.Slot(sid);
            if ((raw[RawSidPool.StateOffset] & RawSidPool.FreeBit) != 0) continue;
            actualServerAll.Add(raw[RawSidPool.TypeOffset]);
        }
        Assert.Equal(expectedServer, actualServerAll);
        Assert.Equal(expectedClient, actualClient);
        Assert.Equal(expectedPredictable, actualPredictable);
    }

    /// <summary>적재와 같은 순서·건너뜀·다리 매핑·내용물 끼워넣기로 타입을 모은다.</summary>
    private static void CollectFortSection(
        Netstorm.Core.Original.FortChunkSection section, uint owner, int territory, FortObjectLoadState state,
        HostSession session, RawIslandRuntime islands,
        IReadOnlyList<OriginalTypeRecord> types, List<uint> server, List<uint> client, List<uint> predictable)
    {
        if (!section.Present) return;
        int index = 0;
        // y 바깥·x 안쪽 순서와 지역 조회 건너뜀은 로더와 같다.
        for (int y = 0; y < state.ChunksY; ++y)
        {
            for (int x = 0; x < state.ChunksX; ++x)
            {
                sbyte region = unchecked((sbyte)session.World.At(x, y).Island);
                if (territory >= 0 && !islands.Matches(region, unchecked((int)owner), territory))
                {
                    continue;
                }
                if (index >= section.Chunks.Count)
                {
                    throw new ArgumentOutOfRangeException(nameof(section), "프로브 청크 자료 부족");
                }
                Netstorm.Core.Original.FortChunk chunk = section.Chunks[index++];
                foreach (Netstorm.Core.Original.FortObject @object in chunk.Objects)
                {
                    uint type = @object.StoredType >= 0xf6 ? state.BridgeType : unchecked((uint)@object.Type);
                    if (type == 0 || type == state.IslandType)
                    {
                        continue;
                    }
                    server.Add(type);
                    // 내용물은 ContainerFlag 타입에만 있으며 Required와 같은 누락 검사를 둔다.
                    if ((types[(int)type].Flags1 & 0x20000u) != 0)
                    {
                        if (@object.Contents is null)
                        {
                            throw new InvalidOperationException("프로브 필수 필드 누락: contents");
                        }
                        CollectFortContents(@object.Contents, 9u, type, types, server, client, predictable);
                    }
                }
            }
        }
    }

    /// <summary>내용물 목록을 로더와 같은 순서·깃발로 훑어 생성될 타입을 모은다.</summary>
    private static void CollectFortContents(
        Netstorm.Core.Original.FortContentList contents, uint flags, uint parentType, IReadOnlyList<OriginalTypeRecord> types,
        List<uint> server, List<uint> client, List<uint> predictable)
    {
        foreach (Netstorm.Core.Original.FortContent item in contents.Items)
        {
            if (item.Type == 0)
            {
                return;
            }
            uint listFlags = types[item.Type].ContentListFlags;
            if (listFlags == 0)
            {
                listFlags = types[(int)parentType].ContainerListFlags;
            }
            if (listFlags == 0)
            {
                listFlags = item.ListFlags;
            }
            uint childFlags = flags;
            if ((listFlags & 0x10u) != 0)
            {
                childFlags = (flags & ~8u) | 16u;
            }
            uint factoryFlags = (childFlags & 0x20u) != 0 ? 1u : (childFlags & 0x10u) != 0 ? 2u : 0u;
            if (factoryFlags == 1u) predictable.Add(6u);
            else if (factoryFlags == 2u) client.Add(6u);
            else server.Add(6u);
            if ((types[item.Type].Flags1 & 0x20000u) != 0)
            {
                if (item.Contents is null)
                {
                    throw new InvalidOperationException("프로브 필수 필드 누락: nested contents");
                }
                CollectFortContents(new Netstorm.Core.Original.FortContentList
                {
                    Items = item.Contents,
                    Complete = item.ContentsComplete,
                }, childFlags, unchecked((uint)item.Type), types, server, client, predictable);
            }
        }
    }

    /// <summary>실제 요새의 내용물 null 위치를 특정한다. 로더와 같은 ContainerFlag 조건으로만 검사한다.</summary>
    [Fact]
    public void ProbeFortContentsNulls()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        MissionFortLoadResult fort = MissionFortLoad.Load(startup, "thewarbegins");
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        var nullObjects = new List<string>();
        var nullItems = new List<string>();
        int total = 0;
        int containers = 0;
        // 전 Terr+Chaff를 건너뜀 없이 훑는 반복문이다.
        for (int territory = -1; territory < 20; ++territory)
        {
            Netstorm.Core.Original.FortChunkSection section =
                territory < 0 ? fort.Fort.Chaff : fort.Fort.Territories[territory];
            if (!section.Present) continue;
            for (int chunk = 0; chunk < section.Chunks.Count; ++chunk)
            {
                int obj = 0;
                foreach (Netstorm.Core.Original.FortObject @object in section.Chunks[chunk].Objects)
                {
                    uint type = @object.StoredType >= 0xf6 ? 82u : unchecked((uint)@object.Type);
                    if (type == 0 || type == 94u || type >= (uint)table.Types.Count)
                    {
                        ++obj;
                        continue;
                    }
                    ++total;
                    if ((table.Types[(int)type].Flags1 & 0x20000u) == 0)
                    {
                        ++obj;
                        continue;
                    }
                    ++containers;
                    if (@object.Contents is null)
                    {
                        nullObjects.Add($"{territory}/{chunk}:{obj}");
                    }
                    else if (@object.Contents.Items is null)
                    {
                        nullItems.Add($"{territory}/{chunk}:{obj}(top)");
                    }
                    else
                    {
                        CollectNulls(@object.Contents.Items, $"{territory}/{chunk}:{obj}", nullItems);
                    }
                    ++obj;
                }
            }
        }
        Assert.True(total > 0);
        Assert.True(containers > 0);
        Assert.Empty(nullObjects);
        Assert.Empty(nullItems);
    }

    /// <summary>중첩 내용물의 null 목록을 재귀로 모은다.</summary>
    private static void CollectNulls(
        System.Collections.Generic.IReadOnlyList<Netstorm.Core.Original.FortContent> items, string path, List<string> nullItems)
    {
        int index = 0;
        // 항목 순서대로 훑는 반복문이다.
        foreach (Netstorm.Core.Original.FortContent item in items)
        {
            if (item.Contents is null)
            {
                nullItems.Add($"{path}/{index}:null-list");
            }
            ++index;
        }
    }
}
