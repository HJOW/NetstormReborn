// PlayerInfo의 전체 입력을 같은 파생 노드/CS·87BYTE 옵션과 실제 메뉴·송신에 연결한다.
// cpppj/src/client/NativePlayerInfoInput.cpp 의 HandleNativePlayerInfo(0047a900) 를 이식한다.
// 0.913에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.

namespace Netstorm.Client.Gump;

// PlayerInfo 원문 입력이 선택한 효과의 실제 외부 장치다.
public sealed class NativePlayerInfoInputHooks
{
    /// <summary>현재 명령 DWORD를 읽는다.</summary>
    public Func<uint, uint>? Command { get; set; }

    /// <summary>현재 자식 serial을 읽는다.</summary>
    public Func<uint, uint>? Serial { get; set; }

    /// <summary>실제 버튼 DWORD 값을 읽는다.</summary>
    public Func<uint, uint>? ButtonValue { get; set; }

    /// <summary>현재 540c70을 같은 Client 장기 저장소에서 읽는다.</summary>
    public Func<uint>? Owner { get; set; }

    /// <summary>현재 540c74를 같은 Client 장기 저장소에서 읽는다.</summary>
    public Func<uint>? Subscriber { get; set; }

    /// <summary>현재 실제 CS의 참가자 기록 물리를 빌린다.</summary>
    public Func<uint, Memory<byte>>? Participant { get; set; }

    /// <summary>현재 실제 CS의 전투 기록 물리를 빌린다.</summary>
    public Func<uint, Memory<byte>>? Battle { get; set; }

    /// <summary>자기 특수 슬롯 변경 때만 실제594fc4 DWORD를 쓴다.</summary>
    public Action<uint>? Auxiliary { get; set; }

    /// <summary>실제 1fh 헤더 뒤 payload/부분효과를 실행하고 TCP11로 보낸다.</summary>
    public Action<Action<Memory<byte>>>? Status { get; set; }

    /// <summary>실제 현재 옵션87BYTE를 같은67h/TCP11 장치로 보낸다.</summary>
    public Action<uint, ReadOnlyMemory<byte>, uint>? Data { get; set; }

    /// <summary>현재 언어의 원본 글꼴 BYTE 번역이다.</summary>
    public Func<string, string>? Translate { get; set; }

    /// <summary>같은 Fancy controlled 메뉴 생성이다.</summary>
    public Action? BeginMenu { get; set; }

    /// <summary>Current를 원시좌표/루트에 부착한다.</summary>
    public Action? AttachMenu { get; set; }

    /// <summary>같은 실제 메뉴 제목을 추가한다.</summary>
    public Action<string>? Title { get; set; }

    /// <summary>원문의 include/enabled1·force0 항목을 작성한다.</summary>
    public Action<string, uint, uint>? Item { get; set; }
}

// 0047a900 전체 준비/시작·좌우 슬롯 변경·추방 확인이며 처리하지 않은 입력은0이다.
public static class NativePlayerInfoInput
{
    /// <summary>원문 PlayerInfo는 별도 모드/권한 검사를 추가하지 않는다. 0.913에서 추가됐다.</summary>
    public enum SlotChange
    {
        /// <summary>좌행 랭크 감소다.</summary>
        RankDown,

        /// <summary>좌행 색 감소다.</summary>
        ColorDown,

        /// <summary>특수 전환이다.</summary>
        Special,

        /// <summary>우행 색 증가다.</summary>
        ColorUp,

        /// <summary>우행 랭크 증가다.</summary>
        RankUp,
    }

    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.913에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("PlayerInfo 입력의 실제 장치 미연결");

    /// <summary>비정렬 실제 DWORD를 낮은 BYTE부터 읽는다. 0.913에서 추가됐다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || 4 > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "PlayerInfo 입력의 실제 CS DWORD 부족");
        }

        return (uint)bytes[offset]
            | ((uint)bytes[offset + 1] << 8)
            | ((uint)bytes[offset + 2] << 16)
            | ((uint)bytes[offset + 3] << 24);
    }

    /// <summary>원문의 저장 폭 네 BYTE만 쓴다. 0.913에서 추가됐다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || 4 > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "PlayerInfo 입력의 실제 DWORD 쓰기 범위");
        }

        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (8 * index));
        }
    }

    /// <summary>색/랭크는 헤더 전에 바꾸며 Special은 헤더/초기 payload 뒤에 바꾼다. 4000h는 CS에 다시 저장하지 않는다. 0.913에서 추가됐다.</summary>
    private static void ChangeSlot(HudPlayerInfoNode node, SlotChange change, NativePlayerInfoInputHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(node);
        ArgumentNullException.ThrowIfNull(hooks);
        Memory<byte> record = Need(hooks.Participant)(node.Player);
        Memory<byte> options = Need(hooks.Battle)(Read(record.Span, 0xa4)).Slice(0, 0x59);
        if (change != SlotChange.Special)
        {
            int slot = unchecked((int)Read(record.Span, 0xa8));
            switch (change)
            {
                case SlotChange.RankDown:
                    BattleOptionSlots.DecreaseBattleSlotRank(options.Span, slot);
                    break;
                case SlotChange.ColorDown:
                    BattleOptionSlots.DecreaseBattleSlotColor(options.Span, slot);
                    break;
                case SlotChange.ColorUp:
                    BattleOptionSlots.IncreaseBattleSlotColor(options.Span, slot);
                    break;
                case SlotChange.RankUp:
                    BattleOptionSlots.IncreaseBattleSlotRank(options.Span, slot);
                    break;
                default:
                    break;
            }
        }

        Need(hooks.Status)(packet =>
        {
            uint subscriber = Read(Need(hooks.Participant)(node.Player).Span, 4);
            if (change == SlotChange.Special)
            {
                Put(packet.Span, 4, subscriber);
                Put(packet.Span, 8, 0);
                BattleOptionSlots.ToggleBattleSlotSpecial(options.Span, unchecked((int)Read(record.Span, 0xa8)));
                if (node.Player == Need(hooks.Owner)())
                {
                    int at = unchecked((int)(Read(record.Span, 0xa8) + 0x3fu));
                    if (at < 0 || at >= options.Length)
                    {
                        throw new ArgumentOutOfRangeException(nameof(options), "PlayerInfo 특수 슬롯의 실제 BYTE 범위");
                    }

                    Need(hooks.Auxiliary)(options.Span[at] != 0 ? 1u : 0u);
                }
            }
            else
            {
                Put(packet.Span, 8, 0);
                Put(packet.Span, 4, subscriber);
            }

            Put(packet.Span, 8, Read(record.Span, 0x64) | 0x4000u);
        });
        Need(hooks.Data)(Read(record.Span, 0xa4), options, 0x57);
    }

    /// <summary>현재 참가자 이름은 BeginFancy 이후 실제 서식 위치에서 읽고 임의 길이 절단을 하지 않는다. 0.913에서 추가됐다.</summary>
    private static void ConfirmKickoff(HudPlayerInfoNode node, NativePlayerInfoInputHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(node);
        ArgumentNullException.ThrowIfNull(hooks);
        Memory<byte> record = Need(hooks.Participant)(node.Player);
        Need(hooks.BeginMenu)();
        string pattern = Need(hooks.Translate)("Kickoff \"%s\"?\n");
        ReadOnlySpan<byte> bytes = record.Span.Slice(0x26);
        int end = bytes.IndexOf((byte)0);
        if (end < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(node), "PlayerInfo 추방 이름의 실제 NUL 없음");
        }

        string name = System.Text.Encoding.Latin1.GetString(bytes.Slice(0, end));
        string text = pattern.Replace("%s", name, StringComparison.Ordinal);
        if (text.Length >= 256)
        {
            throw new ArgumentOutOfRangeException(nameof(node), "PlayerInfo 추방 제목의 실제256BYTE 범위");
        }

        Need(hooks.Title)(text);
        string yes = Need(hooks.Translate)("Yes");
        uint participant = node.Player;
        uint command = Need(hooks.Command)(0x5429dc);
        Need(hooks.Item)(yes, command, participant);
        string no = Need(hooks.Translate)("No");
        Need(hooks.Item)(no, Need(hooks.Command)(0x5429dc), 0);
        Need(hooks.AttachMenu)();
    }

    /// <summary>클릭의 두 자기 버튼과 좌/우 텍스트 serial을 구별하며 태그 사건은 해당 분기에서 항상1로 소비한다. 0.913에서 추가됐다.</summary>
    /// <param name="node">같은 0xbc PlayerInfo 노드다.</param>
    /// <param name="event">원문 입력 event DWORD다.</param>
    /// <param name="first">원문 입력 첫 DWORD다.</param>
    /// <param name="second">원문 입력 둘째 DWORD다.</param>
    /// <param name="hooks">실제 CS/메뉴/송신 장치다.</param>
    public static uint Handle(HudPlayerInfoNode node, uint @event, uint first, uint second, NativePlayerInfoInputHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(node);
        ArgumentNullException.ThrowIfNull(hooks);
        uint code = @event & 0x03ffffffu;
        if (code == Need(hooks.Command)(0x5427a8) && node.Player != 0)
        {
            uint player = node.Player;
            uint readySerial = Need(hooks.Serial)(node.Button);
            uint owner = Need(hooks.Owner)();
            if (first == readySerial && player == owner)
            {
                Memory<byte> record = Need(hooks.Participant)(player);
                Need(hooks.Status)(packet =>
                {
                    Put(packet.Span, 4, Need(hooks.Subscriber)());
                    uint value = Need(hooks.ButtonValue)(node.Button);
                    uint flags = Read(record.Span, 0x64);
                    flags = value != 0 ? flags | 2u : flags & 0xfffffffdU;
                    Put(record.Span, 0x64, flags);
                    Put(packet.Span, 8, flags);
                });
                return 1;
            }

            if (first == Need(hooks.Serial)(node.Start) && player == owner)
            {
                if (node.Reserved == 0)
                {
                    Need(hooks.Status)(packet =>
                    {
                        Put(packet.Span, 4, Need(hooks.Subscriber)());
                        Put(packet.Span, 8, 0x12);
                    });
                    node.Reserved = 1;
                }

                return 1;
            }
        }
        else
        {
            if (code == Need(hooks.Command)(0x542790))
            {
                if (first == Need(hooks.Serial)(node.Text))
                {
                    uint player = node.Player;
                    if (second == player + 300u)
                    {
                        ChangeSlot(node, SlotChange.RankDown, hooks);
                        return 1;
                    }

                    if (second == player + 100u)
                    {
                        ChangeSlot(node, SlotChange.Special, hooks);
                        return 1;
                    }

                    if (second == player + 200u)
                    {
                        ChangeSlot(node, SlotChange.ColorDown, hooks);
                        return 1;
                    }

                    if (second == player + 400u)
                    {
                        ConfirmKickoff(node, hooks);
                    }
                }

                return 1;
            }

            if (code == Need(hooks.Command)(0x54278c))
            {
                if (first == Need(hooks.Serial)(node.Text))
                {
                    uint player = node.Player;
                    if (second == player + 100u)
                    {
                        ChangeSlot(node, SlotChange.Special, hooks);
                        return 1;
                    }

                    if (second == player + 200u)
                    {
                        ChangeSlot(node, SlotChange.ColorUp, hooks);
                        return 1;
                    }

                    if (second == player + 300u)
                    {
                        ChangeSlot(node, SlotChange.RankUp, hooks);
                    }
                }

                return 1;
            }
        }

        return 0;
    }
}
