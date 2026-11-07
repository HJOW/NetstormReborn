using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 <c>Place</c>/<c>PlaceAll</c>(<see cref="RawIslandBuilder"/>)의 전체 청크·3204BYTE와 후속 소비자를 검사한다.
/// cpppj <c>IslandBuilderTests</c>의 장면 4종(양판본 전수 재생, 라이브 청크·런타임·쿼리 결합, 누락 저장소·효과 거부, CanonDecoder 소비)을 옮겼다. (F2c-12, 2026-11-07)
/// </summary>
public sealed class IslandBuilderTests
{
    /// <summary>실제 두 PE의 puzzlePiece 타입 번호다. (F2c-12, 2026-11-07)</summary>
    private const uint RawBuilderPiece = 107;

    /// <summary>fixture의 원본 물리 청크 수(256)다. (F2c-12, 2026-11-07)</summary>
    private const int RawBuilderChunks = 256;

    /// <summary>fixture의 원본 물리 섬 레코드 수(100)다. (F2c-12, 2026-11-07)</summary>
    private const int RawBuilderIslands = 100;

    /// <summary>부호 있는/없는 10진수 문자열을 32비트 DWORD 비트 패턴으로 파싱한다. (F2c-12, 2026-11-07)</summary>
    /// <param name="value">파싱할 문자열이다.</param>
    /// <returns>32비트 부호 없는 값이다.</returns>
    private static uint Number(string value) =>
        value.StartsWith('-')
            ? unchecked((uint)int.Parse(value, CultureInfo.InvariantCulture))
            : uint.Parse(value, CultureInfo.InvariantCulture);

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

    /// <summary>16방향·번호 0..10의 합성 프레임 입력을 만든다. 출력 frame을 미리 계산하지 않는다. (F2c-12, 2026-11-07)</summary>
    /// <param name="profile">프레임 프로파일(0 정방향, 1 역방향, 2 빈 표, 3 G방향 누락)이다.</param>
    /// <returns>합성된 <see cref="RiftTypeFrames"/>다.</returns>
    private static RiftTypeFrames BuilderFrames(uint profile)
    {
        List<RiftFrameCode> codes = [];
        // 빈 profile 2 외에는 모든 방향의 원본 패턴 variation 번호를 공급한다
        for (byte side = (byte)'A'; side <= (byte)'P'; side++)
        {
            // 순서를 바꾼 profile 1도 실제 FindNumber가 프레임을 찾게 한다
            for (byte number = 0; number < 11; number++)
            {
                if (profile != 2)
                {
                    codes.Add(new RiftFrameCode(side, (byte)'P', number, 0));
                }
            }
        }
        if (profile == 1)
        {
            codes.Reverse();
        }
        if (profile != 2)
        {
            codes.Add(codes[0]);
        }
        if (profile == 3)
        {
            codes.RemoveAll(code => code.Side == (byte)'G');
        }
        return new RiftTypeFrames(codes);
    }

    /// <summary>원본 같은 길이 block RLE를 풀며 존재·owner·bounds를 계산해 대신하지 않는다. (F2c-12, 2026-11-07)</summary>
    /// <param name="text">RLE 텍스트다.</param>
    /// <param name="width">블록 하나 바이트 폭이다.</param>
    /// <param name="count">총 블록 개수다.</param>
    /// <returns>풀어낸 바이트 배열이다.</returns>
    private static byte[] BuilderBlocks(string text, int width, int count)
    {
        List<byte> bytes = new(width * count);
        string[] runs = text.Split(';');
        // 모든 run은 같은 물리 폭의 원본 block을 반복한다
        foreach (string run in runs)
        {
            string[] fields = run.Split(':');
            Assert.Equal(2, fields.Length);
            byte[] block = Convert.FromHexString(fields[0]);
            Assert.Equal(width, block.Length);
            uint repeat = X86Fixture.UInt(fields[1]);
            // padding과 count 밖 record도 전체 크기까지 원문으로 복원한다
            for (uint index = 0; index < repeat; index++)
            {
                bytes.AddRange(block);
            }
        }
        Assert.Equal(count * width, bytes.Count);
        return bytes.ToArray();
    }

    /// <summary>현재 <see cref="ChunkMap"/>의 물리 256개 X열 우선 record를 원본 12BYTE로 직렬화한다. (F2c-12, 2026-11-07)</summary>
    /// <param name="chunks">직렬화할 청크 지도다.</param>
    /// <returns>직렬화한 3072바이트 배열이다.</returns>
    private static byte[] BuilderChunkBytes(ChunkMap chunks)
    {
        byte[] bytes = new byte[RawBuilderChunks * 12];
        // 네이티브 inline 배열의 X열을 먼저 읽는다
        for (int x = 0; x < 16; x++)
        {
            // 각 X열의 Y 16칸을 같은 offset으로 저장한다
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
    /// fixture의 builder 전역·현재 저장소를 준비하며 종료 출력은 native에서만 읽는 장면이다.
    /// (F2c-12, 2026-11-07)
    /// </summary>
    private sealed class BuilderReplayScene
    {
        /// <summary>실제 Runtime과 source가 공유하는 판본별 풀이다. (F2c-12, 2026-11-07)</summary>
        public RawSidPool Pool;

        /// <summary>Runtime.Bounds/Add의 현재 실제 월드 해시 참조다. (F2c-12, 2026-11-07)</summary>
        public RawSquidHash Hash = new();

        /// <summary>주소가 같은 월드 청크 지도다. (F2c-12, 2026-11-07)</summary>
        public ChunkMap World = new();

        /// <summary>별도 임시 청크 지도다. (F2c-12, 2026-11-07)</summary>
        public ChunkMap Custom = new();

        /// <summary>실제 100개 32바이트와 개수 DWORD(3204바이트)다. (F2c-12, 2026-11-07)</summary>
        public byte[] Records = new byte[RawIslandStorage.Bytes];

        /// <summary>Runtime의 원래 owner·현재 owner와 같은 모드 전역이다. (F2c-12, 2026-11-07)</summary>
        public FortFinishOwnerState Modes = new();

        /// <summary>현재 거주 목록이며 Place/Add/Bounds에서 읽지 않는다. (F2c-12, 2026-11-07)</summary>
        public RawPostPopList Residences = new();

        /// <summary>실제 Runtime의 지도 리비전 전역이다. (F2c-12, 2026-11-07)</summary>
        public uint Revision;

        /// <summary>실제 Runtime의 월드 다시 그리기 전역이다. (F2c-12, 2026-11-07)</summary>
        public uint Changed;

        /// <summary>현재 puzzlePiece 타입 번호다. (F2c-12, 2026-11-07)</summary>
        public uint Piece = RawBuilderPiece;

        /// <summary>native와 같은 유효·역순·빈 프레임 입력 표다. (F2c-12, 2026-11-07)</summary>
        public RiftTypeFrames[] Frames;

        /// <summary>원본 보고 뒤 계속 진행한 진단의 순서다. (F2c-12, 2026-11-07)</summary>
        public List<char> EventsList = [];

        /// <summary>현재 고유 입력의 진단 횟수다. (F2c-12, 2026-11-07)</summary>
        public uint Assertions;

        /// <summary>위 3204BYTE를 같은 풀·지도에서 읽는 섬 런타임이다. (F2c-12, 2026-11-07)</summary>
        public RawIslandRuntime Islands;

        /// <summary>판본 크기·필수 실제 Runtime 경계를 준비한다. (F2c-12, 2026-11-07)</summary>
        /// <param name="edition">원본 판본이다.</param>
        public BuilderReplayScene(OriginalEdition edition)
        {
            Pool = new RawSidPool(32768, false, null, edition);
            int frameCount = edition == OriginalEdition.Patch1078 ? 188 : 171;
            Frames = new RiftTypeFrames[frameCount];
            RiftTypeFrames empty = new([]);
            // 모든 슬롯을 빈 프레임 표로 초기화한다
            for (int index = 0; index < frameCount; index++)
            {
                Frames[index] = empty;
            }
            Islands = new RawIslandRuntime(
                Pool,
                Hash,
                Records,
                Modes,
                Residences,
                () => Changed,
                value => Changed = value,
                () => Revision,
                value => Revision = value,
                new IslandRuntimeHooks
                {
                    SetResidenceOwner = (_, _) => throw new InvalidOperationException("Add/Bounds는 Residence owner를 요청하지 않는다"),
                    Assertion = _ => throw new InvalidOperationException("Builder fixture의 Runtime 진단은 원본 분류와 불일치한다"),
                });
        }

        /// <summary>각 행에서 native와 같은 full raw·padding·기존 count 밖 영역을 초기화한다. (F2c-12, 2026-11-07)</summary>
        /// <param name="count">초기 섬 개수 DWORD다.</param>
        /// <param name="profile">프레임 프로파일 번호다.</param>
        public void Prepare(uint count, uint profile)
        {
            Records.AsSpan().Fill(0xA5);
            Put(Records, 0, count);
            Frames[Piece] = BuilderFrames(profile);
            EventsList.Clear();
            Assertions = 0;
            // 네이티브 초기 chunk 12BYTE의 모든 필드를 current getter에 준비한다
            for (int x = 0; x < 16; x++)
            {
                // 같은 world·custom은 둘 다 처음부터 현재 명시 입력으로 준비한다
                for (int y = 0; y < 16; y++)
                {
                    byte[] rec = [0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5];
                    World.At(x, y) = new ChunkEntry { Island = 127, Side = (sbyte)'.', Record = rec, Ordinal = 0x12345678 };
                    byte[] recCustom = [0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5];
                    Custom.At(x, y) = new ChunkEntry { Island = 127, Side = (sbyte)'.', Record = recCustom, Ordinal = 0x12345678 };
                }
            }
        }

        /// <summary>각 원본 진단의 순서를 native 기록의 한 문자와 같게 관찰한다. (F2c-12, 2026-11-07)</summary>
        /// <param name="assertion">발생한 빌더 진단 종류다.</param>
        public void Diagnostic(RawIslandBuilderAssertion assertion)
        {
            Assertions++;
            char code = assertion switch
            {
                RawIslandBuilderAssertion.InvalidBounds => 'B',
                RawIslandBuilderAssertion.EqualRange => 'E',
                RawIslandBuilderAssertion.OddDirection => 'O',
                RawIslandBuilderAssertion.MissingFrame => 'F',
                RawIslandBuilderAssertion.ChunkRange => 'R',
                RawIslandBuilderAssertion.BoundsMissingIsland => 'M',
                RawIslandBuilderAssertion.BoundsNotFound => 'N',
                _ => throw new InvalidOperationException("알 수 없는 빌더 진단"),
            };
            EventsList.Add(code);
        }

        /// <summary>진단이 없으면 <c>-</c>이며 중복 진단도 발생 순서로 유지한다. (F2c-12, 2026-11-07)</summary>
        /// <returns>세미콜론으로 합친 진단 문자열이다.</returns>
        public string Events() => EventsList.Count == 0 ? "-" : string.Join(';', EventsList);
    }

    /// <summary><c>Place</c>/<c>PlaceAll</c>의 전수 원본 출력을 모든 기존 record·padding·청크·진단 순서까지 대조한다. (F2c-12, 2026-11-07)</summary>
    [Fact]
    public void IslandBuilderReplaysBothNativeFullStorage()
    {
        string[][] rows = X86Fixture.Read("islandbuilder");
        int checkedCount = 0;
        // 양판본 고유 row만 재생한다
        foreach (string[] row in rows)
        {
            Assert.Equal(10, row.Length);
            OriginalEdition edition = row[0] == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
            string[] input = row[2].Split(',');
            Assert.Equal(14, input.Length);
            uint[] value = new uint[14];
            // 원본 DWORD 입력은 부호 변환 전에 그대로 저장한다
            for (int field = 0; field < value.Length; field++)
            {
                value[field] = Number(input[field]);
            }
            Assert.True(value[10] == 16 && value[11] == 16);

            BuilderReplayScene scene = new(edition);
            scene.Prepare(value[3], value[9]);
            RawIslandBuilderState state = new(
                new ChunkRect(unchecked((int)value[4]), unchecked((int)value[5]), unchecked((int)value[6]), unchecked((int)value[7])),
                value[1]);
            ChunkMap target = value[0] != 0 ? scene.World : scene.Custom;
            RawIslandBuilder builder = new(
                target,
                scene.World,
                scene.Islands,
                scene.Frames,
                () => scene.Piece,
                state,
                scene.Diagnostic);

            byte[] record = Convert.FromHexString(row[3]);
            uint result = 0;
            if (row[1] == "Place")
            {
                Assert.Equal(6, record.Length);
                result = builder.Place(record, value[2], value[8] != 0);
            }
            else
            {
                builder.PlaceAll(record, unchecked((int)value[12]), unchecked((int)value[13]));
            }

            byte[] chunks = BuilderBlocks(row[5], 12, RawBuilderChunks);
            int split = row[7].IndexOf(',', StringComparison.Ordinal);
            Assert.True(split >= 0);
            byte[] islandBlocks = BuilderBlocks(row[7][(split + 1)..], 32, RawBuilderIslands);
            byte[] islands = new byte[4 + islandBlocks.Length];
            Put(islands, 0, X86Fixture.UInt(row[7][..split]));
            islandBlocks.CopyTo(islands.AsSpan(4));

            Assert.Equal(X86Fixture.UInt(row[4]), result);
            Assert.True(BuilderChunkBytes(target).AsSpan().SequenceEqual(chunks));
            Assert.True(islands.AsSpan().SequenceEqual(scene.Records));
            Assert.Equal(row[8], scene.Events());
            Assert.Equal(X86Fixture.UInt(row[9]), scene.Assertions);
            if (value[0] == 0)
            {
                Assert.True(BuilderChunkBytes(scene.World).AsSpan().SequenceEqual(BuilderBlocks(row[6], 12, RawBuilderChunks)));
            }
            Assert.Equal(value[0] != 0, builder.InWorld());
            Assert.Same(scene.Pool, builder.Pool());
            checkedCount++;
        }
        Assert.Equal(1156, checkedCount);
    }

    /// <summary>실제 World의 한 Pool·Runtime·ChunkMap을 후속 Count·Owner·Matches와 <see cref="RawIslandQueries"/>가 직접 소비한다. (F2c-12, 2026-11-07)</summary>
    [Fact]
    public void IslandBuilderLiveWorldChunkRuntimeAndQueryConsumer()
    {
        OriginalEdition[] editions = [OriginalEdition.Patch1078, OriginalEdition.Cd1072];
        // 원래 owner BYTE·현재 owner DWORD의 분리와 같은 현재 청크의 재조회는 양판본에 적용한다
        foreach (OriginalEdition edition in editions)
        {
            BuilderReplayScene scene = new(edition);
            scene.Prepare(0, 0);
            scene.Islands.Reset();
            uint diagnostics = 0;
            RawIslandBuilderState state = new() { Owner = 2 };
            RawIslandBuilder builder = new(
                scene.World,
                scene.World,
                scene.Islands,
                scene.Frames,
                () => scene.Piece,
                state,
                _ => diagnostics++);

            byte[] record = [3, 1, 0, 7, 0xAA, 0xBB];
            uint region = builder.Place(record, 0);
            Assert.True(region == 0 && scene.Islands.Count() == 1 && scene.Islands.Exists(0) == 1 && scene.Islands.Owner(0) == 2 && scene.Islands.Matches(0, 2, 0));

            ChunkRect bounds = scene.World.Bounds(0);
            FortRandomRectangle saved = scene.Islands.Bounds(0);
            Assert.Equal(bounds.Left * 16, saved.Left);
            Assert.Equal(bounds.Top * 16, saved.Top);
            Assert.Equal(bounds.Right * 16, saved.Right);
            Assert.Equal(bounds.Bottom * 16, saved.Bottom);

            scene.Islands.Owner(0, 3);
            RawIslandQueries queries = new(scene.Islands, scene.Modes);
            Assert.Equal(3u, scene.Islands.Owner(0));
            Assert.True(scene.Islands.Matches(0, 2, 0));
            Assert.Equal(1u, queries.CountOwned(3));
            if (edition == OriginalEdition.Patch1078)
            {
                Assert.Equal(1u, scene.Revision);
            }
            Assert.True(scene.Islands.Count() == 1 && builder.InWorld() && ReferenceEquals(builder.Pool(), scene.Pool) && diagnostics == 0);
        }
    }

    /// <summary>형식·필수 진단·물리 Territory 범위를 효과 전 거부하며 정상 empty 범위는 원본 진단 후 반환한다. (F2c-12, 2026-11-07)</summary>
    [Fact]
    public void IslandBuilderRejectsMissingPhysicalStorageAndEffects()
    {
        BuilderReplayScene scene = new(OriginalEdition.Patch1078);
        scene.Prepare(2, 0);
        RawIslandBuilderState state = new();
        Assert.Throws<ArgumentException>(() =>
            new RawIslandBuilder(scene.World, scene.World, scene.Islands, scene.Frames, scene.Piece, state, null));

        RawIslandBuilder builder = new(scene.World, scene.World, scene.Islands, scene.Frames, scene.Piece, state, scene.Diagnostic);
        byte[] before = scene.Records.ToArray();
        byte[] records = new byte[120];
        Assert.Throws<ArgumentOutOfRangeException>(() => builder.PlaceAll(records.AsSpan(0, 119)));
        Assert.Throws<ArgumentOutOfRangeException>(() => builder.PlaceAll(records, -1, 20));
        Assert.Throws<ArgumentOutOfRangeException>(() => builder.PlaceAll(records, 0, 21));
        builder.PlaceAll(records, 4, 4);
        Assert.Equal("E", scene.Events());
        Assert.True(scene.Records.AsSpan().SequenceEqual(before));
    }

    /// <summary>실제 Builder가 패턴을 쓰기 전에 현재 Canon 진단을 읽으며 상태 포인터를 복사해도 같은 owner다. (F2c-12, 2026-11-07)</summary>
    [Fact]
    public void CanonConsumersIslandBuilderBeforeChunkWrites()
    {
        BuilderReplayScene scene = new(OriginalEdition.Patch1078);
        scene.Prepare(0, 0);
        IReadOnlyList<RiftFrameCode> codes = scene.Frames[scene.Piece].Codes;
        List<RiftFrameCode> cracked = new(codes.Count);
        // 기존 정상 패턴 번호·순서는 유지하고 진단 비트만 입력한다
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
            throw new InvalidOperationException("섬 Canon 진단 중단");
        };

        RawIslandBuilderState state = new() { DecoderDiagnostics = diagnostics };
        byte[] before = BuilderChunkBytes(scene.World);
        RawIslandBuilder builder = new(
            scene.World,
            scene.World,
            scene.Islands,
            scene.Frames,
            scene.Piece,
            state,
            _ => throw new InvalidOperationException("별도 Builder 진단 미선택"));
        byte[] record = [3, 1, 0, 7, 0xAA, 0xBB];
        Assert.Throws<InvalidOperationException>(() => builder.Place(record, 0));
        Assert.Equal(1u, reported);
        Assert.True(BuilderChunkBytes(scene.World).AsSpan().SequenceEqual(before));
    }
}
