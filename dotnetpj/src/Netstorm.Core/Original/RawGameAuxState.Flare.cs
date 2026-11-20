// 효과 생성 직전에 다시 읽는 장기 TYPE 역할을 보존한다.
namespace Netstorm.Core.Original;

/// <summary>원본00541200 플레어 TYPE의 장기 소유자 부분이다.</summary>
public sealed partial class RawGameAuxState
{
    /// <summary>최초106이며 일반 월드 준비/세션 교체가 덮어쓰지 않는 현재DWORD다. 근거: cpppj/src/o/RawGameAuxState.h BombFlareType.</summary>
    public UIntBox BombFlareTypeBox { get; } = new() { Value = 106 };
}
