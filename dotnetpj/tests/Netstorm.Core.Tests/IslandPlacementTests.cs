using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 편집기 섬 충돌 검사(<see cref="RawIslandPlacement"/>)에 두 보호 PE의 독립 반환·진단을 같은 입력으로 재생한다.
/// cpppj <c>IslandPlacementTests</c>의 장면 2종(4896행 전수 재생, 동적 청크 맵·경계 차용)을 옮겼다. (F2c-12, 2026-11-07)
/// </summary>
public sealed class IslandPlacementTests
{
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
    private static void Put(Span<byte> raw, int offset, uint value)
    {
        // 하위 바이트부터 4바이트를 순서대로 기록한다
        for (int index = 0; index < 4; index++)
        {
            raw[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>현재 <see cref="ChunkMap"/>의 물리 256개 X열 우선 record를 원본 12BYTE로 직렬화한다. (F2c-12, 2026-11-07)</summary>
    /// <param name="chunks">직렬화할 청크 지도다.</param>
    /// <returns>직렬화한 3072바이트 배열이다.</returns>
    private static byte[] ChunkBytes(ChunkMap chunks)
    {
        byte[] bytes = new byte[256 * 12];
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

    /// <summary>원문 puzzlePiece 입력에 필요한 A~P/번호 0~10 프레임을 제공한다. (F2c-12, 2026-11-07)</summary>
    /// <param name="profile">프레임 프로파일(0 정방향, 1 역방향, 2 빈 표, 3 G방향 누락)이다.</param>
    /// <returns>합성된 <see cref="RiftTypeFrames"/>다.</returns>
    private static RiftTypeFrames PlacementFrames(uint profile)
    {
        List<RiftFrameCode> codes = [];
        // 빈 profile 외에는 현재 디코더가 방향과 번호로 검색할 모든 프레임을 넣는다
        for (byte side = (byte)'A'; side <= (byte)'P'; side++)
        {
            // 각 방향의 원본 variation 번호 범위를 보존한다
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

    /// <summary>새 모듈이 사용하는 실제 세 진단과 decoder 진단을 native 문자 순서로 기록한다. (F2c-12, 2026-11-07)</summary>
    /// <param name="value">빌더 진단 종류다.</param>
    /// <returns>진단 한 글자 코드다.</returns>
    private static char PlacementDiagnostic(RawIslandBuilderAssertion value) => value switch
    {
        RawIslandBuilderAssertion.InvalidBounds => 'B',
        RawIslandBuilderAssertion.OddDirection => 'O',
        RawIslandBuilderAssertion.MissingFrame => 'F',
        RawIslandBuilderAssertion.ChunkRange => 'R',
        _ => throw new InvalidOperationException("CanPlace의 원문 범위 밖 진단"),
    };

    /// <summary>실제 모양 64개·방향 4개·경계·충돌·빈/누락 프레임의 양판본 결과 4896행을 직접 대조한다. (F2c-12, 2026-11-07)</summary>
    [Fact]
    public void IslandPlacementReplaysNativeReadOnlyCollisionAndBounds()
    {
        string[][] rows = X86Fixture.Read("island-placement");
        int checkedCount = 0;
        // 원본 각 고유 입력을 한 번 재생한다
        foreach (string[] row in rows)
        {
            Assert.Equal(4, row.Length);
            string[] raw = row[1].Split(',');
            Assert.Equal(8, raw.Length);
            uint[] input = new uint[8];
            // signed bounds도 DWORD 원문으로 읽어 호스트 부호 변환을 분리한다
            for (int index = 0; index < input.Length; index++)
            {
                input[index] = Number(raw[index]);
            }

            OriginalEdition edition = row[0] == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
            ChunkMap chunks = new();
            uint piece = 107;
            int frameCount = edition == OriginalEdition.Patch1078 ? 188 : 171;
            RiftTypeFrames[] frames = new RiftTypeFrames[frameCount];
            RiftTypeFrames empty = new([]);
            // 모든 슬롯을 빈 프레임 표로 채운다
            for (int index = 0; index < frameCount; index++)
            {
                frames[index] = empty;
            }
            frames[piece] = PlacementFrames(input[2]);

            // 실제 고정 지도 전체의 임의 기존 record·ordinal을 원본 입력과 동일하게 준비한다
            for (int x = 0; x < 16; x++)
            {
                // 한 점 또는 전체 점유는 현재 패턴을 고려하지 않은 독립 입력이다
                for (int y = 0; y < 16; y++)
                {
                    byte island = (byte)(input[3] == 2 || (input[3] == 1 && x == 1 && y == 1) ? 0 : 127);
                    chunks.At(x, y) = new ChunkEntry
                    {
                        Island = island,
                        Side = (sbyte)'.',
                        Record = [0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5],
                        Ordinal = 0x12345678,
                    };
                }
            }

            byte[] before = ChunkBytes(chunks);
            RawIslandBuilderState state = new(
                new ChunkRect(unchecked((int)input[4]), unchecked((int)input[5]), unchecked((int)input[6]), unchecked((int)input[7])),
                1);
            RawIslandBuilderState beforeState = state.Clone();
            byte[] record = [(byte)input[0], 1, (byte)input[1], 0x55, 0xAA, 0xBB];
            List<char> eventsList = [];
            bool result = RawIslandPlacement.CanPlaceIsland(
                edition,
                chunks,
                frames,
                () => piece,
                state,
                record,
                diagnostic => eventsList.Add(PlacementDiagnostic(diagnostic)));

            string events = eventsList.Count == 0 ? "-" : string.Join(';', eventsList);
            Assert.Equal(X86Fixture.UInt(row[2]) != 0, result);
            Assert.Equal(row[3], events);
            Assert.True(ChunkBytes(chunks).AsSpan().SequenceEqual(before));
            Assert.Equal(beforeState.Bounds.Left, state.Bounds.Left);
            Assert.Equal(beforeState.Bounds.Top, state.Bounds.Top);
            Assert.Equal(beforeState.Bounds.Right, state.Bounds.Right);
            Assert.Equal(beforeState.Bounds.Bottom, state.Bounds.Bottom);
            Assert.Equal(beforeState.Owner, state.Owner);
            checkedCount++;
        }
        Assert.Equal(4896, checkedCount);
    }

    /// <summary>판정에서 우측·하단을 제외하고 원본에 없는 하한 거부나 청크 쓰기를 추가하지 않는다. (F2c-12, 2026-11-07)</summary>
    [Fact]
    public void IslandPlacementCurrentMapAndBoundsAreBorrowed()
    {
        ChunkMap chunks = new();
        RawIslandBuilderState state = new(new ChunkRect(0, 0, 16, 16), 1);
        uint piece = 107;
        uint diagnostics = 0;
        RiftTypeFrames[] frames = new RiftTypeFrames[188];
        RiftTypeFrames empty = new([]);
        // 모든 슬롯을 빈 프레임 표로 채운다
        for (int index = 0; index < frames.Length; index++)
        {
            frames[index] = empty;
        }
        frames[piece] = PlacementFrames(0);

        byte[] record = [3, 1, 0, 0, 0, 0];
        Action<RawIslandBuilderAssertion> diagnostic = _ => diagnostics++;
        Assert.True(RawIslandPlacement.CanPlaceIsland(OriginalEdition.Patch1078, chunks, frames, piece, state, record, diagnostic));

        chunks.At(0, 0).Island = 0;
        Assert.False(RawIslandPlacement.CanPlaceIsland(OriginalEdition.Patch1078, chunks, frames, piece, state, record, diagnostic));

        chunks.At(0, 0).Island = 127;
        state.Bounds = new ChunkRect(0, 0, 0, 16);
        Assert.False(RawIslandPlacement.CanPlaceIsland(OriginalEdition.Patch1078, chunks, frames, piece, state, record, diagnostic));
        Assert.Equal(0u, diagnostics);
    }
}
