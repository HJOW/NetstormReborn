// BattleInfo의 입력 분기/현재 옵션 기본값 적용과 메뉴·재부착·송신의 실제 부모를 복원한다.
// cpppj/src/client/NativeBattleInfoInput.cpp 의 HandleNativeBattleInfo(0047b8f0) 를 이식한다.
// 0.915에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.

namespace Netstorm.Client.Gump;

// 실제 입력 연결은 모든 메뉴 자식 장치를 마련한 뒤 분배기에 등록한다.
public sealed class NativeBattleInfoInputHooks
{
    /// <summary>같은 현재594fc8을 최초 관문에서 읽는다.</summary>
    public Func<uint>? Metamap { get; set; }

    /// <summary>같은 현재 CS의292BYTE 전투 기록이다.</summary>
    public Func<uint, Memory<byte>>? Battle { get; set; }

    /// <summary>현재 명령 DWORD를 읽는다.</summary>
    public Func<uint, uint>? Command { get; set; }

    /// <summary>제목 자식serial을 읽는다.</summary>
    public Func<uint, uint>? Serial { get; set; }

    /// <summary>현재 guy.wav(0,0,0,0,0) 음향 장치다.</summary>
    public Action? GuySound { get; set; }

    /// <summary>원문 Sort Game Options By 전체 Fancy 메뉴 자식이다.</summary>
    public Action<HudBattleInfoNode>? SortMenu { get; set; }

    /// <summary>전체 활성 옵션 하위 메뉴와 현재 TYPE 분류/정렬의 제외 추가 메뉴다.</summary>
    public Action<HudBattleInfoNode, ReadOnlyMemory<byte>>? OptionsMenu { get; set; }

    /// <summary>제외 추가 메뉴다.</summary>
    public Action<HudBattleInfoNode, ReadOnlyMemory<byte>>? ExcludeMenu { get; set; }

    /// <summary>선택 옵션 하나의 모든 포함 선택값 메뉴다.</summary>
    public Action<HudBattleInfoNode, ReadOnlyMemory<byte>, int>? OptionMenu { get; set; }

    /// <summary>현재 TYPE 설명을 쓰는 제외 제거 확인 메뉴다.</summary>
    public Action<uint>? RemoveMenu { get; set; }

    /// <summary>같은 파생 Gump의 현재 가상 Push다.</summary>
    public Action<uint>? Push { get; set; }

    /// <summary>Push 뒤 현재 x/y를 읽어 부모0/mode0으로 가상 부착한다.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }

    /// <summary>같은 현재 BattleOptions87BYTE를 실제67h로 송신한다.</summary>
    public Action<uint, ReadOnlyMemory<byte>, uint>? Data { get; set; }
}

// 0047b8f0 입력 부모 전체의 관문/선택·변경·재부착·송신이며 메뉴 자식 본문은 실제 장치로 결합한다.
public static class NativeBattleInfoInput
{
    /// <summary>선택되지 않은 자식은 요구하지 않고 실제 장치 미연결은 명시 실패한다. 0.915에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("BattleInfo 입력의 실제 메뉴/장치 미연결");

    /// <summary>가상 Push 효과 뒤 바뀔 수 있는 현재 위치를 읽어 실제 같은 파생 Attach에 전달한다. 0.915에서 추가됐다.</summary>
    /// <param name="token">같은 BattleInfo 노드 토큰이다.</param>
    /// <param name="node">같은 0x16c BattleInfo 노드다.</param>
    /// <param name="hooks">실제 메뉴/송신 장치다.</param>
    private static void Reattach(uint token, HudBattleInfoNode node, NativeBattleInfoInputHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(node);
        ArgumentNullException.ThrowIfNull(hooks);
        Need(hooks.Push)(token);
        Need(hooks.Attach)(token, node.X, node.Y, 0, 0);
    }

    /// <summary>게임 종류0은 그대로 두고 설정 쌍의 현재 값1..21을 원문 순서로 각각 적용한다. 0.915에서 추가됐다.</summary>
    /// <param name="node">같은 0x16c BattleInfo 노드다.</param>
    /// <param name="options">같은 현재 0x59BYTE 전투 옵션이다.</param>
    public static void ApplyMetaBattleDefaults(HudBattleInfoNode node, Span<byte> options)
    {
        ArgumentNullException.ThrowIfNull(node);
        // 각writer호출직전에현재쌍을다시읽는다.
        for (int index = 1; index < 22; index++)
        {
            BattleOptionMutations.SetBattleOptionChoice(options, index, unchecked((int)node.Settings[index, 0]));
        }
    }

    /// <summary>이 부모는 playerKnown 관문을 추가하지 않는다. 실제 전투 flag BYTE1을 통과한 뒤에만 event를 마스킹한다. 0.915에서 추가됐다.</summary>
    /// <param name="token">같은 BattleInfo 노드 토큰이다.</param>
    /// <param name="node">같은 0x16c BattleInfo 노드다.</param>
    /// <param name="event">원문 입력 event DWORD다.</param>
    /// <param name="first">원문 입력 첫 DWORD다.</param>
    /// <param name="second">원문 입력 둘째 DWORD다.</param>
    /// <param name="hooks">실제 메뉴/송신 장치다.</param>
    public static uint Handle(uint token, HudBattleInfoNode node, uint @event, uint first, uint second, NativeBattleInfoInputHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(node);
        ArgumentNullException.ThrowIfNull(hooks);
        if (Need(hooks.Metamap)() == 0 || node.Battle == 0)
        {
            return 0;
        }

        Memory<byte> battle = Need(hooks.Battle)(node.Battle);
        if (battle.Length == 0)
        {
            throw new ArgumentOutOfRangeException(nameof(node), "BattleInfo 입력의 실제 전투 flag BYTE 부족");
        }

        if ((battle.Span[0] & 1u) == 0)
        {
            return 0;
        }

        Memory<byte> options = battle.Slice(0, 0x59);
        uint code = @event & 0x03ffffffu;
        uint result = 0;
        uint changed = 0;
        if (code == Need(hooks.Command)(0x54278c))
        {
            if (first == Need(hooks.Serial)(node.Title))
            {
                if (unchecked((int)second) < 23)
                {
                    if (node.Changed == 0)
                    {
                        return 0;
                    }

                    if (node.Mode == 2)
                    {
                        int cycle = unchecked((int)(second - 1u));
                        BattleOptionMutations.CycleBattleOption(options.Span, cycle, Need(hooks.GuySound));
                        if (cycle == 0)
                        {
                            HudNsInfoMetaBattle.PrepareSettings(node, BattleOptionMutations.BattleOptionGameType(options.Span));
                            ApplyMetaBattleDefaults(node, options.Span);
                        }

                        changed = 1;
                    }
                    else if (second == 1)
                    {
                        BattleOptionMutations.CycleBattleOption(options.Span, 0, Need(hooks.GuySound));
                        HudNsInfoMetaBattle.PrepareSettings(node, BattleOptionMutations.BattleOptionGameType(options.Span));
                        ApplyMetaBattleDefaults(node, options.Span);
                        changed = 1;
                    }
                    else
                    {
                        int apply = unchecked((int)(second - 1u));
                        if (apply < 0 || apply >= 22)
                        {
                            throw new ArgumentOutOfRangeException(nameof(second), "BattleInfo 입력의 실제 설정 쌍 범위");
                        }

                        BattleOptionMutations.SetBattleOptionChoice(options.Span, apply, unchecked((int)node.Settings[apply, 0]));
                        changed = 1;
                    }
                }
                else if (unchecked((int)second) < 27 || unchecked((int)second) > 48)
                {
                    if (second == 24)
                    {
                        Need(hooks.SortMenu)(node);
                    }
                    else if (second == 23)
                    {
                        if (node.Changed == 0)
                        {
                            return 0;
                        }

                        Need(hooks.OptionsMenu)(node, options);
                    }
                    else if (second == 1000)
                    {
                        if (node.Changed == 0)
                        {
                            return 0;
                        }

                        Need(hooks.ExcludeMenu)(node, options);
                    }
                    else if (unchecked((int)second) > 121)
                    {
                        if (node.Changed == 0)
                        {
                            return 0;
                        }

                        Need(hooks.RemoveMenu)(second - 122u);
                    }
                }
                else
                {
                    if (node.Changed == 0)
                    {
                        return 0;
                    }

                    Need(hooks.OptionMenu)(node, options, unchecked((int)(second - 27u)));
                }
            }

            result = 1;
        }

        if (code == Need(hooks.Command)(0x542b80))
        {
            node.Mode = second;
            Reattach(token, node, hooks);
        }

        if (code == Need(hooks.Command)(0x542b7c))
        {
            if (node.Changed == 0)
            {
                return result;
            }

            int set = unchecked((int)(second >> 16));
            int value = unchecked((int)((unchecked((uint)set) << 16) ^ second));
            BattleOptionMutations.SetBattleOptionChoice(options.Span, set, value);
            ++changed;
        }

        if (code == Need(hooks.Command)(0x5429e0))
        {
            if (node.Changed == 0)
            {
                return result;
            }

            if (second != 0)
            {
                BattleOptionMutations.AddBattleExcludedType(options.Span, second, Need(hooks.GuySound));
                ++changed;
                result = 1;
            }
        }

        if (code == Need(hooks.Command)(0x5429e4))
        {
            if (node.Changed == 0)
            {
                return result;
            }

            if (second != 0)
            {
                BattleOptionMutations.RemoveBattleExcludedType(options.Span, second, Need(hooks.GuySound));
                ++changed;
                result = 1;
            }
        }

        if (changed != 0 && node.Changed != 0)
        {
            Need(hooks.Data)(node.Battle, options, 0x57);
            Reattach(token, node, hooks);
        }

        return result;
    }
}
