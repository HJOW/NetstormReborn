using System.Globalization;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 세 실제 PE의 타입별 패턴·전체 순회·범위·끝 상태와 공통 생성자(<see cref="CanonTypeDecoder"/>, <see cref="CanonDecoder"/>, <see cref="RawCheckingFlag"/>)를 대조한다.
/// cpppj <c>CanonTypeDecoderTests</c>의 장면 5종(패치·CD·1037 전수 재생, 우선순위·경계 방어, 156행 진단·반환값, 패턴 순회 소유자 전파)을 옮겼다. (F2c-12, 2026-11-07)
/// </summary>
public sealed class CanonTypeDecoderTests
{
    /// <summary>PE 초기 전역의 타입 번호와 일반 사제 번호다. (F2c-12, 2026-11-07)</summary>
    private static readonly uint[] Types = [107, 82, 94, 157, 131, 129, 140, 142, 158];

    /// <summary>기계어 분석 입력과 같은 프레임 순서 표를 만든다. 기대 결과는 fixture에서만 읽는다. (F2c-12, 2026-11-07)</summary>
    /// <param name="profile">프레임 구성 프로파일(0 정방향, 1 역방향, 2 빈 표)이다.</param>
    /// <returns>합성된 <see cref="RiftTypeFrames"/>다.</returns>
    private static RiftTypeFrames InputFrames(int profile)
    {
        List<RiftFrameCode> codes = [];
        // 정상 표 또는 빈 비패턴 표를 만든다
        for (int side = 'A'; side <= 'P'; side++)
        {
            // 번호 0~10은 셀 해석의 정상 계약 범위다
            for (int number = 0; number <= 10; number++)
            {
                if (profile != 2)
                {
                    codes.Add(new RiftFrameCode((byte)side, (byte)'P', (byte)number, 0));
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
        return new RiftTypeFrames(codes);
    }

    /// <summary>바이트 스팬을 대문자 16진수 문자열로 바꾼다. (F2c-12, 2026-11-07)</summary>
    /// <param name="bytes">바꿀 바이트다.</param>
    /// <returns>대문자 16진수 문자열이다.</returns>
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

    /// <summary>비트·정수 사각형·선택 표와 현재 원본 상태를 C# 결과에 대조한다. (F2c-12, 2026-11-07)</summary>
    /// <param name="edition">대상 판본 이름(<c>originals</c>, <c>originalCD</c>, <c>original1037</c>)이다.</param>
    private static void Replay(string edition)
    {
        bool patch = edition == "originals";
        OriginalEdition version = patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
        string[][] rows = X86Fixture.Read("canontype");
        int count = 0;
        // 모든 회전과 타입 전역의 별칭 입력을 독립적으로 구성한다
        foreach (string[] row in rows)
        {
            if (row[0] != edition)
            {
                continue;
            }
            Assert.Equal(21, row.Length);
            uint kind = X86Fixture.UInt(row[1]);
            RiftTypeFrames frames = InputFrames(X86Fixture.Int(row[4]));
            uint[] patterns = new uint[8];
            Array.Copy(Types, patterns, 8);
            if (row[10] == "1")
            {
                patterns[0] = patterns[1];
            }
            if (row[10] == "2")
            {
                patterns[2] = patterns[4];
            }

            CanonDecoder decoder = CanonTypeDecoder.DecodeCanonType(
                frames,
                X86Fixture.Int(row[11]),
                patterns,
                new CanonTypeQuery(
                    Types[kind],
                    X86Fixture.Int(row[2]),
                    X86Fixture.Int(row[3]),
                    X86Fixture.FloatBits(row[5]),
                    X86Fixture.FloatBits(row[6]),
                    X86Fixture.UInt(row[9]) != 0),
                version);

            string[] bounds = row[13].Split(',');
            int[] actualBounds = decoder.Bounds(X86Fixture.Int(row[7]), X86Fixture.Int(row[8]));
            // 각 사각형의 기대 정수는 실제 Bounds getter 출력이다
            for (int index = 0; index < 4; index++)
            {
                Assert.Equal(X86Fixture.Int(bounds[index]), actualBounds[index]);
            }

            string[] expected = row[14] == "-" ? [] : row[14].Split(';');
            // 생성자의 첫 Advance와 전체 남은 칸 순서·라벨·방향을 비교한다
            foreach (string entry in expected)
            {
                Assert.True(decoder.Valid());
                string[] fields = entry.Split(',');
                Assert.Equal(X86Fixture.Int(fields[0]), decoder.Frame());
                Assert.Equal(X86Fixture.UInt(fields[1]), BitConverter.SingleToUInt32Bits(decoder.X()));
                Assert.Equal(X86Fixture.UInt(fields[2]), BitConverter.SingleToUInt32Bits(decoder.Y()));
                Assert.Equal(X86Fixture.Int(fields[3]), decoder.Label());
                Assert.Equal((char)X86Fixture.Int(fields[4]), decoder.Side());
                decoder.Advance();
            }

            Assert.False(decoder.Valid());
            Assert.Equal(X86Fixture.Int(row[15]), decoder.Frame());
            Assert.Equal(0u, X86Fixture.UInt(row[16]));
            Assert.Equal(X86Fixture.UInt(row[17]), BitConverter.SingleToUInt32Bits(decoder.X()));
            Assert.Equal(X86Fixture.UInt(row[18]), BitConverter.SingleToUInt32Bits(decoder.Y()));
            Assert.Equal(X86Fixture.Int(row[19]), decoder.Label());

            string[] end = row[20].Split(',');
            int[] actualEnd = decoder.Bounds(X86Fixture.Int(row[7]), X86Fixture.Int(row[8]));
            // 칸이 없어도 마지막 좌표·개수로 계산한 원본 범위를 보존한다
            for (int index = 0; index < 4; index++)
            {
                Assert.Equal(X86Fixture.Int(end[index]), actualEnd[index]);
            }
            count++;
        }
        Assert.Equal(patch ? 2079 : 924, count);
    }

    /// <summary>패치 10.78의 홀수 방향·signed -1까지 원본 생성·전체 순회 결과를 대조한다. (F2c-12, 2026-11-07)</summary>
    [Fact]
    public void CanonTypePatchX86() => Replay("originals");

    /// <summary>CD 10.72의 다른 생성 코드와 실제 범위·프레임 helper를 대조한다. (F2c-12, 2026-11-07)</summary>
    [Fact]
    public void CanonTypeCdX86() => Replay("originalCD");

    /// <summary>추가 10.37 PE도 자체 기계어 기대값을 사용한다. (F2c-12, 2026-11-07)</summary>
    [Fact]
    public void CanonType1037X86() => Replay("original1037");

    /// <summary>패턴 수·선행 전역 비교·범위 밖·홀수 CD와 누락 프레임 계약을 검사한다. (F2c-12, 2026-11-07)</summary>
    [Fact]
    public void CanonTypeTablesPrecedenceAndGuards()
    {
        Assert.Equal(2, CanonTypeDecoder.CanonSpecialPatterns(0).Length);
        Assert.Equal(1, CanonTypeDecoder.CanonSpecialPatterns(1).Length);
        Assert.Equal(1, CanonTypeDecoder.CanonSpecialPatterns(2).Length);
        Assert.Equal((byte)'3', CanonTypeDecoder.CanonSpecialPatterns(0)[1].Cells[0][0]);
        Assert.Equal(3, CanonTypeDecoder.CanonSpecialPatterns(1)[0].Width);
        Assert.Equal((byte)'L', CanonTypeDecoder.CanonSpecialPatterns(2)[0].Cells[0][1]);

        RiftTypeFrames frames = InputFrames(0);
        uint[] patterns = new uint[8];
        Array.Copy(Types, patterns, 8);
        Assert.Throws<ArgumentOutOfRangeException>(() => CanonTypeDecoder.CanonSpecialPatterns(3));

        OriginalEdition[] editions = [OriginalEdition.Patch1078, OriginalEdition.Cd1072];
        // 패턴 표 밖이나 CD 홀수 방향은 원본 assert·UI·잘못된 접근 대신 예외로 진단한다
        foreach (OriginalEdition edition in editions)
        {
            Assert.Throws<ArgumentOutOfRangeException>(() =>
                CanonTypeDecoder.DecodeCanonType(frames, 3, patterns, new CanonTypeQuery(94, 2, 0, 0, 0, false), edition));
            Assert.Throws<ArgumentOutOfRangeException>(() =>
                CanonTypeDecoder.DecodeCanonType(frames, 3, patterns, new CanonTypeQuery(157, -1, 0, 0, 0, false), edition));
            Assert.Throws<ArgumentOutOfRangeException>(() =>
                CanonTypeDecoder.DecodeCanonType(frames, 3, patterns, new CanonTypeQuery(131, 1, 0, 0, 0, true), edition));
            RiftTypeFrames missing = InputFrames(2);
            CanonDecoder empty = CanonTypeDecoder.DecodeCanonType(missing, 3, patterns, new CanonTypeQuery(157, 0, 0, 0, 0, false), edition);
            Assert.False(empty.Valid());
        }

        Assert.Throws<ArgumentOutOfRangeException>(() =>
            CanonTypeDecoder.DecodeCanonType(frames, 3, patterns, new CanonTypeQuery(94, 0, 1, 0, 0, false), OriginalEdition.Cd1072));
        Assert.Throws<ArgumentOutOfRangeException>(() =>
            CanonTypeDecoder.DecodeCanonType(frames, 3, patterns, new CanonTypeQuery(94, 0, 8, 0, 0, false), OriginalEdition.Patch1078));
        Assert.Throws<ArgumentException>(() =>
            CanonTypeDecoder.DecodeCanonType(frames, 3, patterns, new CanonTypeQuery(94, 0, 0, float.PositiveInfinity, 0, false), OriginalEdition.Patch1078));

        CanonDecoder explicitPattern = CanonTypeDecoder.DecodeCanonType(
            frames,
            3,
            patterns,
            new CanonTypeQuery(94, 1, 0, 0, 0, true),
            OriginalEdition.Patch1078);
        Assert.Equal(168, explicitPattern.Frame());
    }

    /// <summary>비패턴 다음 칸의 공개 상태와 원문 진단 위치·복귀를 독립 156입력으로 대조한다. (F2c-12, 2026-11-07)</summary>
    [Fact]
    public void CanonCheckingNativeFrameDiagnosticAndReturn156()
    {
        string[][] rows = X86Fixture.Read("canon-decoder-checking");
        int count = 0;
        // 반복자 구성에서 이미 호출하는 첫 Advance와 유효 칸 뒤 끝 상태를 함께 비교한다
        foreach (string[] row in rows)
        {
            Assert.Equal(6, row.Length);
            string[] input = row[0].Split(',');
            Assert.Equal(6, input.Length);
            int frame = X86Fixture.Int(input[1]);
            uint checking = X86Fixture.UInt(input[2]);
            string events = "";
            string outcome = "return";
            string first = "-";
            string last = "-";
            byte flagByte = (byte)X86Fixture.UInt(input[3]);
            RiftTypeFrames frames = new([
                new RiftFrameCode((byte)'A', (byte)'P', 0, flagByte),
                new RiftFrameCode((byte)'A', (byte)'P', 1, flagByte),
            ]);

            CanonDecoderDiagnostics diagnostics = new();
            diagnostics.Checking.Bind(() => checking);
            diagnostics.Assertion = (expression, file, line) =>
            {
                if (events.Length > 0)
                {
                    events += ';';
                }
                events += string.Create(
                    CultureInfo.InvariantCulture,
                    $"X:{Hex(Encoding.Latin1.GetBytes(expression))}:{Hex(Encoding.Latin1.GetBytes(file))}:{line}");
                if (X86Fixture.UInt(input[4]) == 0)
                {
                    throw new InvalidOperationException("명시 Canon 진단 중단");
                }
                if (X86Fixture.UInt(input[5]) != 0)
                {
                    checking = 0;
                }
            };

            static string Snapshot(CanonDecoder decoder) => string.Create(
                CultureInfo.InvariantCulture,
                $"{decoder.Frame()},{(decoder.Valid() ? 1 : 0)},{BitConverter.SingleToUInt32Bits(decoder.X())},{BitConverter.SingleToUInt32Bits(decoder.Y())}");

            try
            {
                CanonDecoder decoder = new(
                    frames,
                    frame,
                    frame,
                    0,
                    20.5f,
                    -3.25f,
                    X86Fixture.UInt(input[0]) != 0,
                    OriginalEdition.Patch1078,
                    diagnostics);
                first = Snapshot(decoder);
                if (decoder.Valid())
                {
                    decoder.Advance();
                }
                last = Snapshot(decoder);
            }
            catch (InvalidOperationException error) when (error.Message == "명시 Canon 진단 중단")
            {
                outcome = "assertion";
            }

            Assert.Equal(row[1], events);
            Assert.Equal(row[2], outcome);
            Assert.Equal(row[3], first);
            Assert.Equal(row[4], last);
            Assert.Equal(X86Fixture.UInt(row[5]), checking);
            count++;
        }
        Assert.Equal(156, count);
    }

    /// <summary>같은 owner의 현재 checking·교체한 진단 콜백을 다음 Advance에서 읽으며 CD에 패치 진단을 추가하지 않는다. (F2c-12, 2026-11-07)</summary>
    [Fact]
    public void CanonCheckingCurrentOwnerAcrossPatternAdvance()
    {
        RiftTypeFrames frames = new([new RiftFrameCode((byte)'A', (byte)'P', 1, 0x20)]);
        CanonPattern pattern = new()
        {
            Width = 2,
            Height = 1,
        };
        pattern.Cells[0] = new CanonPatternCell('1', 'A', 0, 'a');
        pattern.Cells[1] = new CanonPatternCell('1', 'A', 0, 'b');

        uint checking = 0;
        CanonDecoderDiagnostics diagnostics = new();
        diagnostics.Checking.Bind(() => checking);
        int called = 0;

        CanonDecoder decoder = new(frames, pattern, 0, 20, 30, null, OriginalEdition.Patch1078, diagnostics);
        Assert.True(decoder.Valid() && decoder.X() == 20.0f && called == 0);

        diagnostics.Assertion = (expression, file, line) =>
        {
            Assert.Equal("frame == EMPTY_FRAME || !(typeStruct->frameInfo[frame].flags & FF_CRACKED)", expression);
            Assert.Equal("\\Ns\\O\\Canondecoder.cpp", file);
            Assert.Equal(0x325u, line);
            called++;
            checking = 0;
        };

        checking = 0x80000000u;
        decoder.Advance();
        Assert.True(decoder.Valid() && decoder.X() == 21.0f && called == 1 && checking == 0);
        decoder.Advance();
        Assert.False(decoder.Valid());

        checking = 1;
        diagnostics.Assertion = null;
        CanonDecoder cd = new(frames, pattern, 0, 0, 0, null, OriginalEdition.Cd1072, diagnostics);
        Assert.True(cd.Valid());
        cd.Advance();
        Assert.True(cd.Valid());

        Assert.Throws<ArgumentOutOfRangeException>(() =>
            new CanonDecoder(frames, pattern, 0, 0, 0, null, OriginalEdition.Patch1078, diagnostics));

        checking = 0;
        diagnostics.Assertion = (_, _, line) => Assert.Equal(0x305u, line);
        CanonDecoder invalid = new(frames, 0, 2, 0, 0, 0, true, OriginalEdition.Patch1078, diagnostics);
        Assert.True(invalid.Valid() && invalid.Frame() == 2);
        Assert.Throws<ArgumentOutOfRangeException>(() => invalid.Side());
    }
}
