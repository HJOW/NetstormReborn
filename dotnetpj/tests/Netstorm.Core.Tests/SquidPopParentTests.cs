using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 원문 부모의 부분 공간 변경과 콜백 뒤 현재 읽기를 같은 물리에서 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>cpp SquidPopParentTests의 독립 부모 계약을 실제 해시·spot·현재 TYPE로 재생한다.</summary>
public sealed class SquidPopParentTests
{
    /// <summary>현재 자산 표·물리 공간과 외부 변경 입력을 한 수명으로 소유한다.</summary>
    private sealed class Scene
    {
        /// <summary>원본 PATCH의 물리 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, false);
        /// <summary>원본 네 단계 공간 해시다.</summary>
        public readonly RawSquidHash Hash = new();
        /// <summary>부분 OR를 관찰하는 전체 점유 지도다.</summary>
        public readonly byte[] Spots = new byte[65536];
        /// <summary>같은 번호의 현재 필드를 읽는 실제 TYPE 칸이다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>부모가 검사 시점마다 읽는 현재 checking이다.</summary>
        public uint CheckingWord = 1;
        /// <summary>잘못된 좌표 로그를 생략하는 현재 모드다.</summary>
        public bool Metamap = true;
        /// <summary>명시 SHP 경계의 크기 입력이다.</summary>
        public float[] Size = [1, 1];
        /// <summary>최초 Pop 효과의 명시 반환 입력이다.</summary>
        public uint FirstFlags = 1;
        /// <summary>미니맵이 받은 저장된 genus다.</summary>
        public uint MiniGenus;
        /// <summary>Activate가 받은 최종 flags다.</summary>
        public uint ActivateFlags;
        /// <summary>SHP 조회 횟수다.</summary>
        public int FrameReads;
        /// <summary>표시 효과 횟수다.</summary>
        public int Updates;
        /// <summary>최초 Pop 효과 횟수다.</summary>
        public int Firsts;
        /// <summary>Activate 효과 횟수다.</summary>
        public int Activations;
        /// <summary>원문 진단 줄 순서다.</summary>
        public readonly List<uint> Lines = [];
        /// <summary>진단 파일 경로 순서다.</summary>
        public readonly List<string> Files = [];
        /// <summary>완성된 원문 로그다.</summary>
        public readonly List<string> Logs = [];
        /// <summary>하위 효과의 호출 순서다.</summary>
        public readonly List<string> Events = [];
        /// <summary>진단에서 요청할 현재 변경·명시 실패다.</summary>
        public Action<uint>? OnAssertion;
        /// <summary>미니맵에서 요청할 현재 변경이다.</summary>
        public Action? OnMiniMap;
        /// <summary>SHP 경계에서 요청할 현재 변경·실패다.</summary>
        public Action? OnFrame;
        /// <summary>표시에서 요청할 현재 변경이다.</summary>
        public Action? OnUpdate;
        /// <summary>Activate에서 요청할 현재 변경이다.</summary>
        public Action? OnActivate;
        /// <summary>로그 writer에서 요청할 현재 변경이다.</summary>
        public Action? OnLog;
        /// <summary>부모가 도달할 때만 선택하는 실제 경계다.</summary>
        public SquidPopParentHooks Hooks;
        /// <summary>같은 checking 소유자를 빌린 보기다.</summary>
        private readonly RawCheckingFlag _checking;

        /// <summary>같은 raw와 현재 조회·관측 장치만 준비하며 부모 분기 결과는 만들지 않는다.</summary>
        public Scene()
        {
            // 합성 입력에 쓰는 실제 client 슬롯만 확보한다.
            for (int sid = 5; sid <= 60; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
            Types[74].Name = "base"; Types[75].Name = "NOW"; Types[76].Name = "old"; Types[77].Name = "THEN"; Types[74].Flags2 = 1;
            Raw().Clear(); Put(0, 0x501dd0); Raw()[10] = 74; Raw()[11] = 4; _checking = new RawCheckingFlag(() => CheckingWord);
            Hooks = new SquidPopParentHooks
            {
                Type = number => new SquidPopTypeReference(() => Types[number]), Metamap = () => Metamap,
                FrameSize = sid => { Assert.Equal(50, sid); FrameReads++; Events.Add("frame"); OnFrame?.Invoke(); return Size; },
                Trace = (level, text) => { Assert.Equal(29, level); Logs.Add(text); Events.Add("log"); OnLog?.Invoke(); },
                MiniMap = (sid, genus) => { Assert.Equal(50, sid); MiniGenus = genus; Events.Add("mini"); OnMiniMap?.Invoke(); },
                Update = (sid, _) => { Assert.Equal(50, sid); Updates++; Events.Add("update"); OnUpdate?.Invoke(); },
                FirstPop = sid => { Assert.Equal(50, sid); Firsts++; Events.Add("first"); return FirstFlags; },
                Activate = (sid, flags) => { Assert.Equal(50, sid); Activations++; ActivateFlags = flags; Events.Add("activate"); Raw()[11] &= 0xfb; OnActivate?.Invoke(); },
                Assertion = (_, file, line) => { Lines.Add(line); Files.Add(file); Events.Add("assert"); OnAssertion?.Invoke(line); },
            };
        }
        /// <summary>대상 또는 명시 이웃의 같은 물리 슬롯을 읽고 쓴다.</summary>
        public Span<byte> Raw(int sid = 50) => Pool.RawSlot(sid);
        /// <summary>이웃 필드에 영향을 주지 않고 명시 DWORD 입력을 쓴다.</summary>
        public void Put(int offset, uint value, int sid = 50) => BinaryPrimitives.WriteUInt32LittleEndian(Raw(sid)[offset..], value);
        /// <summary>대상의 현재 float 좌표를 추가 반올림 없이 읽는다.</summary>
        public float Coordinate(int offset) => BinaryPrimitives.ReadSingleLittleEndian(Raw()[offset..]);
        /// <summary>호출자가 가진 진단 보기 자체의 연결을 교체한다. 부모는 입구의 값 사본을 유지해야 한다.</summary>
        public void RebindChecking(Func<uint> getter) => _checking.Bind(getter);
        /// <summary>현재 경계·checking을 원문 부모에 공급한다.</summary>
        public RawPopResult Run(float x = 30, float y = 40, uint flags = 0, int sid = 50) => RawSquidPopParent.Run(Pool, Hash, Spots, sid, x, y, flags, _checking, Hooks);
    }

    /// <summary>범위·void·contained 앞단은 미연결 TYPE/SHP/장치를 읽지 않는다.</summary>
    [Fact]
    public void EarlyReturnsDoNotReadAnyDevice()
    {
        var scene = new Scene { Hooks = new SquidPopParentHooks() }; scene.Raw()[10] = 255; byte[] before = scene.Pool.Bytes.ToArray();
        Assert.Equal(RawPopResult.Unchanged, scene.Run(sid: 0)); Assert.Equal(RawPopResult.Unchanged, scene.Run(sid: 24000)); Assert.Equal(before, scene.Pool.Bytes.ToArray());
        // 서로 다른 두 앞단 상태를 독립적으로 공급하고 슬롯이 보존되는지 확인한다.
        foreach (byte state in new byte[] { 0, 12 }) { scene.Raw()[11] = state; byte[] raw = scene.Raw().ToArray(); Assert.Equal(RawPopResult.Unchanged, scene.Run()); Assert.Equal(raw, scene.Raw().ToArray()); }
    }

    /// <summary>좌표 로그는 저장보다 앞서며 writer가 바꾼 현재 TYPE를 이후 사용한다.</summary>
    [Fact]
    public void InvalidCoordinateLogPrecedesStoreAndReadsCurrentType()
    {
        var scene = new Scene { Metamap = false, Size = [3, 1] }; scene.Types[75].Flags2 = 1;
        scene.OnLog = () => { Assert.Equal(0, scene.Coordinate(14)); Assert.Equal(0, scene.Coordinate(18)); scene.Raw()[10] = 75; scene.Put(14, BitConverter.SingleToUInt32Bits(99)); };
        Assert.Equal(RawPopResult.Registered, scene.Run(-1, 7)); Assert.Equal(new[] { "Error base popped to(  -1.0,  7.0). Set to( 10,10)\n" }, scene.Logs);
        Assert.Equal(10, scene.Coordinate(14)); Assert.Equal(10, scene.Coordinate(18)); Assert.Equal(1, scene.Spots[10 * 256 + 10]); Assert.Equal(2, scene.Raw()[33]);
        Assert.Equal((ushort)160, BinaryPrimitives.ReadUInt16LittleEndian(scene.Raw()[22..])); Assert.Equal((ushort)110, BinaryPrimitives.ReadUInt16LittleEndian(scene.Raw()[24..])); Assert.Equal((ushort)50, scene.Hash.Bucket(2, 10, 10));
    }

    /// <summary>초기 TYPE 칸의 현재 발자국·저장 입력 범위와 매 칸 현재 TYPE/좌표를 구별한다.</summary>
    [Fact]
    public void MiniMapPreservesInputBoundsAndCurrentTypeReference()
    {
        var scene = new Scene { Size = [3, 2] }; scene.Types[74].Flags2 = 2; scene.Types[75].Flags2 = 1;
        scene.OnMiniMap = () =>
        {
            Assert.Equal(30, scene.Coordinate(14)); Assert.Equal(40, scene.Coordinate(18));
            scene.Types[74] = new OriginalTypeRecord { FootX = 2, FootY = 1, Flags2 = 2 }; scene.Raw()[10] = 75;
            scene.Put(14, BitConverter.SingleToUInt32Bits(100)); scene.Put(18, BitConverter.SingleToUInt32Bits(110));
        };
        scene.OnFrame = () => { Assert.Equal(1, scene.Spots[40 * 256 + 29]); Assert.Equal(1, scene.Spots[40 * 256 + 30]); Assert.Equal(75, scene.Raw()[10]); };
        Assert.Equal(RawPopResult.Registered, scene.Run()); Assert.Equal(2u, scene.MiniGenus); Assert.Equal((ushort)50, scene.Hash.Bucket(2, 100, 110));
        Assert.Equal((ushort)0, scene.Hash.Bucket(2, 30, 40)); Assert.Equal(0, scene.Spots[110 * 256 + 100]); Assert.Equal(new[] { "mini", "frame", "update", "first", "activate" }, scene.Events);
    }

    /// <summary>checking0의 시작점 진단도 정상 복귀하며 물리0번 칸은 계속 처리한다.</summary>
    [Fact]
    public void InvalidStartResumesCellZeroWithCheckingOff()
    {
        var scene = new Scene { CheckingWord = 0 }; Assert.Equal(RawPopResult.Registered, scene.Run(0.25f, 20.5f));
        Assert.Equal(new uint[] { 0x6b7 }, scene.Lines); Assert.Equal(@"\Ns\O\Squid.cpp", scene.Files.Single()); Assert.Equal(1, scene.Spots[20 * 256]);
        Assert.Equal(0.25f, scene.Coordinate(14)); Assert.Equal(20.5f, scene.Coordinate(18)); Assert.Equal(1, scene.FrameReads);
    }

    /// <summary>grid 후보는 저장하고 진단 뒤 이름·다음 칸의 genus와 해시 좌표는 현재 상태로 읽는다.</summary>
    [Fact]
    public void GridDiagnosticKeepsSavedCandidateAndReadsCurrentNames()
    {
        var scene = new Scene(); scene.Types[74].Flags2 = 0; scene.Types[74].Flags1 = 0x800; scene.Types[74].FootX = 2; scene.Types[75].Flags2 = 1;
        scene.Raw(9)[10] = 76; scene.Hash.Cell(0, 29, 40) = 9;
        scene.OnAssertion = line => { Assert.Equal(0x6e8u, line); scene.CheckingWord = 0; scene.Raw()[10] = 75; scene.Raw(9)[10] = 77; scene.Hash.Cell(0, 29, 40) = 0; scene.Put(14, BitConverter.SingleToUInt32Bits(60)); scene.Put(18, BitConverter.SingleToUInt32Bits(80)); };
        Assert.Equal(RawPopResult.Registered, scene.Run()); Assert.Equal(new uint[] { 0x6e8 }, scene.Lines); Assert.Equal(new[] { "ERROR: gridSid 50( NOW) replacing 9( THEN)\n" }, scene.Logs);
        Assert.Equal(0, scene.Spots[40 * 256 + 29]); Assert.Equal(1, scene.Spots[40 * 256 + 30]); Assert.Equal((ushort)50, scene.Hash.Bucket(1, 60, 80));
    }

    /// <summary>입구에서 값 복사한 checking 보기는 원래 owner를 유지하고 호출자 보기 재연결로 바뀌지 않는다.</summary>
    [Fact]
    public void CheckingViewValueCopyKeepsOriginalOwner()
    {
        var scene = new Scene(); scene.Types[74].Flags2 = 2; scene.Types[74].Flags1 = 0x800; scene.Raw(9)[10] = 76; scene.Hash.Cell(0, 30, 40) = 9;
        scene.OnMiniMap = () => scene.RebindChecking(() => 0);
        Assert.Equal(RawPopResult.Registered, scene.Run()); Assert.Equal(new uint[] { 0x6e8 }, scene.Lines);
        Assert.Equal(new[] { "ERROR: gridSid 50( base) replacing 9( old)\n" }, scene.Logs);
    }

    /// <summary>옆면과 SHP의 도달 실패는 앞선 좌표·점유 OR를 보존하고 이후 해시는 만들지 않는다.</summary>
    [Fact]
    public void ReachedSideAndFrameFailuresPreserveEarlierSpots()
    {
        var side = new Scene(); side.Types[74].Flags2 = 0x4001; side.Types[74].FootX = side.Types[74].FootY = 3;
        side.Raw(9)[10] = 75; side.Types[75].Flags1 = 0x800; side.Hash.Cell(0, 28, 39) = 9;
        Assert.Throws<InvalidOperationException>(() => side.Run()); Assert.Equal(30, side.Coordinate(14)); Assert.Equal(4, side.Raw()[11]); Assert.Equal(0, side.FrameReads);
        Assert.Equal(4, side.Spots.Count(value => value != 0)); Assert.Equal(1, side.Spots[38 * 256 + 28]); Assert.Equal(1, side.Spots[39 * 256 + 28]);
        var frame = new Scene(); frame.Types[74].FootX = 2; frame.OnFrame = () => throw new InvalidOperationException("명시 SHP 실패");
        Assert.Throws<InvalidOperationException>(() => frame.Run()); Assert.Equal(1, frame.FrameReads); Assert.Equal(1, frame.Spots[40 * 256 + 29]); Assert.Equal(1, frame.Spots[40 * 256 + 30]);
        Assert.Equal((ushort)0, frame.Hash.Bucket(1, 30, 40)); Assert.Equal(4, frame.Raw()[11]); Assert.Equal(0, frame.Raw()[40]); Assert.Equal(0, frame.Updates);
    }

    /// <summary>겹침은 미지원 가상표·SHP·가상 후반을 선택하기 전에 정상 반환한다.</summary>
    [Fact]
    public void OverlapPrecedesUnknownVirtualAndFrame()
    {
        var scene = new Scene(); scene.Put(0, 0x12345678); scene.Spots[40 * 256 + 30] = 1; scene.Hooks.FrameSize = null; scene.Hooks.Update = null;
        Assert.Equal(RawPopResult.Overlap, scene.Run()); Assert.Equal(30, scene.Coordinate(14)); Assert.Equal(40, scene.Coordinate(18)); Assert.Equal(4, scene.Raw()[11]); Assert.Equal(0, scene.FrameReads); Assert.Equal((ushort)0, scene.Hash.Bucket(1, 30, 40));
    }

    /// <summary>next 진단 뒤에도 선택 버킷·저장 next를 쓰며 섬 분기는 현재 레벨·TYPE로 선택한다.</summary>
    [Fact]
    public void NextDiagnosticKeepsSelectedBucketAndCurrentIslandBranch()
    {
        var scene = new Scene(); scene.Hash.Bucket(1, 30, 40) = 50; BinaryPrimitives.WriteUInt16LittleEndian(scene.Raw()[8..], 41); scene.Types[75].Flags2 = 2;
        scene.OnAssertion = line => { Assert.Equal(0x123u, line); Assert.Equal(@"c:\ns\o\squid.h", scene.Files.Last()); scene.Hash.Bucket(1, 30, 40) = 9; scene.Raw()[10] = 75; scene.Raw()[33] = 0; scene.Put(14, BitConverter.SingleToUInt32Bits(90)); scene.Put(18, BitConverter.SingleToUInt32Bits(80)); };
        Assert.Equal(RawPopResult.Registered, scene.Run()); Assert.Equal(new uint[] { 0x123 }, scene.Lines); Assert.Equal((ushort)50, BinaryPrimitives.ReadUInt16LittleEndian(scene.Raw()[4..])); Assert.Equal((ushort)127, BinaryPrimitives.ReadUInt16LittleEndian(scene.Raw()[8..]));
        Assert.Equal((ushort)50, scene.Hash.Bucket(1, 30, 40)); Assert.Equal((ushort)0, scene.Hash.Bucket(1, 90, 80)); Assert.Equal(0, scene.Raw()[33]);
    }

    /// <summary>표시 뒤의 현재 extra가 최초 Pop 실행과 flags 병합을 결정한다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void UpdateControlsCurrentFirstPopAndFlags(bool skip)
    {
        var scene = new Scene { FirstFlags = 2 }; scene.Raw()[40] = skip ? (byte)0 : (byte)0x80;
        scene.OnUpdate = () => { Assert.Equal((ushort)50, scene.Hash.Bucket(1, 30, 40)); Assert.Equal(4, scene.Raw()[11] & 4); scene.Raw()[40] = skip ? (byte)0x80 : (byte)1; };
        Assert.Equal(RawPopResult.Registered, scene.Run(flags: 0x2000)); Assert.Equal(skip ? 0 : 1, scene.Firsts); Assert.Equal(skip ? 0x2000u : 0x2002u, scene.ActivateFlags); Assert.Equal(skip ? 0x80 : 0x81, scene.Raw()[40]);
    }

    /// <summary>최종 contained 진단은 checking0에서도 실행하고 정상 복귀·중단 모두 완료 효과를 남긴다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void FinalContainedDiagnosticPreservesEffects(bool abort)
    {
        var scene = new Scene { CheckingWord = 0 }; scene.OnActivate = () => { scene.Raw()[11] |= 8; scene.Raw()[10] = 75; };
        scene.OnAssertion = line => { Assert.Equal(0x71au, line); Assert.Equal(1, scene.Activations); Assert.Equal(0x80, scene.Raw()[40]); if (abort) throw new InvalidOperationException("명시 후단 진단 종료"); };
        if (abort) Assert.Throws<InvalidOperationException>(() => scene.Run()); else Assert.Equal(RawPopResult.Registered, scene.Run());
        Assert.Equal(new uint[] { 0x71a }, scene.Lines); Assert.Equal((ushort)50, scene.Hash.Bucket(1, 30, 40)); Assert.Equal(8, scene.Raw()[11]); Assert.Equal(75, scene.Raw()[10]);
    }

    /// <summary>NaN 입구의 크기 조회 생략과 미니맵 뒤 현재 크기의 시작점 재검사를 구별한다.</summary>
    [Fact]
    public void CurrentWorldBoundsKeepOrderedComparisonsAndReadTiming()
    {
        var clamped = new Scene(); int limit = 32, reads = 0; clamped.Hooks.WorldCells = () => { reads++; return limit; };
        Assert.Equal(RawPopResult.Registered, clamped.Run(40, 20)); Assert.Equal(10, clamped.Coordinate(14)); Assert.Equal(2, reads); Assert.Empty(clamped.Lines);
        var unordered = new Scene(); int nanReads = 0; unordered.Hooks.WorldCells = () => { nanReads++; return 32; };
        Assert.Equal(RawPopResult.Registered, unordered.Run(float.NaN, 20)); Assert.Equal(10, unordered.Coordinate(14)); Assert.Equal(1, nanReads); Assert.Empty(unordered.Lines);
        var current = new Scene(); current.Types[74].Flags2 = 2; limit = 64; reads = 0; current.Hooks.WorldCells = () => { reads++; return limit; }; current.OnMiniMap = () => limit = 16;
        Assert.Equal(RawPopResult.Registered, current.Run(40, 20)); Assert.Equal(40, current.Coordinate(14)); Assert.Equal(2, reads); Assert.Equal(new uint[] { 0x6b7 }, current.Lines); Assert.Equal(2, current.Spots[20 * 256 + 40]);
    }

    /// <summary>동일 MSVC 도구 집합의 실제 CRT 관측으로 특수값·부호 있는0·짝수 반올림을 고정한다.</summary>
    [Theory]
    [InlineData(0x7fc00000u, "  nan")]
    [InlineData(0xffc00000u, "-nan(ind)")]
    [InlineData(0x7f800000u, "  inf")]
    [InlineData(0xff800000u, " -inf")]
    [InlineData(0x80000000u, " -0.0")]
    [InlineData(0x40100000u, "  2.2")]
    [InlineData(0x7fffffffu, "  nan")]
    [InlineData(0xffffffffu, " -nan")]
    [InlineData(0x40900000u, "  4.5")]
    public void CoordinateLogsMatchActualCppCrt(uint bits, string formatted)
    {
        var scene = new Scene { Metamap = false }; Assert.Equal(RawPopResult.Registered, scene.Run(BitConverter.UInt32BitsToSingle(bits), -1));
        Assert.Equal(new[] { $"Error base popped to( {formatted}, -1.0). Set to( 10,10)\n" }, scene.Logs);
    }

    /// <summary>원문 화면 WORD는 넓은 ftol 뒤 하위 비트를 보존하며 .NET uint 포화로 대체하지 않는다.</summary>
    [Fact]
    public void WideScreenWordPrecedesFrameFailure()
    {
        var scene = new Scene(); scene.Types[74].Flags2 = 0; scene.Hooks.WorldCells = () => int.MaxValue; scene.OnFrame = () => throw new InvalidOperationException("명시 SHP 실패");
        Assert.Throws<InvalidOperationException>(() => scene.Run(268435456, 1)); Assert.Equal((ushort)0, BinaryPrimitives.ReadUInt16LittleEndian(scene.Raw()[22..])); Assert.Equal((ushort)11, BinaryPrimitives.ReadUInt16LittleEndian(scene.Raw()[24..]));
        Assert.Equal(268435456, scene.Coordinate(14)); Assert.Equal(4, scene.Raw()[11]);
    }

    /// <summary>원본Point 전체·Pop 앞단260입력의 signed 크기·float 저장·NaN 단락을 대조한다.</summary>
    [Fact]
    public void Native260WorldBoundsMatchReturnBitsAndReadCounts()
    {
        string[][] rows = X86Fixture.Read("pop-world-bounds"); Assert.Equal(260, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(8, row.Length); int limit = X86Fixture.Int(row[0]), reads = 0;
            float x = X86Fixture.FloatBits(row[1]), y = X86Fixture.FloatBits(row[2]);
            Func<int> current = () => { reads++; return limit; };
            bool valid = RawSquidPopParent.IsCurrentPopPointValid(x, y, current);
            bool same = valid == (X86Fixture.UInt(row[3]) != 0) && reads == X86Fixture.UInt(row[4]);
            reads = 0; float mappedX = x, mappedY = y;
            if (!RawSquidPopParent.IsCurrentPopPointValid(mappedX, mappedY, current)) mappedX = mappedY = 10;
            return same && BitConverter.SingleToUInt32Bits(mappedX) == X86Fixture.UInt(row[5])
                && BitConverter.SingleToUInt32Bits(mappedY) == X86Fixture.UInt(row[6]) && reads == X86Fixture.UInt(row[7]);
        });
    }

    /// <summary>현재 getter는 선택 뒤 자기 연결을 비워도 끝까지 실행하며 x 불가에서는 조회하지 않는다.</summary>
    [Fact]
    public void PointGetterSelfUnbindingAndInvalidXPreserveSelection()
    {
        Func<int>? getter = null; int calls = 0;
        getter = () => { getter = null; calls++; return 32; };
        Assert.True(RawSquidPopParent.IsCurrentPopPointValid(31, 20, getter)); Assert.Null(getter); Assert.Equal(1, calls);
        getter = () => { calls++; throw new InvalidOperationException("선택 세계 크기 조회 실패"); };
        Assert.False(RawSquidPopParent.IsCurrentPopPointValid(float.NaN, 10, getter)); Assert.False(RawSquidPopParent.IsCurrentPopPointValid(-1, 10, getter)); Assert.Equal(1, calls);
        Assert.Throws<InvalidOperationException>(() => RawSquidPopParent.IsCurrentPopPointValid(1, float.NaN, getter)); Assert.Equal(2, calls);
        Assert.True(RawSquidPopParent.IsCurrentPopPointValid(255, 255)); Assert.False(RawSquidPopParent.IsCurrentPopPointValid(256, 10));
    }
}
