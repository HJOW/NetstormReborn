// 실제 픽셀 파편의 입자·등록·시간과 공통 입자 표시 전역을 보관한다.
namespace Netstorm.Core.Original;

/// <summary>원본 세 가상 생성기다. 셋째 종류의 의미는 추측하지 않는다.</summary>
public enum ShrapnelKind
{
    /// <summary>중력6과 방사 속도를 쓰는 폭발이다.</summary>
    Explosion,
    /// <summary>중력float0.24와 높이별 수명을 쓰는 붕괴다.</summary>
    Collapse,
    /// <summary>섬 낙하가 요청하는 중력float1.2의 셋째 생성기다.</summary>
    Variant2,
}

/// <summary>원본24BYTE 입자다. 색 주소만 같은 bitmap의offset+1 토큰으로 표현한다.</summary>
public struct ShrapnelParticle
{
    /// <summary>현재 월드 픽셀 가로 좌표다.</summary>
    public float X;
    /// <summary>현재 월드 픽셀 세로 좌표다.</summary>
    public float Y;
    /// <summary>가로 속도다.</summary>
    public float VelocityX;
    /// <summary>중력으로 갱신하는 세로 속도다.</summary>
    public float VelocityY;
    /// <summary>10배 시간 단위의 남은 수명이다.</summary>
    public float Life;
    /// <summary>0은 미사용이며 나머지는 캡처 픽셀offset+1이다.</summary>
    public uint Pixel;
}

/// <summary>원본104BYTE 객체의 소유 필드다. 목록은 모든 효과가 공유한다.</summary>
public sealed class ShrapnelFrameFields
{
    /// <summary>같은 Form·부모·목록·표시 영역이다.</summary>
    public RenderProcessFields Registration { get; } = new();
    /// <summary>미사용0을 포함한 용량 전체이며 죽은 슬롯을 재사용하지 않는다.</summary>
    public ShrapnelParticle[] Particles { get; set; } = [];
    /// <summary>실제로 사용한 꼬리 수다.</summary>
    public int Total { get; set; }
    /// <summary>마지막 게임 시각이다.</summary>
    public double LastTime { get; set; }
    /// <summary>10배 간격의 누적값이다.</summary>
    public double Elapsed { get; set; }
    /// <summary>저장된 한 프레임 전체 간격이다.</summary>
    public float Delta { get; set; }
    /// <summary>표시 배율을 적용한 간격이다.</summary>
    public float ScaledDelta { get; set; }
    /// <summary>공통 생성자가0을 쓰지만 파편 프레임은 검사하지 않는 필드다.</summary>
    public bool First { get; set; }
    /// <summary>현재 색·치수·기준점의 소유 캡처다.</summary>
    public ConstructionVisualImage Image { get; set; } = new();
    /// <summary>현재 파생 생성기의 중력이다.</summary>
    public float Gravity { get; set; } = 6;
}

/// <summary>RawFieryTailFrame.h의 공통 입자 전역이다. FieryTail 실행 본체의 완료를 뜻하지 않는다.</summary>
public class FieryTailFrameState
{
    /// <summary>같은 Kernel의 현재 게임 시각이다.</summary>
    public double Now { get; set; }
    /// <summary>raw 가로 좌표의 현재 픽셀 배율이다.</summary>
    public float PixelScaleX { get; set; } = 16;
    /// <summary>raw 세로 좌표의 현재 픽셀 배율이다.</summary>
    public float PixelScaleY { get; set; } = 11;
    /// <summary>ScaledDelta에 적용하는 현재 비율이다.</summary>
    public float Rate { get; set; } = 1;
    /// <summary>현재 카메라 가로 픽셀 좌표다.</summary>
    public float CameraX { get; set; }
    /// <summary>현재 카메라 세로 픽셀 좌표다.</summary>
    public float CameraY { get; set; }
    /// <summary>원본005318ec의 현재 DWORD다.</summary>
    public int Diagnostic { get; set; }
}

/// <summary>Source의FieryTailFrameState 별칭이며 같은 공통 필드만 사용한다.</summary>
public sealed class ShrapnelFrameState : FieryTailFrameState { }

/// <summary>1픽셀 여유와 현재 카메라를 적용한 dirty 영역이다.</summary>
/// <param name="Left">왼쪽 정수 좌표다.</param>
/// <param name="Top">위쪽 정수 좌표다.</param>
/// <param name="Right">오른쪽 정수 좌표다.</param>
/// <param name="Bottom">아래쪽 정수 좌표다.</param>
public readonly record struct FieryTailDirtyRect(int Left, int Top, int Right, int Bottom);

/// <summary>실제004a2230으로 전달하는 현재 픽셀 요청이다.</summary>
/// <param name="X">화면 가로 좌표다.</param>
/// <param name="Y">화면 세로 좌표다.</param>
/// <param name="Color">현재 캡처byte를 확장한DWORD다.</param>
public readonly record struct ShrapnelPixel(int X, int Y, uint Color);

/// <summary>원문 메모리·dirty·픽셀·소진의 외부 장치다. 계산을 대신하지 않는다.</summary>
public sealed class ShrapnelFrameHooks
{
    /// <summary>입자count×24 또는 작업 bitmap 크기를 확보한다.</summary>
    public Func<uint, bool>? Reserve { get; set; }
    /// <summary>true는 작업 복사본, false는 이전 입자 저장소다.</summary>
    public Action<bool>? Release { get; set; }
    /// <summary>이전 영역 다음 새 영역을 받는 실제 dirty 소비자다.</summary>
    public Action<FieryTailDirtyRect>? Invalidate { get; set; }
    /// <summary>현재clip·pitch를 가진 표시 소비자다.</summary>
    public Action<ShrapnelPixel>? Pixel { get; set; }
    /// <summary>소진을 실제41bf10(this,0)에 전달한다.</summary>
    public Action? RemoveForm { get; set; }
    /// <summary>함수 사본을 보관하고 캡처한 같은 외부 장치는 빌린다.</summary>
    public ShrapnelFrameHooks Copy() => new() { Reserve = Reserve, Release = Release, Invalidate = Invalidate, Pixel = Pixel, RemoveForm = RemoveForm };
}
