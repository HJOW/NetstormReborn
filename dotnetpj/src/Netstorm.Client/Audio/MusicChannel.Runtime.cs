// 원문 두 채널 초기화에서 실제 채널의 선택 필드만 같은 잠금 아래 지운다.
namespace Netstorm.Client.Audio;

/// <summary>Runtime 초기화에 필요한 같은 실제 채널 잠금과96BYTE 저장소의 접근 입구다.</summary>
public sealed partial class MusicChannel
{
    /// <summary>+14/+3a 패딩과+44 이후 원문 잠금 공간을 보존한다. 일반 채널 Guard의 E/L 관측은 추가하지 않는다.</summary>
    internal void ResetForRuntime(uint index, Action<uint, bool>? section)
    {
        lock (_gate)
        {
            Span<byte> raw = _state.Raw().Span;
            raw[..0x14].Clear();
            raw.Slice(0x18, 0x22).Clear();
            raw.Slice(0x3c, 8).Clear();
            section?.Invoke(index, true);
        }
    }
}
