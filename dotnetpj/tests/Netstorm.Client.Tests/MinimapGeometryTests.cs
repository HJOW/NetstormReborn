using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;

namespace Netstorm.Client.Tests; // 원본 미니맵의 기하와 실제8BIT 출력을 대조한다.

/// <summary>보호 PE/CRT 관측의 입력만 제품에 공급하고 상태·경계·전체 픽셀을 비교한다.</summary>
public sealed class MinimapGeometryTests
{
    /// <summary>signed 입력도 동일DWORD로 복원한다.</summary>
    private static uint N(string value) => unchecked((uint)long.Parse(value, CultureInfo.InvariantCulture));
    /// <summary>원본 입력의 signed32 표현이다.</summary>
    private static int S(string value) => unchecked((int)N(value));
    /// <summary>float 입력 비트를 변환 없이 복원한다.</summary>
    private static float F(string value) => BitConverter.UInt32BitsToSingle(N(value));
    /// <summary>실제 float 결과의 원시DWORD다.</summary>
    private static uint Bits(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>현재 원시 값을 원본 출력 순서로 묶는다.</summary>
    private static string Pack(params uint[] values) => string.Join(':', values.Select(value => value.ToString(CultureInfo.InvariantCulture)));
    /// <summary>한 소비자에만 링크한 기존 독립 관측을 읽는다.</summary>
    private static IEnumerable<string[]> Rows(string name) => File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", name)).Where(line => line.Length != 0 && !line.StartsWith('#')).Select(line => line.Split('\t'));
    /// <summary>같은 실제 외부 진입 인자를 한 번만 찾는다.</summary>
    private static string[] Event(string trace, string name) => Assert.Single(trace.Split(';'), item => item.StartsWith(name + ':', StringComparison.Ordinal)).Split(':');
    /// <summary>원본497f20이 받은 두 float 비트와 대조한다.</summary>
    private static void Center(string[] row, (float X, float Y) target)
    {
        var observed = Event(row[23], "center"); Assert.Equal(3, observed.Length);
        Assert.Equal(N(observed[1]), Bits(target.X)); Assert.Equal(N(observed[2]), Bits(target.Y));
    }
    /// <summary>1054행 중 callback 변이26행을 제외한 순수1028행의 전체 state/view/기하 경계를 대조한다.</summary>
    [Fact]
    public void NativePure1028PreserveFullStateViewAndBoundaryArguments()
    {
        int total = 0, excluded = 0, compared = 0; var counts = new Dictionary<string, int>();
        // 같은 앞20개 입력 열만 실제 제품 상태/목표에 공급한다.
        foreach (var row in Rows("minimap-geometry-x86.tsv"))
        {
            Assert.Equal(27, row.Length); total++;
            if (row[19] != "-") { excluded++; continue; }
            var state = new MinimapGeometryState { Width = F(row[2]), Height = F(row[3]), CropX = S(row[4]), CropY = S(row[5]), Scrolling = N(row[6]) };
            var view = new MinimapGeometryView(N(row[7]), N(row[8]), new(S(row[9]), S(row[10]), S(row[11]), S(row[12])), S(row[13]), S(row[14]));
            counts[row[1]] = counts.GetValueOrDefault(row[1]) + 1;
            switch (row[1])
            {
                case "rebuild": Assert.Equal(row[23] == "dirty:0", MinimapGeometry.Rebuild(state, view)); break;
                case "move": Center(row, MinimapGeometry.Target(state, F(row[15]), F(row[16]))); break;
                case "idle": Center(row, MinimapGeometry.Drag(state, F(row[15]), F(row[16]))); break;
                case "attach": MinimapGeometry.ClampAttachment(state); break;
                case "draw":
                    var plan = MinimapGeometry.PlanDraw(state, view, new(S(row[17]), S(row[18])));
                    var blit = Event(row[23], "blit"); var frame = Event(row[23], "frame"); Assert.Equal(9, blit.Length); Assert.Equal(6, frame.Length);
                    Assert.Equal(new(S(blit[1]), S(blit[2])), plan.Destination); Assert.Equal(S(blit[3]), unchecked(plan.Source.Y * 128 + plan.Source.X));
                    Assert.Equal("128", blit[4]); Assert.Equal("0", blit[5]); Assert.Equal("0", blit[6]); Assert.Equal(new(S(row[4]), S(row[5])), plan.Source);
                    Assert.Equal(new(S(blit[7]), S(blit[8])), plan.Size); Assert.Equal(new(S(frame[1]), S(frame[2]), S(frame[3]), S(frame[4])), plan.Camera); break;
                default: Assert.Fail("미니맵 원본 기하 연산 누락"); break;
            }
            Assert.Equal(row[21], Pack(Bits(state.Width), Bits(state.Height), unchecked((uint)state.CropX), unchecked((uint)state.CropY), state.Scrolling));
            Assert.Equal(row[22], Pack(view.CameraX, view.CameraY, unchecked((uint)view.Viewport.Left), unchecked((uint)view.Viewport.Top), unchecked((uint)view.Viewport.Right), unchecked((uint)view.Viewport.Bottom), unchecked((uint)view.PixelsX), unchecked((uint)view.PixelsY)));
            Assert.Equal("1", row[20]); Assert.Equal("0", row[24]); Assert.True(N(row[25]) > 0 && N(row[25]) >= N(row[26])); compared++;
        }
        Assert.Equal(1054, total); Assert.Equal(26, excluded); Assert.Equal(1028, compared);
        Assert.Equal(new[] { 254, 36, 216, 272, 250 }, new[] { counts["rebuild"], counts["move"], counts["idle"], counts["attach"], counts["draw"] });
    }
    /// <summary>명시 bitmap/guard 입력 패턴이다. 음수 주소에서도 Python의 바닥 나눗셈과 같다.</summary>
    private static byte Pattern(long index, long seed) => unchecked((byte)(index * 73 + (index >> 7) * 19 + seed));
    /// <summary>독립 원문이 실제 변경한 구간을 전체 배경에 복원한다.</summary>
    private static byte[] Pixels(string runs, byte background)
    {
        byte[] result = new byte[192 * 160]; Array.Fill(result, background);
        if (runs == "-") return result;
        // 출력 hex는 원본 관측 그대로이며 제품의 복사/선 알고리즘으로 기대값을 만들지 않는다.
        foreach (string run in runs.Split(';')) { var pair = run.Split(':'); Convert.FromHexString(pair[1]).CopyTo(result, S(pair[0])); }
        return result;
    }
    /// <summary>원본이 읽은 연속 주소에서 할당 밖 BYTE 집합만 복원한다.</summary>
    private static HashSet<long> Guards(string ranges)
    {
        var result = new HashSet<long>(); if (ranges == "-") return result;
        // 실제 읽기 구간만 펼쳐 겹치는 dword 읽기를 BYTE 주소 한 개로 센다.
        foreach (string range in ranges.Split(';'))
        {
            var pair = range.Split(':'); long start = long.Parse(pair[0], CultureInfo.InvariantCulture), length = long.Parse(pair[1], CultureInfo.InvariantCulture);
            // bitmap의 앞뒤 guard 주소만 비교한다.
            for (long offset = start; offset < start + length; offset++) if (offset < 0 || offset >= 0x4000) result.Add(offset);
        }
        return result;
    }
    /// <summary>기존 실제 RawBitmap/LinesClipped와 새 기하를 조립해120관측 전체 표면과40guard 관측을 대조한다.</summary>
    [Fact]
    public void Native120BlitsMatchFullSurfaceAndAllGuardReads()
    {
        int rows = 0, guardRows = 0;
        // 기대값을 입력으로 쓰지 않고 앞24개 입력만 실제 제품에 전달한다.
        foreach (var row in Rows("minimap-blit-x86.tsv"))
        {
            Assert.Equal(33, row.Length); var geometry = new MinimapGeometryState { Width = F(row[2]), Height = F(row[3]), CropX = S(row[4]), CropY = S(row[5]) };
            var view = new MinimapGeometryView(N(row[12]), N(row[13]), new(S(row[14]), S(row[15]), unchecked(S(row[14]) + S(row[16])), unchecked(S(row[15]) + S(row[17]))), S(row[18]), S(row[19]));
            var destination = new ScreenPoint(S(row[6]), S(row[7])); var plan = MinimapGeometry.PlanDraw(geometry, view, destination); var blit = row[28].Split(':'); var frame = row[29].Split(':');
            Assert.Equal(8, blit.Length); Assert.Equal(5, frame.Length); long sourceBase = (long)plan.Source.Y * 128 + plan.Source.X;
            Assert.Equal(new(S(blit[0]), S(blit[1])), plan.Destination); Assert.Equal(long.Parse(blit[2], CultureInfo.InvariantCulture), sourceBase);
            Assert.Equal("128", blit[3]); Assert.Equal("0", blit[4]); Assert.Equal("0", blit[5]); Assert.Equal(new(S(blit[6]), S(blit[7])), plan.Size);
            Assert.Equal(new(S(frame[0]), S(frame[1]), S(frame[2]), S(frame[3])), plan.Camera); Assert.Equal(N(row[23]), N(frame[4]));
            byte background = unchecked((byte)N(row[22])); byte[] bitmap = new byte[0x4000], pixels = new byte[192 * 160]; Array.Fill(pixels, background);
            // 명시한 입력 메모리만 복원한다.
            for (int i = 0; i < bitmap.Length; i++) bitmap[i] = Pattern(i, N(row[20]));
            var state = new GumpDrawState(); var clip = new ScreenRect(S(row[8]), S(row[9]), S(row[10]), S(row[11])); GumpDraw.SetGumpDrawClip(state, clip);
            var target = new GumpPixelTarget(pixels, 192, 160, state); var bounds = new ScreenRect(destination.X, destination.Y, destination.X + (int)geometry.Width, destination.Y + (int)geometry.Height); var guards = new HashSet<long>();
            target.RawBitmap(bitmap, 128, sourceBase, plan.Size, destination, bounds, offset => { guards.Add(offset); return Pattern(offset, N(row[21])); });
            int left = plan.Camera.Left, top = plan.Camera.Top, right = unchecked(plan.Camera.Right - 1), bottom = unchecked(plan.Camera.Bottom - 1); uint ink = N(row[23]);
            target.LinesClipped([new(new(left, top), new(right, top), ink), new(new(left, bottom), new(right, bottom), ink), new(new(left, top), new(left, bottom), ink), new(new(right, top), new(right, bottom), ink)], bounds);
            Assert.Equal(Pixels(row[25], background), pixels); Assert.True(guards.SetEquals(Guards(row[26])), row[0]); Assert.Equal(guards.Count != 0, N(row[27]) != 0); if (guards.Count != 0) guardRows++;
            Assert.Equal(clip, state.Clip); Assert.Equal("1", row[24]); Assert.True(N(row[31]) > 0); Assert.Equal("0", row[32]); rows++;
        }
        Assert.Equal(120, rows); Assert.Equal(40, guardRows);
    }
    /// <summary>실제 카메라 getter가 잘못된 칸 배율을 주면 성공으로 넘기지 않는다.</summary>
    [Fact]
    public void RejectInvalidCurrentCellScaleBeforeChangingCrop()
    {
        var state = new MinimapGeometryState { CropX = 9, CropY = 11 };
        Assert.Throws<ArgumentException>(() => MinimapGeometry.Rebuild(state, new(0, 0, new(0, 0, 1024, 768), 0, 11)));
        Assert.Equal(9, state.CropX); Assert.Equal(11, state.CropY);
    }
}
