// 원문 소리 이름 표·재생 전역·장치 호출의 같은32비트 경계를 공유한다.
namespace Netstorm.Client.Audio;

/// <summary>호스트 포인터로 역참조하지 않는 원본 소리 표의 상수다.</summary>
public static class SoundConstants
{
    /// <summary>원본 new0x8000의 전체 예약 크기다.</summary>
    public const uint ListBytes = 0x8000;
    /// <summary>새 이름 추가를 멈추는 시작 오프셋이다.</summary>
    public const uint ListLimit = 0x7fb0;
    /// <summary>항목 머리 뒤 이름이 시작하는 오프셋이다.</summary>
    public const uint HeaderBytes = 0x1c;
    /// <summary>표 내부 이름 포인터 경로에서 원문 그대로 빼는 값이다.</summary>
    public const uint PointerBias = 0x1f;
    /// <summary>원본 대조 자료와 같은 명시 핸들 기준값이다.</summary>
    public const uint ListBase = 0x15000000;
    /// <summary>원본 철자 그대로인 이름 표의 대체 소리다.</summary>
    public const string FallbackName = "nonexistant.wav";
    /// <summary>파일/버퍼가 없음을 뜻하는 원본 전역 표식이다.</summary>
    public const uint SilentBuffer = uint.MaxValue;
    /// <summary>원본 장치 감쇠/좌우의 하한이다.</summary>
    public const int Minimum = -10000;
    /// <summary>원본 장치 좌우의 상한이다.</summary>
    public const int Maximum = 10000;
}

/// <summary>원본0x1c 항목 머리의 실제 DWORD 위치다.</summary>
public enum SoundField : uint
{
    /// <summary>미적재0/소리없음0xffffffff/실제 장치 버퍼다.</summary>
    Buffer = 0,
    /// <summary>같은 이름의 첫 항목이며 자기 자신이면 원본 항목이다.</summary>
    Root = 4,
    /// <summary>같은 소리를 겹쳐 재생할 다음 복제 항목이다.</summary>
    Next = 8,
    /// <summary>이름에서 읽은 소리별 감쇠이며 복제에서는0이다.</summary>
    Attenuation = 12,
    /// <summary>최근 재생/갱신의 음량 인자다.</summary>
    Volume = 16,
    /// <summary>재생 시작 때 부여한 일련번호다.</summary>
    Serial = 20,
    /// <summary>최근 재생의 우선 인자다.</summary>
    Priority = 24,
}

/// <summary>현재 위치 음향 계산에 쓰는 값형 카메라/영역 snapshot이다.</summary>
/// <param name="CameraX">현재 픽셀 카메라 X다.</param>
/// <param name="CameraY">현재 픽셀 카메라 Y다.</param>
/// <param name="Left">표시 영역의 왼쪽이다.</param>
/// <param name="Top">표시 영역의 위쪽이다.</param>
/// <param name="Right">표시 영역의 오른쪽이다.</param>
/// <param name="Bottom">표시 영역의 아래쪽이다.</param>
public readonly record struct SoundView(int CameraX, int CameraY, int Left, int Top, int Right, int Bottom)
{
    /// <summary>독립 typed 장치의 원본640×480 영역을 만든다.</summary>
    public SoundView() : this(0, 0, 0, 0, 640, 480) { }
}

/// <summary>투영 후 같은 위치 계산에 보존하는 픽셀 점이다.</summary>
/// <param name="X">signed X픽셀이다.</param>
/// <param name="Y">signed Y픽셀이다.</param>
public readonly record struct SoundPoint(int X, int Y);

/// <summary>원문 Client가 소유하고 실제 재생이 현재값을 읽는 소리 전역이다.</summary>
public sealed class SoundState
{
    /// <summary>소리 장치 초기화가 완료됐는지다.</summary>
    public bool Initialized { get; set; }
    /// <summary>실제 장치 객체의 비영 여부다.</summary>
    public bool Device { get; set; }
    /// <summary>현재 sound 옵션이다.</summary>
    public bool Enabled { get; set; }
    /// <summary>현재 좌우 스피커 교환 옵션이다.</summary>
    public bool SwapSpeakers { get; set; }
    /// <summary>동시 재생 한도이며0이면 무제한이다.</summary>
    public int MaxPlaying { get; set; } = 8;
    /// <summary>실제 재생/정지가 증감하고 Recount가 교정하는 현재 개수다.</summary>
    public int Playing { get; set; }
    /// <summary>1/100dB 단위의 현재 전체 효과음 음량이다.</summary>
    public int MasterVolume { get; set; }
    /// <summary>음수도 변경을 보류하는 원본 signed 깊이다.</summary>
    public int VolumeHoldDepth { get; set; }
    /// <summary>보류 중 가장 최근 효과음 음량 요청이다.</summary>
    public int PendingMasterVolume { get; set; }
    /// <summary>효과음 장치와 별개인 음악 갱신 준비 상태다.</summary>
    public bool MusicInitialized { get; set; }
    /// <summary>현재 음악 음량이다.</summary>
    public int MusicVolume { get; set; }
    /// <summary>보류 중 최근 음악 음량 요청이다.</summary>
    public int PendingMusicVolume { get; set; }
    /// <summary>다음 재생에 부여하며 uint로 감기는 일련번호다.</summary>
    public uint Serial { get; set; }
    /// <summary>독립 장치의 명시 카메라/영역 값이다.</summary>
    public SoundView View { get; set; } = new();
    /// <summary>실제 공용 전역을 각 투영/판정/좌우/음량 소비 위치에서 값으로 읽는다.</summary>
    public Func<SoundView>? CurrentView { get; set; }
}

/// <summary>현재 이름을 실제 장치에 적재한 결과다.</summary>
/// <param name="Buffer">장치 버퍼/미적재0/소리없음 표식이다.</param>
/// <param name="Attenuation">이름에서 읽은 signed 감쇠다.</param>
public readonly record struct SoundLoad(uint Buffer, int Attenuation);

/// <summary>원본 HRESULT와 함께 실제 복제 버퍼를 돌려주며 실패 때 기존 출력값을 보존하는 장치 경계다.</summary>
public delegate int SoundDuplicate(uint original, ref uint copy);

/// <summary>원본 DirectSound 호출 위치에 대응하는 실제 장치 함수 객체다.</summary>
public sealed class SoundDeviceHooks
{
    /// <summary>버퍼 상태의 재생1/소실2/반복4 비트를 읽는다.</summary>
    public Func<uint, uint>? Status { get; set; }
    /// <summary>버퍼를 원문 반복 플래그0/1로 재생한다.</summary>
    public Action<uint, uint>? Play { get; set; }
    /// <summary>현재 재생 위치를 설정한다.</summary>
    public Action<uint, uint>? SetPosition { get; set; }
    /// <summary>현재 버퍼의 signed 감쇠를 설정하고 HRESULT를 반환한다.</summary>
    public Func<uint, int, int>? SetVolume { get; set; }
    /// <summary>현재 버퍼의 signed 좌우를 설정하고 HRESULT를 반환한다.</summary>
    public Func<uint, int, int>? SetPan { get; set; }
    /// <summary>현재 버퍼를 정지한다.</summary>
    public Action<uint>? Stop { get; set; }
    /// <summary>같은 실제 소리의 장치 버퍼를 복제한다.</summary>
    public SoundDuplicate? Duplicate { get; set; }
    /// <summary>현재 이름으로 파일/버퍼를 적재한다.</summary>
    public Func<string, SoundLoad>? Load { get; set; }
    /// <summary>선택적 원문 채널/완성 문장 로그다.</summary>
    public Action<uint, string>? Log { get; set; }
    /// <summary>원문 함수 객체처럼 복사하고 캡처 장치는 같은 수명으로 빌린다.</summary>
    internal SoundDeviceHooks Copy() => new()
    {
        Status = Status, Play = Play, SetPosition = SetPosition, SetVolume = SetVolume, SetPan = SetPan,
        Stop = Stop, Duplicate = Duplicate, Load = Load, Log = Log,
    };
}
