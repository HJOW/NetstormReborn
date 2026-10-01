using System.Globalization;

namespace Netstorm.AnalyzeManager;

/// <summary>
/// 운영체제 그림 API 없이 관찰표를 만드는 경로. ffmpeg 로 각 프레임을 칸 크기 RGB 로 줄이고,
/// <see cref="PortableImage"/> 로 합치고 시각 글자를 쓴 뒤 PNG 로 저장한다. 리눅스용 YouTube 전용 빌드가 쓴다.
/// </summary>
public sealed partial class YouTubeAnalyzer
{
    /// <summary>내장 글꼴 확대 배율 (5×7 점 → 10×14 픽셀, 글자 영역 22픽셀 안에 들어감)</summary>
    private const int PortableLabelScale = 2;

    /// <summary>관찰표 배경 밝기 (Windows 경로와 같은 짙은 회색)</summary>
    private const byte SheetBackground = 32;

    /// <summary>프레임들을 시각과 함께 한 장으로 합쳐 PNG 바이트를 돌려준다</summary>
    /// <param name="directory">영상 기록 폴더</param>
    /// <param name="frames">시간순 프레임</param>
    /// <param name="columns">열 수</param>
    /// <param name="cellWidth">칸 폭</param>
    /// <param name="cancellation">취소</param>
    private static async Task<byte[]> PortableContactSheetAsync(string directory, IReadOnlyList<FrameRecord> frames, int columns,
        int cellWidth, CancellationToken cancellation)
    {
        FrameRecord first = frames[0];
        int cellHeight = (int)Math.Round(cellWidth * (double)first.Height / first.Width);
        int rows = (frames.Count + columns - 1) / columns;
        int sheetWidth = columns * cellWidth;
        int sheetHeight = rows * (cellHeight + LabelHeight);
        byte[] canvas = PortableImage.Canvas(sheetWidth, sheetHeight, SheetBackground, SheetBackground, SheetBackground);
        string ffmpeg = Tool(FfmpegVariable, "ffmpeg");
        // 프레임을 왼쪽 위부터 시간순으로 놓고 아래에 시각을 쓴다
        for (int i = 0; i < frames.Count; i++)
        {
            int x = i % columns * cellWidth;
            int y = i / columns * (cellHeight + LabelHeight);
            ProcessOutput output = await RunAsync(ffmpeg,
                ["-v", "error", "-i", Path.Combine(directory, frames[i].Path),
                 "-vf", $"scale={cellWidth.ToString(CultureInfo.InvariantCulture)}:{cellHeight.ToString(CultureInfo.InvariantCulture)}:flags=bilinear",
                 "-frames:v", "1", "-f", "rawvideo", "-pix_fmt", "rgb24", "-"],
                TimeSpan.FromSeconds(FrameTimeoutSeconds), cancellation);
            output.ThrowIfFailed("ffmpeg 관찰표 칸 축소");
            if (output.Stdout.Length != cellWidth * cellHeight * 3)
            {
                throw new InvalidDataException($"관찰표 칸 크기가 맞지 않습니다: {output.Stdout.Length}바이트");
            }
            PortableImage.Blit(canvas, sheetWidth, output.Stdout, cellWidth, cellHeight, x, y);
            string label = YouTubeVideo.FormatTime(frames[i].Time) + (frames[i].Segment != null ? $"  [{frames[i].Segment!.Category}]" : "");
            // 게임 화면이 아닐 수 있는 구간(SponsorBlock)은 주황, 그 밖은 흰 글자
            (byte r, byte g, byte b) = frames[i].Segment != null ? ((byte)255, (byte)165, (byte)0) : ((byte)255, (byte)255, (byte)255);
            PortableImage.DrawText(canvas, sheetWidth, label, x + 4, y + cellHeight + 4, PortableLabelScale, r, g, b);
        }
        return PortableImage.EncodeRgbPng(canvas, sheetWidth, sheetHeight);
    }
}
