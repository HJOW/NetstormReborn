// 건설 표시의 수명/등록 어댑터가 공유하는 원문 의미 필드다. 생성/프레임 몸체는 별도 모듈이다.
namespace Netstorm.Core.Original;

/// <summary>원문12바이트 건설 입자의 의미 필드다.</summary>
public struct ConstructionVisualParticle
{
    /// <summary>현재 활성 표식이다.</summary>
    public byte Active;
    /// <summary>완성 뒤 출력 버퍼로 되돌릴 원래 색이다.</summary>
    public byte Color;
    /// <summary>현재 입자의 남은 수명이다.</summary>
    public float Life;
    /// <summary>원본 포인터 대신 사용하는 출력 픽셀 번호다.</summary>
    public uint Pixel;
}

/// <summary>건설 표시가 단독 소유하는 캡처/출력/입자와 원문 상태다.</summary>
public sealed class ConstructionVisualFields
{
    /// <summary>등록 부착이 변경할 수 있는 현재 부모다.</summary>
    public int Parent { get; set; }
    /// <summary>즉시 건설 파생의 수명 범위를 선택한다.</summary>
    public bool Instant { get; set; }
    /// <summary>현재 부모의 signed 깊이다.</summary>
    public int ZOrder { get; set; }
    /// <summary>현재 표시 영역의 왼쪽이다.</summary>
    public float Left { get; set; }
    /// <summary>현재 표시 영역의 위쪽이다.</summary>
    public float Top { get; set; }
    /// <summary>현재 표시 영역의 오른쪽이다.</summary>
    public float Right { get; set; }
    /// <summary>현재 표시 영역의 아래쪽이다.</summary>
    public float Bottom { get; set; }
    /// <summary>이전 시각 helper의 현재 값이다.</summary>
    public double LastTime { get; set; }
    /// <summary>공통 시간 helper의 누적 경과다.</summary>
    public double Elapsed { get; set; }
    /// <summary>공유 색 표의 현재 토큰이며 객체 삭제 때 반납하지 않는다.</summary>
    public uint Palette { get; set; }
    /// <summary>현재 SHP 캡처 버퍼/기준점이다.</summary>
    public ConstructionVisualImage Image { get; set; } = new();
    /// <summary>투명색과 완성 픽셀을 담는 소유 출력 버퍼다.</summary>
    public byte[] Output { get; set; } = [];
    /// <summary>마지막 픽셀 행의 시작 번호다.</summary>
    public int LastRow { get; set; }
    /// <summary>선택하는 입자 수명의 하한이다.</summary>
    public float Minimum { get; set; }
    /// <summary>선택하는 입자 수명의 상한이다.</summary>
    public float Maximum { get; set; }
    /// <summary>누적 선택 수이며 Seed의 재진입 때도 보존한다.</summary>
    public uint Selected { get; set; }
    /// <summary>현재 캡처의 불투명 픽셀 수다.</summary>
    public uint Opaque { get; set; }
    /// <summary>현재 활성 입자 수다.</summary>
    public uint Active { get; set; }
    /// <summary>첫 시각 helper 초기화 여부다.</summary>
    public bool FirstDraw { get; set; }
    /// <summary>확보한 입자 칸 수다.</summary>
    public uint Capacity { get; set; }
    /// <summary>초기화한 입자 수다.</summary>
    public uint Total { get; set; }
    /// <summary>같은 파생이 소유하는 실제 입자 배열이다.</summary>
    public ConstructionVisualParticle[] Particles { get; set; } = [];
}

/// <summary>건설 생성·등록·캡처가 사용하는 원문 함수 값 묶음이다.</summary>
public sealed class ConstructionVisualHooks
{
    /// <summary>선택된 원문 확보량을 공급한다.</summary>
    public Func<uint, bool>? Reserve { get; set; }
    /// <summary>같은 표시 필드와 프로세스 역할을 등록/부착한다.</summary>
    public Action<ConstructionVisualFields, uint>? Attach { get; set; }
    /// <summary>공유8바이트 색 표의 실제 토큰을 생성한다.</summary>
    public Func<uint>? CreatePalette { get; set; }
    /// <summary>색 수/시작RGB/끝RGB/보정으로 실제 색 띠를 만든다.</summary>
    public Action<uint, double[], double[], double>? Gradient { get; set; }
    /// <summary>같은 토큰에 두 색 띠를 합친다.</summary>
    public Action<uint>? BuildPalette { get; set; }
    /// <summary>현재 부모 SHP의 실제 픽셀/크기/기준점을 캡처한다.</summary>
    public Func<int, ConstructionVisualImage>? Capture { get; set; }
    /// <summary>재선택 때 이전 입자 버퍼를 반납한다.</summary>
    public Action? ReleaseParticles { get; set; }
    /// <summary>함수 값은 복사하고 실제 owner의 수명은 빌린다.</summary>
    internal ConstructionVisualHooks Copy() => new()
    { Reserve = Reserve, Attach = Attach, CreatePalette = CreatePalette, Gradient = Gradient, BuildPalette = BuildPalette, Capture = Capture, ReleaseParticles = ReleaseParticles };
}
