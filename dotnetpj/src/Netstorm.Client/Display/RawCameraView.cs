using Netstorm.Core.Original;

// 원문 앞64BYTE와 전역 snapshot은 값만 복사하며 같은 주소로 합치지 않는다.
namespace Netstorm.Client.Display;

/// <summary>0059a900의16DWORD다. managed 배열은 고정 소유하며 원문 저장 순서로 복사한다.</summary>
public sealed class RawCameraViewState
{
    /// <summary>+00/04의 현재 월드 float 비트다.</summary>
    public uint[] WorldBits { get; } = new uint[2];
    /// <summary>+08/0c의 현재 칸 픽셀 크기다.</summary>
    public int[] CellPixels { get; } = new int[2];
    /// <summary>+10/14의 기본 칸 픽셀 크기다.</summary>
    public int[] BaseCellPixels { get; } = new int[2];
    /// <summary>+18의 현재 Q16 배율이다.</summary>
    public uint Zoom { get; set; }
    /// <summary>+1c/20의 현재 픽셀 원점이며 커서/음향이 같은 배열을 빌린다.</summary>
    public int[] Pixels { get; } = new int[2];
    /// <summary>+24/28의 현재 칸 float 비트다.</summary>
    public uint[] CellFloatBits { get; } = new uint[2];
    /// <summary>+2c/30의 기본 칸 float 비트다.</summary>
    public uint[] BaseCellFloatBits { get; } = new uint[2];
    /// <summary>+34의 현재 배율 float 비트다.</summary>
    public uint ZoomFloatBits { get; set; }
    /// <summary>+38/3c의 픽셀 원점 float 사본이며 월드 분수와 별개다.</summary>
    public uint[] PixelFloatBits { get; } = new uint[2];
}

/// <summary>4973e0/497490/4993c0/497220의 현재 카메라 계산이다.</summary>
public static class RawCameraView
{
    /// <summary>원본504194의 단정도1/11이며 double 1/11로 대체하지 않는다.</summary>
    private static readonly float InverseY = BitConverter.UInt32BitsToSingle(0x3dba2e8c);
    /// <summary>signed64 ftol의 하위DWORD다. indefinite의 하위0을 보존한다.</summary>
    public static int FtolLow(double value) => !double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0 ? 0 : unchecked((int)(uint)(long)value);
    /// <summary>모든 선행 쓰기 뒤 X→Y로 현재 필드를 재조회한다.</summary>
    private static void Diagnose(RawCameraViewState view, Action<string, uint>? assertion, uint line)
    {
        if (BitConverter.UInt32BitsToSingle(view.WorldBits[0]) >= 0 && view.Pixels[0] < 0)
            (assertion ?? throw new InvalidOperationException("카메라X원점원문진단미연결"))("scrollPosPixX >= 0", line);
        if (BitConverter.UInt32BitsToSingle(view.WorldBits[1]) >= 0 && view.Pixels[1] < 0)
            (assertion ?? throw new InvalidOperationException("카메라Y원점원문진단미연결"))("scrollPosPixY >= 0", line + 3);
    }
    /// <summary>월드 분수 비트를 먼저 보존하고 정수 칸 크기 곱의 signed64 절삭을 쓴다.</summary>
    public static void SetWorld(RawCameraViewState view, float x, float y, Action<string, uint>? assertion = null)
    {
        view.WorldBits[0] = BitConverter.SingleToUInt32Bits(x); view.WorldBits[1] = BitConverter.SingleToUInt32Bits(y);
        view.Pixels[0] = FtolLow((double)view.CellPixels[0] * x); view.Pixels[1] = FtolLow((double)view.CellPixels[1] * y);
        view.PixelFloatBits[0] = BitConverter.SingleToUInt32Bits(view.Pixels[0]); view.PixelFloatBits[1] = BitConverter.SingleToUInt32Bits(view.Pixels[1]);
        Diagnose(view, assertion, 0x344);
    }
    /// <summary>RawCursorPosition.cpp의497490 공통 환산이다. 정수를 먼저 float로 줄이지 않는다.</summary>
    public static (uint X, uint Y) CameraFromPixels(int x, int y) => (BitConverter.SingleToUInt32Bits((float)((double)x * 0.0625f)), BitConverter.SingleToUInt32Bits((float)((double)y * InverseY)));
    /// <summary>Y/X→월드 float→픽셀 float 순서로 쓰고 나머지10DWORD를 보존한다.</summary>
    public static void SetPixels(RawCameraViewState view, int x, int y, Action<string, uint>? assertion = null)
    {
        view.Pixels[1] = y; view.Pixels[0] = x; (uint wx, uint wy) = CameraFromPixels(x, y); view.WorldBits[0] = wx; view.WorldBits[1] = wy;
        view.PixelFloatBits[0] = BitConverter.SingleToUInt32Bits(x); view.PixelFloatBits[1] = BitConverter.SingleToUInt32Bits(y); Diagnose(view, assertion, 0x354);
    }
    /// <summary>전역 BSS0과 구분되는 명시 Native Renderer 초기화다.</summary>
    public static void Initialize(RawCameraViewState view, Action<string, uint>? assertion = null)
    {
        view.BaseCellFloatBits[1] = 0x41300000; view.CellFloatBits[1] = 0x41300000; view.BaseCellFloatBits[0] = 0x41800000; view.CellFloatBits[0] = 0x41800000;
        view.BaseCellPixels[1] = 11; view.BaseCellPixels[0] = 16; view.Zoom = 0x10000; view.ZoomFloatBits = 0x3f800000; view.CellPixels[0] = 16; view.CellPixels[1] = 11; SetWorld(view, 0, 0, assertion);
    }
    /// <summary>16DWORD 값만 복사하며 destination의 고정 배열 정체성을 보존한다.</summary>
    public static void CopySnapshot(RawCameraViewState destination, RawCameraViewState source)
    {
        if (ReferenceEquals(destination, source)) return;
        source.WorldBits.CopyTo(destination.WorldBits, 0); source.CellPixels.CopyTo(destination.CellPixels, 0); source.BaseCellPixels.CopyTo(destination.BaseCellPixels, 0); destination.Zoom = source.Zoom;
        source.Pixels.CopyTo(destination.Pixels, 0); source.CellFloatBits.CopyTo(destination.CellFloatBits, 0); source.BaseCellFloatBits.CopyTo(destination.BaseCellFloatBits, 0); destination.ZoomFloatBits = source.ZoomFloatBits; source.PixelFloatBits.CopyTo(destination.PixelFloatBits, 0);
    }
    /// <summary>각 축 ftol 뒤 같은 현재 snapshot의 signed 픽셀 원점을 DWORD 감김으로 뺀다.</summary>
    public static ScreenPoint Project(RawCameraViewState view, float x, float y) => new(unchecked(FtolLow((double)x * 16 + 0.5) - view.Pixels[0]), unchecked(FtolLow((double)y * 11 + 0.5) - view.Pixels[1]));
}
