// 세 NSInfo 파생 입력은 같은 실제 저장소와 각 전체 원문 반환/관문을 사용한다.
// cpppj/src/client/RawMenuControls.cpp 의 NsHandle(0047e9d0 앞단 분배) 뼈대를 이식한다.
// 0.919에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.

namespace Netstorm.Client.Gump;

// 토큰 소유 판정과 파생별 실제 처리기를 빌린다.
public sealed class NativeNsInfoDispatchHooks
{
    /// <summary>같은 실제 NSInfo 루트 저장소에 속한다.</summary>
    public Func<uint, bool>? IsRoot { get; set; }

    /// <summary>같은 실제 PlayerInfo 저장소에 속한다.</summary>
    public Func<uint, bool>? IsPlayer { get; set; }

    /// <summary>같은 실제 BattleInfo 저장소에 속한다.</summary>
    public Func<uint, bool>? IsBattle { get; set; }

    /// <summary>실제 루트 입력 처리다.</summary>
    public Func<uint, uint, uint, uint>? Root { get; set; }

    /// <summary>실제 플레이어 입력 처리다.</summary>
    public Func<uint, uint, uint, uint>? Player { get; set; }

    /// <summary>실제 배틀 입력 처리다.</summary>
    public Func<uint, uint, uint, uint>? Battle { get; set; }

    /// <summary>어느 파생에도 속하지 않을 때 같은 기본 처리에 전달한다.</summary>
    public Func<uint, uint, uint, uint, uint>? Base { get; set; }
}

// 세 파생의 토큰 분배 뼈대다. 후크 합성(정렬표·CS·메뉴·송신)은 생산 결선에서 잇는다.
public static class NativeNsInfoDispatch
{
    /// <summary>선택되지 않은 소유 판정·처리기를 요구하지 않고 미연결은 명시 실패한다. 0.919에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("NSInfo 입력 분배의 실제 소유/처리 장치 미연결");

    /// <summary>소유 저장소 순서대로 파생 처리에 보내고 그 외는 같은 기본 처리에 전달한다. 0.919에서 추가됐다.</summary>
    /// <param name="token">입력 대상 토큰이다.</param>
    /// <param name="event">원문 입력 event DWORD다.</param>
    /// <param name="first">원문 입력 첫 DWORD다.</param>
    /// <param name="second">원문 입력 둘째 DWORD다.</param>
    /// <param name="hooks">실제 소유 판정·파생 처리기다.</param>
    public static uint Dispatch(uint token, uint @event, uint first, uint second, NativeNsInfoDispatchHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (Need(hooks.IsRoot)(token))
        {
            return Need(hooks.Root)(@event, first, second);
        }

        if (Need(hooks.IsPlayer)(token))
        {
            return Need(hooks.Player)(@event, first, second);
        }

        if (Need(hooks.IsBattle)(token))
        {
            return Need(hooks.Battle)(@event, first, second);
        }

        return Need(hooks.Base)(token, @event, first, second);
    }
}
