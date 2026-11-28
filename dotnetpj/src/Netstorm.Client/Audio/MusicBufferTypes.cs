// 실제 음악 버퍼 생성의 입력 형식·고정 크기·부분 출력 토큰을 전달한다.
namespace Netstorm.Client.Audio;

/// <summary>18BYTE 형식·원문 기능 비트·버퍼 크기를 받아 HRESULT와 같은 ref 토큰 부분 출력을 보존한다.</summary>
public delegate int MusicBufferCreate(ReadOnlySpan<byte> format, uint flags, uint bytes, ref uint buffer);

/// <summary>실제 음악 버퍼 생성·갱신의 COM 경계다. 함수 슬롯은 같은 장치가 제공하고 worker 수명은 별도로 조립한다.</summary>
public sealed partial class MusicBufferHooks
{
    /// <summary>현재 실제 장치가 음악 링 버퍼를 생성하고 HRESULT를 반환한다.</summary>
    public MusicBufferCreate? Create { get; set; }
    /// <summary>함수 슬롯은 값 복사하고 실제 callback 소유자는 공유한다.</summary>
    public MusicBufferHooks Copy() => (MusicBufferHooks)MemberwiseClone();
}
