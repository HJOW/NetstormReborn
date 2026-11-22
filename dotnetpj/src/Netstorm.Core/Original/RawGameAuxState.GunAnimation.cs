// 같은 장기 Client의 총 애니메이션 헤더와 썬 캐논 역할을 보관한다.
namespace Netstorm.Core.Original;

/// <summary>RawGameAuxState.h의00540468/6e와00541258 현재 상태다.</summary>
public sealed partial class RawGameAuxState
{
    /// <summary>현재128 썬 캐논 역할이며 수신/현재프레임이 함께 빌린다.</summary>
    public UIntBox SunCannonTypeBox { get; } = new() { Value = 128 };
    /// <summary>회전 writer가 매번 읽는 현재메시지/프로토콜 두BYTE다.</summary>
    public byte[] RotateAnimationHeader { get; } = [0x3c, 1];
    /// <summary>발사 writer의 별개 현재메시지/프로토콜 두BYTE다.</summary>
    public byte[] FireAnimationHeader { get; } = [0x3b, 1];
}
