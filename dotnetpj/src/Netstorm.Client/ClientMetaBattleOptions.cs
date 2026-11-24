using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

// 0x67 메타 전투 옵션의 같은 원문 observer·버전·길이와 고정 F를 같은 분배기에 연결한다.
// cpppj/src/client/NativeMetaBattleOptions.cpp 의 ReceiveNativeMetaBattleOptions 와
// cpppj/src/client/ClientCampaignTransport.cpp 의 InstallClientMetaPacketReceivers 중 0x67 등록,
// cpppj/src/o/RawMissionBattleOptions.cpp 의 공급 범위·TYPE 비용·환불 비율 조회를 이식한다.
// 0.959에서 R4-6 메타 참가자 묶음용으로 추가됐다.
namespace Netstorm.Client;

// 0x67 전투 옵션의 원문 전투 기록 수신이 선택한 효과의 실제 외부 장치다.
public sealed class ClientNativeMetaBattleOptionsHooks
{
    /// <summary>0040da40의 같은 현재 전투 소유자를 읽는다.</summary>
    public Func<uint>? Owner { get; set; }

    /// <summary>원문 수준9의 진단 문자열 장치다.</summary>
    public Action<string>? Log { get; set; }

    /// <summary>실제 guy.wav 재생 장치다.</summary>
    public Action? GuySound { get; set; }

    /// <summary>현재 NSInfo 갱신 장치다.</summary>
    public Action? RefreshInfo { get; set; }
}

// Ch.cpp의 67h 전투 옵션 수신을 같은 현재 CS/자기 참가자·실제 음향/NSInfo에 연결한다.
public static class ClientNativeMetaBattleOptions
{
    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.959에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : class => value ?? throw new InvalidOperationException("메타 전투 옵션의 실제 장치 미연결");

    /// <summary>선택된 실제 값 대리자만 요구하며 가짜 성공으로 대신하지 않는다. 0.959에서 추가됐다.</summary>
    private static T NeedDelegate<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("메타 전투 옵션의 실제 장치 미연결");

    /// <summary>원문 비정렬 BYTE 폭을 낮은 자리부터 조립한다. 0.959에서 추가됐다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "메타 전투 옵션의 실제 읽기 범위");
        }

        uint value = 0;
        // 낮은 BYTE부터 지정 폭을 조립한다.
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>현재 전투의 flag BYTE를 먼저 검사하고 자기 master면 음향/복사/NSInfo를 모두 생략한다. 0.959에서 추가됐다.</summary>
    /// <param name="challenge">같은 실제 CS 전투10 기록 owner다.</param>
    /// <param name="packet">원문 0x67 패킷이다.</param>
    /// <param name="hooks">실제 소유자·로그·음향·NSInfo 장치다.</param>
    public static void Receive(ClientChallengeGlobals challenge, ReadOnlyMemory<byte> packet, ClientNativeMetaBattleOptionsHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(challenge);
        ArgumentNullException.ThrowIfNull(hooks);
        ReadOnlySpan<byte> bytes = packet.Span;
        Memory<byte> battle = challenge.Battle(Read(bytes, 4, 1));
        if ((Read(battle.Span, 0, 1) & 1u) == 0)
        {
            NeedDelegate(hooks.Log)("ERROR in handleBattleOptions: battle " + Read(bytes, 4, 1).ToString(System.Globalization.CultureInfo.InvariantCulture) + " is not valid!\n");
            return;
        }

        if (Read(bytes, 5, 2) > 160u)
        {
            NeedDelegate(hooks.Log)("ERROR in handleChalBattleOptions: option data too long\n[bufferLen == 160, optionLen == " + Read(bytes, 5, 2).ToString(System.Globalization.CultureInfo.InvariantCulture) + "]\n");
            return;
        }

        Memory<byte> own = challenge.Participant(NeedDelegate(hooks.Owner)());
        if (Read(own.Span, 0xa4) == Read(bytes, 4, 1) && (Read(own.Span, 0x64, 1) & 4u) != 0)
        {
            return;
        }

        NeedDelegate(hooks.GuySound)();
        uint count = Read(bytes, 5, 2);
        uint offset = 0;
        // rep movsd는 같은 네 BYTE를 먼저 읽고 저장한다. 전체 선행 복사/남은 필드 초기화는 없다.
        for (; offset + 4u <= count; offset += 4u)
        {
            if (7u + offset > (uint)bytes.Length || 4u > (uint)bytes.Length - 7u - offset)
            {
                throw new ArgumentOutOfRangeException(nameof(packet), "메타 전투 옵션의 실제 source DWORD 부족");
            }

            if (0x59u + offset > (uint)battle.Length || 4u > (uint)battle.Length - 0x59u - offset)
            {
                throw new ArgumentOutOfRangeException(nameof(packet), "메타 전투 옵션의 실제 기록 DWORD 부족");
            }

            bytes.Slice(7 + unchecked((int)offset), 4).CopyTo(battle.Span.Slice(0x59 + unchecked((int)offset), 4));
        }

        // 나머지 0..3BYTE도 각 실제 읽기 후 쓰기 순서로 처리한다.
        for (; offset < count; offset++)
        {
            uint value = Read(bytes, 7 + unchecked((int)offset), 1);
            if (0x59u + offset >= (uint)battle.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(packet), "메타 전투 옵션의 실제 기록 BYTE 부족");
            }

            battle.Span[0x59 + unchecked((int)offset)] = (byte)value;
        }

        battle.Span[0x58] = (byte)Read(bytes, 5, 1);
        uint ownBattle = Read(own.Span, 0xa4);
        if (ownBattle != 0 && ownBattle == Read(bytes, 4, 1))
        {
            NeedDelegate(hooks.RefreshInfo)();
        }
    }
}

// 0x67 메타 전투 옵션이 선택한 효과의 실제 외부 장치다.
public sealed class ClientMetaBattleOptionsHooks
{
    /// <summary>같은 실제 장기 게임 전역 owner다.</summary>
    public ClientGameplayGlobals? Game { get; set; }

    /// <summary>같은 실제 CS owner다.</summary>
    public ClientChallengeGlobals? Challenge { get; set; }

    /// <summary>같은 실제 표면 저장소 owner다.</summary>
    public RawSurfaceStorage? Storage { get; set; }

    /// <summary>같은 실제 로컬 상태 owner다.</summary>
    public ClientLocalStateGlobals? Local { get; set; }

    /// <summary>원문 진단의 실제 선택 줄 소비자다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>원문 수준9의 진단 문자열 장치다.</summary>
    public Action<string>? Log { get; set; }

    /// <summary>실제 guy.wav 재생 장치다.</summary>
    public Action? GuySound { get; set; }

    /// <summary>현재 NSInfo 갱신 장치다.</summary>
    public Action? RefreshInfo { get; set; }

    /// <summary>최종 전투·비미션·저장 관문을 모두 통과할 때 실제 Uber 재생성 장치다.</summary>
    public Action? Reconnect { get; set; }
}

// 004b4900 전투 옵션의 원문 복사·선택 CS·파생값·UI 부모다.
public static class ClientMetaBattleOptions
{
    /// <summary>원문 0x67 메타 전투 옵션 종류다. 0.959에서 추가됐다.</summary>
    public const uint Type = 0x67;

    /// <summary>원문 0x67 수신의 실제 고정 F 주소다. 0.959에서 추가됐다.</summary>
    public const uint NativeEntry = 0x004b4900;

    /// <summary>0041c8b0의 실제 TYPE 비용 스택표다. 0.959에서 추가됐다.</summary>
    private static readonly int[] TypeCosts = [1000, 2000, 3000, 5000];

    /// <summary>0041ca70의 실제 환불 비율 스택표다. 0.959에서 추가됐다.</summary>
    private static readonly uint[] RefundValues = [0, 25, 50, 75, 100, 150];

    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.959에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : class => value ?? throw new InvalidOperationException("메타 전투 옵션의 실제 장치 미연결");

    /// <summary>선택된 실제 값 대리자만 요구하며 가짜 성공으로 대신하지 않는다. 0.959에서 추가됐다.</summary>
    private static T NeedDelegate<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("메타 전투 옵션의 실제 장치 미연결");

    /// <summary>현재 전투 옵션 BYTE만 읽는다. 0.959에서 추가됐다.</summary>
    private static uint OptionByte(ReadOnlySpan<byte> options, int index)
    {
        // 선택한 현재 BYTE만 읽고 전체 87BYTE를 선복사하지 않는다.
        if (index < 0 || index >= options.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(options), "현재 전투 옵션 BYTE 물리 범위");
        }

        return options[index];
    }

    /// <summary>원문 순서에서 실제 필요한 float만 조회한다. 0.959에서 추가됐다.</summary>
    private static float RangeChoice(float[] choices, int index)
    {
        if (index < 0 || index >= choices.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(choices), "현재 공급 반경 선택표 물리 범위");
        }

        return choices[index];
    }

    /// <summary>004b4867의 FLD/FSTP32는 signaling NaN을 quiet로 저장한다. 0.959에서 추가됐다.</summary>
    private static float StoreX87Float(float value)
    {
        uint bits = BitConverter.SingleToUInt32Bits(value);
        if ((bits & 0x7f800000u) == 0x7f800000u && (bits & 0x007fffffu) != 0)
        {
            bits |= 0x00400000u;
        }

        return BitConverter.UInt32BitsToSingle(bits);
    }

    /// <summary>원본은 초기 선택값을 먼저 저장한다. 0.959에서 추가됐다.</summary>
    private static void UpdateSupplyRange(ReadOnlySpan<byte> options, float[] choices, ref float range, ref float squared)
    {
        int index = unchecked((int)OptionByte(options, 4));
        range = StoreX87Float(RangeChoice(choices, index));
        float candidate = RangeChoice(choices, 2) < range ? RangeChoice(choices, 2) : range;
        if (RangeChoice(choices, 0) > candidate)
        {
            range = StoreX87Float(RangeChoice(choices, 0));
        }
        else if (RangeChoice(choices, 2) < range)
        {
            range = StoreX87Float(RangeChoice(choices, 2));
        }

        squared = range * range;
    }

    /// <summary>옵션 BYTE는 호출 때마다 같은 원문에서 읽는다. 0.959에서 추가됐다.</summary>
    private static int OptionTypeCost(ReadOnlySpan<byte> options)
    {
        uint index = OptionByte(options, 0x10);
        if (index >= (uint)TypeCosts.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(options), "미션 TYPE 비용 옵션 인덱스");
        }

        return TypeCosts[index];
    }

    /// <summary>이후 설정 변경을 이전 파생값 캐시로 가리지 않는다. 0.959에서 추가됐다.</summary>
    private static uint OptionRefundPercent(ReadOnlySpan<byte> options)
    {
        uint index = OptionByte(options, 0xf);
        if (index >= (uint)RefundValues.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(options), "미션 파생값 옵션 인덱스");
        }

        return RefundValues[index];
    }

    /// <summary>원문 복사·선택 CS·파생값·UI 부모를 같은 순서로 수행한다. 0.959에서 추가됐다.</summary>
    /// <param name="hooks">실제 게임·CS·저장소·진단·음향·NSInfo·재생성 장치다.</param>
    /// <param name="sender">원문 송신자 번호다.</param>
    /// <param name="packet">원문 0x67 패킷이다.</param>
    public static void Receive(ClientMetaBattleOptionsHooks hooks, uint sender, ReadOnlyMemory<byte> packet)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        ClientGameplayGlobals game = Need(hooks.Game);
        ClientChallengeGlobals challenge = Need(hooks.Challenge);
        RawSurfaceStorage storage = Need(hooks.Storage);
        ClientLocalStateGlobals local = Need(hooks.Local);
        if (!ReferenceEquals(local.Gameplay, game))
        {
            throw new ArgumentException("메타 전투 옵션의 같은 실제 Client 소유자 필요", nameof(hooks));
        }

        RawStateBattleOptions.Receive(sender, packet,
            new StateBattleOptionGlobals
            {
                Options = game.BattleOptions,
                Challenge = () => challenge.Current,
                Value = game.GameAux.RefundPercentBox,
                Battle = () => storage.OwnerMode.Battle,
                Mission = () => game.World.MissionPost,
                Saved = () => local.SavedGame.Value,
            },
            new StateBattleOptionHooks
            {
                Assertion = text => NeedDelegate(hooks.Assertion)(text, ".\\State.cpp", 0xd51),
                ChallengeOptions = (_, bytes) => ClientNativeMetaBattleOptions.Receive(challenge, bytes, new ClientNativeMetaBattleOptionsHooks
                {
                    Owner = () => game.Player.BattleOwner,
                    Log = text => NeedDelegate(hooks.Log)(text),
                    GuySound = () => NeedDelegate(hooks.GuySound)(),
                    RefreshInfo = () => NeedDelegate(hooks.RefreshInfo)(),
                }),
                Range = () =>
                {
                    float range = game.ManaRadius;
                    float squared = game.ManaRadiusSquared;
                    UpdateSupplyRange(game.BattleOptions, game.ManaRanges, ref range, ref squared);
                    game.ManaRadius = range;
                    game.ManaRadiusSquared = squared;
                },
                Cost = () => OptionTypeCost(game.BattleOptions),
                StoreCost = cost =>
                {
                    uint type = game.GameAux.GeyserType;
                    if (type >= (uint)storage.Types.Count)
                    {
                        throw new ArgumentOutOfRangeException(nameof(hooks), "메타 전투 옵션의 현재 비용 TYPE 물리 범위");
                    }

                    storage.Types[(int)type].Cost = cost;
                },
                CurrentValue = () => OptionRefundPercent(game.BattleOptions),
                Reconnect = () => NeedDelegate(hooks.Reconnect)(),
            });
    }

    /// <summary>0x67의 원문 빈 observer·버전·가변 길이와 고정 F를 설치한다. 40ae80의 기존 공용 실제 송신 관찰기를 유지한다. 0.959에서 추가됐다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    /// <param name="hooks">실제 게임·CS·저장소·진단·음향·NSInfo·재생성 장치다.</param>
    public static void Install(RawPacketDispatch dispatch, ClientMetaBattleOptionsHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        ArgumentNullException.ThrowIfNull(hooks);
        dispatch.Version(Type, 1, 0);
        dispatch.RequireLength(Type, ushort.MaxValue);
        dispatch.RegisterCanonicalDirect(Type, NativeEntry, (sender, _, packet) => Receive(hooks, sender, packet));
    }
}
