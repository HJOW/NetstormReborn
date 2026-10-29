using Netstorm.Client.Display;

namespace Netstorm.Client.Gump; // 원본 배경 질감의 화면 기준 반복과 명암 테두리 영역이다.

/// <summary>원본 배경 분기와 각 테두리·명암을 선택하는 DWORD 비트다.
/// 근거: cpppj/src/client/GumpBackground.h.</summary>
public static class GumpBackgroundFlags
{
    /// <summary>질감 반복 경로.</summary>
    public const uint Texture = 0x02000000;
    /// <summary>낮은 BYTE 색으로 단색 fill. 질감보다 우선한다.</summary>
    public const uint Solid = 0x04000000;
    /// <summary>체크 fill. 단색과 질감보다 우선한다.</summary>
    public const uint Checker = 0x08000000;
    /// <summary>왼쪽 테두리.</summary>
    public const uint Left = 0x01000000;
    /// <summary>위쪽 테두리.</summary>
    public const uint Top = 0x00800000;
    /// <summary>오른쪽 테두리.</summary>
    public const uint Right = 0x00400000;
    /// <summary>아래쪽 테두리.</summary>
    public const uint Bottom = 0x00200000;
    /// <summary>어두운 소스 색 변환표.</summary>
    public const uint Dark = 0x00100000;
    /// <summary>밝은 소스 색 변환표. 어두움과 함께 있으면 우선한다.</summary>
    public const uint Bright = 0x00080000;
    /// <summary>눌렸을 때 테두리 명암 방향만 뒤집는다.</summary>
    public const uint Pressed = 0x00040000;
    /// <summary>부모 버튼이 배경 자식에 전달하는 기본 질감과 네 테두리.</summary>
    public const uint Button = 0x03e00000;
}

/// <summary>전체 질감 함수 한 호출의 요청 영역·일시 클립·명암·화면 기준 출력점이다.</summary>
public sealed class GumpTexturePass
{
    /// <summary>요청한 끝 제외 영역.</summary>
    public ScreenRect Rect { get; init; }
    /// <summary>타일이 유효할 때 설치할 일시 클립.</summary>
    public ScreenRect Clip { get; set; }
    /// <summary>현재 호출의 배경 플래그.</summary>
    public uint Flags { get; init; }
    /// <summary>0 원색·1 밝은 표·2 어두운 표.</summary>
    public uint Shade { get; set; }
    /// <summary>signed WORD 폭/높이가 모두2 이상일 때만 clip/map/draw를 호출한다.</summary>
    public bool ValidTile { get; set; }
    /// <summary>원본의 행 우선 여분 타일 출력 순서.</summary>
    public List<ScreenPoint> Origins { get; } = [];
}

/// <summary>전체 배경 질감과 2픽셀 테두리의 출력 계획이다.
/// 근거: cpppj/src/client/GumpBackground.cpp, 00465880→00465690/CD00494a20→00494720.</summary>
public static class GumpBackground
{
    /// <summary>끝나지 않는 원본 반복을 실행하지 않는 C++ 안전 API의 최대 호출 수.</summary>
    private const ulong MaximumDraws = 1_000_000;
    /// <summary>안전 API가 허용하는 정상 순서 경계인지 검사한다. 빈 영역은 허용한다.</summary>
    private static bool Ordered(ScreenRect r) => r.Left <= r.Right && r.Top <= r.Bottom;

    /// <summary>DWORD 감김과0 방향 나눗셈으로 화면 배수 타일을 만든다. 잘못된 크기는 클립/색을 쓰지 않는다.</summary>
    private static GumpTexturePass TilePass(ScreenRect rect, ScreenRect clip, short width, short height, uint flags)
    {
        var result = new GumpTexturePass { Rect = rect, Flags = flags }; int dx = width - 1, dy = height - 1;
        if (dx < 1 || dy < 1) return result;
        if (!Ordered(rect)) throw new ArgumentException("감긴 GUI 질감 경계");
        result.ValidTile = true; result.Clip = ScreenGeometry.ClipScreenRect(clip, rect);
        result.Shade = (flags & GumpBackgroundFlags.Bright) != 0 ? 1U : (flags & GumpBackgroundFlags.Dark) != 0 ? 2U : 0U;
        int startX = unchecked(rect.Left / dx * dx), startY = unchecked(rect.Top / dy * dy);
        int endX = unchecked((unchecked(rect.Right - rect.Left) / dx + 2 + rect.Left / dx) * dx);
        int endY = unchecked((unchecked(rect.Bottom - rect.Top) / dy + 2 + rect.Top / dy) * dy);
        if (startX > endX || startY > endY) return result;
        long nx = ((long)endX - startX) / dx + 1, ny = ((long)endY - startY) / dy + 1;
        if ((ulong)nx * (ulong)ny > MaximumDraws || (long)endX + dx > int.MaxValue || (long)endY + dy > int.MaxValue)
            throw new ArgumentOutOfRangeException(nameof(rect), "GUI 질감 반복 안전 범위 초과");
        result.Origins.EnsureCapacity((int)(nx * ny));
        // 화면 y 배수를 끝점까지 포함한다. 실제 픽셀 출력은 일시 클립이 제한한다.
        for (long y = startY; y <= endY; y += dy)
        {
            // 각 행에서 같은 화면 x 배수를 재사용한다. 버튼 상대 원점이 아니다.
            for (long x = startX; x <= endX; x += dx) result.Origins.Add(new((int)x, (int)y));
        }
        return result;
    }

    /// <summary>매 질감 pass에서 새로 조회한 프레임 치수와 현재 클립으로 호출 계획을 만든다.</summary>
    public static GumpTexturePass PlanGumpTexture(ScreenRect rect, ScreenRect clip, short width, short height, uint flags)
    {
        if (!Ordered(rect) || !Ordered(clip)) throw new ArgumentException("배경 질감 경계 순서 오류");
        return TilePass(rect, clip, width, height, flags);
    }

    /// <summary>질감 전용 안전 계획. 바탕→왼쪽→오른쪽→위→아래 순서이며 기존 명암 비트에 OR한다.</summary>
    public static List<GumpTexturePass> PlanGumpBackground(ScreenRect rect, ScreenRect clip, short width, short height, uint flags)
    {
        if (!Ordered(rect) || !Ordered(clip) || (flags & 0x0c000000) != 0 || (flags & GumpBackgroundFlags.Texture) == 0)
            throw new ArgumentException("GUI 질감 경로 오류");
        List<GumpTexturePass> result = [TilePass(rect, clip, width, height, flags)];
        uint near = (flags & GumpBackgroundFlags.Pressed) != 0 ? GumpBackgroundFlags.Dark : GumpBackgroundFlags.Bright;
        uint far = (flags & GumpBackgroundFlags.Pressed) != 0 ? GumpBackgroundFlags.Bright : GumpBackgroundFlags.Dark;
        if ((flags & GumpBackgroundFlags.Left) != 0) result.Add(TilePass(ScreenGeometry.ClipScreenRect(new(rect.Left, rect.Top, unchecked(rect.Left + 2), rect.Bottom), clip), clip, width, height, flags | near));
        if ((flags & GumpBackgroundFlags.Right) != 0) result.Add(TilePass(ScreenGeometry.ClipScreenRect(new(unchecked(rect.Right - 2), rect.Top, rect.Right, rect.Bottom), clip), clip, width, height, flags | far));
        if ((flags & GumpBackgroundFlags.Top) != 0) result.Add(TilePass(ScreenGeometry.ClipScreenRect(new(rect.Left, rect.Top, rect.Right, unchecked(rect.Top + 2)), clip), clip, width, height, flags | near));
        if ((flags & GumpBackgroundFlags.Bottom) != 0) result.Add(TilePass(ScreenGeometry.ClipScreenRect(new(rect.Left, unchecked(rect.Bottom - 2), rect.Right, rect.Bottom), clip), clip, width, height, flags | far));
        return result;
    }

    /// <summary>계획 순서로 원색/명암을 합성한다. 프레임 상대 left/top을 화면 기준점에 더하고 opacity는 보존한다.</summary>
    public static void DrawGumpBackground(IndexedImage canvas, IndexedImage texture, ScreenPoint frameOffset, IEnumerable<GumpTexturePass> passes, GumpShadeMaps maps)
    {
        // 겹치는 모서리는 나중 pass가 덮으며 투명 질감은 이전 픽셀을 보존한다.
        foreach (GumpTexturePass pass in passes)
        {
            if (!pass.ValidTile) continue;
            byte[]? map = pass.Shade == 1 ? maps.Bright : pass.Shade == 2 ? maps.Dark : null;
            // 각 기준점은 모든 위젯이 공유하는 전체 화면 격자다.
            foreach (ScreenPoint origin in pass.Origins) RendererDraw.DrawIndexedImage(canvas.Indices, canvas.Width, canvas.Height, texture,
                unchecked(origin.X + frameOffset.X), unchecked(origin.Y + frameOffset.Y), pass.Clip, map);
        }
    }
}
