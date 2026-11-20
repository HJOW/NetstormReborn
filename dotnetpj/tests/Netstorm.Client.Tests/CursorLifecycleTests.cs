using System.Buffers.Binary;
using System.Globalization;
using System.Runtime.InteropServices;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 원문 취득/취소/임시 정리의 모든 현재 전역과 실제 raw WORD를 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>독립565/156/854입력과 고정 데이터/실제 자산 바인딩을 검사한다.</summary>
public sealed class CursorLifecycleTests
{
    /// <summary>원문 십진 DWORD 목록이다.</summary>
    private static uint[] Numbers(string text) => text.Split(',').Select(X86Fixture.UInt).ToArray();
    /// <summary>원문의 각 입력을 같은 물리/고정 상태에 독립 준비한다.</summary>
    private sealed class Scene
    {
        /// <summary>확보한 client 슬롯을 재사용하는 실제 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, false);
        /// <summary>현재 비용/flags를 공급하는188TYPE다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>동일 내려놓기 전역이다.</summary>
        public readonly CursorDropState Drop = new();
        /// <summary>동일 수명 전역/고정 배열이다.</summary>
        public readonly CursorLifecycleState State = new();
        /// <summary>현재17개 명시 입력이다.</summary>
        public uint[] Input = [];
        /// <summary>실제 외부 경계의 호출 순서다.</summary>
        public readonly List<string> Events = [];
        /// <summary>첫 준비/현재 측정 경계를 구분한다.</summary>
        private int _prepared, _measured;
        /// <summary>독립 명시 입력 슬롯만 실제 client 공간에서 확보한다.</summary>
        public Scene()
        {
            // 부모/미리보기/임시 객체의 모든 후보가 같은 실제 풀을 사용한다.
            for (int sid = 5; sid <= 101; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
        }
        /// <summary>독립 dead 표식도 원문 그대로 주입하는 검사 전용 실제 뷰다.</summary>
        public Span<byte> Raw(int sid) => MemoryMarshal.CreateSpan(ref MemoryMarshal.GetReference(Pool.Slot(sid)), Pool.SlotStride);
        /// <summary>현재 인자와 원문 DWORD 비트를 사건 셀로 기록한다.</summary>
        public void Event(string name, params uint[] values) => Events.Add(name + (values.Length == 0 ? "" : ":" + string.Join(':', values.Select(value => value.ToString(CultureInfo.InvariantCulture)))));
        /// <summary>이전 실행 결과를 버리고 원문17DWORD와 같은 무관 BYTE 표식을 준비한다.</summary>
        public void Prepare(uint[] input)
        {
            Input = input; Drop.Reset(); State.Reset(); Events.Clear(); _prepared = _measured = 0;
            // 전체 raw BYTE와 독립 상태 표식만 설치하며 기대값은 읽지 않는다.
            foreach (int sid in new[] { 50, 51, 77, 100, 101 })
            {
                Raw(sid).Fill(0xa5); BinaryPrimitives.WriteUInt32LittleEndian(Raw(sid), 1);
                Raw(sid)[11] = (byte)(sid is 50 or 51 ? input[8] : sid == 77 ? input[9] : 0);
                BinaryPrimitives.WriteUInt16LittleEndian(Raw(sid)[12..], (ushort)input[3]); BinaryPrimitives.WriteUInt16LittleEndian(Raw(sid)[18..], 100); BinaryPrimitives.WriteUInt16LittleEndian(Raw(sid)[20..], (ushort)input[2]);
            }
            Types[100].Flags2 = input[4]; Types[100].Cost = BitConverter.UInt32BitsToSingle(input[5]); Drop.Type = State.AbstractType = 100;
            Drop.LockDepth = input[6]; Drop.InputFlags = input[2]; Drop.CostFlags = input[1]; Drop.Preview = input[0] == 1 ? input[7] : 0; Drop.CursorGump = input[0] == 1 ? 1u : 0u;
            State.PreviewCount = input[0] == 2 ? unchecked((int)input[7]) : 2; State.Previews[0] = 100; State.Previews[1] = 101; State.WarningFont = 17; State.CostFont = 19;
        }
        /// <summary>수명 분기를 계산하지 않고 원문 도구와 같은 명시 효과/측정만 제공한다.</summary>
        public CursorLifecycleHooks Hooks() => new()
        {
            Highlight = value => { Event("H", value); if (Input[10] == 1) Drop.LockDepth = 3; if (Input[10] == 2) Drop.Preview = 51; },
            PrepareCursor = () => { Event("P"); if (Input[11] != 0 && ++_prepared == 1) State.PreviewCount = 1; }, SelectCursor = value => Event("C", value), ClosePopups = () => Event("X"),
            Source = sid => { Event("U", (uint)sid); return 77; }, SourceNotice = (source, preview, flags) => Event("N", (uint)source, (uint)preview, flags),
            Detach = (sid, flags) => Event("D", (uint)sid, flags), Unpop = (sid, flags) => Event("O", (uint)sid, flags), Destroy = (sid, flags) => Event("K", (uint)sid, flags), RemoveProcess = (process, flags) => Event("R", process, flags),
            Sound = name => Event("S:" + name, 0, 0, 0, 0, 0), AddLocalSp = delta => Event("A", BitConverter.SingleToUInt32Bits(delta)),
            DefaultArgument = type => { Assert.Equal(100u, type); return 11; },
            CreatePreviews = () => { Event("B"); State.Previews[0] = 100; State.Previews[1] = 101; State.PreviewCount = 2; Drop.LegalGump = Input[14] != 0 ? 44u : 0u; },
            CreateMana = (first, flags) => { Event("M", (uint)first, flags); State.CurrentGump = Input[15] != 0 ? 45u : 0u; return Input[12] != 0 ? 1u : 0u; },
            CreateRange = (first, flags) => { Event("G", (uint)first, flags); return 2; }, ProcessEvent = (process, ev, first, second) => Event("E", process, ev, first, second),
            TypeNumber = type => { Event("F", type); return 1; }, HasRange = type => { Event("L", type); return Input[13] != 0; }, QueryCursor = () => { Event("Q"); State.CurrentGump = 46; }, Translate = value => value,
            Measure = (text, context) =>
            {
                Event("T:" + text + ":" + string.Join(',', context.ToArray().Select(value => value.ToString(CultureInfo.InvariantCulture))), uint.MaxValue); int index = _measured++;
                int[] widths = [20, 33, -3, 120, 55], heights = [10, 30, -1, 20, 17]; return index < 5 ? (widths[index], heights[index]) : (37, Input[16] != 0 ? -9 : 12);
            },
            CostImage = (type, frame) => { Assert.Equal(92u, type); Assert.Equal(3u, frame); return new(1, (short)(Input[16] != 0 ? -11 : 23), (short)(Input[16] != 0 ? -7 : 19), -3, 5); },
            Mouse = () => (-3, 47), UpdatePosition = (x, y) => Event("Z", unchecked((uint)x), unchecked((uint)y)),
        };
        /// <summary>원문과 같은78DWORD를 의미 필드에서 원래 순서대로 읽는다.</summary>
        public uint[] Snapshot()
        {
            var result = new List<uint> { Drop.LockDepth, Drop.Type, Drop.Argument, Drop.Direction, State.DirectionArgument, Drop.Preview, Drop.InputFlags, Drop.CostFlags, Drop.CursorGump, Drop.LegalGump, State.CurrentGump,
                unchecked((uint)State.PreviewCount), unchecked((uint)State.WarningWidth), unchecked((uint)State.WarningHeight), unchecked((uint)State.CostTextWidth), unchecked((uint)State.CostHeight), unchecked((uint)State.CostHotspotX), unchecked((uint)State.CostHotspotY), unchecked((uint)Drop.CursorWidth), unchecked((uint)State.ImageHeight) };
            result.AddRange(State.WarningWidths.Select(value => unchecked((uint)value))); result.AddRange(State.WarningHeights.Select(value => unchecked((uint)value))); result.AddRange(State.WarningContext); result.AddRange(State.CostContext); result.AddRange(State.Previews); result.Add(State.AbstractType); result.Add(State.CostImage); return result.ToArray();
        }
        /// <summary>전체 raw50BYTE를 원문 소문자16진수로 읽는다.</summary>
        public string Hex(int sid) => Convert.ToHexString(Pool.Slot(sid)).ToLowerInvariant();
        /// <summary>빈 사건도 원문 셀 표기를 유지한다.</summary>
        public string Trace() => Events.Count == 0 ? "-" : string.Join(';', Events);
        /// <summary>원문78DWORD와 costText/raw150BYTE를 모두 대조한다.</summary>
        public bool Matches(string[] row) => Trace() == row[1] && Snapshot().SequenceEqual(Numbers(row[2])) && State.CostText == row[3] && Hex(50) == row[4] && Hex(51) == row[5] && Hex(77) == row[6];
    }

    /// <summary>Take/Cancel/Activate565개 독립 입력의 모든 출력 필드를 비교한다.</summary>
    [Fact]
    public void EveryNativeLifecycleSequenceMatches()
    {
        string[][] rows = X86Fixture.Read("cursor-lifecycle"); Assert.Equal(565, rows.Length); var scene = new Scene();
        X86Fixture.CheckRows(rows, row =>
        {
            uint[] input = Numbers(row[0]); scene.Prepare(input); var logic = new RawCursorLifecycle(scene.Pool, scene.Types, scene.Drop, scene.State, scene.Hooks());
            if (input[0] == 0) logic.Take(50, input[1]); else if (input[0] == 1) logic.Cancel(); else logic.Activate(input[6]); return scene.Matches(row);
        });
    }
    /// <summary>현재 checking/TYPE를 바꾸는 음향·진단 복귀/중단156입력을 전수 대조한다.</summary>
    [Fact]
    public void EveryNativeCancelCheckingSequenceMatches()
    {
        string[][] rows = X86Fixture.Read("cursor-cancel-checking"); Assert.Equal(156, rows.Length); var scene = new Scene();
        X86Fixture.CheckRows(rows, row =>
        {
            uint[] values = Numbers(row[0]); scene.Prepare(values[3..]); uint checking = values[0]; scene.State.Checking = new RawCheckingFlag(() => checking);
            scene.Types[101].Cost = BitConverter.UInt32BitsToSingle(0xc0700000); scene.Types[102].Cost = BitConverter.UInt32BitsToSingle(0x80000000); CursorLifecycleHooks hooks = scene.Hooks();
            hooks.Sound = name => { scene.Event("S:" + name, 0, 0, 0, 0, 0); if (values[1] != 0) { checking = values[1] == 1 ? 0u : 0x80000000u; scene.State.AbstractType = 101; } };
            hooks.Assertion = (expression, file, line) =>
            {
                Assert.Equal("false", expression); Assert.Equal(".\\Cursor.cpp", file); Assert.Equal(0x3f1u, line); scene.Event("D:66616c7365:2e5c437572736f722e637070:1009");
                if (values[2] == 2) throw new IOException("명시 진단 중단"); if (values[2] == 1) { checking = 0; scene.State.AbstractType = 102; }
            };
            var logic = new RawCursorLifecycle(scene.Pool, scene.Types, scene.Drop, scene.State, hooks); string outcome = "return";
            try { logic.Cancel(); } catch (IOException error) when (error.Message == "명시 진단 중단") { outcome = "assertion"; }
            return scene.Matches(row) && outcome == row[7] && checking == X86Fixture.UInt(row[8]);
        });
    }
    /// <summary>실제 타입 번호와 사정거리 조건854개를 원본 출력과 대조한다.</summary>
    [Fact]
    public void EveryNativeTypeRangeMatches()
    {
        string[][] rows = X86Fixture.Read("cursor-type-range"); Assert.Equal(854, rows.Length); var types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        X86Fixture.CheckRows(rows, row => { uint type = X86Fixture.UInt(row[0]); types[type].Group = unchecked((int)X86Fixture.UInt(row[1])); types[type].Flags2 = X86Fixture.UInt(row[2]); return type == X86Fixture.UInt(row[3]) && RawCursorLifecycle.HasCursorRange(types, type) == (X86Fixture.UInt(row[4]) != 0); });
    }
    /// <summary>잠겨도 취소는 실제 준비를 요구하고 누락 시 기존 꼬리를 실행한 것처럼 만들지 않는다.</summary>
    [Fact]
    public void MissingPrepareAndSelectedDiagnosticKeepPrefix()
    {
        var scene = new Scene(); scene.Drop.LockDepth = 1; scene.Drop.Preview = 50; scene.State.PreviewCount = 1;
        var missing = new RawCursorLifecycle(scene.Pool, scene.Types, scene.Drop, scene.State, new()); Assert.Throws<InvalidOperationException>(missing.Cancel); Assert.Equal(50u, scene.Drop.Preview); Assert.Equal(1, scene.State.PreviewCount);
        scene.Prepare([1, 0x80, 0x80, 0, 0, 0x437ac000, 0, 50, 0, 0, 0, 0, 1, 0, 0, 0, 0]); scene.State.TeleportBlocked = 1;
        var logic = new RawCursorLifecycle(scene.Pool, scene.Types, scene.Drop, scene.State, scene.Hooks()); Assert.Throws<InvalidOperationException>(logic.Cancel);
        Assert.Equal("S:teleporterActive.wav:0:0:0:0:0", scene.Events[^1]); Assert.Equal(50u, scene.Drop.Preview); Assert.Equal(1u, scene.Drop.CursorGump); Assert.Equal(2, scene.State.PreviewCount);
        scene.State.TeleportBlocked = 0; logic.Cancel(); Assert.Equal(0u, scene.Drop.Preview); Assert.Equal(1u, scene.Drop.LockDepth); Assert.Equal(0u, scene.Drop.CursorGump);
    }
    /// <summary>미리보기의 높은 WORD를 잃지 않고 실제 소비에서만 거부한다.</summary>
    [Fact]
    public void PreviewDwordCannotFoldIntoAnotherValidSid()
    {
        var scene = new Scene(); scene.Prepare([2, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0]); scene.State.Previews[0] = 0x10032;
        var logic = new RawCursorLifecycle(scene.Pool, scene.Types, scene.Drop, scene.State, scene.Hooks()); Assert.Throws<ArgumentOutOfRangeException>(() => logic.Activate(1));
        Assert.Equal(0x10032u, scene.State.Previews[0]); Assert.Equal(1, scene.State.PreviewCount); Assert.Equal(new[] { "P" }, scene.Events);
    }
    /// <summary>일반 준비는 같은 객체/배열/checking/픽셀을 유지하고 회전 옵션과 별도 기록을 보존한다.</summary>
    [Fact]
    public void GeneralResetKeepsPhysicalOwnersAndCurrentChecking()
    {
        var owner = new ClientCursorGlobals(); var life = owner.Lifecycle; var placement = owner.Placement; uint[] previews = life.Previews, context = life.WarningContext; int[] bounds = placement.Bounds; uint checking = 0x80000000; int[] pixels = [37, 41];
        life.Checking = new RawCheckingFlag(() => checking); life.Previews[29] = uint.MaxValue; life.CostType = 100; placement.SharedScreenOrigin = pixels; placement.Bounds[3] = 71; owner.RotationMode.Value = 7; owner.SnapToGrid.Value = uint.MaxValue;
        owner.ResetForGeneralWorld(); Assert.Same(life, owner.Lifecycle); Assert.Same(placement, owner.Placement); Assert.Same(previews, life.Previews); Assert.Same(context, life.WarningContext); Assert.Same(bounds, placement.Bounds);
        Assert.Equal(0x80000000u, life.ReadChecking()); checking = 0; Assert.Equal(0u, life.ReadChecking()); Assert.Equal(92u, life.CostType); Assert.Equal(0u, previews[29]); Assert.Equal(0, bounds[3]); Assert.Equal(7u, owner.RotationMode.Value); Assert.Equal(0u, owner.SnapToGrid.Value);
        pixels[0] = 73; Assert.Equal(73, placement.CurrentOriginX()); Assert.Equal(41, placement.CurrentOriginY());
        var drop = new CursorDropState { Recording = 33, Recorder = 44, Preview = uint.MaxValue, X = 7, ScreenY = 9 }; drop.ResetForGeneralWorld(); Assert.Equal(1u, drop.LockDepth); Assert.Equal(0u, drop.Preview); Assert.Equal(33u, drop.Recording); Assert.Equal(44u, drop.Recorder); Assert.Equal(0f, drop.X); Assert.Equal(0, drop.ScreenY);
    }
    /// <summary>실제 자산의 TYPE 번호로 사정거리 프로세스를 생성하고 잘못된 블록/프레임을 거부한다.</summary>
    [Fact]
    public void ActualAssetTypeNumberDrivesTakeRange()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); var startup = new HostStartupScene(data); var assets = new HostAssetTables(startup, new()); var bound = new CursorLifecycleHooks(); HostCursorLifecycleAssets.Bind(bound, assets);
        // 내장0~69도 번호 변환은 유효하며 자산 블록 조회를 요구하지 않는다.
        for (uint type = 0; type < 188; type++) Assert.Equal(type, bound.TypeNumber!(type));
        Assert.True(bound.HasRange!(bound.TypeNumber!(128))); Assert.False(bound.HasRange!(92)); Assert.Throws<ArgumentOutOfRangeException>(() => bound.TypeNumber!(188)); Assert.Throws<ArgumentOutOfRangeException>(() => bound.DefaultArgument!(69)); Assert.Throws<InvalidOperationException>(() => bound.CostImage!(92, 10000));
        CursorCostImage image = bound.CostImage!(92, 3); Assert.Equal(23u, image.Token);
        var scene = new Scene(); scene.Prepare(Numbers(X86Fixture.Read("cursor-lifecycle")[0][0]));
        // 같은 최초 자산 표를 읽되 이 본체는 TYPE를 수정하지 않는다.
        for (int type = 0; type < scene.Types.Length; type++) scene.Types[type] = assets.LoadedTypes[type];
        BinaryPrimitives.WriteUInt16LittleEndian(scene.Raw(50)[18..], 128); CursorLifecycleHooks hooks = scene.Hooks(); hooks.DefaultArgument = bound.DefaultArgument; hooks.TypeNumber = bound.TypeNumber; hooks.HasRange = bound.HasRange;
        var logic = new RawCursorLifecycle(scene.Pool, scene.Types, scene.Drop, scene.State, hooks); logic.Take(50, 0x160);
        Assert.Equal(128u, scene.State.AbstractType); Assert.Equal(128u, scene.Drop.Type); Assert.Equal(2u, scene.Drop.CursorGump); Assert.Contains("G:100:0", scene.Events);
    }
}
