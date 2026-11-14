using System.Buffers.Binary;
using Netstorm.Core.Display;

// 원문 연소의 실제5픽셀/색 조회와 프레임gate·소멸 순서를 공유한다.
namespace Netstorm.Core.Original;

/// <summary>공유 색수와 실제 읽을 수 있는 인접DWORD snapshot을 구별한다.</summary>
public sealed class BurningDrawPalette
{
    /// <summary>원문 공유 색 표의 요청 색수다.</summary>
    public int Count { get; set; }
    /// <summary>실제 할당에서 읽을 수 있는 현재 색DWORD다. 끝색을 임의로 추가하지 않는다.</summary>
    public ReadOnlyMemory<uint> Readable { get; set; }
}
/// <summary>소멸자가 반납하는 원문 소유 버퍼/객체의 구별이다.</summary>
public enum BurningRelease
{
    /// <summary>현재SHP 캡처 픽셀이다.</summary>
    Image,
    /// <summary>연소점 행 표식이다.</summary>
    Rows,
    /// <summary>실제 입자 저장소다.</summary>
    Particles,
    /// <summary>scalar deleting destructor의 객체다.</summary>
    Object,
}
/// <summary>연소 출력/삭제·실제 발사기 캡처·RenderProcess+18 진입판정의 원문 몸체다.</summary>
public static class RawBurningLifetimeDraw
{
    /// <summary>실제CRT의 유한 signed범위를0방향으로 정수화한다.</summary>
    private static int Truncate(double value)
    {
        if (!double.IsFinite(value) || value < -2147483648.0 || value >= 2147483648.0) throw new ArgumentOutOfRangeException(nameof(value), "Burning Draw 원본 정수화 범위 오류");
        return (int)value;
    }
    /// <summary>원문 두float저장과 넓은 곱 뒤 실제 색DWORD를 읽는다. life9의 인접index8도 검증한다.</summary>
    private static byte Color(float life, BurningDrawPalette palette)
    {
        if (palette.Count < 0) throw new ArgumentException("Burning Draw 음수 색수");
        float reciprocal = (float)(1.0 / 9);
        int index = Math.Max(0, Truncate((double)(float)palette.Count * Math.Min(life, 9f) * reciprocal));
        if (index >= palette.Readable.Length) throw new ArgumentOutOfRangeException(nameof(palette), "Burning Draw 인접색 DWORD 미공급");
        return (byte)palette.Readable.Span[index];
    }
    /// <summary>복원한 실제TYPE71/128과 보호VT 두 값만 판정한다.</summary>
    public static bool IsArcherCapture(ReadOnlySpan<byte> raw)
    {
        uint table = BinaryPrimitives.ReadUInt32LittleEndian(raw);
        return (raw[10] == 71 && table == 0x50b888) || (raw[10] == 128 && table == 0x50b9b8);
    }
    /// <summary>발사기의 현재 그림/조건부83디스크를 순서대로 출력한다. 캐논+a4는 빈ret8이다.</summary>
    public static void DrawArcherCapture(ReadOnlySpan<byte> raw, int baseFrame, int muzzleX, int muzzleY, Action<uint, uint, int, int> draw, int x, int y)
    {
        if (!IsArcherCapture(raw) || draw is null) throw new ArgumentException("연소 캡처의 다른 발사기/그림 경계");
        if (raw[10] == 128) return;
        uint current = BinaryPrimitives.ReadUInt32LittleEndian(raw[36..]);
        if ((raw[40] & 9) != 0) { draw(71, current, x, y); return; }
        if (current == unchecked((uint)baseFrame)) return;
        draw(71, current, x, y);
        if ((raw[12] & 1) == 0) draw(83, 0, unchecked(x + muzzleX), unchecked(y + muzzleY));
    }
    /// <summary>원문 첫7slot은항상, 이후는(slot XOR 현재frame)의하위2bit0일때만+24다.</summary>
    public static bool FrameDue(int slot, uint frame) => slot < 7 || ((unchecked((uint)slot) ^ frame) & 3) == 0;
    /// <summary>현재total의 양수수명 이동입자만 실제clip 안에서 원문5픽셀순서로 출력한다.</summary>
    public static void Draw(BurningVisualFields fields, BurningFrameState state, BurningDrawPalette palette, Action<int, int, byte> pixel)
    {
        if (fields.Capacity == 0) return;
        if (fields.Total > fields.Capacity || fields.Particles.Length != fields.Capacity || pixel is null) throw new ArgumentException("Burning Draw 실제입자/출력 연결 오류");
        DisplayRect clip = state.Viewport;
        // capacity가 아닌 현재total만 순회하며 source표식은 출력하지 않는다.
        for (int index = 0; index < fields.Total; index++)
        {
            BurningParticle p = fields.Particles[index]; if (!(p.Life > 0) || p.Source) continue;
            float fx = (float)((double)Truncate(p.X) - state.CameraX), fy = (float)((double)Truncate(p.Y) - state.CameraY);
            byte color = Color(p.Life, palette); int x = Truncate(fx), y = Truncate(fy), right = unchecked(x + 1), top = unchecked(y - 2);
            if (x < clip.Left || x >= clip.Right || y < clip.Top || y >= clip.Bottom || right < clip.Left || right >= clip.Right || top < clip.Top || top >= clip.Bottom) continue;
            pixel(x, y, color); pixel(right, y, color); pixel(right, unchecked(y - 1), color); pixel(x, unchecked(y - 1), color); pixel(x, top, color);
        }
    }
    /// <summary>이미지→행→현재소리→입자→공통등록/base→선택객체 순서이며 공유팔레트는 보존한다.</summary>
    public static void Destroy(BurningVisualFields fields, RawRenderRegistry registry, uint flags, Action<BurningRelease> release, Action<uint> stopSound)
    {
        ArgumentNullException.ThrowIfNull(release); ArgumentNullException.ThrowIfNull(stopSound);
        if (fields.Image.Pixels.Length != 0) { release(BurningRelease.Image); fields.Image.Pixels = []; }
        if (fields.Rows.Length != 0) { release(BurningRelease.Rows); fields.Rows = []; }
        stopSound(fields.Sound);
        if (fields.Particles.Length != 0) { release(BurningRelease.Particles); fields.Particles = []; }
        registry.Destroy(fields.Registration);
        if ((flags & 1) != 0) release(BurningRelease.Object);
    }
}
