using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 호스트 세션이 지형 바인딩을 세션 저장소로 조립하고 생성 입력을 묶는지 검사한다.
public sealed class HostTerrainBindingTests
{
    /// <summary>실제 자료로 바인딩 장면을 만든다. 타입 표·프레임은 실파일 값이다.</summary>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) LoadRealBindingScene()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        session.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        return (session, table.Types);
    }

    /// <summary>기록만 하는 진단 묶음을 만든다. 생성 진입의 필수 Context를 만족한다.</summary>
    private static NormalTerrainDiagnostics RecordingDiagnostics() => new()
    {
        Context = _ => { },
        Stamp = _ => { },
        Region = _ => { },
        Farm = _ => { },
        Postpass = _ => { },
    };

    /// <summary>기록만 하는 섬 런타임 갈고리를 만든다. 조립은 효과를 부르지 않는다.</summary>
    private static IslandRuntimeHooks RecordingIslandHooks() => new()
    {
        SetResidenceOwner = (_, _) => { },
        Assertion = _ => { },
    };

    /// <summary>세션 저장소로 바인딩을 조립하고 입력을 묶어 같은 상자를 공유하는지 확인한다.</summary>
    [Fact]
    public void BuildsBindingAndBindsSessionInputs()
    {
        var (session, types) = LoadRealBindingScene();
        Assert.Null(session.TerrainBinding);
        var binding = session.BuildTerrainBinding(types, RecordingDiagnostics(), RecordingIslandHooks());
        Assert.Same(binding, session.TerrainBinding);
        Assert.True(binding.HasGenerationInputs());
        // 바인딩이 보관한 입력은 세션 상자를 공유한다.
        Assert.Same(session.GenerationInputs.Span, binding.GenerationInputs().Span);
        Assert.Same(session.GenerationInputs.Archipelago, binding.GenerationInputs().Archipelago);
        Assert.True(binding.GenerationInputs().Enabled.IsEnabled);
        // 군집이 입력 상자를 공유한다.
        Assert.False(session.TerrainFarm.CurrentArchipelago());
        session.GenerationInputs.Archipelago.Value = 1u;
        Assert.True(session.TerrainFarm.CurrentArchipelago());
        // 전투 옵션 5·6번을 읽는다.
        Assert.Equal(session.BattleOptions[5], binding.GenerationInputs().Layout!());
        Assert.Equal(RawWorldGeyserSpacing.Spacing(session.BattleOptions[6]), binding.GenerationInputs().Spacing!());
    }

    /// <summary>빠진 장치를 조립 전에 거부하고 바인딩을 두지 않는지 확인한다.</summary>
    [Fact]
    public void BindingRejectsMissingDevices()
    {
        var (session, types) = LoadRealBindingScene();
        Assert.Throws<ArgumentNullException>(() => session.BuildTerrainBinding(null!, RecordingDiagnostics(), RecordingIslandHooks()));
        Assert.Throws<ArgumentNullException>(() => session.BuildTerrainBinding(types, null!, RecordingIslandHooks()));
        Assert.Throws<ArgumentNullException>(() => session.BuildTerrainBinding(types, RecordingDiagnostics(), null!));
        Assert.Null(session.TerrainBinding);
    }

    /// <summary>도장 기록 효과를 세는 스파이다. 풀 할당은 실제 풀에 쓴다.</summary>
    private sealed class StampCreateSpy(RawSidPool pool)
    {
        /// <summary>생성 슬롯을 빌릴 실제 풀이다.</summary>
        private readonly RawSidPool _pool = pool;

        /// <summary>생성 호출 횟수다.</summary>
        public int Creates;

        /// <summary>도장에 넘길 기록 월드 효과다. ChunkRegion·Assertion은 바인딩이 덮는다.</summary>
        public TerrainStampHooks Hooks => new()
        {
            Create = (type, predictable) =>
            {
                int sid = _pool.Allocate(SidAllocation.Client);
                Span<byte> raw = _pool.AllocatedBytes(sid);
                raw.Clear();
                raw[10] = unchecked((byte)type);
                Creates++;
                return sid;
            },
            SetOwner = (_, _) => { },
            Pop = (_, _, _, _) => { },
            SetRegion = (_, _) => { },
            ChunkRegion = (_, _) => (sbyte)127,
            Assertion = _ => { },
        };
    }

    /// <summary>같은 입력의 드라이런을 두 번 돌려 같은 도장 결과를 내는지 확인한다.</summary>
    private static (int Sid, int Creates) RunDryrunOnce()
    {
        var (session, types) = LoadRealBindingScene();
        // 성장 통로가 읽는 청크를 유효 문자·지역·기록으로 둔다.
        session.World.At(2, 2).Island = 0;
        session.World.At(2, 2).Side = (sbyte)'A';
        session.World.At(2, 2).Record = [3, 1, 0, 7, 0, 0];
        var binding = session.BuildTerrainBinding(types, RecordingDiagnostics(), RecordingIslandHooks());
        var spy = new StampCreateSpy(session.Pool);
        var places = new List<(FortRandomRectangle Area, int Region, uint Count)>();
        // 임무이므로 후보가 아니어서 일반 성장을 돈다.
        int sid = binding.GenerateOriginal(1u, new ChunkRect(1, 1, 5, 5), 127, new NetstormRandom(1u), new UIntBox { Value = 1u }, spy.Hooks, (area, region, count) => places.Add((area, region, count)));
        return (sid, spy.Creates);
    }

    /// <summary>기록 효과의 드라이런이 도장을 돌리고 같은 입력에 같은 결과를 내는지 확인한다.</summary>
    [Fact]
    public void GenerateDryrunStampsAndIsDeterministic()
    {
        (int firstSid, int firstCreates) = RunDryrunOnce();
        (int secondSid, int secondCreates) = RunDryrunOnce();
        Assert.NotEqual(0, firstCreates);
        Assert.Equal(firstSid, secondSid);
        Assert.Equal(firstCreates, secondCreates);
    }
}
