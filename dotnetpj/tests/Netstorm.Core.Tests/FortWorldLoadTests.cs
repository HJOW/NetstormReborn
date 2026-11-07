using System.Globalization;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 실제 부모·Territory(<see cref="RawFortWorldLoad"/>)의 독립 관찰을 같은 현재 Builder·3204BYTE·청크·owner로 대조한다.
/// cpppj <c>FortWorldLoadTests</c>의 장면 3종(534행 전수 재생, 외부 풀·누락 자식·절단 영역 거부, CanonDecoder 소비)을 옮겼다. (F2c-12, 2026-11-07)
/// </summary>
public sealed class FortWorldLoadTests
{
    /// <summary>원본 입력의 고정 열 순서다. 기대 후처리·섬 수·owner는 포함하지 않는다. (F2c-12, 2026-11-07)</summary>
    private enum FortWorldField
    {
        /// <summary>실행 종류(0 Load, 1 ReadTerritory)다. (F2c-12, 2026-11-07)</summary>
        Kind = 0,

        /// <summary>Load의 alternate 인자다. (F2c-12, 2026-11-07)</summary>
        Alternate = 1,

        /// <summary>초기 owner 번호다. (F2c-12, 2026-11-07)</summary>
        Owner = 2,

        /// <summary>초기 섬 개수다. (F2c-12, 2026-11-07)</summary>
        Count = 3,

        /// <summary>섬 0의 초기 존재 플래그다. (F2c-12, 2026-11-07)</summary>
        Exists = 4,

        /// <summary>영역 데이터 프로파일 번호다. (F2c-12, 2026-11-07)</summary>
        Profile = 5,

        /// <summary>스트림 초기 위치다. (F2c-12, 2026-11-07)</summary>
        Position = 6,

        /// <summary>스트림 끝 위치다. (F2c-12, 2026-11-07)</summary>
        End = 7,

        /// <summary>초기 로딩 깊이다. (F2c-12, 2026-11-07)</summary>
        Depth = 8,

        /// <summary>중간 콜백 변형 번호다. (F2c-12, 2026-11-07)</summary>
        Mutation = 9,

        /// <summary>Template 존재 여부 초기값이다. (F2c-12, 2026-11-07)</summary>
        TemplatePresent = 10,
    }

    /// <summary>리틀 엔디안 정수를 바이트 스팬의 지정 오프셋에 기록한다. (F2c-12, 2026-11-07)</summary>
    /// <param name="raw">기록할 대상 버퍼다.</param>
    /// <param name="offset">바이트 오프셋이다.</param>
    /// <param name="value">기록할 부호 없는 32비트 값이다.</param>
    /// <param name="width">기록할 바이트 너비다.</param>
    private static void Put(Span<byte> raw, int offset, uint value, int width = 4)
    {
        // 하위 바이트부터 지정 너비만큼 순서대로 기록한다
        for (int index = 0; index < width; index++)
        {
            raw[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>바이트 스팬을 소문자 16진수 문자열로 바꾼다. (F2c-12, 2026-11-07)</summary>
    /// <param name="bytes">바꿀 바이트다.</param>
    /// <returns>소문자 16진수 문자열이다.</returns>
    private static string Hex(ReadOnlySpan<byte> bytes)
    {
        StringBuilder builder = new(bytes.Length * 2);
        // 각 바이트를 두 자리 소문자 16진수로 변환한다
        for (int index = 0; index < bytes.Length; index++)
        {
            builder.Append(bytes[index].ToString("x2", CultureInfo.InvariantCulture));
        }
        return builder.ToString();
    }

    /// <summary>전체 원본 block을 복원하며 비활성 기록·padding을 생략하지 않는다. (F2c-12, 2026-11-07)</summary>
    /// <param name="text">RLE 텍스트다.</param>
    /// <returns>복원한 바이트 배열이다.</returns>
    private static byte[] WorldBlocks(string text)
    {
        List<byte> bytes = [];
        string[] runs = text.Split(';');
        // 저장 run의 그대로인 바이트·횟수를 사용한다
        foreach (string run in runs)
        {
            string[] fields = run.Split(':');
            byte[] raw = Convert.FromHexString(fields[0]);
            uint repeat = X86Fixture.UInt(fields[1]);
            // 원본 block을 해석 없이 그 횟수만 반복한다
            for (uint count = 0; count < repeat; count++)
            {
                bytes.AddRange(raw);
            }
        }
        return bytes.ToArray();
    }

    /// <summary>실제 기존 Builder의 16방향·P변형·번호 0..10 합성 코드 입력이다. (F2c-12, 2026-11-07)</summary>
    /// <returns>합성된 <see cref="RiftTypeFrames"/>다.</returns>
    private static RiftTypeFrames WorldPieceFrames()
    {
        List<RiftFrameCode> codes = [];
        // 원본도 같은 native 입력표를 읽으며 frame 결과를 미리 만들지 않는다
        for (byte side = (byte)'A'; side <= (byte)'P'; side++)
        {
            // 각 방향에서 실제 패턴의 번호 검색을 허용한다
            for (byte number = 0; number < 11; number++)
            {
                codes.Add(new RiftFrameCode(side, (byte)'P', number, 0));
            }
        }
        codes.Add(codes[0]);
        return new RiftTypeFrames(codes);
    }

    /// <summary>현재 소비자 청크를 원본 X열 우선 12BYTE로 직렬화한다. (F2c-12, 2026-11-07)</summary>
    /// <param name="chunks">직렬화할 청크 지도다.</param>
    /// <returns>직렬화한 3072바이트 배열이다.</returns>
    private static byte[] WorldChunks(ChunkMap chunks)
    {
        byte[] bytes = new byte[256 * 12];
        // X열이 바깥이며 각 열의 Y 16칸을 저장한다
        for (int x = 0; x < 16; x++)
        {
            // 각 청크의 이웃 record·ordinal 폭도 그대로 남긴다
            for (int y = 0; y < 16; y++)
            {
                ref ChunkEntry entry = ref chunks.At(x, y);
                int offset = (x * 16 + y) * 12;
                bytes[offset] = entry.Island;
                bytes[offset + 1] = unchecked((byte)entry.Side);
                entry.Record.AsSpan(0, 6).CopyTo(bytes.AsSpan(offset + 2, 6));
                Put(bytes, offset + 8, unchecked((uint)entry.Ordinal));
            }
        }
        return bytes;
    }

    /// <summary>
    /// 단일 원본 입력의 실제 풀·현재 3204BYTE·후처리 경계를 소유하는 장면이다.
    /// (F2c-12, 2026-11-07)
    /// </summary>
    private sealed class FortWorldReplayScene
    {
        /// <summary>현재 Builder·Runtime가 읽는 단일 판본 풀이다. (F2c-12, 2026-11-07)</summary>
        public RawSidPool Pool;

        /// <summary>같은 Runtime의 현재 공간 참조다. (F2c-12, 2026-11-07)</summary>
        public RawSquidHash Hash = new();

        /// <summary>실제 주소 동일성이 있는 world 지도다. (F2c-12, 2026-11-07)</summary>
        public ChunkMap Chunks = new();

        /// <summary>원본 count+100개 32BYTE/padding 기록(3204바이트)이다. (F2c-12, 2026-11-07)</summary>
        public byte[] Raw = new byte[RawIslandStorage.Bytes];

        /// <summary>Add·Bounds와 같은 mode 전역이다. (F2c-12, 2026-11-07)</summary>
        public FortFinishOwnerState Modes = new();

        /// <summary>선택 Builder는 owner 가상 전파를 요청하지 않는다. (F2c-12, 2026-11-07)</summary>
        public RawPostPopList Residences = new();

        /// <summary>현재 지도 리비전 전역이다. (F2c-12, 2026-11-07)</summary>
        public uint Revision;

        /// <summary>현재 월드 다시 그리기 전역이다. (F2c-12, 2026-11-07)</summary>
        public uint Changed;

        /// <summary>현재 puzzlePiece 타입 번호다. (F2c-12, 2026-11-07)</summary>
        public uint Piece = 107;

        /// <summary>현재 로컬 owner 번호다. (F2c-12, 2026-11-07)</summary>
        public uint Owner;

        /// <summary>현재 로딩 깊이 값이다. (F2c-12, 2026-11-07)</summary>
        public uint Depth;

        /// <summary>Generate가 호출 직전 다시 읽는 현재 지역 없음 전역이다. (F2c-12, 2026-11-07)</summary>
        public int NoRegion = 127;

        /// <summary>원본과 같은 현재 puzzlePiece 프레임 입력이다. (F2c-12, 2026-11-07)</summary>
        public RiftTypeFrames[] Frames;

        /// <summary>native의 11개 입력 DWORD다. (F2c-12, 2026-11-07)</summary>
        public uint[] Input;

        /// <summary>현재 pos 앞 prefix와 실제 120BYTE 섹션이다. (F2c-12, 2026-11-07)</summary>
        public byte[] Data;

        /// <summary>현재 pos·end를 각 실제 child가 읽는 스트림 커서다. (F2c-12, 2026-11-07)</summary>
        public FortTerritoryStream Stream;

        /// <summary>CurrentTemplate이 다음 stage마다 현재 owner 칸을 다시 읽는 템플릿 배열이다. (F2c-12, 2026-11-07)</summary>
        public FortTemplate[] Templates = new FortTemplate[9];

        /// <summary>진단 callback이 현재 Template 포인터를 복구하는 명시 입력이다. (F2c-12, 2026-11-07)</summary>
        public bool[] Present = new bool[9];

        /// <summary>원본 외부 경계의 순서를 기록한다. (F2c-12, 2026-11-07)</summary>
        public List<string> EventsList = [];

        /// <summary>보고 뒤 실제 반환한 진단 수다. (F2c-12, 2026-11-07)</summary>
        public int Assertions;

        /// <summary>위 기록을 그대로 빌리며 복사 목록을 만들지 않는 섬 런타임이다. (F2c-12, 2026-11-07)</summary>
        public RawIslandRuntime Islands;

        /// <summary>원본 입력만 구성하고 최종 지도·owner·후처리의 기대값은 계산하지 않는다. (F2c-12, 2026-11-07)</summary>
        /// <param name="edition">원본 판본이다.</param>
        /// <param name="fields">쉼표로 구분된 11개 입력 필드 문자열이다.</param>
        public FortWorldReplayScene(OriginalEdition edition, string fields)
        {
            Pool = new RawSidPool(32768, false, null, edition);
            int frameCount = edition == OriginalEdition.Patch1078 ? 188 : 171;
            Frames = new RiftTypeFrames[frameCount];
            RiftTypeFrames empty = new([]);
            // 모든 슬롯을 빈 프레임 표로 채운다
            for (int index = 0; index < frameCount; index++)
            {
                Frames[index] = empty;
            }

            string[] parts = fields.Split(',');
            Assert.Equal(11, parts.Length);
            Input = new uint[11];
            // 모든 현재 DWORD를 원본 열 순서로 읽는다
            for (int index = 0; index < parts.Length; index++)
            {
                Input[index] = X86Fixture.UInt(parts[index]);
            }

            Owner = Input[(int)FortWorldField.Owner];
            Depth = Input[(int)FortWorldField.Depth];
            Raw.AsSpan().Fill(0xA5);
            Put(Raw, 0, Input[(int)FortWorldField.Count]);
            Put(Raw, 8, Input[(int)FortWorldField.Exists]);
            Frames[Piece] = WorldPieceFrames();

            // 초기 원본 청크의 모든 12BYTE를 준비한다
            for (int x = 0; x < 16; x++)
            {
                // 다음 input에서 이전 순회 결과를 재사용하지 않는다
                for (int y = 0; y < 16; y++)
                {
                    Chunks.At(x, y) = new ChunkEntry
                    {
                        Island = 127,
                        Side = (sbyte)'.',
                        Record = [0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5],
                        Ordinal = 0x12345678,
                    };
                }
            }

            uint pos = Input[(int)FortWorldField.Position];
            Data = new byte[pos + 120];
            if (Input[(int)FortWorldField.Profile] != 0)
            {
                ReadOnlySpan<byte> first = [3, 1, 0x11, 7, 0xAA, 0xBB];
                first.CopyTo(Data.AsSpan((int)pos, 6));
            }
            if (Input[(int)FortWorldField.Profile] == 2)
            {
                ReadOnlySpan<byte> second = [3, 1, 0x55, 31, 0xCC, 0xDD];
                second.CopyTo(Data.AsSpan((int)pos + 18, 6));
            }

            Stream = new FortTerritoryStream { Bytes = Data, Position = pos, End = Input[(int)FortWorldField.End] };
            Array.Fill(Present, Input[(int)FortWorldField.TemplatePresent] != 0);
            // 각 Template의 Flag는 관찰용 identity이며 월드 효과를 대신 계산하지 않는다
            for (int index = 0; index < Templates.Length; index++)
            {
                Templates[index] = new FortTemplate { Flag = (byte)index };
            }

            Islands = new RawIslandRuntime(
                Pool,
                Hash,
                Raw,
                Modes,
                Residences,
                () => Changed,
                value => Changed = value,
                () => Revision,
                value => Revision = value,
                new IslandRuntimeHooks
                {
                    SetResidenceOwner = (_, _) => throw new InvalidOperationException("선택 Builder는 Residence owner 경계를 요청하지 않는다"),
                    Assertion = _ => throw new InvalidOperationException("선택 Builder의 Runtime 진단 입력 불일치"),
                });
        }

        /// <summary>기존 Builder는 실제 실행하고 상위 child는 명시 관찰 경계로 남긴다. (F2c-12, 2026-11-07)</summary>
        /// <returns>구성한 <see cref="FortWorldLoadHooks"/>다.</returns>
        public FortWorldLoadHooks Hooks()
        {
            return new FortWorldLoadHooks
            {
                TemplatePresent = slot => Present[slot],
                Territory = _ =>
                {
                    if (Input[(int)FortWorldField.Mutation] == 1)
                    {
                        Owner = 2;
                    }
                    if (Input[(int)FortWorldField.Mutation] == 8)
                    {
                        NoRegion = 22;
                    }
                    if (Input[(int)FortWorldField.Mutation] == 9)
                    {
                        Owner = 5;
                    }
                    return Stream;
                },
                CurrentTemplate = slot => Templates[slot],
                Assertion = reason =>
                {
                    Assertions++;
                    EventsList.Add(reason == FortWorldLoadAssertion.MissingTerritoryTemplate ? "A:T" : "A:E");
                    if (reason == FortWorldLoadAssertion.MissingTerritoryTemplate)
                    {
                        Present[Input[(int)FortWorldField.Owner]] = true;
                    }
                },
                LoadingDepthChanged = value =>
                {
                    Assert.Equal(Depth, value);
                    EventsList.Add(string.Create(CultureInfo.InvariantCulture, $"L:{value}"));
                },
                BuilderAssertion = _ => throw new InvalidOperationException("유효 방향·프레임의 선택 Builder 진단 불일치"),
                Generate = (state, region) =>
                {
                    Assert.True(state.Owner == 1 && state.Bounds.Left == 1 && state.Bounds.Right == 15);
                    EventsList.Add(string.Create(CultureInfo.InvariantCulture, $"G:{region}"));
                    if (Input[(int)FortWorldField.Mutation] == 2)
                    {
                        Owner = 3;
                    }
                    if (Input[(int)FortWorldField.Mutation] == 7)
                    {
                        Put(Raw, 8, 1);
                    }
                },
                Objects = (fort, slot) =>
                {
                    Assert.Equal(slot, fort.Flag);
                    EventsList.Add(string.Create(CultureInfo.InvariantCulture, $"O:{slot}"));
                    if (Input[(int)FortWorldField.Mutation] == 3)
                    {
                        Owner = 4;
                    }
                },
                Money = (fort, slot) =>
                {
                    Assert.Equal(slot, fort.Flag);
                    EventsList.Add(string.Create(CultureInfo.InvariantCulture, $"M:{slot}"));
                    if (Input[(int)FortWorldField.Mutation] == 4)
                    {
                        Owner = 5;
                    }
                },
                Badges = (fort, slot) =>
                {
                    Assert.Equal(slot, fort.Flag);
                    EventsList.Add(string.Create(CultureInfo.InvariantCulture, $"B:{slot}"));
                    if (Input[(int)FortWorldField.Mutation] == 5)
                    {
                        Owner = 6;
                    }
                },
                Technology = (fort, slot) =>
                {
                    Assert.Equal(slot, fort.Flag);
                    EventsList.Add(string.Create(CultureInfo.InvariantCulture, $"T:{slot}"));
                    if (Input[(int)FortWorldField.Mutation] == 6)
                    {
                        Owner = 7;
                    }
                },
                PopulateIsland = (state, region) =>
                {
                    Assert.True(state.Owner == 1 && region == 0);
                    EventsList.Add("I:0");
                },
                RebuildGraph = reset =>
                {
                    Assert.False(reset);
                    EventsList.Add("R:0");
                },
            };
        }

        /// <summary>실제 기록한 경계 요청만 원본 순서로 연결한다. (F2c-12, 2026-11-07)</summary>
        /// <returns>세미콜론으로 합친 이벤트 문자열이다.</returns>
        public string Events() => EventsList.Count == 0 ? "-" : string.Join(';', EventsList);
    }

    /// <summary>원본 534 고유 입력의 전체 청크·3204BYTE·현재 owner·depth와 stage 순서를 전수 대조한다. (F2c-12, 2026-11-07)</summary>
    [Fact]
    public void FortWorldLoadReplaysBothNativeParentAndTerritory()
    {
        string[][] rows = X86Fixture.Read("fortworldload");
        int checkedCount = 0;
        // 각 원본 행은 새 저장소에서 시작하며 기대 Builder 효과는 계산하지 않는다
        foreach (string[] row in rows)
        {
            Assert.Equal(12, row.Length);
            FortWorldReplayScene scene = new(
                row[0] == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072,
                row[1]);
            RawFortWorldLoad loader = new(
                scene.Pool,
                scene.Chunks,
                scene.Islands,
                scene.Frames,
                () => scene.Piece,
                () => scene.Owner,
                () => scene.NoRegion,
                () => scene.Depth,
                value => scene.Depth = value,
                scene.Hooks());

            byte[] output = new byte[120];
            if (scene.Input[(int)FortWorldField.Kind] != 0)
            {
                loader.ReadTerritory(scene.Owner, output);
            }
            else
            {
                loader.Load(unchecked((int)scene.Input[(int)FortWorldField.Alternate]));
            }

            string[] islandFields = row[9].Split(',');
            byte[] expectedIslands = WorldBlocks(islandFields[1]);
            ReadOnlySpan<byte> actualRecords = scene.Raw.AsSpan(4);

            Assert.Equal(X86Fixture.UInt(row[2]), scene.Stream.Position);
            Assert.Equal(X86Fixture.UInt(row[3]), scene.Stream.End);
            Assert.Equal(X86Fixture.UInt(row[4]), scene.Depth);
            Assert.Equal(X86Fixture.UInt(row[5]), scene.Owner);
            Assert.Equal(X86Fixture.Int(row[6]), scene.NoRegion);
            if (scene.Input[(int)FortWorldField.Kind] != 0)
            {
                Assert.Equal(row[7], Hex(output));
            }
            Assert.True(WorldChunks(scene.Chunks).AsSpan().SequenceEqual(WorldBlocks(row[8])));
            Assert.Equal(X86Fixture.UInt(islandFields[0]), scene.Islands.Count());
            Assert.True(actualRecords.SequenceEqual(expectedIslands));
            Assert.Equal(row[10], scene.Events());
            Assert.Equal(X86Fixture.Int(row[11]), scene.Assertions);
            checkedCount++;
        }
        Assert.Equal(534, checkedCount);
    }

    /// <summary>동일 풀·필수 효과·물리 120BYTE 경계를 보존하며 없는 child를 성공 처리하지 않는다. (F2c-12, 2026-11-07)</summary>
    [Fact]
    public void FortWorldLoadRejectsForeignPoolMissingChildAndTruncatedTerritory()
    {
        FortWorldReplayScene scene = new(OriginalEdition.Patch1078, "0,0,1,0,0,1,0,120,0,0,1");
        FortWorldLoadHooks hooks = scene.Hooks();
        hooks.PopulateIsland = null;
        Assert.Throws<ArgumentException>(() =>
            new RawFortWorldLoad(
                scene.Pool,
                scene.Chunks,
                scene.Islands,
                scene.Frames,
                () => scene.Piece,
                () => scene.Owner,
                () => scene.NoRegion,
                () => scene.Depth,
                value => scene.Depth = value,
                hooks));

        RawSidPool foreign = new(32768, false, null, OriginalEdition.Patch1078);
        Assert.Throws<ArgumentException>(() =>
            new RawFortWorldLoad(
                foreign,
                scene.Chunks,
                scene.Islands,
                scene.Frames,
                () => scene.Piece,
                () => scene.Owner,
                () => scene.NoRegion,
                () => scene.Depth,
                value => scene.Depth = value,
                scene.Hooks()));

        RawFortWorldLoad loader = new(
            scene.Pool,
            scene.Chunks,
            scene.Islands,
            scene.Frames,
            () => scene.Piece,
            () => scene.Owner,
            () => scene.NoRegion,
            () => scene.Depth,
            value => scene.Depth = value,
            scene.Hooks());
        scene.Stream.Bytes = scene.Data.AsMemory(0, 119);
        byte[] output = new byte[120];
        Assert.Throws<ArgumentOutOfRangeException>(() => loader.ReadTerritory(1, output));
        Assert.Equal(0u, scene.Stream.Position);
    }

    /// <summary>Fort 부모의 선택 진단 owner가 지역 Builder까지 전달되어 생성·Objects 후처리 전에 중단한다. (F2c-12, 2026-11-07)</summary>
    [Fact]
    public void CanonConsumersFortLoaderBeforeWorldGeneration()
    {
        FortWorldReplayScene scene = new(OriginalEdition.Patch1078, "0,0,1,0,0,1,0,120,0,0,1");
        IReadOnlyList<RiftFrameCode> codes = scene.Frames[scene.Piece].Codes;
        List<RiftFrameCode> cracked = new(codes.Count);
        // 원문 fixture의 패턴 모양은 유지한다
        foreach (RiftFrameCode code in codes)
        {
            cracked.Add(new RiftFrameCode(code.Side, code.Variant, code.Number, (byte)(code.Flags | 0x20)));
        }
        scene.Frames[scene.Piece] = new RiftTypeFrames(cracked);

        CanonDecoderDiagnostics diagnostics = new();
        uint checking = 1;
        uint reported = 0;
        diagnostics.Checking.Bind(() => checking);
        diagnostics.Assertion = (_, _, line) =>
        {
            Assert.Equal(0x325u, line);
            reported++;
            throw new InvalidOperationException("Fort Canon 진단 중단");
        };

        FortWorldLoadHooks hooks = scene.Hooks();
        hooks.DecoderDiagnostics = diagnostics;
        RawFortWorldLoad loader = new(
            scene.Pool,
            scene.Chunks,
            scene.Islands,
            scene.Frames,
            () => scene.Piece,
            () => scene.Owner,
            () => scene.NoRegion,
            () => scene.Depth,
            value => scene.Depth = value,
            hooks);
        Assert.Throws<InvalidOperationException>(() => loader.Load(0));
        Assert.Equal(1u, reported);
        Assert.Empty(scene.EventsList);
    }
}
